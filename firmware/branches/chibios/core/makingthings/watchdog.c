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
  Enable the watchdog timer.
  Specify the number of milliseconds that the watchdog should wait before 
  resetting.  Remember that you enable() or disable() can only be called
  once until the processor is reset again.

  The maximum countdown length is 16 seconds.
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
  Restart the watchdog timer countdown.
  Call enable() first, and then call this occasionally to restart
  the watchdog countdown so that it doesn't reset.
*/
void watchdogReset()
{
  AT91C_BASE_WDTC->WDTC_WDCR = WATCHDOG_KEY | AT91C_WDTC_WDRSTT;
}

/**
  Disable the watchdog timer.
  Turn the watchdog off completely if you don't need it.
  
  This is done by default, unless you define WATCHDOG_ENABLE in your config.h
*/
void watchdogDisable()
{
  AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS;
}

#endif // WATCHDOG_ENABLE

