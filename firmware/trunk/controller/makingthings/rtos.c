/*********************************************************************************

 Copyright 2006 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

/** \file rtos.c	
	RTOS - Real Time Operating System.
	Functions to work within FreeRTOS on the Make Controller Board.
*/

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "rtos.h"
#include "string.h"
#include "queue.h"

void* findTask( char *taskName, int taskID );
void* iterateForNextTask( void** lowTask, int* lowID, void** highTask, int* highID, int currentID, xList* pxList );
void* iterateByID( int id, xList* pxList );
void* iterateByName( char* taskName, xList* pxList );
void* TaskGetNext_internal( void* task );

/** \defgroup RTOS RTOS
	The RTOS subsystem provides a real-time operating system (RTOS) for the MAKE Controller.
	Currently, the Make Controller uses FreeRTOS, an open source Real-Time Operating System.  It implements a scheduler that 
	gives time to "concurrent" tasks according to their given priority.  It allows programmers to 
	focus on programming each process on the board separately, letting the RTOS 
	determine when each of those tasks should be given processor time.
	
	More info at http://www.freertos.org
* \ingroup Controller
*/

/** \defgroup Tasks Tasks
	The individual processes that make up your application.
	Tasks can all be written separately and the RTOS will take of switching between them, 
	making sure they all get the processing time they need.  Each task is implemented as a continuous loop, and
	all tasks have the same signature:
	\code void MyTask( void* parameters) \endcode
	The void* parameters is a value that can be passed in at the time the task is created.  Make sure that you put your task
	to sleep when it's not doing anything so that other tasks can get time as well.  Otherwise, the Controller will lock up
	and none of the other tasks will run.
	
	\par Example
	\code
	void MyTask( void* p )
	{
		// some initialization here
		int newVal = 0;
		
		// then just sit in our loop
		// we're going to turn an LED on whenever an AnalogIn is over a certain value 
		while( 1 )
		{
			newVal = AnalogIn_GetValue( 1 );
			if( newVal < 512 )
				AppLed_SetState( 0, 1 );
			else
				AppLed_SetState( 0, 0 );
			Sleep( 100 ); // wait for 100 milliseconds, allowing other tasks to do their thing
		}
	}
	\endcode
	
	Notice we don't need to include anything about Ethernet or USB communication, or anything else that might
	be going on.  These are taken care of elsewhere by the RTOS, allowing us to focus on our LED blinking.
	
	More info at http://www.freertos.org
* \ingroup RTOS
* @{
*/

/**	
	Put a task to sleep for a given number of milliseconds.
	This lets the processor give time to other tasks.  You'll always want to include some amount of Sleep( ) in your tasks
	to avoid locking up the Controller.
	@param timems An integer specifying how long to sleep in milliseconds.
	
\par Example
\code
void MyTask( void *p) // in your task
{
  while( 1 )
  {
    Led_SetState( 0 ); // turn the LED off
    Sleep( 900 ); // leave it off for 900 milliseconds
    Led_SetState( 1 ); // turn the LED on
    Sleep( 10 ); // leave it on for 10 milliseconds
  }
}
\endcode
*/
void Sleep( int timems )
{
  vTaskDelay( timems / portTICK_RATE_MS );
}

/**	
	Give up the remaining time allotted to this task by the processor.
	The presently running task immeidately gives up the remaining time it has in the current timeslice 
	so that other tasks can run.  While Sleep() will wait the specified number of milliseconds 
	before continuing, TaskYield() will cause the task to take processor time again 
	as soon as it's available.
*/
void TaskYield( )
{
  taskYIELD(); 
}

