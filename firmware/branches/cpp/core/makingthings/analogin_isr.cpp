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

#include "analogin.h"
#include "rtos.h"
#include "AT91SAM7X256.h"

/*-----------------------------------------------------------*/

/* The interrupt entry point is naked so we can control the context saving. */
void AnalogInIsr_Wrapper( void ) __attribute__ ((naked));

/* The interrupt handler function must be separate from the entry function
to ensure the correct stack frame is set up. */
void AnalogIn_Isr( void );

/*-----------------------------------------------------------*/


void AnalogIn_Isr( void )
{
  int cTaskWokenByPost = pdFALSE; 
  AnalogIn::Manager* manager = &AnalogIn::manager;
  int status = AT91C_BASE_ADC->ADC_SR;
  if(manager->waitingForMulti)
  {
    unsigned int i, mask;
    // check if we got an End Of Conversion in any of our channels
    for( i = 0; i < ANALOGIN_CHANNELS; i++ )
    {
      mask = ( 0x01 << i );
      if( status &  mask )
        manager->multiConversionsComplete |= mask;
    }
    // if we got End Of Conversion in all our channels, indicate we're done
    if( manager->multiConversionsComplete == 0xFF )
    {
      status = AT91C_BASE_ADC->ADC_LCDR; // dummy read to clear
      manager->doneSemaphore.giveFromISR( &cTaskWokenByPost );
    }
  }
  else if ( status & AT91C_ADC_DRDY )
  {
    manager->doneSemaphore.giveFromISR( &cTaskWokenByPost );
    status = AT91C_BASE_ADC->ADC_LCDR; // dummy read to clear
  }

  AT91C_BASE_AIC->AIC_EOICR = 0; // Clear AIC to complete ISR processing

  /* If a task was woken by either a frame being received then we may need to 
  switch to another task.  If the unblocked task was of higher priority then
  the interrupted task it will then execute immediately that the ISR
  completes. */
  if( cTaskWokenByPost )
  {
    portYIELD_FROM_ISR();
  }
}

void  AnalogInIsr_Wrapper( void )
{
  /* Save the context of the interrupted task. */
  portSAVE_CONTEXT();

  /* Call the handler to do the work.  This must be a separate
  function to ensure the stack frame is set up correctly. */
  AnalogIn_Isr();

  /* Restore the context of whichever task will execute next. */
  portRESTORE_CONTEXT();
}

