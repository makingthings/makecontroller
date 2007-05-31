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

/* 
  BASIC INTERRUPT DRIVEN DRIVER FOR CAN BUS. 

  This file contains all the usb components that must be compiled
  to ARM mode.  The components that can be compiled to either ARM or THUMB
  mode are contained in can.c

*/

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Demo application includes. */
#include "Board.h"

#include "can_internal.h"

/*-----------------------------------------------------------*/

extern struct Can_ Can;

/*-----------------------------------------------------------*/

/* The ISR can cause a context switch so is declared naked. */
void CanIsr( void ) __attribute__ ((naked));

/*-----------------------------------------------------------*/


void CanIsr( void )
{
	/* This ISR can cause a context switch.  Therefore a call to the 
	portENTER_SWITCHING_ISR() macro is made.  This must come BEFORE any 
	stack variable declarations. */
	portENTER_SWITCHING_ISR();

  portCHAR cTaskWokenByPost = pdFALSE; 

  // int status = AT91C_BASE_CAN->CAN_SR;
/*
  if ( status & AT91C_ADC_DRDY )
  	cTaskWokenByPost = xSemaphoreGiveFromISR( Can.doneSemaphore, cTaskWokenByPost );

  int value = AT91C_BASE_ADC->ADC_LCDR;
  (void)value;
*/
	/* Clear AIC to complete ISR processing */
	AT91C_BASE_AIC->AIC_EOICR = 0;

	/* Do a task switch if needed */
	portEXIT_SWITCHING_ISR( cTaskWokenByPost );
}

