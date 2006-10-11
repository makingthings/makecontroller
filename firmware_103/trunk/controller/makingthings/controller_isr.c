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

void FastIsr( void ) __attribute__ ((interrupt("FIQ")));

void FastIsr( void )
{
  int status;
  
  status = AT91C_BASE_TC0->TC_SR;
  if ( status & AT91C_TC_CPCS )
  {
    AT91C_BASE_AIC->AIC_ICCR = 0x1 << AT91C_ID_TC0;
  }

  status = AT91C_BASE_TC1->TC_SR;
  if ( status & AT91C_TC_CPCS )
  {
    AT91C_BASE_AIC->AIC_ICCR = 0x1 << AT91C_ID_TC1;

    // int jitter = AT91C_BASE_TC1->TC_CV;
    int period;

    switch ( Servo.state )
    {
      case 0:
      {
        if ( ++Servo.index >= SERVO_COUNT || Servo.index < 0 )
          Servo.index = 0;
        ServoControl* s = &Servo.control[ Servo.index ];
        
        if ( s->position != s->positionRequested )
        {
          if ( s->speed == -1 )
            s->position = s->positionRequested;
          else
          {
            int diff = s->positionRequested - s->position;
            if ( diff < 0 )
            {
              s->position -= s->speed;
              if ( s->position < s->positionRequested )
                s->position = s->positionRequested;
            }
            else
            {
              s->position += s->speed;
              if ( s->position > s->positionRequested )
                s->position = s->positionRequested;
            }
          }
        }

        period = s->position >> 6;
        if ( period >= 0 && period <= 1023 )
        {
          s->pIoBase->PIO_CODR = s->pin;
        }
        else
          period = 1023;
        AT91C_BASE_TC1->TC_RC = ( period + 988 ) * 6;
        Servo.state = 1;
        break;
      }
      case 1:
      {
        ServoControl* s = &Servo.control[ Servo.index ];
        period = s->position >> 6;
        s->pIoBase->PIO_SODR = s->pin;
        AT91C_BASE_TC1->TC_RC = ( Servo.gap + ( 1023 - period ) ) * 6;
        Servo.state = 0;
        break;
      }
    }
  }
 
  status = AT91C_BASE_TC2->TC_SR;
  if ( status & AT91C_TC_CPCS )
  {
    AT91C_BASE_AIC->AIC_ICCR = 0x1 << AT91C_ID_TC2;
  }
}