/**	
	Create a new task.
	
	When creating a new task, be sure that you allocate it enough stack space.  A task must have enough stack space 
	to load any of the potential data structures it will be working with, otherwise it will die and the Controller will crash.
	
	Since FreeRTOS is pre-emptive, higher priority tasks will get processor time before lower priority tasks.  The priority
	level is a bit arbitrary, but choose something that makes sense in relation to the other tasks you have running. Always be 
	sure to block a task when possible, in order to give processor time to other tasks.
	@param taskCode A pointer to the function that will run when the task starts.
	@param name A character string containing the name of the task.
	@param stackDepth An integer specifying in bytes the size of the task's stack.
	@param parameters A void* parameter which will be passed into the task at create time.
	@param priority An integer specifying the task's priority (0 lowest - 7 highest priority).
	@return A pointer to the handle for the newly created task, or NULL if it didn't succeed.
	@see TaskDelete( )
	
\par Example
\code
// create the task MyTask, called "Me", with a stack of 1000 bytes, no parameters, at priority 1
void *taskPtr = TaskCreate( MyTask, "Me", 1000, 0, 1 );

void MyTask( void *p) // in your task
{
  // set up any variables that you'll need
  int counter = 0;
  // then start your continuous loop
  while( 1 )
  {
    counter++;
    Sleep( 100 );
  }
}
\endcode
*/
void* TaskCreate( void (taskCode)( void*), char* name, int stackDepth, void* parameters, int priority )
{
  void* taskHandle;
  if( xTaskCreate( taskCode, (signed char*)name, ( stackDepth >> 2 ), parameters, priority, &taskHandle ) == 1 )
    return taskHandle;
  else
    return NULL;
}

/**	
	Delete an existing task.
	TaskCreate( ) returns a pointer to the newly created task.  You'll need to hang onto this pointer in order to delete the task.
	@param task A pointer to the handle of the task to be deleted.
	
\par Example
\code
void *taskPtr = TaskCreate( MyTask, "Me", 1000, 0, 1 ); // create a new task
// ... run the task ...
TaskDelete( taskPtr ); // then delete it
\endcode
*/
void TaskDelete( void* task )
{
  vTaskDelete( task );
}

/**	
	Specify that a task is entering a period during which it should not be interrupted.  
  Call this from within your task.  Be sure to minimize use of this, and to call TaskExitCritical() as soon as the 
	task has completed its critical stage.  Other tasks can't get any processor time while in
	this state, and performance might be compromised.
	@see TaskExitCritical( )
*/
void  TaskEnterCritical( )
{
  taskENTER_CRITICAL();
}

/**	
	Specify that a task is exiting a critical period, in response to TaskEnterCritical().
	Once the task exits a critical period, FreeRTOS will resume switching this task as usual.
	@see TaskEnterCritical( )
*/
void  TaskExitCritical( )
{
  taskEXIT_CRITICAL();
}

/**	
Check how much stack is available for a given task.
When a task is created, it is initialized with a stack depth.  This function will return the
numbers of bytes available from that initial allocation.
@return An integer specifying the available stack in bytes.
@see TaskGetStackAllocated( )

\par Example
\code
void *taskPtr = TaskCreate( MyTask, "Mine", 1000, 0, 1 ); // create a new task
int freeBytes = TaskGetRemainingStack( taskPtr ); // check how much of the task's stack has been used
\endcode
*/
int TaskGetRemainingStack( void* task )
{
  if( task != NULL )
    return usVoidTaskCheckFreeStackSpace( task );
  else
    return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;
}

/**	
	Get a pointer to a task by its name.
	When a task is created, it is initialized with a name.  If you don't have a pointer to the task itself
  but you know its name, you can still get a reference to the task.  This is a costly function, as it
  shuts down the scheduler to search for the task, so it should really only be used in a debug setting.
	@param taskName A character string specifying the name of the task to find.
  @return A pointer to the task, or NULL if the task was not found.
  @see getTaskByID()
	
\par Example
\code
TaskCreate( MyTask, "Mine", 1000, 0, 1 ); // a task has been created, but we don't have a pointer to it
void *taskPtr = getTaskByName( "Mine" ); // but we can still get a pointer to it by its name
\endcode
*/
void* getTaskByName( char *taskName )
{
  void *tcb = NULL;
  vTaskSuspendAll();
  {
    tcb = findTask( taskName, -1 );
  }
  xTaskResumeAll();
  return tcb;
}

/**	
	Get a pointer to a task by its ID number.
	Each task is given a unique ID number by freeRTOS.  If you know this number, but don't have a pointer
  to the task, use this function to get a pointer to the task itself.  This is a costly function, as it
  shuts down the scheduler to search for the task, so it should really only be used in a debug setting.
	@param taskID An integer specifying the unique ID number given to the task by FreeRTOS
  @return A pointer to the task, or NULL if the task was not found.
	@see TaskGetIDNumber( )
*/
void* getTaskByID( int taskID )
{
  void* tcb = NULL;
  vTaskSuspendAll();
  {
    tcb = findTask( NULL, taskID );
  }
  xTaskResumeAll();
  return tcb;
}

