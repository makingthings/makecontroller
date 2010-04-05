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

#ifndef WATCHDOG_H
#define WATCHDOG_H

/**
  The Watchdog timer can reset the board in the event that it's not behaving as expected.
  
  \section Usage
  If you don't need the watchdog, simply turn it off by calling
  \code Watchdog::disable() \endcode

  If you want to use it, specify the length of the countdown to enable() and then
  periodically call restart() to restart the countdown.  If the countdown ever gets
  all the way to zero, the board will reset.  

  \b Example
  \code
  Watchdog::enable(2000); // start the countdown

  void MyTask()
  {
    while(true)
    {
      if( everything_is_normal )
        Watchdog::restart();
      // if everything is not normal for long enough,
      // the countdown will expire and the board will reset
    }
  }
  \endcode
*/

#ifdef __cplusplus
extern "C" {
#endif
void watchdogEnable(int millis);
void watchdogReset(void);
void watchdogDisable(void);
#ifdef __cplusplus
}
#endif
#endif // WATCHDOG_H


