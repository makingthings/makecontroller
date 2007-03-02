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
	Put a task to sleep.
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
	Always be sure to block a task when not urgent to give priority to other tasks.
	@param taskCode A pointer to the function that will run when the task starts.
	@param name A character string containing the name of the task.
	@param stackDepth An integer specifying in bytes the size of the task's stack.
	@param parameters A void* parameter which will be passed into the task at create time.
	@param priority An integer specifying the task's priority (0 lowest - 7 highest priority).
	@return A pointer to the handle for the newly created task.
*/
void* TaskCreate( void (taskCode)( void*), char* name, int stackDepth, void* parameters, int priority )
{
  void* taskHandle;
  xTaskCreate( taskCode, name, stackDepth, parameters, priority, &taskHandle );
  return taskHandle;
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

void* Malloc( int size )
{
  return pvPortMalloc( size ); 
}

void Free( void* memory )
{
  vPortFree( memory ); 
}

/** @}
*/