/**	
	Get the priority of a task.
	When a task is created, it's given a priority from 1-7.  Use this function to get the task's priority
	after it has been created.
	@param task A pointer to the task.
  @return An integer specifying the task's priority
	
\par Example
\code
void *taskPtr = TaskCreate( MyTask, "Mine", 1000, 0, 1 );
int priority = TaskGetPriority( taskPtr ); // will return 1
\endcode
*/
int TaskGetPriority( void* task )
{
  return xTaskGetPriority( task );
}

/**	
	Get the ID number of a task.
  FreeRTOS assigns each task a unique ID number.  Use this function to retrieve a particular task's ID number.
	Task ID numbers are not necessarily sequential.
	@param task A pointer to the task.
  @return An integer specifying the task's unique ID within freeRTOS, or -1 on fail.
	
\par Example
\code
void *taskPtr = TaskCreate( MyTask, "Mine", 1000, 0, 1 );
int id = TaskGetIDNumber( taskPtr );
\endcode
*/
int TaskGetIDNumber( void* task )
{
  return xTaskGetIDNumber( task );
}

/**	
	Get the name of a task.
  Read back the name a task was given when it was created.
	@param task A pointer to the task.
  @return The task's name as a string
	
\par Example
\code
void *taskPtr = TaskCreate( MyTask, "Mine", 1000, 0, 1 );
char* name = TaskGetName( taskPtr ); // will return "Mine"
\endcode
*/
char* TaskGetName( void* task )
{
  return (char*)xTaskGetName( task );
}

/**	
	Read the amount of stack initially allocated for a task.
	Each task is allocated a certain amount of stack space when it's created.  Use this function to
	determine how much stack space a task was originally given.
	@param task A pointer to the task.
  @return An integer specifying the amount of stack, in bytes, that this task was allocated.
	@see TaskGetRemainingStack( )
	
\par Example
\code
void *taskPtr = TaskCreate( MyTask, "Mine", 1000, 0, 1 );
int stack = TaskGetStackAllocated( taskPtr ); // will return 1000
\endcode
*/
int TaskGetStackAllocated( void* task )
{
  return xTaskGetStackAllocated( task );
}

/**	
	Get a pointer to the task that's currently running.
  @return A pointer to the task that's currently running.
	
\par Example
\code
void *currentTask = TaskGetCurrent( );
\endcode
*/
void* TaskGetCurrent( )
{
  return xTaskGetCurrentTaskHandle( );
}

/**	
	Read the highest priority being used by any existing task.
  @return An integer specifying the priority.
	
\par Example
\code
int highestPriority = TaskGetTopPriorityUsed( );
\endcode
*/
int TaskGetTopPriorityUsed( )
{
  return xTaskGetTopUsedPriority( );
}

/**	
	Get a pointer to the task whose ID comes after a given ID.
  FreeRTOS assigns each task a unique ID number internally.  This function will
  return a pointer to the task with the next ID.  Note that this does not necessarily
  mean that this task will be given processor time next, just that its ID is the next 
  one along in the list of all tasks.

  This is an expensive function since it needs to stop the scheduler to search the lists
  of tasks.  It should only really be used in a debug setting.
  @param task A pointer to the previous task.
  @return A pointer to the task that's next in the ID list.
	
\par Example
\code
void *oneTask = TaskCreate( MyTask, "Mine", 1000, 0, 1 );
void *nextTask = TaskGetNext( oneTask );
\endcode
*/
void* TaskGetNext( void* task )
{
  void* taskreturn = NULL;
  vTaskSuspendAll();
  {
    taskreturn = TaskGetNext_internal( task );
  }
  xTaskResumeAll( );
  return taskreturn;
}

/**	
	Get the total number of tasks that exist at a given moment.
  @return An integer specifying the number of tasks.
	
\par Example
\code
int numberOfTasks = GetNumberOfTasks( );
\endcode
*/
int GetNumberOfTasks( )
{
  return uxTaskGetNumberOfTasks( );
}

