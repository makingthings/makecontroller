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


#include "types.h"
#include "timer.h"
//#include "timer_internal.h"
#include "error.h"
#include "rtos_.h"

#define TIMER_CYCLES_PER_MS 48

bool Timer::manager_init = false;
Timer::Manager Timer::manager;

void Timer_Isr( );

/**
  Make a new timer.
  Note - the timer index selected will be used for all subsequent timers created.
*/
Timer::Timer(int timer)
{
  if(!manager_init)
  {
    managerInit(timer);
    manager_init = true;
  }
}

Timer::~Timer()
{
//  return CONTROLLER_OK;
}

void Timer::setHandler(TimerHandler handler, int id, int millis, bool repeat)
{
  callback = handler;
  id = id;
  timeCurrent = 0;
  timeInitial = millis * TIMER_CYCLES_PER_MS;
  repeat = repeat;
  next = NULL;
}

/**
  Sets a given TimerEntry to run.
  This routine adds the entry to the running queue and then decides if it needs
  to start the timer (if it's not running) or alter the timer's clock for a shorter
  period.
  @param timerEntry pointer to the FastTimerEntry to be run. 
*/
int Timer::start( )
{
  // this could be a lot smarter - for example, modifying the current period?
  if ( !manager.servicing ) 
    Task::enterCritical();

  if ( !manager.running )
  {
//    Timer_SetActive( true );
    setTimeTarget( this->timeInitial );
    enable();
  }  

  // Calculate how long remaining
  int target = getTimeTarget();
  int timeCurrent = getTime();
  int remaining = target - timeCurrent;

  // Get the entry ready to roll
  this->timeCurrent = this->timeInitial;

  // Add entry
  Timer* first = manager.first;
  manager.first = this;
  this->next = first;

  // Are we actually servicing an interupt right now?
  if ( !manager.servicing )
  {
    // No - so does the time requested by this new timer make the time need to come earlier?
    if ( this->timeCurrent < ( remaining - TIMER_MARGIN ) )
    {
      // Damn it!  Reschedule the next callback
      setTimeTarget( target - ( remaining - this->timeCurrent ));
    }
    else
    {
      // pretend that the existing time has been with us for the whole slice so that when the 
      // IRQ happens it credits the correct (reduced) time.
      this->timeCurrent += timeCurrent;
    }
  }
  else
  {
    // Yep... we're servicing something right now

    // Make sure the previous pointer is OK.  This comes up if we were servicing the first item
    // and it subsequently wants to delete itself, it would need to alter the next pointer of the 
    // the new head... err... kind of a pain, this
    if ( manager.previous == NULL )
      manager.previous = this;

    // Need to make sure that if this new time is the lowest yet, that the IRQ routine 
    // knows that.  Since we added this entry onto the beginning of the list, the IRQ
    // won't look at it again
    if ( manager.nextTime == -1 || manager.nextTime > this->timeCurrent )
        manager.nextTime = this->timeCurrent;
  }

  if ( !manager.servicing ) 
    Task::exitCritical();

  return CONTROLLER_OK;
}

