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
  BASIC INTERRUPT DRIVEN DRIVER FOR MAKE BOARD. 
*/

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "make.h"
#include "servo_internal.h"

/* Demo application includes. */
#include "Board.h"

void DisableFIQFromThumb( void )
{
	asm volatile ( "STMDB	SP!, {R0}" );		/* Push R0.									*/
	asm volatile ( "MRS		R0, CPSR" );		/* Get CPSR.								*/
	asm volatile ( "ORR		R0, R0, #0x40" );	/* Disable FIQ.						*/
	asm volatile ( "MSR		CPSR, R0" );		/* Write back modified value.				*/
	asm volatile ( "LDMIA	SP!, {R0}" );		/* Pop R0.									*/
	asm volatile ( "BX		R14" );				/* Return back to thumb.					*/
}
		
void EnableFIQFromThumb( void )
{
	asm volatile ( "STMDB	SP!, {R0}" );		/* Push R0.									*/	
	asm volatile ( "MRS		R0, CPSR" );		/* Get CPSR.								*/	
	asm volatile ( "BIC		R0, R0, #0x40" );	/* Enable FIQ.							*/	
	asm volatile ( "MSR		CPSR, R0" );		/* Write back modified value.				*/	
	asm volatile ( "LDMIA	SP!, {R0}" );		/* Pop R0.									*/
	asm volatile ( "BX		R14" );				/* Return back to thumb.					*/
}


extern Servo_ Servo;

void Servo_IRQCallback( int id );

void FastIsr( void ) __attribute__ ((interrupt("FIQ")));

void FastIsr( void )
{
  int status;

  status = AT91C_BASE_TC1->TC_SR;
  if ( status & AT91C_TC_CPCS )
  {
    AT91C_BASE_AIC->AIC_ICCR = 0x1 << AT91C_ID_TC1;

    Servo_IRQCallback( 0 );
  }
}