/**	
	Update the priority of a task.
  Higher numbers mean higher priority/more processor time.
  @param task A pointer to the task you want to update.
  @param priority An integer specifying the new priority.
	
\par Example
\code
void *task = TaskCreate( MyTask, "Mine", 1000, 0, 1 ); // create a task
// ... now update the task's priority to 6
TaskSetPriority( task, 6 );
\endcode
*/
void TaskSetPriority( void* task, int priority )
{
  vTaskPrioritySet( task, priority );
}

/**	
  Returns the number of ticks since the scheduler started.
  @return the number of ticks
*/
int TaskGetTickCount( void )
{
  return xTaskGetTickCount( );
}

/** @}
*/

/** \defgroup Utils Utilities
	General utilities provided by the RTOS.
	
	FreeRTOS has several options for memory management - we've chosen a default one here which
	is pretty sensible in most cases, but you're of course welcome to use whatever makes most 
	sense for your application.  See http://www.freertos.org/a00111.html for
	the available options.

* \ingroup RTOS
* @{
*/

/**	
	Dynamically allocate memory from the heap.
	This is pretty much the same as a normal malloc(), but for the SAM7X. 
	@return A pointer to the allocated memory, or NULL if the memory was not available.
  @see Free(), System_GetFreeMemory( )
	
	\par Example
	\code
	int bufferSize = 1024;
	char *bufferPtr = Malloc( bufferSize );
	// now you have a 1024 character buffer
	\endcode
*/
void* Malloc( int size )
{
  return pvPortMalloc( size );
}

/**	
	Same as Malloc, but keeps trying until it succeeds.
	This is a convenience function that will continue to call Malloc( ) at a given interval until
	it returns successfully.  Once this function returns, you know you have been allocated the memory
	you asked for.
	
	@param size An integer specifying the amount of memory, in bytes, you want to allocate.
	@param interval An integer specifying the number of milliseconds to wait between successive attempts to allocate memory.  
	It's best to not set this too low, as it will be a bit taxing on the system.
	@return A pointer to the allocated memory, or NULL if the memory was not available.
  @see Free(), System_GetFreeMemory( )
	
	\par Example
	\code
	char *bufferPtr = MallocWait( 1024, 100 );
	// now you have a 1024 character buffer
	\endcode
*/
void* MallocWait( int size, int interval )
{
	void* retval = NULL;
	while( retval == NULL )
	{
		retval = Malloc( size );
		if( retval )
			return retval;
		else
			Sleep( interval );
	}
	return NULL; // should never get here
}

/**	
	Free memory from the heap.
	Once you've allocated memory using Malloc(), free it when you're done with it.
  It's usually a good idea to set the pointer the freed memory to NULL after you've freed it.
	@param memory A pointer to the memory created by Malloc( )
  @see Malloc(), System_GetFreeMemory( )
	
	\par Example
	\code
	char *bufferPtr = Malloc( 1024 ); // allocate 1024 bytes of memory
	// ...your processing here...
	Free( bufferPtr ); // then free the memory when you're done with it
	\endcode
*/
void Free( void* memory )
{
  vPortFree( memory ); 
}

/** @}
*/

/** \defgroup Queues Queues
	Queues allow for thread-safe inter-process communication.
	A queue is a list of items that can be passed from one task to another.
	Items are placed in a queue by copy - not by reference. It is therefore preferable, 
	when queuing large items, to only queue a pointer to the item.  Tasks can block on a 
	queue to wait for either data to become available on the queue, or space to become available to write to the queue.
	
	More info at http://www.freertos.org - some documentation here used from the FreeRTOS doc by Richard Barry.
* \ingroup RTOS
* @{
*/

/**	
	Create a new queue.
  This allocates storage for the queue.  It's usually a good idea to pass around pointers on queues
  as opposed to whole data structures, as that's quite resource intensive.
  @param length The maximum number of items that the queue can contain.
  @param itemSize The number of bytes each item in the queue will require.  Items are queued by copy, not by reference, 
  so this is the number of bytes that will be copied for each posted item.  Each item on the queue must be the same size.
  @return A pointer to the queue on success, 0 if the queue couldn't be created.
	
  \par Example
  \code
  struct myData
  {
    int count;
    char buffer[100];
  };

  // create a queue that can hold 5 pointers to myData structures
  void* myQueue = QueueCreate( 5, sizeof( myData* ) );
  if( myQueue == 0 )
    // then the queue can't be used.
  else
    // continue processing...
  \endcode
*/
void* QueueCreate( uint length, uint itemSize )
{
  return xQueueCreate( length, itemSize );
}

