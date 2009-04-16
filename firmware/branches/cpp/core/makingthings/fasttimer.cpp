/*********************************************************************************

 Copyright 2006-2009 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#include "fasttimer.h"
#include "error.h"
#include "rtos.h"

#define FAST_TIMER_CYCLES_PER_US 6

// statics
FastTimer::Manager FastTimer::manager;
// extern
void FastTimer_Isr( );

/**	
  Create a new FastTimer

  Note - the timer index selected will be used for all subsequent timers created.
  @param timer The hardware timer to use - valid options are 0, 1 and 2.  2 is the default.
*/
FastTimer::FastTimer( int timer )
{
  if(!manager.timer_count++)
    managerInit(timer);
}

FastTimer::~FastTimer()
{
  stop();
  if(--manager.timer_count == 0)
    managerDeinit( );
}

/**	
  Register a handler for this timer.
  Specify a handler function that should be called back at
  an interval specified in start().  If you have a handler registered with
  more than one timer, use the \b id to distinguish which timer is calling
  it at a given time.

  The longest period for a fast timer entry is 2^32 / 1000000 = 4294s.

  \par Example
  \code
  FastTimer myTimer;
  myTimer.setHandler( myHandler, 0 );
  myTimer.start( 250 ); // start our timer

  void myHandler( int id ) // our code that will get called by the timer every 250 microseconds.
  {
    // do something here
  }
  \endcode
*/
void FastTimer::setHandler( FastTimerHandler handler, int id )
{
  callback = handler;
  this->id = id;
}

/**
  Sets the requested entry to run.
  This routine adds the entry to the running queue and then decides if it needs
  to start the timer (if it's not running) or alter the timer's clock for a shorter
  period.
  @param micros The interval (in microseconds) at which the handler should be called.
  @param repeat Whether to call the handler repeatedly.  True by default.

  \b Example
  \code
  FastTimer t;
  t.setHandler( myHandler, 345 );
  t.start(250); // call myHandler every 250 microseconds
  \endcode
  */
int FastTimer::start( int micros, bool repeat )
{
  timeCurrent = 0;
  timeInitial = micros * FAST_TIMER_CYCLES_PER_US;
  this->repeat = repeat;
  next = NULL;
  // this could be a lot smarter - for example, modifying the current period?
  if ( !manager.servicing ) 
    Task::enterCritical();

  if ( !manager.running )
  {
    setTimeTarget( this->timeInitial );
    enable();
  }  

  int target = getTimeTarget();
  int remaining = target - getTime(); // Calculate how long remaining
  this->timeCurrent = this->timeInitial; // Get the entry ready to roll

  // Add entry
  FastTimer* first = manager.first;
  manager.first = this;
  this->next = first;

  // Are we actually servicing an interupt right now?
  if ( !manager.servicing )
  {
    // No - so does the time requested by this new timer make the time need to come earlier?
    if ( this->timeCurrent < ( remaining - FASTTIMER_MARGIN ) )
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
  Change the requeted time of an entry.
  This must only be called within a callback caused by the Entry specified or when the
  entry is not being used.  If you need to change the duration of a timer, you need to cancel it
  and re-add it, or alter the time inside a callback.

  @param micros The time in microseconds desired for the callback.
*/
bool FastTimer::setPeriod( int micros )
{
  int time = micros * FAST_TIMER_CYCLES_PER_US;
  timeCurrent = time;
  timeInitial = time;
  return true;
}

/**
  Stops a fast timer.
  You should always stop the timer, then start() it again
  if you need to change its interval.

  \code
  FastTimer t;
  t.start(250);
  t.stop();
  \endcode
*/
void FastTimer::stop( )
{
  if ( !manager.servicing ) 
    Task::enterCritical();

  // Look through the running list - clobber the entry
  FastTimer* te = manager.first;
  FastTimer* previousEntry = NULL;
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
}

// Enable the timer.  Disable is performed by the ISR when timer is at an end
void FastTimer::enable( )
{
  // Enable the device
  // AT91C_BASE_TC0->TC_CCR = AT91C_TC_SWTRG;
  manager.tc->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
  manager.running = true;
}

int FastTimer::getTimeTarget( )
{
  return manager.tc->TC_RC;
}

int FastTimer::getTime( )
{
  return manager.tc->TC_CV;
}

void FastTimer::setTimeTarget( int target )
{
  manager.tc->TC_RC = ( target < FASTTIMER_MAXCOUNT ) ? target : FASTTIMER_MAXCOUNT;
}

int FastTimer::managerInit(int timer)
{
  switch(timer)
  {
    case 0:
      manager.tc = AT91C_BASE_TC0;
      manager.channel_id = AT91C_ID_TC0;
      break;
    case 1:
      manager.tc = AT91C_BASE_TC1;
      manager.channel_id = AT91C_ID_TC1;
      break;
    default:
      manager.tc = AT91C_BASE_TC2;
      manager.channel_id = AT91C_ID_TC2;
      break;
  }
  
  manager.first = NULL;
  manager.count = 0;
  manager.jitterTotal = 0;
  manager.jitterMax = 0;  
  manager.jitterMaxAllDay = 0;
  manager.running = false;
  manager.servicing = false;
                                    
  unsigned int mask = 0x1 << manager.channel_id;
  AT91C_BASE_PMC->PMC_PCER = mask;

  // Disable the interrupt, configure it, reenable it
  AT91C_BASE_AIC->AIC_IDCR = mask;
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_FIQ ] = (unsigned int)FastTimer_Isr;
  AT91C_BASE_AIC->AIC_SMR[ manager.channel_id ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 7  ;
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
  manager.tc->TC_CMR = AT91C_TC_CLKS_TIMER_DIV2_CLOCK |  AT91C_TC_CPCTRG;
                   
  // Only interested in interrupts when the RC happens
  manager.tc->TC_IDR = 0xFF; 
  manager.tc->TC_IER = AT91C_TC_CPCS; 
  manager.tc->TC_RC = FASTTIMER_MAXCOUNT; // load the RC value with something
  AT91C_BASE_AIC->AIC_FFER = 0x1 << manager.channel_id; // Make it fast forcing
  AT91C_BASE_AIC->AIC_IECR = mask; // Enable the interrupt
  manager.tc->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG; // Enable the device

  /// Finally, prep the IO flag if it's being used
#ifdef FASTIRQ_MONITOR_IO
    Io_Start( FASTIRQ_MONITOR_IO, true );
    Io_PioEnable( FASTIRQ_MONITOR_IO );
    Io_SetOutput( FASTIRQ_MONITOR_IO );
#endif

  return CONTROLLER_OK;
}

void FastTimer::managerDeinit( )
{
  AT91C_BASE_AIC->AIC_IDCR = manager.channel_id; // disable the interrupt
  AT91C_BASE_PMC->PMC_PCDR = manager.channel_id; // power down
}
