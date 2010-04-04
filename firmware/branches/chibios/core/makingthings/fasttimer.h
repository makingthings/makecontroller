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

#ifndef FASTTIMER_H
#define FASTTIMER_H

#include "types.h"

/**
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
*/

typedef void (*FastTimerHandler)( int id );

typedef struct FastTimerEntry_t
{
  FastTimerHandler callback;
  short id;
  int   timeCurrent;
  int   timeInitial;
  bool  repeat;
  struct FastTimerEntry_t* next;
} FastTimer;

void fasttimerInit(int channel);
void fasttimerDeinit(void);
void fasttimerStop(FastTimer *ft);

//void DisableFIQFromThumb( void );
//void EnableFIQFromThumb( void );

#endif