/**	
	Post an item onto a queue.
  The item is queued by copy, not by reference.  This function must not be called from an interrupt service routine.
  See xQueueSendFromISR () for an alternative which may be used in an ISR.
  @param queue The queue to send to.
  @param itemToQueue A pointer to the item to send on the queue.  
  @param msToWait The maximum number of milliseconds the task should block waiting for space to become available on the queue, 
  should it already be full.  The call will return immediately if this is set to 0.
  @return 1 on success, 0 on failure.
	
  \par Example
  \code
  struct MyData_
  {
    int count;
    char buffer[100];
  };

  // create a queue that can hold 5 pointers to myData structures
  void* myQueue = QueueCreate( 5, sizeof( myData* ) );
  struct MyData_* myData;
  if( myQueue )
  {
    if( QueueSend( myQueue, (void*)&myData, 10 ) ) // wait 10 ms to send if queue is full
      // then we're all set
    else
      // deal with an unsuccessful send
  }
  \endcode
*/
int QueueSendToFront( void* queue, void* itemToQueue, int msToWait )
{
  return xQueueSend( queue, itemToQueue, msToWait );
}

int QueueSendToBack( void* queue, void* itemToQueue, int msToWait )
{
  return xQueueSend( queue, itemToQueue, msToWait );
}

int QueueReceive( void* queue, void* buffer, int msToWait )
{
  return xQueueReceive( queue, buffer, msToWait );
}

int QueueMessagesWaiting( void* queue )
{
  return uxQueueMessagesWaiting( queue );
}

void QueueDelete( void* queue )
{
  vQueueDelete( queue );
}

int QueueSendFromISR( void* queue, void* itemToSend, int taskPreviouslyWoken )
{
  return xQueueSendFromISR( queue, itemToSend, taskPreviouslyWoken );
}

int QueueReceiveFromISR( void* queue, void* buffer, long* taskWoken )
{
  return xQueueReceiveFromISR( queue, buffer, taskWoken );
}

/** @}
*/

void* TaskGetNext_internal( void* task )
{
  int currentID;
  int lowID = 1024; // some large number that will be greater than the number of tasks...
  int highID = 1024;
  void* lowTask = NULL;
  void* highTask = NULL;
  void* tcb = NULL;
  unsigned portBASE_TYPE uxQueue = TaskGetTopPriorityUsed( ) + 1;

  if( task == NULL )
    currentID = 1;
  else
    currentID = TaskGetIDNumber( task );

  // look through all the possible lists of tasks
    do
    {
      uxQueue--;
      if( !listLIST_IS_EMPTY( GetReadyTaskByPriority( uxQueue ) ) )
      {
        tcb = iterateForNextTask( &lowTask, &lowID, &highTask, &highID, currentID, GetReadyTaskByPriority( uxQueue ) );
        if( tcb != NULL )
          return tcb;
      }   
    }while( uxQueue > ( unsigned portSHORT ) tskIDLE_PRIORITY );

    // check the list of delayed tasks
    if( !listLIST_IS_EMPTY( GetDelayedTaskList( ) ) )
    {
      tcb = iterateForNextTask( &lowTask, &lowID, &highTask, &highID, currentID, GetDelayedTaskList( ) );
      if( tcb != NULL )
        return tcb;
    }
    // check the list of overflow delayed tasks
    if( !listLIST_IS_EMPTY( GetOverflowDelayedTaskList( ) ) )
    {
      tcb = iterateForNextTask( &lowTask, &lowID, &highTask, &highID, currentID, GetOverflowDelayedTaskList( ) );
      if( tcb != NULL )
        return tcb;
    }
    // check the list of tasks about to die
    if( !listLIST_IS_EMPTY( GetListOfTasksWaitingTermination( ) ) )
    {
      tcb = iterateForNextTask( &lowTask, &lowID, &highTask, &highID, currentID, GetListOfTasksWaitingTermination( ) );
      if( tcb != NULL )
        return tcb;
    }
    // check the list of suspended tasks
    if( !listLIST_IS_EMPTY( GetSuspendedTaskList( ) ) )
      tcb = iterateForNextTask( &lowTask, &lowID, &highTask, &highID, currentID, GetSuspendedTaskList( ) );

    
  if( highTask )
    return highTask;
  else
    return lowTask;
}

