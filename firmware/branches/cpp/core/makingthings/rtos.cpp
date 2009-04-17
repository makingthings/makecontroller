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

#include "rtos.h"
#include "string.h"


/**********************************************************************************

                                  Task
                                  
**********************************************************************************/

/**
  Create a new Task.
  
  A task is the basic unit that runs a program on the Make Controller.  
  
  @param loop The loop that will run this task.
  @param name A name for your task.
  @param stackDepth How many bytes of memory this task can use.
  @param priority This task's priority, relative to other tasks.  Valid options are from 1-9, 9 being highest priority.
  @param params (optional) A value that will be passed into your task when it's created.
  
  \b Example
  \code
  
  \endcode
*/
Task::Task( TaskLoop loop, const char* name, int stackDepth, int priority, void* params )
{
  if( xTaskCreate( loop, (const signed char*)name, ( stackDepth >> 2 ), params, priority, &_task ) != 1 )
    _task = NULL;
  else
    xTaskSetContext( _task, this );
  return;
}

Task::~Task( )
{
  if(_task)
    vTaskDelete( _task );
}

/**
  Delay your task for a number of milliseconds.
  
  It's important to always include some sleep, or a yield() in your
  task so that other tasks in the system have a chance to run.
  
  @param ms How many milliseconds to sleep.
  @see yield()
  
  \b Example
  \code
  void myLoop(void* p)
  {
    while(1) // looping forever
    {
      // ...some other processing here...
      
      Task::sleep(10); // wait 10 millis each time through the loop
    }
  }
  \endcode
*/
void Task::sleep( int ms ) // static
{
  vTaskDelay( ms / portTICK_RATE_MS );
}

/**
  Stop processing the current task, giving time to other tasks.
  
  \b Example
  \code
  void myLoop(void* p)
  {
    while(1) // looping forever
    {
      // ...some other processing here...
      
      Task::yield(); // wait 10 millis each time through the loop
    }
  }
  \endcode
*/
void Task::yield( ) // static
{
  taskYIELD(); 
}

/**
  Start a section that prevents other tasks from interrupting you.
  
  If you have some very time sensitive operations that all need to be
  performed at once, you may want to make sure that no other tasks
  interrupt you.  You can place this code in a 'critical' section enterCritical().
  
  Make sure to always match your calls to enterCritical() with a follow
  up call to exitCritical(), otherwise nothing else in the system will
  be able to run.
  
  @see exitCritical()
  
  \b Example
  \code
  void myLoop(void* p)
  {
    while(1) // looping forever
    {
      Task::enterCritical()
      // ...some very time sensitive processing here...
      Task::exitCritical() // now we're done
    }
  }
  \endcode
*/
void Task::enterCritical( ) // static
{
  taskENTER_CRITICAL();
}

/**
  Complete a section that prevents others from interrupting you.
  
  If you're in a time sensitive section of code that that uses
  enterCritical(), be sure to call this when you're done so that
  other tasks can continue to run.
  
  @see enterCritical()
  
  \b Example
  \code
  void myLoop(void* p)
  {
    while(1) // looping forever
    {
      Task::enterCritical()
      // ...some very time sensitive processing here...
      Task::exitCritical() // now we're done
    }
  }
  \endcode
*/
void Task::exitCritical( ) // static
{
  taskEXIT_CRITICAL();
}

/**
  How much memory is available on this task's stack.
  
  When you created your task, you specified the amount of memory available
  to it.  You can check how much of that memory is remaining at any time
  with a call to remainingStack().
  
  @return How many bytes of free memory are remaining.
  
  \b Example
  \code
  Task* myTask = new Task( myLoop, "Me!", 1000, 1);
  int stack = myTask->remainingStack();
  \endcode
*/
int Task::remainingStack( )
{
  if( !_task )
    return -1;
  else
    return usVoidTaskCheckFreeStackSpace( _task );
}

