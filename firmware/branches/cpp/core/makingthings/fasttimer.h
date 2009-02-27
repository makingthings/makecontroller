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

#ifndef FASTTIMER_H
#define FASTTIMER_H

#include "AT91SAM7X256.h"
#define FASTTIMER_COUNT 8
#define FASTTIMER_MARGIN 2
#define FASTTIMER_MAXCOUNT 0xFF00
#define FASTTIMER_MINCOUNT 20

typedef void (*FastTimerHandler)( int id );

class FastTimer
{
public:
  FastTimer(int timer = 2);
  ~FastTimer();
  void setHandler(FastTimerHandler handler, int id, int micros, bool repeat = true);
  int start( );
  int stop( );

protected:
  short id;
  int   timeCurrent;
  int   timeInitial;
  bool  repeat;
  FastTimerHandler callback;
  FastTimer* next;
  static bool manager_init; // has the manager been set up?

  int getTimeTarget( );
  int getTime( );
  void setTimeTarget( int target );
  int managerInit(int timerindex);
  void enable( );

  friend void FastTimer_Isr( );

  typedef struct
  {
    char users;
    short count;
  
    int jitterTotal;
    int jitterMax;
    int jitterMaxAllDay;
  
    char running;
    char servicing;
  
    int nextTime;
    int temp;
    
    FastTimer* first;
    FastTimer* next;
    FastTimer* previous;
    FastTimer* lastAdded;
    AT91S_TC* tc;
  } Manager;
  static Manager manager;
};

void DisableFIQFromThumb( void );
void EnableFIQFromThumb( void );

#endif
