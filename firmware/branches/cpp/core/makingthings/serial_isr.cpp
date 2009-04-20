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

#include "serial.h"
#include "rtos.h"

// The interrupt entry point is naked so we can control the context saving.
void Serial0Isr_Wrapper( void ) __attribute__ ((naked));
void Serial1Isr_Wrapper( void ) __attribute__ ((naked));

/* The interrupt handler function must be separate from the entry function
to ensure the correct stack frame is set up. */
void SerialIsr_Handler( int index );

void SerialIsr_Handler( int index )
{
  signed portCHAR cChar;
  int xTaskWokenByTx = false;
  int xTaskWokenByPost = false;
  long xTaskWokenByTxThis = false;
  long xTaskWokenByPostThis = false;
  Serial::Internal* si = &Serial::internals[index];
  
  unsigned int status = ( si->uart->US_CSR ) & ( si->uart->US_IMR ); // What caused the interrupt?
  if( status & AT91C_US_TXRDY ) 
  { 
    /* The interrupt was caused by the THR becoming empty. Are there any 
       more characters to transmit? */ 
    if( si->txQueue->receiveFromISR( &cChar, &xTaskWokenByTx ) == pdTRUE ) 
    { 
      // A character was retrieved from the queue so can be sent to the THR now.
      si->uart->US_THR = cChar; 
    } 
    else // Queue empty, nothing to send so turn off the Tx interrupt.
      si->uart->US_IDR = AT91C_US_TXRDY; 
  } 
   
  if( status & AT91C_US_RXRDY ) 
  { 
    /* The interrupt was caused by a character being received. Grab the 
    character from the RHR and place it in the queue or received  
    characters. */ 
    int t = si->uart->US_RHR;
    cChar = t & 0xFF; 
    si->rxQueue->sendFromISR( &cChar, &xTaskWokenByPost );
  }

  xTaskWokenByTx = xTaskWokenByTx || xTaskWokenByTxThis; 
  xTaskWokenByPost = xTaskWokenByPost || xTaskWokenByPostThis; 
   
  /* End the interrupt in the AIC. */ 
  AT91C_BASE_AIC->AIC_EOICR = 0;

  /* If a task was woken by either a frame being received then we may need to 
  switch to another task.  If the unblocked task was of higher priority then
  the interrupted task it will then execute immediately that the ISR
  completes. */
  if( xTaskWokenByPost || xTaskWokenByTx )
  {
    portYIELD_FROM_ISR();
  }
}

void Serial0Isr_Wrapper( void )
{
  portSAVE_CONTEXT(); // Save the context of the interrupted task.
  /* Call the handler to do the work.  This must be a separate
  function to ensure the stack frame is set up correctly. */
  SerialIsr_Handler(0);
  portRESTORE_CONTEXT(); // Restore the context of whichever task will execute next.
}

void Serial1Isr_Wrapper( void )
{
  portSAVE_CONTEXT(); // Save the context of the interrupted task.
  /* Call the handler to do the work.  This must be a separate
  function to ensure the stack frame is set up correctly. */
  SerialIsr_Handler(1);
  portRESTORE_CONTEXT(); // Restore the context of whichever task will execute next.
}


