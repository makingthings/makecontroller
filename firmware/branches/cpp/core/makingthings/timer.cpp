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

#include "types.h"
#include "timer.h"
#include "error.h"
#include "rtos.h"

#define TIMER_CYCLES_PER_MS 48

// statics
Timer::Manager Timer::manager;
// extern
void TimerIsr_Wrapper( );

/**
  Make a new timer.
  Note - the timer index selected will be used for all subsequent timers created.
  @param timer The hardware timer to use - valid options are 0, 1 and 2.  0 is the default.
*/
Timer::Timer(int timer)
{
  if(!manager.timer_count++)
    managerInit(timer);
}

Timer::~Timer()
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

  @param handler A function of the form \code void myHandler( int id ); \endcode
  @param id An id that will be passed into your handler, telling it which timer is calling it.
*/
void Timer::setHandler(TimerHandler handler, int id )
{
  callback = handler;
  this->id = id;
}

/**
  Start a timer.
  Specify if you'd like the timer to repeat and, if so, the interval at which 
  you'd like it to repeat.  If you have set up a handler with setHandler() then your 
  handler function will get called at the specified interval.  If the timer is already
  running, this will reset it.
  
  @param millis The number of milliseconds 
  @param repeat Whether or not to repeat - true by default.
*/
int Timer::start( int millis, bool repeat )
{
  timeCurrent = 0;
  timeInitial = millis * TIMER_CYCLES_PER_MS;
  this->repeat = repeat;
  next = NULL;

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
  Stop a timer.
  @return 0 on success.
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

// Enable the timer.  Disable is performed by the ISR when timer is at an end
void Timer::enable( )
{
  // Enable the device
  // AT91C_BASE_TC0->TC_CCR = AT91C_TC_SWTRG;
  manager.tc->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
  manager.running = true;
}

int Timer::getTimeTarget( )
{
  return manager.tc->TC_RC;
}

int Timer::getTime( )
{
  return manager.tc->TC_CV;
}

void Timer::setTimeTarget( int target )
{
  manager.tc->TC_RC = ( target < 0xFFFF ) ? target : 0xFFFF;
}

int Timer::managerInit(int timerindex)
{
  switch(timerindex)
  {
    case 1:
      manager.tc = AT91C_BASE_TC1;
      manager.channel_id = AT91C_ID_TC1;
      break;
    case 2:
      manager.tc = AT91C_BASE_TC2;
      manager.channel_id = AT91C_ID_TC2;
      break;
    default:
      manager.tc = AT91C_BASE_TC0;
      manager.channel_id = AT91C_ID_TC0;
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
	AT91C_BASE_PMC->PMC_PCER = mask; // power up the selected channel
   
  // disable the interrupt, configure interrupt handler and re-enable
  AT91C_BASE_AIC->AIC_IDCR = mask;
  AT91C_BASE_AIC->AIC_SVR[ manager.channel_id ] = (unsigned int)TimerIsr_Wrapper;
  AT91C_BASE_AIC->AIC_SMR[ manager.channel_id ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 4  ;
  AT91C_BASE_AIC->AIC_ICCR = mask;

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

void Timer::managerDeinit( )
{
  AT91C_BASE_AIC->AIC_IDCR = manager.channel_id; // disable the interrupt
  AT91C_BASE_PMC->PMC_PCDR = manager.channel_id; // power down
}



