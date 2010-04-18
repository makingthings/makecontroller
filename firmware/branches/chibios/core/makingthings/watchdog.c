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

#include "config.h"
#ifdef WATCHDOG_ENABLE

#include "watchdog.h"
#include "at91sam7.h"

#define WATCHDOG_KEY (0xA5 << 24)

/**
  \defgroup Watchdog
  The Watchdog timer resets the board in the event that it's not behaving as expected.
  This is more robust than using other kinds of timers, because in the worst case, when
  your app has malfunctioned, it can still reset since it's not relying on your app to
  actually be running.

  \section Usage
  The watchdog is disabled by default.  If you want to make use of it, add the following
  line to your config.h file: \code #define WATCHDOG_ENABLE \endcode

  If you want to use it, specify the length of the countdown to watchdogEnable() and then
  periodically call watchdogReset() to reset the countdown.  If the countdown ever gets
  all the way to zero, the board will reset.

  \b Example
  \code
  watchdogEnable(2000); // start the countdown

  void MyTask()
  {
    while (1) {
      if (everything_is_normal()) {
        watchdogReset();
      }
      else {
        // if things are not normal, the timer is not reset and will eventually expire
      }
    }
  }
  \endcode
  \ingroup Core
  @{
*/

/**
  Enable the watchdog timer.
  Specify the number of milliseconds that the watchdog should wait before 
  resetting.  Remember that watchdogEnable() or watchdogDisable() can only be called
  once until the processor is reset again.

  The maximum countdown length is 16 seconds (16000 ms).
  @param millis The number of milliseconds in which a reset will occur.
*/
void watchdogEnable(int millis)
{
  int period = (millis * 256) / 1000;
  AT91C_BASE_WDTC->WDTC_WDMR =  AT91C_WDTC_WDRSTEN |        // enable reset on timeout
                                AT91C_WDTC_WDDBGHLT |       // respect debug mode
                                AT91C_WDTC_WDIDLEHLT |      // respect idle mode
                                ((period << 16 ) & AT91C_WDTC_WDD) | // delta is as wide as the period, so we can restart anytime
                                (period & AT91C_WDTC_WDV);  // set the period
}

/**
  Reset the watchdog timer countdown.
  Call watchdogEnable() first, and then call this occasionally to reset
  the watchdog countdown so that it doesn't expire.
*/
void watchdogReset()
{
  AT91C_BASE_WDTC->WDTC_WDCR = WATCHDOG_KEY | AT91C_WDTC_WDRSTT;
}

/**
  Disable the watchdog timer.
  Turn the watchdog off completely if you don't need it.
  
  If \b WATCHDOG_ENABLE is not defined in your config.h, this is done automatically.
*/
void watchdogDisable()
{
  AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS;
}

/** @}
*/

#endif // WATCHDOG_ENABLE