/**
  Get a task's priority
  
  Each task is assigned a priority, relative to other tasks, when it's created.  
  You can check a task's priority with this method.
  
  @return The task's priority, from 1-9
  
  \b Example
  \code
  Task* myTask = new Task( myLoop, "Me!", 1000, 1);
  int priority = myTask->priority();
  \endcode
*/
int Task::priority( )
{
  return xTaskGetPriority( _task );
}

/**
  Set a task's priority.
  
  Each task is assigned a priority, relative to other tasks, when it's created.  
  You can modify an existing task's priority with this method.
  
  @param priority The new priority for this task.
  
  \b Example
  \code
  Task* myTask = new Task( myLoop, "Me!", 1000, 1);
  void myLoop(void* p)
  {
    while(1) // looping forever
    {
      // ...normal task processing...
      if(need_to_be_more_important)
        myTask->setPriority(9);
    }
  }
  \endcode
*/
void Task::setPriority( int priority )
{
  vTaskPrioritySet( _task, priority );
}

/**
  Get a task's ID number.
  
  Each task is internally assigned an ID number by the operating system.
  You can read a task's ID number by calling its id() method.
  
  @return The task's ID number
  
  \b Example
  \code
  Task* myTask = new Task( myLoop, "Me!", 1000, 1);
  int some_id = myTask->id();
  \endcode
*/
int Task::id( )
{
  return xTaskGetIDNumber( _task );
}

/**
  Get a task's name.
  
  Each task is given a name when it's created.  You can check a 
  task's name with this method.
  
  @return The task's name
  
  \b Example
  \code
  Task* myTask = new Task( myLoop, "Me!", 1000, 1);
  char* name = myTask->name();
  // now name should be "Me!"
  \endcode
*/
char* Task::name( )
{
  return (char*)xTaskGetName( _task );
}

/**
  Get the next task in the system.
  
  You can loop through all the tasks in the system using the nextTask()
  method.  Once it has reached the last task, it will wrap back around
  and return the first task in the list.
  
  @return The next Task
  
  \b Example
  \code
  // find the task with the highest priority
  Task* t = RTOS::currentTask();
  int tasks_to_find = 5; // adjust for the number of tasks you want to check
  int highest = 0;
  while(tasks_to_find--)
  {
    t = t->nextTask();
    if(t->priority() > highest)
      highest = t->priority();
  }
  
  \endcode
*/
Task* Task::nextTask( )
{
  void* tcb = NULL;
  vTaskSuspendAll();
  {
    tcb = RTOS::getNextTaskControlBlock( _task );
  }
  xTaskResumeAll( );
  return (tcb) ? (Task*)xTaskGetContext(tcb) : NULL;
}

/**********************************************************************************

                                  RTOS
                                  
**********************************************************************************/

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
Task* RTOS::findTaskByName( const char* name ) // static
{
  void *tcb = NULL;
  vTaskSuspendAll();
  {
    tcb = findTask( (char*)name, -1 );
  }
  xTaskResumeAll();
  return (tcb) ? (Task*)xTaskGetContext(tcb) : NULL;
}

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
Task* RTOS::findTaskByID( int id ) // static
{
  void* tcb = NULL;
  vTaskSuspendAll();
  {
    tcb = findTask( NULL, id );
  }
  xTaskResumeAll();
  return (tcb) ? (Task*)xTaskGetContext(tcb) : NULL;
}

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
Task* RTOS::currentTask( ) // static
{
  return (Task*)xTaskGetContext(xTaskGetCurrentTaskHandle());
}

int RTOS::numberOfTasks( ) // static
{
  return uxTaskGetNumberOfTasks( );
}

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
int RTOS::topTaskPriority( ) // static
{
  return xTaskGetTopUsedPriority( );
}

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
int RTOS::ticksSinceBoot( ) // static
{
  return xTaskGetTickCount( );
}

