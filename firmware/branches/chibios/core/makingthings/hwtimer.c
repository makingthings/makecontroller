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
#include "hwtimer.h"
#include "error.h"
#include "at91lib/aic.h"
#include <ch.h>

#define TIMER_CYCLES_PER_MS 48

struct HwTimerManager
{
  unsigned int timer_count; // how many timers have been created?
  unsigned int channel_id;
  short count;

#ifdef HWTIMER_STATS
  int jitterTotal;
  int jitterMax;
  int jitterMaxAllDay;
#endif

  char running;
  char servicing;

  int nextTime;
  int temp;

  HwTimer* first;
  HwTimer* next;
  HwTimer* previous;
  HwTimer* lastAdded;
  AT91S_TC* tc;
};

static struct HwTimerManager manager;

static int  hwtimerGetTimeTarget(void);
static int  hwtimerGetTime(void);
static void hwtimerSetTimeTarget( int target );
static void hwtimerEnable(void);

/**
  Start a timer.
  Specify if you'd like the timer to repeat and, if so, the interval at which 
  you'd like it to repeat.  If you have set up a handler with setHandler() then your 
  handler function will get called at the specified interval.  If the timer is already
  running, this will reset it.
  
  @param millis The number of milliseconds 
  @param repeat Whether or not to repeat - true by default.
*/
int hwtimerStart( HwTimer* hwt, int millis, bool repeat )
{
  hwt->timeCurrent = 0;
  hwt->timeInitial = millis * TIMER_CYCLES_PER_MS;
  hwt->repeat = repeat;
  hwt->next = NULL;

  // this could be a lot smarter - for example, modifying the current period?
  if ( !manager.servicing ) 
    chSysLock();

  if ( !manager.running ) {
//    Timer_SetActive( true );
    hwtimerSetTimeTarget( hwt->timeInitial );
    hwtimerEnable();
  }  

  // Calculate how long remaining
  int target = hwtimerGetTimeTarget();
  int timeCurrent = hwtimerGetTime();
  int remaining = target - timeCurrent;

  // Get the entry ready to roll
  hwt->timeCurrent = hwt->timeInitial;

  // Add entry
  HwTimer* first = manager.first;
  manager.first = hwt;
  hwt->next = first;

  // Are we actually servicing an interrupt right now?
  if ( !manager.servicing ) {
    // No - so does the time requested by this new timer make the time need to come earlier?
    if ( hwt->timeCurrent < ( remaining - TIMER_MARGIN ) ) {
      // Damn it!  Reschedule the next callback
      hwtimerSetTimeTarget( target - ( remaining - hwt->timeCurrent ));
    }
    else {
      // pretend that the existing time has been with us for the whole slice so that when the 
      // IRQ happens it credits the correct (reduced) time.
      hwt->timeCurrent += timeCurrent;
    }
  }
  else {
    // Yep... we're servicing something right now

    // Make sure the previous pointer is OK.  This comes up if we were servicing the first item
    // and it subsequently wants to delete itself, it would need to alter the next pointer of the 
    // the new head... err... kind of a pain, this
    if ( manager.previous == NULL )
      manager.previous = hwt;

    // Need to make sure that if this new time is the lowest yet, that the IRQ routine 
    // knows that.  Since we added this entry onto the beginning of the list, the IRQ
    // won't look at it again
    if ( manager.nextTime == -1 || manager.nextTime > hwt->timeCurrent )
      manager.nextTime = hwt->timeCurrent;
  }

  if ( !manager.servicing ) 
    chSysUnlock();

  return CONTROLLER_OK;
}

/**	
  Stop a timer.
  @return 0 on success.
*/
int hwtimerStop( HwTimer* hwt )
{
  if ( !manager.servicing ) 
    chSysLock();

  // Look through the running list - clobber the entry
  HwTimer* te = manager.first;
  HwTimer* previousEntry = NULL;
  while ( te != NULL ) {
    // check for the requested entry
    if ( te == hwt ) {
      // remove the entry from the list
      if ( te == manager.first )
        manager.first = te->next;
      else
        previousEntry->next = te->next;
      
      // make sure the in-IRQ pointers are all OK
      if ( manager.servicing ) {
        if ( manager.previous == hwt )
          manager.previous = previousEntry;
        if ( manager.next == hwt )
          manager.next = te->next;
      }

      // update the pointers - leave previous where it is
      te = te->next;
    }
    else {
      previousEntry = te;
      te = te->next;
    }
  }

  if ( !manager.servicing ) 
    chSysUnlock();

  return CONTROLLER_OK;
}

