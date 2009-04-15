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

#ifndef RTOS_H
#define RTOS_H

#include "types.h"
#include "projdefs.h"
#include "portmacro.h"
#include "queue.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

typedef void (TaskLoop)(void*);

class Task
{
public:
  Task( TaskLoop loop, const char* name, int stackDepth, void* params, int priority );
  Task( void* taskPtr );
  Task operator=(const Task t);
  ~Task( );
  static void sleep( int ms );
  static void yield( );
  static void enterCritical( );
  static void exitCritical( );
  int getRemainingStack( );
  int getPriority( );
  void setPriority( int priority );
  int getIDNumber( );
  char* getName( );
  int getStackAllocated( );
  Task getNext( );
private:
  void* _task;
  bool observer; // flag to signify that only tasks created with the full constructor should delete the internal task on deletion
};

class RTOS
{
public:
  static Task getTaskByName( const char* name );
  static Task getTaskByID( int id );
  static Task getCurrentTask( );
  static int numberOfTasks( );
  static int topTaskPriority( );
  static int ticksSinceBoot( );
};

class Queue
{
public:
  Queue( uint length, uint itemSize );
  ~Queue( );
  
  int send( void* itemToQueue, int timeout );
  int receive( void* buffer, int timeout );
  int msgsAvailable( );
  int sendFromISR( void* itemToSend, int taskPreviouslyWoken );
  int sendToFrontFromISR( void* itemToSend, int taskPreviouslyWoken );
  int sendToBackFromISR( void* itemToSend, int taskPreviouslyWoken );
  int receiveFromISR( void* buffer, long* taskWoken );
private:
  void* _q;
};

class Semaphore
{
public:
  Semaphore( );
  ~Semaphore( );
  int take( int timeout = -1 );
  int give( );
  int giveFromISR( int taskPreviouslyWoken );
private:
  void* _sem;
};

#endif // RTOS__H
