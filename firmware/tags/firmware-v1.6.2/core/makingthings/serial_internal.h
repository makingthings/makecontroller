/*********************************************************************************

 Copyright 2006-2008 MakingThings

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
	serial_internal.h

  serial Internal headers

  MakingThings
*/

#ifndef SERIAL_INTERNAL_H
#define SERIAL_INTERNAL_H

#include "types.h"

/* OS includes. */
#include "queue.h"

#define SERIAL_PORTS 2

typedef struct
{
  bool active;
  int baud;
  int bits;
  int parity;
  int stopBits;
  int hardwareHandshake;

  int rxQSize;
  int txQSize;

  int detailsInitialized;

  AT91S_USART* at91UARTRegs;

  xQueueHandle receiveQueue;  
  xQueueHandle transmitQueue;  

} Serial_;

#endif
