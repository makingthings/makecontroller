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

#ifndef SPI_H
#define SPI_H

#include "types.h"

/** 
  Communicate with peripheral devices via SPI.
  Many external devices use the <b>Serial Peripheral Interface</b> to communicate
  with other devices.  The Make Controller SPI interface has 4 channels, although 2 of these
  are not available since they're used internally.  Channels 2 and 3 can still be used, though.
*/

void spiInit(void);
void spiDeinit(void);
bool spiEnableChannel( int channel );
int  spiConfigure( int channel, int bits, int clockDivider, int delayBeforeSPCK, int delayBetweenTransfers );
int  spiReadWriteBlock( int channel, unsigned char* buffer, int count );
void spiLock(void);
void spiUnlock(void);

#endif // SPI__H
