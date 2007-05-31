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

void* findTask( char *taskName, int taskID );
void* iterateForNextTask( void** lowTask, int* lowID, void** highTask, int* highID, int currentID, xList* pxList );
void* iterateByID( int id, xList* pxList );
void* iterateByName( char* taskName, xList* pxList );
void* TaskGetNext_internal( void* task );

/** \defgroup Rtos
	The FreeRTOS subsystem provides a real-time operating system (RTOS) for the MAKE Controller.
	FreeRTOS is an open source Real-Time Operating System.  It implements scheduling algorithms that 
	give time to "concurrent" tasks according to their given priority.  It allows programmers to 
	focus on programming each process on the board more or less separately, letting the RTOS 
	dictate when each of those tasks should be given processor time.

	Each process on the board is known as a task, and each task is generally implemented as a 
	continuous loop.  It is important to ensure that each task will block at some point so that
	other tasks will have a chance to run - otherwise the Controller will lock up. The easiest way to
	do this is with a call to Sleep( ) when the task is not urgent.
	
	More info at http://www.freertos.org
* \ingroup Controller
* @{
*/

/**	
	Put a task to sleep for a given number of milliseconds.
	This lets the processor give time to other tasks.  
	@param timems An integer specifying how long to sleep in milliseconds.
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
	Always be sure to block a task when not urgent to give priority to other tasks.  It's also a good idea to use
  as little stack as possible, as it's a limited resource.
	@param taskCode A pointer to the function that will run when the task starts.
	@param name A character string containing the name of the task.
	@param stackDepth An integer specifying in bytes the size of the task's stack.
	@param parameters A void* parameter which will be passed into the task at create time.
	@param priority An integer specifying the task's priority (0 lowest - 7 highest priority).
	@return A pointer to the handle for the newly created task, or NULL if it didn't succeed.
*/
void* TaskCreate( void (taskCode)( void*), char* name, int stackDepth, void* parameters, int priority )
{
  void* taskHandle;
  if( xTaskCreate( taskCode, (signed char*)name, stackDepth, parameters, priority, &taskHandle ) == 1 )
    return taskHandle;
  else
    return NULL;
}

/**	
	Delete an existing task.
	@param task A pointer to the handle of the task to be deleted.
*/
void TaskDelete( void* task )
{
  vTaskDelete( task );
}

/**	
	Specify that a task is entering a period during which it should not be interrupted.  
  Be sure to minimize use of this, and to call TaskExitCritical() as soon as the 
	task has completed its critical stage.  Other tasks can't get any processor time while in
	this state, and performance might be compromised.
*/
void  TaskEnterCritical( )
{
  taskENTER_CRITICAL();
}

/**	
	Specify that a task is exiting a critical period, in response to TaskEnterCritical().
	Once the task exits a critical period, the RTOS will resume switching this task as usual.  
*/
void  TaskExitCritical( )
{
  taskEXIT_CRITICAL();
}

/**	
	Allocate memory from the heap.
	Pretty much the same as a normal malloc(), but for the SAM7X. 
	@return A pointer to the allocated memory. 
  @see Free()
*/
void* Malloc( int size )
{
  return pvPortMalloc( size ); 
}

/**	
	Free memory from the heap.
	Once you've allocated memory using Malloc(), free it when you're done with it.
  It's usually a good idea to set the pointer the freed memory to NULL after you've freed it.
  @see Malloc()
*/
void Free( void* memory )
{
  vPortFree( memory ); 
}

/**	
	Check how much stack is available for a given task.
	When a task is created, it is initialized with a stack depth.  This function will return the
  numbers of bytes available from that initial allocation.
  @return An integer specifying the available stack in bytes.
  @see getTaskByName(), getTaskByID()
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

  This will fail if it's called from within the task you're trying to find.
  @return A pointer to the task, or NULL if the task was not found.
  @see getTaskByID()
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

  This will fail if it's called from within the task you're trying to find.
  @return A pointer to the task, or NULL if the task was not found.
  @see getTaskByName()
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
  @return An integer specifying the task's priority
*/
int TaskGetPriority( void* task )
{
  return xTaskGetPriority( task );
}

/**	
	Get the ID number of a task.
  Each task in freeRTOS has a unique ID number.  
  @return An integer specifying the task's unique ID within freeRTOS, or -1 on fail.
*/
int TaskGetIDNumber( void* task )
{
  return xTaskGetIDNumber( task );
}

/**	
	Get the name of a task.
  Read back the name a task was given when it was created.
  @return The task's name as a string
*/
char* TaskGetName( void* task )
{
  return (char*)xTaskGetName( task );
}

/**	
	Read the amount of stack initially allocated for a task.
  @return An integer specifying the amount of stack, in bytes, that this task was allocated.
*/
int TaskGetStackAllocated( void* task )
{
  return xTaskGetStackAllocated( task );
}

/**	
	Get a pointer to the task that's currently running.
  @return A pointer to the task that's currently running.
*/
void* TaskGetCurrent( )
{
  return xTaskGetCurrentTaskHandle( );
}

/**	
	Read the highest priority being used by any existing task.
  @return An integer specifying the priority.
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

  This is another expensive function since it needs to stop the scheduler to search the lists
  of tasks.  It should only really be used in a debug setting.
  @param task A pointer to the previous task.
  @return A pointer to the task that's next in the ID list.
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

/**	
	Get the total number of tasks that exist at a given moment.
  @return An integer specifying the number of tasks.
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
*/
void TaskSetPriority( void* task, int priority )
{
  vTaskPrioritySet( task, priority );
}

/** @}
*/

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





