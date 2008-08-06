/*********************************************************************************

 Copyright 2006-2008 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

/** \file timer.c	
	Functions to use the timer on the Make Controller Board.
  This uses the regular interrupt mechanism and as such may be blocked by other
  routines for significant numbers of milliseconds. 
*/

#include "AT91SAM7X256.h"
#include "types.h"
#include "timer.h"
#include "timer_internal.h"
#include "config.h"
#include "rtos.h"

#define TIMER_CYCLES_PER_MS 48

struct Timer_ Timer;

static int Timer_Init( void );
static int Timer_Deinit( void );
//static int Timer_GetCount( void );
static int Timer_GetTimeTarget( void );
static int Timer_GetTime( void );
static void Timer_SetTimeTarget( int );
static void Timer_Enable( void );

void Timer_Isr( void );

/** \defgroup Timer Timer
* The Timer subsystem provides a timer in a millisecond timeframe.
  For higher resolution timing, check the \ref FastTimer

  The Timer subsystem is based on a collection of \b TimerEntries.  To start a new timer, create a new
  \b TimerEntry structure, initialize it with Timer_InitializeEntry( ), and start it with Timer_Set( ).

  There are currently a couple of limitations to the Timer system:
  - In your callback function, you must not sleep or make any FreeRTOS-related calls.
  - To modify an existing TimerEntry, you must cancel the timer with Timer_Cancel( ), and reinitialize the TimerEntry.

  \todo Allow the timer callbacks to cooperate with the \ref RTOS
  \todo Allow existing timer entries to be modified (repeat or not, modify the period, etc.)
* \ingroup Core
* @{
*/

/**	
  Controls the active state of the \b Timer subsystem
  @param active whether this subsystem is active or not
	@return Zero on success.
	@see Timer_Set, Timer_Cancel
*/
int Timer_SetActive( bool active )
{
  if ( active )
  {
    if ( Timer.users++ == 0 )
    {
      int status;
  
      status = Timer_Init();  
      if ( status != CONTROLLER_OK )
      {
        Timer.users--;
        return status;
      }
    }
  }
  else
  {
    if ( --Timer.users == 0 )
      Timer_Deinit();
  }
  return CONTROLLER_OK;
}

/**	
  Returns whether the timer subsystem is active or not
	@return active.
	@see Timer_Set, Timer_Cancel
*/
int Timer_GetActive( )
{
  return Timer.users > 0;
}

/**	
  Initializes a new timer structure.  
	The event is signified by a callback to the function provided, after the interval specified.  
	The specified ID is passed back to the function to permit one function to work for many events.  
	Pass repeat = true to make the event continue to create callbacks until it is canceled.
  @param timerEntry pointer to the TimerEntry to be intialized. 
  @param timerCallback pointer to the callback function.  The function must
         be of the form \verbatim void timerCallback( int id )\endverbatim
  @param id An integer specifying the ID the callback function is to be provided with.
  @param timeMs The time in milliseconds desired for the callback.
  @param repeat Set whether the timer repeats or is a one-time event.
	@see Timer_Cancel

  \par Example
  \code
  TimerEntry myTimer; // our TimerEntry
  Timer_InitializeEntry( &myTimer, myCallback, 0, 250, true );
  Timer_Set( &myTimer ); // start our timer

  void myCallback( int id ) // our code that will get called by the timer every 250 ms.
  {
    // do something here
  }
  \endcode
*/
void Timer_InitializeEntry( TimerEntry* timerEntry, void (*timerCallback)( int id ), int id, int timeMs, bool repeat )
{
  int time = timeMs * TIMER_CYCLES_PER_MS;

  // Set the details into the free dude
  timerEntry->callback = timerCallback;
  timerEntry->id = id;
  timerEntry->timeCurrent = 0;
  timerEntry->timeInitial = time;
  timerEntry->repeat = repeat;
  timerEntry->next = NULL;
}

/**
  Sets a given TimerEntry to run.
  This routine adds the entry to the running queue and then decides if it needs
  to start the timer (if it's not running) or alter the timer's clock for a shorter
  period.
  @param timerEntry pointer to the FastTimerEntry to be run. 
  */
int Timer_Set( TimerEntry* timerEntry )
{
  // this could be a lot smarter - for example, modifying the current period?
  if ( !Timer.servicing ) 
    TaskEnterCritical();

  if ( !Timer.running )
  {
    Timer_SetActive( true );
    Timer_SetTimeTarget( timerEntry->timeInitial );
    Timer_Enable();
  }  

  // Calculate how long remaining
  int target = Timer_GetTimeTarget();
  int timeCurrent = Timer_GetTime();
  int remaining = target - timeCurrent;

  // Get the entry ready to roll
  timerEntry->timeCurrent = timerEntry->timeInitial;

  // Add entry
  TimerEntry* first = Timer.first;
  Timer.first = timerEntry;
  timerEntry->next = first;

  // Are we actually servicing an interupt right now?
  if ( !Timer.servicing )
  {
    // No - so does the time requested by this new timer make the time need to come earlier?
    if ( timerEntry->timeCurrent < ( remaining - TIMER_MARGIN ) )
    {
      // Damn it!  Reschedule the next callback
      Timer_SetTimeTarget( target - ( remaining - timerEntry->timeCurrent ));
    }
    else
    {
      // pretend that the existing time has been with us for the whole slice so that when the 
      // IRQ happens it credits the correct (reduced) time.
      timerEntry->timeCurrent += timeCurrent;
    }
  }
  else
  {
    // Yep... we're servicing something right now

    // Make sure the previous pointer is OK.  This comes up if we were servicing the first item
    // and it subsequently wants to delete itself, it would need to alter the next pointer of the 
    // the new head... err... kind of a pain, this
    if ( Timer.previous == NULL )
      Timer.previous = timerEntry;

    // Need to make sure that if this new time is the lowest yet, that the IRQ routine 
    // knows that.  Since we added this entry onto the beginning of the list, the IRQ
    // won't look at it again
    if ( Timer.nextTime == -1 || Timer.nextTime > timerEntry->timeCurrent )
        Timer.nextTime = timerEntry->timeCurrent;
  }

  if ( !Timer.servicing ) 
    TaskExitCritical();

  return CONTROLLER_OK;
}