void* RTOS::getNextTaskControlBlock( void* task )
{
  int currentID;
  int lowID = 1024; // some large number that will be greater than the number of tasks...
  int highID = 1024;
  void* lowTask = NULL;
  void* highTask = NULL;
  void* tcb = NULL;
  Task* t;
  unsigned portBASE_TYPE uxQueue = topTaskPriority( ) + 1;

  if( task == NULL )
    currentID = 1;
  else
  {
    t = (Task*)xTaskGetContext(task);
    currentID = t->id();
  }

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
void* RTOS::findTask( char *taskName, int taskID )
{
  bool byName;
  if( taskName == NULL && taskID > 0 )
    byName = false;
  if( taskName != NULL && taskID < 0 )
    byName = true;

  unsigned portBASE_TYPE uxQueue = topTaskPriority( ) + 1;
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

void* RTOS::iterateByID( int id, xList* pxList )
{
  void *pxNextTCB, *pxFirstTCB;
  Task* t;
  listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );
  do
  {
    listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
    t = (Task*)xTaskGetContext( pxNextTCB );
    if( t->id() == id )
      return pxNextTCB;
    
  } while( pxNextTCB != pxFirstTCB );
  // if we get down here, we didn't find it.
  return NULL;
}

void* RTOS::iterateByName( char* taskName, xList* pxList )
{
  void *pxNextTCB, *pxFirstTCB;
  Task* t;
  listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );
  do
  {
    listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
    t = (Task*)xTaskGetContext( pxNextTCB );
    if( strcmp( t->name(), taskName ) == 0 )
      return pxNextTCB;
    
  } while( pxNextTCB != pxFirstTCB );
  // if we get down here, we didn't find it.
  return NULL;
}

void* RTOS::iterateForNextTask( void** lowTask, int* lowID, void** highTask, int* highID, int currentID, xList* pxList )
{
  void *pxNextTCB, *pxFirstTCB;
  Task* t;
  listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );
  do
  {
    listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
    t = (Task*)xTaskGetContext( pxNextTCB );
    int id = t->id();
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

/**********************************************************************************

                                  Queue
                                  
**********************************************************************************/

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
Queue::Queue( uint length, uint itemSize )
{
  _q = xQueueCreate( length, itemSize );
}

Queue::~Queue( )
{
  vQueueDelete( _q );
}

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
int Queue::send( void* itemToQueue, int timeout )
{
  return xQueueSend( _q, itemToQueue, timeout / portTICK_RATE_MS );
}

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
int Queue::receive( void* buffer, int timeout )
{
  return xQueueReceive( _q, buffer, timeout / portTICK_RATE_MS );
}

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
int Queue::msgsAvailable( )
{
  return uxQueueMessagesWaiting( _q );
}

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
bool Queue::sendFromISR( void* itemToSend, int* taskWoken )
{
  return xQueueSendFromISR( _q, itemToSend, (long int*)taskWoken );
}

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
bool Queue::sendToFrontFromISR( void* itemToSend, int* taskWoken )
{
  return xQueueSendToFrontFromISR( _q, itemToSend, (long int*)taskWoken );
}

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
bool Queue::sendToBackFromISR( void* itemToSend, int* taskWoken )
{
  return xQueueSendToBackFromISR( _q, itemToSend, (long int*)taskWoken );
}

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
int Queue::receiveFromISR( void* buffer, long* taskWoken )
{
  return xQueueReceiveFromISR( _q, buffer, taskWoken );
}

/**********************************************************************************

                                  Semaphore
                                  
**********************************************************************************/

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
Semaphore::Semaphore( )
{
  vSemaphoreCreateBinary( _sem );
}

Semaphore::~Semaphore( )
{
  vQueueDelete( _sem );
}

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
int Semaphore::take( int timeout )
{
  if( timeout < 0 )
    return xSemaphoreTake( _sem, portMAX_DELAY );
  else
    return xSemaphoreTake( _sem, timeout / portTICK_RATE_MS );
}

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
int Semaphore::give( )
{
  return xSemaphoreGive( _sem );
}

/**
  
  
  @param 
  
  \b Example
  \code
  
  \endcode
*/
bool Semaphore::giveFromISR( int* taskWoken )
{
  return xSemaphoreGiveFromISR( _sem, (long int*)taskWoken );
}





