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

#include "io.h"
#include "rtos.h"

void IoAIsr_Wrapper( ) __attribute__ ((naked));
void IoBIsr_Wrapper( ) __attribute__ ((naked));
void Io_Isr( AT91S_PIO* basePio );

unsigned int status;

void Io_Isr( AT91S_PIO* basePio )
{
  status = basePio->PIO_ISR;
  status &= basePio->PIO_IMR;

  // Check pending events
  if(status) {
    unsigned int i = 0;
    Io::InterruptSource* is;
    while( status != 0  && i < Io::isrSourceCount ) {
      is = &(Io::isrSources[i]);
      if( is->port == basePio) { // Source is configured on the same controller
        if ((status & is->mask) != 0) { // Source has PIOs whose statuses have changed
          is->handler(is->context); // callback the handler
          status &= ~(is->mask);    // mark this channel as serviced
        }
      }
      i++;
    }
  }
}

void IoAIsr_Wrapper( )
{
  portSAVE_CONTEXT();        // Save the context of the interrupted task.
  Io_Isr( AT91C_BASE_PIOA ); // execute the handler
  portRESTORE_CONTEXT();     // Restore the context of whichever task will execute next.
}

void IoBIsr_Wrapper( )
{
  portSAVE_CONTEXT();        // Save the context of the interrupted task.
  Io_Isr( AT91C_BASE_PIOB ); // execute the handler
  portRESTORE_CONTEXT();     // Restore the context of whichever task will execute next.
}


