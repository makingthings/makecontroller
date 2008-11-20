
#include "rtos_.h"


/**********************************************************************************

                                  Task
                                  
**********************************************************************************/

Task::Task( void (loop)(void*), const char* name, int stackDepth, void* parameters, int priority )
{
  if( xTaskCreate( loop, (const signed char*)name, ( stackDepth >> 2 ), parameters, priority, &_task ) != 1 )
    _task = NULL;
  return;
}

Task::~Task( )
{
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

// Task* RTOS::getTaskByName( char *taskName )
// {
//   void *tcb = NULL;
//   vTaskSuspendAll();
//   {
//     tcb = findTask( taskName, -1 );
//   }
//   xTaskResumeAll();
//   return tcb;
// }

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

Task* Task::getNext( )
{
  // void* taskreturn = NULL;
  // vTaskSuspendAll();
  // {
  //   taskreturn = TaskGetNext_internal( task );
  // }
  // xTaskResumeAll( );
  // return taskreturn;
}

/**********************************************************************************

                                  RTOS
                                  
**********************************************************************************/

Task* RTOS::getTaskByName( const char* name ) // static
{
  // void *tcb = NULL;
  // vTaskSuspendAll();
  // {
  //   tcb = findTask( taskName, -1 );
  // }
  // xTaskResumeAll();
  // return tcb;
}

Task* RTOS::getTaskByID( int id ) // static
{
  // void* tcb = NULL;
  // vTaskSuspendAll();
  // {
  //   tcb = findTask( NULL, taskID );
  // }
  // xTaskResumeAll();
  // return tcb;
}

Task* RTOS::getCurrentTask( ) // static
{
  // return xTaskGetCurrentTaskHandle( );
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

int Queue::sendToFront( void* itemToQueue, int timeout )
{
  return xQueueSend( _q, itemToQueue, timeout / portTICK_RATE_MS );
}

int Queue::sendToBack( void* itemToQueue, int timeout )
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
  delete _sem;
}

int Semaphore::take( int timeout )
{
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





