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
	RTOS.H

  MakingThings
*/

#ifndef RTOS_H
#define RTOS_H

#include "types.h"

void  Sleep( int timems );
void* TaskCreate(  void (taskCode)(void*), char* name, int stackDepth, void* parameters, int priority );
void  TaskYield( void );
void  TaskDelete( void* task );
void  TaskEnterCritical( void );
void  TaskExitCritical( void );
int TaskGetRemainingStack( void* task );
void* getTaskByName( char *taskName );
void* getTaskByID( int taskID );
int TaskGetPriority( void* task );
void TaskSetPriority( void* task, int priority );
int TaskGetIDNumber( void* task );
char* TaskGetName( void* task );
int TaskGetStackAllocated( void* task );
void* TaskGetCurrent( void );
void* TaskGetNext( void* task );
int GetNumberOfTasks( void );
int TaskGetTopPriorityUsed( void );
int TaskGetTickCount( void );

void* Malloc( int size );
void* MallocWait( int size, int interval );
void Free( void* memory );

void* QueueCreate( uint length, uint itemSize );
int QueueSendToFront( void* queue, void* itemToQueue, int msToWait );
int QueueSendToBack( void* queue, void* itemToQueue, int msToWait );
int QueueReceive( void* queue, void* buffer, int msToWait );
int QueueMessagesWaiting( void* queue );
void QueueDelete( void* queue );
int QueueSendToFrontFromISR( void* queue, void* itemToSend, int taskPreviouslyWoken );
int QueueSendToBackFromISR( void* queue, void* itemToSend, int taskPreviouslyWoken );
int QueueReceiveFromISR( void* queue, void* buffer, long* taskWoken );

void* SemaphoreCreate( void );
void* MutexCreate( void );
int SemaphoreTake( void* semaphore, int blockTime );
int SemaphoreGive( void* semaphore );
int SemaphoreGiveFromISR( void* semaphore, int taskPreviouslyWoken );

#endif