/**	
  Cancel a timer event.
  @param timerEntry The entry to be removed.
  @return 0 on success.
	@see Timer_Set
*/
int Timer::stop( )
{
  if ( !manager.servicing ) 
    Task::enterCritical();

  // Look through the running list - clobber the entry
  Timer* te = manager.first;
  Timer* previousEntry = NULL;
  while ( te != NULL )
  {
    // check for the requested entry
    if ( te == this )
    {
      // remove the entry from the list
      if ( te == manager.first )
        manager.first = te->next;
      else
        previousEntry->next = te->next;
      
      // make sure the in-IRQ pointers are all OK
      if ( manager.servicing )
      {
        if ( manager.previous == this )
          manager.previous = previousEntry;
        if ( manager.next == this )
          manager.next = te->next;
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

  if ( !manager.servicing ) 
    Task::exitCritical();

  return CONTROLLER_OK;
}


//#define TIMER_CYCLES_PER_MS 48
//
//struct Timer_ Timer;
//
//static int Timer_Init( void );
//static int Timer_Deinit( void );
////static int Timer_GetCount( void );
//static int Timer_GetTimeTarget( void );
//static int Timer_GetTime( void );
//static void Timer_SetTimeTarget( int );
//static void Timer_Enable( void );
//
//void Timer_Isr( void );
//
///** \defgroup Timer Timer
//* The Timer subsystem provides a timer in a millisecond timeframe.
//  For higher resolution timing, check the \ref FastTimer
//
//  The Timer subsystem is based on a collection of \b TimerEntries.  To start a new timer, create a new
//  \b TimerEntry structure, initialize it with Timer_InitializeEntry( ), and start it with Timer_Set( ).
//
//  There are currently a couple of limitations to the Timer system:
//  - In your callback function, you must not sleep or make any FreeRTOS-related calls.
//  - To modify an existing TimerEntry, you must cancel the timer with Timer_Cancel( ), and reinitialize the TimerEntry.
//
//  \todo Allow the timer callbacks to cooperate with the \ref RTOS
//  \todo Allow existing timer entries to be modified (repeat or not, modify the period, etc.)
//* \ingroup Core
//* @{
//*/
//
//
///** @}
//*/
//
// Enable the timer.  Disable is performed by the ISR when timer is at an end
void Timer::enable( )
{
  // Enable the device
  // AT91C_BASE_TC0->TC_CCR = AT91C_TC_SWTRG;
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
  manager.running = true;
}

int Timer::getTimeTarget( )
{
  return AT91C_BASE_TC0->TC_RC;
}

int Timer::getTime( )
{
  return AT91C_BASE_TC0->TC_CV;
}

void Timer::setTimeTarget( int target )
{
  AT91C_BASE_TC0->TC_RC = ( target < 0xFFFF ) ? target : 0xFFFF;
}

int Timer::managerInit(int timerindex)
{
  unsigned int channel_id;
  switch(timerindex)
  {
    case 1:
      manager.tc = AT91C_BASE_TC1;
      channel_id = AT91C_ID_TC1;
      break;
    case 2:
      manager.tc = AT91C_BASE_TC2;
      channel_id = AT91C_ID_TC2;
      break;
    default:
      manager.tc = AT91C_BASE_TC0;
      channel_id = AT91C_ID_TC0;
      break;
  }
  
  manager.first = NULL;
  manager.count = 0;
  manager.jitterTotal = 0;
  manager.jitterMax = 0;  
  manager.jitterMaxAllDay = 0;
  manager.running = false;
  manager.servicing = false;

  // Configure TC by enabling PWM clock
	AT91C_BASE_PMC->PMC_PCER = 1 << channel_id;
                                    
  unsigned int mask;
  mask = 0x1 << channel_id;
   
  /* Disable the interrupt on the interrupt controller */
  AT91C_BASE_AIC->AIC_IDCR = mask;

  /* Save the interrupt handler routine pointer */	
  AT91C_BASE_AIC->AIC_SVR[ channel_id ] = (unsigned int)Timer_Isr;

  /* Store the Source Mode Register */
  // 4 PRIORITY is random
  AT91C_BASE_AIC->AIC_SMR[ channel_id ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 4  ;
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
  manager.tc->TC_CMR = AT91C_TC_CLKS_TIMER_DIV5_CLOCK | AT91C_TC_CPCTRG;
                   
  // Only really interested in interrupts when the RC happens
  manager.tc->TC_IDR = 0xFF; 
  manager.tc->TC_IER = AT91C_TC_CPCS; 

  // load the RC value with something
  manager.tc->TC_RC = 0xFFFF;

  // Enable the interrupt
  AT91C_BASE_AIC->AIC_IECR = mask;

  return CONTROLLER_OK;
}



