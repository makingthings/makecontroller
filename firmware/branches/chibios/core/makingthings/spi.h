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
#include "at91lib/AT91SAM7X256.h"

/// Calculates the value of the SCBR field of the Chip Select Register given
/// MCK and SPCK.
#define SPID_CSR_SCBR(mck, spck)    ((((mck) / (spck)) << 8) & AT91C_SPI_SCBR)

/// Calculates the value of the DLYBS field of the Chip Select Register given
/// the delay in ns and MCK.
#define SPID_CSR_DLYBS(mck, delay) \
    ((((((delay) * ((mck) / 1000000)) / 1000) + 1)  << 16) & AT91C_SPI_DLYBS)

/// Calculates the value of the DLYBCT field of the Chip Select Register given
/// the delay in ns and MCK.
#define SPID_CSR_DLYBCT(mck, delay) \
    ((((((delay) / 32 * ((mck) / 1000000)) / 1000) + 1) << 24) & AT91C_SPI_DLYBCT)

#define AT45_CSR(mck, spck) \
    (AT91C_SPI_NCPHA | SPID_CSR_DLYBCT(mck, 1000) \
     | SPID_CSR_DLYBS(mck, 0) | SPID_CSR_SCBR(mck, spck))

/** 
  Communicate with peripheral devices via SPI.
  Many external devices use the <b>Serial Peripheral Interface</b> to communicate
  with other devices.  The Make Controller SPI interface has 4 channels, although 2 of these
  are not available since they're used internally.  Channels 2 and 3 can still be used, though.
*/

void spiInit(void);
void spiDeinit(void);
bool spiEnableChannel(int channel);
int  spiConfigure(int channel, int bits, int clockDivider, int delayBeforeSPCK, int delayBetweenTransfers);
int  spiReadWriteBlock(int channel, unsigned char* buffer, int count);
void spiLock(void);
void spiUnlock(void);

#endif // SPI__H
