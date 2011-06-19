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
#include "ch.h"
#include "at91lib/AT91SAM7X256.h"

#define FASTTIMER_COUNT 8
#define FASTTIMER_MARGIN 2
#define FASTTIMER_MAXCOUNT 0xFF00
#define FASTTIMER_MINCOUNT 20
#define FAST_TIMER_CYCLES_PER_US 6

struct FastTimerManager {
  char users;
  short count;

#ifdef FASTIRQ_STATS
  int jitterTotal;
  int jitterMax;
  int jitterMaxAllDay;
  int durationTotal;
  int durationMax;
  int durationMaxAllDay;
#endif

  char servicing;

  int nextTime;

  AT91S_TC* tc;
  unsigned short channel_id;

  FastTimer* first;
  FastTimer* next;
  FastTimer* previous;
  FastTimer* lastAdded;
};

static struct FastTimerManager manager;

#define fasttimerEnable() (manager.tc->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG)
#define fasttimerTimeTarget() (manager.tc->TC_RC)
#define fasttimerCurrentTime() (manager.tc->TC_CV)
#define fasttimerSetTimeTarget(target) (manager.tc->TC_RC = (uint16_t)target)
#define fasttimerIsRunning()  (manager.tc->TC_CCR & AT91C_TC_CLKSTA)

static void fasttimerServeInterrupt(void);

