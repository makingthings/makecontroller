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
Task::Task( TaskLoop loop, const char* name, int stackDepth, void* params, int priority )
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

void Task::sleep( int ms ) // static
{
  vTaskDelay( ms / portTICK_RATE_MS );
}

void Task::yield( ) // static
{
  taskYIELD(); 
}

void Task::enterCritical( ) // static
{
  taskENTER_CRITICAL();
}

void Task::exitCritical( ) // static
{
  taskEXIT_CRITICAL();
}

int Task::remainingStack( )
{
  if( !_task )
    return -1;
  else
    return usVoidTaskCheckFreeStackSpace( _task );
}

int Task::priority( )
{
  return xTaskGetPriority( _task );
}

void Task::setPriority( int priority )
{
  vTaskPrioritySet( _task, priority );
}

int Task::id( )
{
  return xTaskGetIDNumber( _task );
}

char* Task::name( )
{
  return (char*)xTaskGetName( _task );
}

int Task::stackAllocated( )
{
  return xTaskGetStackAllocated( _task );
}

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

Task* RTOS::currentTask( ) // static
{
  return (Task*)xTaskGetContext(xTaskGetCurrentTaskHandle());
}

int RTOS::numberOfTasks( ) // static
{
  return uxTaskGetNumberOfTasks( );
}

int RTOS::topTaskPriority( ) // static
{
  return xTaskGetTopUsedPriority( );
}

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

Queue::Queue( uint length, uint itemSize )
{
  _q = xQueueCreate( length, itemSize );
}

Queue::~Queue( )
{
  vQueueDelete( _q );
}

int Queue::send( void* itemToQueue, int timeout )
{
  return xQueueSend( _q, itemToQueue, timeout / portTICK_RATE_MS );
}

int Queue::receive( void* buffer, int timeout )
{
  return xQueueReceive( _q, buffer, timeout / portTICK_RATE_MS );
}

int Queue::msgsAvailable( )
{
  return uxQueueMessagesWaiting( _q );
}

bool Queue::sendFromISR( void* itemToSend, int* taskWoken )
{
  return xQueueSendFromISR( _q, itemToSend, (long int*)taskWoken );
}

bool Queue::sendToFrontFromISR( void* itemToSend, int* taskWoken )
{
  return xQueueSendToFrontFromISR( _q, itemToSend, (long int*)taskWoken );
}

bool Queue::sendToBackFromISR( void* itemToSend, int* taskWoken )
{
  return xQueueSendToBackFromISR( _q, itemToSend, (long int*)taskWoken );
}

int Queue::receiveFromISR( void* buffer, long* taskWoken )
{
  return xQueueReceiveFromISR( _q, buffer, taskWoken );
}

/**********************************************************************************

                                  Semaphore
                                  
**********************************************************************************/

Semaphore::Semaphore( )
{
  vSemaphoreCreateBinary( _sem );
}

Semaphore::~Semaphore( )
{
  vQueueDelete( _sem );
}

int Semaphore::take( int timeout )
{
  if( timeout < 0 )
    return xSemaphoreTake( _sem, portMAX_DELAY );
  else
    return xSemaphoreTake( _sem, timeout / portTICK_RATE_MS );
}

int Semaphore::give( )
{
  return xSemaphoreGive( _sem );
}

bool Semaphore::giveFromISR( int* taskWoken )
{
  return xSemaphoreGiveFromISR( _sem, (long int*)taskWoken );
}





