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
	timer.h

  MakingThings
*/

#ifndef TIMER_H
#define TIMER_H

int Timer_SetActive( bool active );
int Timer_GetActive( void );

typedef struct TimerEntryS
{
  void (*callback)( int id );
  short id;
  int   timeCurrent;
  int   timeInitial;
  bool  repeat;
  struct TimerEntryS* next;
} TimerEntry;

void Timer_InitializeEntry( TimerEntry* timerEntry, void (*timer_callback)( int id ), int id, int timeMs, bool repeat );

int Timer_Set( TimerEntry* timerEntry );
int Timer_Cancel( TimerEntry* timerEntry );

#endif