/**
  \defgroup fasttimer Fast Timer
  Provides a high resolution timer in a microsecond context.
  
  \section usage Usage
  The interface for the FastTimer is essentially the same as the \ref Timer system, so 
  that's the best place to check for an overview.  

  \section notes Notes
  A few things to be aware of when using FastTimers:
  - In your handler, you must not sleep or make any calls that will take a long time.  You may, however, use
  the Queue and Semaphore calls that end in \b fromISR in order to synchronize with running tasks.
  - To modify an existing FastTimer, stop() it and then start() it again.  Modifying it while running is not recommended.
  - There are 3 identical hardware timers on the Make Controller.  The first FastTimer that you create
  will specify which of them to use, and it will be used for all subsequent fast timers created.  
  If you don't specify a channel, 2 is used which is usually fine.  Specifically, the \ref Timer is on 
  channel 0 by default, so make sure to keep them separate if you're running them at the same time.
  - if you have lots of FastTimers, the timing can start to get a little jittery.  For instance, the \ref Servo and \ref Stepper
  libraries use the FastTimer and they can become a little unstable if too many of them are running at once.
  \ingroup Core
  @{
*/

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
int fasttimerStart(FastTimer *ft, int micros)
{
  ft->timeInitial = micros * FAST_TIMER_CYCLES_PER_US;
  ft->next = NULL;
  // this could be a lot smarter - for example, modifying the current period?
  if (!manager.servicing)
    chSysLock();

  if (!fasttimerIsRunning()) {
    fasttimerSetTimeTarget(ft->timeInitial);
    fasttimerEnable();
  }  

  int target = fasttimerTimeTarget();
  int remaining = target - fasttimerCurrentTime(); // Calculate how long remaining
  ft->timeCurrent = ft->timeInitial; // Get the entry ready to roll

  // Add entry
  FastTimer* first = manager.first;
  manager.first = ft;
  ft->next = first;

  // Are we actually servicing an interrupt right now?
  if (!manager.servicing) {
    // No - so does the time requested by this new timer make the time need to come earlier?
    if (ft->timeCurrent < (remaining - FASTTIMER_MARGIN)) {
      // Damn it!  Reschedule the next callback
      fasttimerSetTimeTarget(target - (remaining - ft->timeCurrent));
    }
    else {
      // pretend that the existing time has been with us for the whole slice so that when the 
      // IRQ happens it credits the correct (reduced) time.
      ft->timeCurrent += ft->timeCurrent;
    }
  }
  else {
    // Yep... we're servicing something right now

    // Make sure the previous pointer is OK.  This comes up if we were servicing the first item
    // and it subsequently wants to delete itself, it would need to alter the next pointer of the 
    // the new head... err... kind of a pain, this
    if (manager.previous == NULL)
      manager.previous = ft;

    // Need to make sure that if this new time is the lowest yet, that the IRQ routine 
    // knows that.  Since we added this entry onto the beginning of the list, the IRQ
    // won't look at it again
    if (manager.nextTime == -1 || manager.nextTime > ft->timeCurrent)
        manager.nextTime = ft->timeCurrent;
  }

  if (!manager.servicing)
    chSysUnlock();

  return CONTROLLER_OK;
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
void fasttimerStop(FastTimer *ft)
{
  if (!manager.servicing)
    chSysLock();

  // Look through the running list - clobber the entry
  FastTimer* te = manager.first;
  FastTimer* previousEntry = NULL;
  while (te != NULL) {
    // check for the requested entry
    if (te == ft) {
      // remove the entry from the list
      if (te == manager.first)
        manager.first = te->next;
      else
        previousEntry->next = te->next;
      
      // make sure the in-IRQ pointers are all OK
      if (manager.servicing) {
        if (manager.previous == ft)
          manager.previous = previousEntry;
        if (manager.next == ft)
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

  if (!manager.servicing)
    chSysUnlock();
}

CH_FAST_IRQ_HANDLER(FiqHandler) {
  fasttimerServeInterrupt();
}

void fasttimerInit(int channel)
{
  switch (channel) {
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
  manager.servicing = false;
#ifdef FASTIRQ_STATS
  manager.jitterTotal = 0;
  manager.jitterMax = 0;  
  manager.jitterMaxAllDay = 0;
#endif
                                    
  unsigned int mask = 0x1 << manager.channel_id;
  if (AT91C_BASE_PMC->PMC_PCSR & mask) // we're already configured on this channel
    return;
  AT91C_BASE_PMC->PMC_PCER = mask;

  // Disable the interrupt, configure it, reenable it
  AT91C_BASE_AIC->AIC_IDCR = mask;
//  AT91C_BASE_AIC->AIC_SVR[AT91C_ID_FIQ] = (unsigned int)fasttimerIsr;
  AT91C_BASE_AIC->AIC_SMR[manager.channel_id] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 7  ;
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
    Io_Start(FASTIRQ_MONITOR_IO, true);
    Io_PioEnable(FASTIRQ_MONITOR_IO);
    Io_SetOutput(FASTIRQ_MONITOR_IO);
#endif
}


//FastTimerEntry* te;
//int jitter;

void fasttimerServeInterrupt()
{
  // only process if RC compare match has happened
  if (manager.tc->TC_SR & AT91C_TC_CPCS) {
    manager.servicing = true;

    //AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKDIS;

    // make sure there's not another IRQ while we're processing
    int timeReference = manager.tc->TC_RC;
    manager.tc->TC_RC = 0xFF00;

#ifdef FASTIRQ_MONITOR_IO
    pinOn(FASTIRQ_MONITOR_IO);
#endif

#ifdef FASTIRQ_STATS
    int startCount = manager.tc->TC_CV;

    // moved outside for debugging
    //int jitter;
    jitter = manager.tc->TC_CV;

    if (++manager.count == 1000) {
      // need to not do division here... takes too long
      //manager.jitterTotal = manager.jitterTotal / manager.count;
      manager.jitterTotal = 0;
      manager.jitterMax = 0;
      // need to not do division here... takes too long
      // manager.durationTotal = manager.durationTotal / manager.count;
      manager.durationTotal = 0;
      manager.durationMax = 0;
      manager.count = 1;
    }

    manager.jitterTotal += jitter;

    if (jitter > manager.jitterMax)
      manager.jitterMax = jitter;
    if (jitter > manager.jitterMaxAllDay)
      manager.jitterMaxAllDay = jitter;
#endif

    // Use this during debuggin
    FastTimer *ftimer = manager.first;
    manager.next = NULL;
    manager.previous = NULL;
    manager.nextTime = 0xFF00;
    int removed = false;
    // timeReference = AT91C_BASE_TC2->TC_CV;

    while (ftimer != NULL) {
      manager.next = ftimer->next;
      ftimer->timeCurrent -= (timeReference + fasttimerCurrentTime());
      if (ftimer->timeCurrent <= FASTTIMER_MINCOUNT) {
        // Watch out for gross errors
        //if ( ftimer->timeCurrent < -FASTTIMER_MINCOUNT)
        //  ftimer->timeCurrent = -FASTTIMER_MINCOUNT;

        if (0) //ftimer->repeat)
          ftimer->timeCurrent += ftimer->timeInitial;
        else {
          // remove it if necessary (do this first!)
          if (manager.previous == NULL)
            manager.first = manager.next;
          else
            manager.previous->next = manager.next;
          removed = true;
        }

        if (ftimer->handler != NULL) {
          // in this callback, the callee is free to add and remove any members of this list
          // which might effect the first, next and previous pointers
          // so don't assume any of those local variables are good anymore
          (*ftimer->handler)(ftimer->id);
        }
      }

      // note that this has to be better than this ultimately - since
      // the callback routine can remove the entry without letting us know
      // at all.
      if (!removed) {
        if (manager.nextTime == -1 || ftimer->timeCurrent < manager.nextTime)
          manager.nextTime = ftimer->timeCurrent;
        manager.previous = ftimer;
      }
      ftimer = manager.next;
    }

    if (manager.first != NULL) {
      // Make sure it's not too big
      if (manager.nextTime > 0xFF00)
        manager.nextTime = 0xFF00;
      // Make sure it's not too small
      if (manager.nextTime < (int)manager.tc->TC_CV + 20)
        manager.nextTime = manager.tc->TC_CV + 20;
      manager.tc->TC_RC = manager.nextTime;
    }
    else {
      manager.tc->TC_CCR = AT91C_TC_CLKDIS;
    }

#ifdef FASTIRQ_STATS
    int duration = manager.tc->TC_CV - startCount;
    manager.durationTotal += duration;
    if (duration > manager.durationMax)
      manager.durationMax = duration;
    if (duration > manager.durationMaxAllDay)
      manager.durationMaxAllDay = duration;
#endif

#ifdef FASTIRQ_MONITOR_IO
    pinOff(FASTIRQ_MONITOR_IO);
#endif

    // AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
    manager.servicing = false;
  }
}

void fasttimerDeinit()
{
  AT91C_BASE_AIC->AIC_IDCR = manager.channel_id; // disable the interrupt
  AT91C_BASE_PMC->PMC_PCDR = manager.channel_id; // power down
}

/** @} */

