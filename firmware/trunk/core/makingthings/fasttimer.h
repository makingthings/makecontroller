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

#ifndef FASTTIMER_H
#define FASTTIMER_H

int FastTimer_SetActive( bool active );
int FastTimer_GetActive( void );

typedef struct FastTimerEntryS
{
  void (*callback)( int id );
  short id;
  int   timeCurrent;
  int   timeInitial;
  bool  repeat;
  struct FastTimerEntryS* next;
} FastTimerEntry;

void FastTimer_InitializeEntry( FastTimerEntry* fastTimerEntry, void (*timer_callback)( int id ), int id, int timeUs, bool repeat );
void FastTimer_SetTime( FastTimerEntry* timerEntry, int timeUs );

int FastTimer_Set( FastTimerEntry* fastTimerEntry );
int FastTimer_Cancel( FastTimerEntry* fastTimerEntry );

void DisableFIQFromThumb( void );
void EnableFIQFromThumb( void );

#endif
