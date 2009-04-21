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

#ifndef RTOS_H
#define RTOS_H

#include "types.h"
#include "FreeRTOS.h"
#include "projdefs.h"
#include "portmacro.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

/**
  A run loop.
*/
typedef void (TaskLoop)(void*);

/**
  A run loop, or thread, that can run simultaneously with many others.
  
  \section Usage
  Tasks are usually...
  
  \section Memory
  Memory is a pain...
  
  \section Priority
  A few words on priority...
  
  \section Synchronization
  To synchronize...
  
  \ingroup rtos
*/
class Task
{
public:
  Task( TaskLoop loop, const char* name, int stackDepth, int priority, void* params = 0 );
  ~Task( );
  static void sleep( int ms );
  static void yield( );
  static void enterCritical( );
  static void exitCritical( );
  int remainingStack( );
  int priority( );
  void setPriority( int priority );
  int id( );
  char* name( );
  Task* nextTask( );
protected:
  void* _task;

};

/**
  The Real Time Operating System at the heart of the Make Controller.
  
  \ingroup rtos
*/
class RTOS
{
public:
  static Task* findTaskByName( const char* name );
  static Task* findTaskByID( int id );
  static Task* currentTask( );
  static int numberOfTasks( );
  static int topTaskPriority( );
  static int ticksSinceBoot( );

protected:
  static void* getNextTaskControlBlock( void* task );
  static void* findTask( char *taskName, int taskID );
  static void* iterateByID( int id, xList* pxList );
  static void* iterateByName( char* taskName, xList* pxList );
  static void* iterateForNextTask( void** lowTask, int* lowID, void** highTask, int* highID, int currentID, xList* pxList );

  friend class Task;
};

/**
  An inter-task mechanism to pass data.
  
  \ingroup rtos
*/
class Queue
{
public:
  Queue( uint length, uint itemSize );
  ~Queue( );
  
  bool send( void* itemToQueue, int timeout = -1 );
  bool receive( void* buffer, int timeout = -1 );
  int msgsAvailable( );
  bool sendFromISR( void* itemToSend, int* taskWoken );
  bool receiveFromISR( void* buffer, int* taskWoken );
private:
  void* _q;
};

/**
  A way to synchronize between different tasks.
  
  \ingroup rtos
*/
class Semaphore
{
public:
  Semaphore( );
  ~Semaphore( );
  bool take( int timeout = -1 );
  bool give( );
  bool giveFromISR( int* taskWoken );
private:
  void* _sem;
};

#endif // RTOS__H