// Enable the timer.  Disable is performed by the ISR when timer is at an end
void hwtimerEnable( )
{
  manager.tc->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
  manager.running = true;
}

int hwtimerGetTimeTarget( )
{
  return manager.tc->TC_RC;
}

int hwtimerGetTime( )
{
  return manager.tc->TC_CV;
}

void hwtimerSetTimeTarget( int target )
{
  manager.tc->TC_RC = ( target < 0xFFFF ) ? target : 0xFFFF;
}

static void hwtimerServeInterrupt( void )
{
  int status = manager.tc->TC_SR;
  if ( status & AT91C_TC_CPCS ) {
    manager.servicing = true;

    manager.count++;
    int jitter = manager.tc->TC_CV;

#ifdef HWTIMER_STATS
    manager.jitterTotal += jitter;
    if ( jitter > manager.jitterMax )
      manager.jitterMax = jitter;
    if ( jitter > manager.jitterMaxAllDay )
      manager.jitterMaxAllDay = jitter;
#endif

    // Run through once to make the callback calls
    HwTimer* timer = manager.first;
    manager.next = NULL;
    manager.previous = NULL;
    manager.nextTime = -1;
    while ( timer != NULL ) {
      manager.next = timer->next;
      timer->timeCurrent -= (manager.tc->TC_RC + manager.tc->TC_CV);
      if ( timer->timeCurrent <= 0 ) {
        if ( timer->repeat )
          timer->timeCurrent += timer->timeInitial;
        else {
          // remove it if necessary (do this first!)
          if ( manager.previous == NULL )
            manager.first = manager.next;
          else
            manager.previous->next = manager.next;
        }

        if ( timer->callback != NULL ) {
          // in this callback, the callee is free to add and remove any members of this list
          // which might effect the first, next and previous pointers
          // so don't assume any of those local variables are good anymore
          (*timer->callback)( timer->id );
        }

        // Assuming we're still on the list (if we were removed, then re-added, we'd be on the beggining of
        // the list with this task already performed) see whether our time is the next to run
        if ( ( manager.previous == NULL && manager.first == timer ) ||
             ( manager.previous != NULL && manager.previous->next == timer ) )
        {
          if ( manager.nextTime == -1 || timer->timeCurrent < manager.nextTime )
            manager.nextTime = timer->timeCurrent;
        }
      }
      else
        manager.previous = timer;

      timer = manager.next;
    }

    if ( manager.first != NULL ) {
      // Add in whatever we're at now
      manager.nextTime += manager.tc->TC_CV;
      // Make sure it's not too big
      if ( manager.nextTime > 0xFFFF )
        manager.nextTime = 0xFFFF;
      manager.tc->TC_RC = manager.nextTime;
    }
    else {
      manager.tc->TC_CCR = AT91C_TC_CLKDIS;
      manager.running = false;
    }

    jitter = manager.tc->TC_CV;
    manager.servicing = false;
  }

  AT91C_BASE_AIC->AIC_EOICR = 0; // Clear AIC to complete ISR processing
}

CH_IRQ_HANDLER( hwtimerIsr ) {
  CH_IRQ_PROLOGUE();
  hwtimerServeInterrupt();
  CH_IRQ_EPILOGUE();
}

int hwtimerInit(int channel)
{
  switch(channel)
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
#ifdef HWTIMER_STATS
  manager.jitterTotal = 0;
  manager.jitterMax = 0;  
  manager.jitterMaxAllDay = 0;
#endif
  manager.running = false;
  manager.servicing = false;

  unsigned int mask = 0x1 << manager.channel_id;
	AT91C_BASE_PMC->PMC_PCER = mask; // power up the selected channel
   
  // disable the interrupt, configure interrupt handler and re-enable
	AIC_ConfigureIT(manager.channel_id, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 4, hwtimerIsr);

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
  manager.tc->TC_RC = 0xFFFF; // load the RC value with something

  AIC_EnableIT(manager.channel_id);

  return CONTROLLER_OK;
}

void hwtimerDeinit( )
{
  AT91C_BASE_AIC->AIC_IDCR = manager.channel_id; // disable the interrupt
  AT91C_BASE_PMC->PMC_PCDR = manager.channel_id; // power down
}



