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

#ifndef TIMER_H
#define TIMER_H

#include "AT91SAM7X256.h"

#define TIMER_COUNT 8
#define TIMER_MARGIN 2

typedef void (*TimerHandler)( int id );

class Timer
{
public:
  Timer(int timer = 0);
  ~Timer();
  void setHandler(TimerHandler handler, int id, int millis, bool repeat = true);
  int start( );
  int stop( );

protected:
  short id;
  int   timeCurrent;
  int   timeInitial;
  bool  repeat;
  TimerHandler callback;
  Timer* next;
  static bool manager_init; // has the manager been set up?

  int getTimeTarget( );
  int getTime( );
  void setTimeTarget( int target );
  int managerInit(int timerindex);
  void enable( );

  friend void Timer_Isr( );

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
    
    Timer* first;
    Timer* next;
    Timer* previous;
    Timer* lastAdded;
    AT91S_TC* tc;
  } Manager;
  static Manager manager;
};

#endif
