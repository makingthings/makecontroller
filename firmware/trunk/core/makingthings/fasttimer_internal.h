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

/*
	timer_internal.h

  timer Internal headers

  MakingThings
*/

#ifndef FASTTIMER_INTERNAL_H
#define FASTTIMER_INTERNAL_H

#include "io.h"

// Uncomment this to put a pulse out on the specified input when the FastIRQ routine is running
// Note that this make timing a bit more rough
// #define FASTIRQ_MONITOR_IO IO_PA04
// Uncomment this to track some stats
// #define FASTIRQ_STATS      1

#include "types.h"

#define FASTTIMER_COUNT 8
#define FASTTIMER_MARGIN 2
#define FASTTIMER_MAXCOUNT 0xFF00
#define FASTTIMER_MINCOUNT 20

struct FastTimer_
{
  char users;
  short count;

  int jitterTotal;
  int jitterMax;
  int jitterMaxAllDay;

  int durationTotal;
  int durationMax;
  int durationMaxAllDay;

  char running;
  char servicing;

  int nextTime;
  int temp;
  
  FastTimerEntry* first;
  FastTimerEntry* next;
  FastTimerEntry* previous;
  FastTimerEntry* lastAdded;
};

#endif