/**	
  Cancel a timer event.
  @param timerEntry The entry to be removed.
  @return 0 on success.
	@see Timer_Set
*/
int Timer_Cancel( TimerEntry* timerEntry )
{
  if ( !Timer.servicing ) 
    TaskEnterCritical();

  // Look through the running list - clobber the entry
  TimerEntry* te = Timer.first;
  TimerEntry* previousEntry = NULL;
  while ( te != NULL )
  {
    // check for the requested entry
    if ( te == timerEntry )
    {
      // remove the entry from the list
      if ( te == Timer.first )
        Timer.first = te->next;
      else
        previousEntry->next = te->next;
      
      // make sure the in-IRQ pointers are all OK
      if ( Timer.servicing )
      {
        if ( Timer.previous == timerEntry )
          Timer.previous = previousEntry;
        if ( Timer.next == timerEntry )
          Timer.next = te->next;
      }

      // update the pointers - leave previous where it is
      te = te->next;
    }
    else
    {
      previousEntry = te;
      te = te->next;
    }
  }

  if ( !Timer.servicing ) 
    TaskExitCritical();

  return CONTROLLER_OK;
}

/** @}
*/

//
// INTERNAL
//

/* commented out to avoid 'defined but not used' error
int Timer_GetCount()
{
  int count;
  TaskEnterCritical();
  count = Timer.count;
  Timer.count = 0;
  TaskExitCritical();
  return count;
}
*/

// Enable the timer.  Disable is performed by the ISR when timer is at an end
void Timer_Enable( )
{
  // Enable the device
  // AT91C_BASE_TC0->TC_CCR = AT91C_TC_SWTRG;
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
  Timer.running = true;
}

int Timer_GetTimeTarget( )
{
  return AT91C_BASE_TC0->TC_RC;
}

int Timer_GetTime( )
{
  return AT91C_BASE_TC0->TC_CV;
}

void Timer_SetTimeTarget( int target )
{
  AT91C_BASE_TC0->TC_RC = ( target < 0xFFFF ) ? target : 0xFFFF;
}

int Timer_Init()
{
  Timer.first = NULL;

  Timer.count = 0;
  Timer.jitterTotal = 0;
  Timer.jitterMax = 0;  
  Timer.jitterMaxAllDay = 0;

  Timer.running = false;
  Timer.servicing = false;

  // Configure TC by enabling PWM clock
	AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_TC0;
                                    
  unsigned int mask ;
  mask = 0x1 << AT91C_ID_TC0;
   
  /* Disable the interrupt on the interrupt controller */
  AT91C_BASE_AIC->AIC_IDCR = mask;

  /* Save the interrupt handler routine pointer */	
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_TC0 ] = (unsigned int)Timer_Isr;

  /* Store the Source Mode Register */
  // 4 PRIORITY is random
  AT91C_BASE_AIC->AIC_SMR[ AT91C_ID_TC0 ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 4  ;
  /* Clear the interrupt on the interrupt controller */
  AT91C_BASE_AIC->AIC_ICCR = mask ;

  // Set the timer up.  We want just the basics, except when the timer compares 
  // with RC, retrigger
  //
  // MCK is 47923200
  // DIV1: A tick MCK/2 times a second
  // This makes every tick every 41.73344ns
  // DIV2: A tick MCK/8 times a second
  // This makes every tick every 167ns
  // DIV3: A tick MCK/32 times a second
  // This makes every tick every 668ns  
  // DIV4: A tick MCK/128 times a second
  // This makes every tick every 2.671us
  // DIV5: A tick MCK/1024 times a second
  // This makes every tick every 21.368us
  // CPCTRG makes the RC event reset the counter and trigger it to restart
  AT91C_BASE_TC0->TC_CMR = AT91C_TC_CLKS_TIMER_DIV5_CLOCK | AT91C_TC_CPCTRG;
                   
  // Only really interested in interrupts when the RC happens
  AT91C_BASE_TC0->TC_IDR = 0xFF; 
  AT91C_BASE_TC0->TC_IER = AT91C_TC_CPCS; 

  // load the RC value with something
  AT91C_BASE_TC0->TC_RC = 0xFFFF;

  // Enable the interrupt
  AT91C_BASE_AIC->AIC_IECR = mask;

  return CONTROLLER_OK;
}

int Timer_Deinit()
{
  return CONTROLLER_OK;
}
