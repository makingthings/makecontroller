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
  BASIC INTERRUPT DRIVEN DRIVER FOR SERIAL PORT. 

  This file contains all the components that must be compiled
  to ARM mode.  The components that can be compiled to either ARM or THUMB
  mode are contained in Serial2.c

*/

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Demo application includes. */
#include "Board.h"

#include "Serial2_internal.h"

/*-----------------------------------------------------------*/

extern Serial2_ Serial2[ 2 ];

/*-----------------------------------------------------------*/

/* The ISR can cause a context switch so is declared naked. */
void Serial2_Isr( void ) __attribute__ ((naked));

/*-----------------------------------------------------------*/

void Serial2_Isr( void )
{
	/* This ISR can cause a context switch.  Therefore a call to the 
	portENTER_SWITCHING_ISR() macro is made.  This must come BEFORE any 
	stack variable declarations. */
	portENTER_SWITCHING_ISR();
    
  unsigned portLONG ulStatus; 
  signed portCHAR cChar; 

  long xTaskWokenByTx = false; 
  long xTaskWokenByPost = false; 

  int index;
  for ( index = 0; index < 2; index++ )
  {
    long xTaskWokenByTxThis = false; 
    long xTaskWokenByPostThis = false; 
 
    Serial2_* s2p = &Serial2[ index ];

    /* What caused the interrupt? */ 
    ulStatus = ( s2p->at91UARTRegs->US_CSR ) & ( s2p->at91UARTRegs->US_IMR ); 
   
    if( ulStatus & AT91C_US_TXRDY ) 
    { 
      /* The interrupt was caused by the THR becoming empty. Are there any 
         more characters to transmit? */ 
      if( xQueueReceiveFromISR( s2p->transmitQueue, &cChar, &xTaskWokenByTx ) == pdTRUE ) 
      { 
        /* A character was retrieved from the queue so can be sent to the 
           THR now. */ 
        s2p->at91UARTRegs->US_THR = cChar; 
      } 
      else 
      {    
        /* Queue empty, nothing to send so turn off the Tx interrupt. */ 
        s2p->at91UARTRegs->US_IDR = AT91C_US_TXRDY; 
      }   
    } 
     
    if( ulStatus & AT91C_US_RXRDY ) 
    { 
      /* The interrupt was caused by a character being received. Grab the 
      character from the RHR and place it in the queue or received  
      characters. */ 
      int t = s2p->at91UARTRegs->US_RHR;
      cChar = t & 0xFF; 
      xTaskWokenByPost = xQueueSendFromISR( s2p->receiveQueue, &cChar, xTaskWokenByPost ); 
    }

    xTaskWokenByTx = xTaskWokenByTx || xTaskWokenByTxThis; 
    xTaskWokenByPost = xTaskWokenByPost || xTaskWokenByPostThis; 
  }
   
  /* End the interrupt in the AIC. */ 
  AT91C_BASE_AIC->AIC_EOICR = 0;

  /* Do a task switch if needed */
  portEXIT_SWITCHING_ISR( ( xTaskWokenByPost || xTaskWokenByTx ) );
}

