
#include "timer.h"

/**
  \defgroup timer Timer
  Provides a timer in a millisecond timeframe.

  \section Usage
  Timers can be used to schedule an event some time in the future by specifying
  the amount of time, and a handler function to be called at that time.

  TimerCallback functions must have the format \code void myHandler(void* p); \endcode

  Timers are single shot, meaning that if you want them to trigger repeatedly
  you'll need to reload them from within the handler, as shown in the example below.

  \code
  Timer mytimer;
  // first, we'll create a function that will get called by the timer.
  int count = 0; // our current count
  void myHandler(void* p)
  {
    count++;
    if (count > 500) {
      // then do something here
      count = 0; // and reset the count
    }
    timerGo(&mytimer, 250, myHandler); // reload the timer
  }
  
  // schedule the timer for the first time
  timerGo(&mytimer, 250, myHandler);
  \endcode
  
  \section Note
  TimerHandlers are called in an interrupt context.  This means you can't sleep(), or
  do anything that might take a long time.  Typical actions that might take place in
  a TimerHandler are things like updating variables, counts or turning
  pins on/off.
  
  For higher resolution timing, check the \ref FastTimer
  \ingroup Core
  @{
*/

/**
  Schedule a timer.
  @param timer The timer to use.
  @param millis How long to wait before calling the handler.
  @param handler The handler function to be called when this timer triggers.
*/
void timerGo(Timer* timer, int millis, TimerHandler handler)
{
  chVTSetI(timer, MS2ST(millis), handler, 0);
}

/**
  Cancel a timer in progress.
  If the timer is not currently active, this has no effect.
  @param timer The timer to cancel.
*/
void timerCancel(Timer* timer)
{
  if (chVTIsArmedI(timer)) {
    chVTResetI(timer);
  }
}

/** @} */

