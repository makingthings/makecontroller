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

typedef void (*FastTimerHandler)(int id);

typedef struct FastTimer_t {
  FastTimerHandler handler;
  short id;
  int   timeCurrent;
  int   timeInitial;
  bool  repeat;
  struct FastTimer_t* next;
} FastTimer;


#ifdef __cplusplus
extern "C" {
#endif
void fasttimerInit(int channel);
void fasttimerDeinit(void);
int  fasttimerStart(FastTimer *ft, int micros, bool repeat);
void fasttimerStop(FastTimer *ft);
#ifdef __cplusplus
}
#endif

#endif
