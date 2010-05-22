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

#ifndef TIMER_H
#define TIMER_H

#include "ch.h"
#include "core.h"

typedef VirtualTimer Timer;
typedef void(*TimerHandler)(void);

/**
  Return the current system time.
  This represents the number of ticks since the system started up.

  \b Example
  \code
  // do some rough timing measurements
  int start = timerNow();
  // ... do something here
  int elapsed = timerNow() - start;
  \endcode
  \ingroup timer
*/
#define timerNow() chTimeNow()

#ifdef __cplusplus
extern "C" {
#endif
void timerGo(Timer* timer, int millis, TimerHandler handler);
void timerCancel(Timer* timer);
#ifdef __cplusplus
}
#endif
#endif // HWTIMER_H
