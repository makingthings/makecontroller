
#include "rtos.h"


/**********************************************************************************

                                  Task
                                  
**********************************************************************************/
Task::Task( TaskLoop loop, const char* name, int stackDepth, void* params, int priority )
{
  observer = false;
  if( xTaskCreate( loop, (const signed char*)name, ( stackDepth >> 2 ), params, priority, &_task ) != 1 )
    _task = NULL;
  return;
}

Task::Task( void* taskPtr )
{
  _task = taskPtr;
  observer = true;
}

Task Task::operator=(const Task t)
{
  _task = t._task;
  observer = true;
  return *this;
}

Task::~Task( )
{
  if(!observer && _task != NULL)
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

int Task::getRemainingStack( )
{
  if( !_task )
    return -1;
  else
    return usVoidTaskCheckFreeStackSpace( _task );
}

int Task::getPriority( )
{
  return xTaskGetPriority( _task );
}

void Task::setPriority( int priority )
{
  vTaskPrioritySet( _task, priority );
}

int Task::getIDNumber( )
{
  return xTaskGetIDNumber( _task );
}

char* Task::getName( )
{
  return (char*)xTaskGetName( _task );
}

int Task::getStackAllocated( )
{
  return xTaskGetStackAllocated( _task );
}

Task Task::getNext( )
{
  void* taskreturn = NULL;
  vTaskSuspendAll();
  {
    // taskreturn = TaskGetNext_internal( task );
  }
  xTaskResumeAll( );
  return Task(taskreturn);
}

/**********************************************************************************

                                  RTOS
                                  
**********************************************************************************/

Task RTOS::getTaskByName( const char* name ) // static
{
  void *tcb = NULL;
  vTaskSuspendAll();
  {
    //tcb = findTask( taskName, -1 );
  }
  xTaskResumeAll();
  return Task(tcb);
}

Task RTOS::getTaskByID( int id ) // static
{
  void* tcb = NULL;
  vTaskSuspendAll();
  {
    // tcb = findTask( NULL, taskID );
  }
  xTaskResumeAll();
  return Task(tcb);
}

Task RTOS::getCurrentTask( ) // static
{
  return Task(xTaskGetCurrentTaskHandle( ));
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

int Queue::sendFromISR( void* itemToSend, int taskPreviouslyWoken )
{
  return xQueueSendFromISR( _q, itemToSend, taskPreviouslyWoken );
}

int Queue::sendToFrontFromISR( void* itemToSend, int taskPreviouslyWoken )
{
  return xQueueSendToFrontFromISR( _q, itemToSend, taskPreviouslyWoken );
}

int Queue::sendToBackFromISR( void* itemToSend, int taskPreviouslyWoken )
{
  return xQueueSendToBackFromISR( _q, itemToSend, taskPreviouslyWoken );
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

int Semaphore::giveFromISR( int taskPreviouslyWoken )
{
  return xSemaphoreGiveFromISR( _sem, taskPreviouslyWoken );
}





