/*********************************************************************************

 Copyright 2006 MakingThings

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
	Timer.
	Functions to use the timer on the Make Controller Board.
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
static int Timer_GetCount( void );
static int Timer_GetTimeTarget( void );
static int Timer_GetTime( void );
static void Timer_SetTimeTarget( int );
static void Timer_Enable( void );

void Timer_Isr( void );

/** \defgroup Timer
* The Timer subsystem provides a high resolution timer in a 4 usec timeframe.
* \ingroup Controller
* @{
*/

/**	
  Controls the activation of the timer
  @param active whether the Timer subsystem is active or not
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
	Timer_GetActive.
  Returns whether the timer subsystem is active or not
	@return active.
	@see Timer_Set, Timer_Cancel
*/
int Timer_GetActive( )
{
  return Timer.users > 0;
}

/**	
  Create a new timer event.  
	The event is signified by a callback to the function provided, after the interval specified.  
	The specified ID is passed back to the function to permit one function to work for many events.  
	Pass repeat = true to make the event continue to create callbacks until it is canceled.
  @param timerCallback pointer to the callback function.  The function must
         be of the form <em>void callback( int id )</em>.
  @param id An integer specifying the ID the callback function is to be provided with.
  @param timeMs The time in milliseconds desired for the callback.
  @param repeat Set whether the timer repeats or is a one-time event.
  @return Status - an integer indicating success (0) or failure (non-zero).
	@see Timer_Start, Timer_Cancel and Timer_Stop
*/
int Timer_Set( void (*timerCallback)( int id ), int id, int timeMs, bool repeat )
{
  int time = timeMs * TIMER_CYCLES_PER_MS;

  // this could be a lot smarter - for example, modifying the current period?
  if ( !Timer.servicing ) 
    TaskEnterCritical();

  /// ToDo: Get the new timer record from the heap

  // Get one from the free queue
  TimerEntry* tne = Timer.freeFirst;
  if ( tne == NULL )
  {
    if ( !Timer.servicing ) 
      TaskExitCritical();
    return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;
  }
  Timer.freeFirst = tne->next;

  // Set the details into the free dude
  tne->callback = timerCallback;
  tne->id = id;
  tne->time = time;
  tne->timeInitial = time;
  tne->repeat = repeat;

  // Now insert it.  Two cases: 1) timer is running or 2) timer is not
  if ( !Timer.running )
  {
    // Nothing running right now
    // Assert Timer.runningFirst == NULL
    Timer.runningFirst = tne;
    tne->next = NULL;

    Timer_SetTimeTarget( time );
    Timer_Enable();
  }  
  else
  {
    // There's something running - insert the new one

    // Could be servicing the interrupt right now!

    // Calculate how long remaining
    int target = Timer_GetTimeTarget();
    int timeCurrent = Timer_GetTime();
    int remaining = target - timeCurrent;

    if ( !Timer.servicing )
    {
      // Add entry
      TimerEntry* first = Timer.runningFirst;
      Timer.runningFirst = tne;
      tne->next = first;
  
      if ( time < ( remaining - TIMER_MARGIN ) )
      {
        Timer_SetTimeTarget( target - ( remaining - time  ));
      }
      else
      {
        // pretend that the existing time has been with us for the whole slice
        tne->time += timeCurrent;
      }
    }
    else
    {
      // Add entry
      TimerEntry* first = Timer.newFirst;
      Timer.newFirst = tne;
      tne->next = first;
    }
  }

  if ( !Timer.servicing ) 
    TaskExitCritical();

  return CONTROLLER_OK;
}

/**	
  Cancel a timer event.
	The event is specified by the callback function and id provided.  All events that match 
	these two parameters are terminated.
  @param timerCallback A pointer to the callback function.
  @param id An integer specifying the ID of the callback function.
  @return 0 on success.
	@see Timer_Start, Timer_Set and Timer_Start
*/
int Timer_Cancel( void (*timerCallback)( int id ), int id )
{
  if ( !Timer.servicing ) 
    TaskEnterCritical();

  // Look through the running list
  TimerEntry* te = Timer.runningFirst;
  while ( te != NULL )
  {
    if ( te->callback == timerCallback && te->id == id )
    {
      te->callback = NULL;
      te->repeat = false;
    }
    te = te->next;
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


int Timer_GetCount()
{
  int count;
  TaskEnterCritical();
  count = Timer.count;
  Timer.count = 0;
  TaskExitCritical();
  return count;
}

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
  // One time datastructure initialization
  TimerEntry* te = &Timer.entry[ 0 ];
  int i;
  Timer.freeFirst = te;
  for ( i = 0; i < TIMER_COUNT; i++ ) 
  {
    te->callback = NULL;
    if ( i < TIMER_COUNT - 1 )
      te->next = te + 1;
    else
      te->next = NULL;
    te++;
  }

  Timer.runningFirst = NULL;
  Timer.newFirst = NULL;

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
  AT91C_BASE_AIC->AIC_SMR[ AT91C_ID_TC0 ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 7  ;
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