// one function to search either by name or ID
// pass in a garbage value for the parameter you're not interested in
void* findTask( char *taskName, int taskID )
{
  bool byName;
  if( taskName == NULL && taskID > 0 )
    byName = false;
  if( taskName != NULL && taskID < 0 )
    byName = true;

  unsigned portBASE_TYPE uxQueue = TaskGetTopPriorityUsed( ) + 1;
  void *tcb = NULL;

  // look through all the possible lists of tasks
    do
    {
      uxQueue--;
      if( !listLIST_IS_EMPTY( GetReadyTaskByPriority( uxQueue ) ) )
      {
        if( byName )
          tcb = iterateByName( taskName, GetReadyTaskByPriority( uxQueue ) );
        else
          tcb = iterateByID( taskID, GetReadyTaskByPriority( uxQueue ) );

        if( tcb != NULL )
          return tcb;
      }   
    }while( uxQueue > ( unsigned portSHORT ) tskIDLE_PRIORITY );

    // check the list of delayed tasks
    if( !listLIST_IS_EMPTY( GetDelayedTaskList( ) ) )
    {
      if( byName)
        tcb = iterateByName( taskName, GetDelayedTaskList( ) );
      else
        tcb = iterateByID( taskID, GetDelayedTaskList( ) );

      if( tcb != NULL )
        return tcb;
    }
    // check the list of overflow delayed tasks
    if( !listLIST_IS_EMPTY( GetOverflowDelayedTaskList( ) ) )
    {
      if( byName )
        tcb = iterateByName( taskName, GetOverflowDelayedTaskList( ) );
      else
        tcb = iterateByID( taskID, GetOverflowDelayedTaskList( ) );
      if( tcb != NULL )
        return tcb;
    }
    // check the list of tasks about to die
    if( !listLIST_IS_EMPTY( GetListOfTasksWaitingTermination( ) ) )
    {
      if( byName )
        tcb = iterateByName( taskName, GetListOfTasksWaitingTermination( ) );
      else
        tcb = iterateByID( taskID, GetListOfTasksWaitingTermination( ) );

      if( tcb != NULL )
        return tcb;
    }
    // check the list of suspended tasks
    if( !listLIST_IS_EMPTY( GetSuspendedTaskList( ) ) )
    {
      if( byName )
        tcb = iterateByName( taskName, GetSuspendedTaskList( ) );
      else
        tcb = iterateByID( taskID, GetSuspendedTaskList( ) );
    }
    return tcb;
}

void* iterateByID( int id, xList* pxList )
{
  void *pxNextTCB, *pxFirstTCB;
  listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );
  do
  {
    listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
    if( TaskGetIDNumber( pxNextTCB ) == id )
      return pxNextTCB;
    
  } while( pxNextTCB != pxFirstTCB );
  // if we get down here, we didn't find it.
  return NULL;
}

void* iterateByName( char* taskName, xList* pxList )
{
  void *pxNextTCB, *pxFirstTCB;
  listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );
  do
  {
    listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
    if( strcmp( TaskGetName( pxNextTCB ), taskName ) == 0 )
      return pxNextTCB;
    
  } while( pxNextTCB != pxFirstTCB );
  // if we get down here, we didn't find it.
  return NULL;
}

void* iterateForNextTask( void** lowTask, int* lowID, void** highTask, int* highID, int currentID, xList* pxList )
{
  void *pxNextTCB, *pxFirstTCB;
  listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );
  do
  {
    listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
    int id = TaskGetIDNumber( pxNextTCB );
    if( id == (currentID + 1) ) // if it's the ID we're looking for plus one, we got it and we're out.
      return pxNextTCB;
    if( id > currentID && id < *highID )
    {
      *highID = id;
      *highTask = pxNextTCB;
    }
    if( id < currentID && id < *lowID && id != 1 ) // we don't want id 1 since that's IDLE and there's no real task there.
    {
      *lowID = id;
      *lowTask = pxNextTCB;
    }
  } while( pxNextTCB != pxFirstTCB );
  // if we get down here, we didn't find it.
  return NULL;
}





