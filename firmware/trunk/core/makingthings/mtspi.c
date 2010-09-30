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

#include "mtspi.h"
#include "core.h"

#if ( (CONTROLLER_VERSION == 50) || (CONTROLLER_VERSION >= 95) )
  #define SPI_SEL0_IO           PIN_PA12
  #define SPI_SEL1_IO           PIN_PA13
  #define SPI_SEL2_IO           PIN_PA8
  #define SPI_SEL3_IO           PIN_PA9
#elif ( CONTROLLER_VERSION == 90 )
  #define SPI_SEL0_IO           PIN_PA12
  #define SPI_SEL1_IO           PIN_PA13
  #define SPI_SEL2_IO           PIN_PB14
  #define SPI_SEL3_IO           PIN_PB17
#endif

#define SPI_SEL0_MODE PERIPHERAL_A
#define SPI_SEL1_MODE PERIPHERAL_A
#define SPI_SEL2_MODE PERIPHERAL_B
#define SPI_SEL3_MODE PERIPHERAL_B

static int  spiGetPin(int channel);
static bool spiGetMode(int channel);

/** 
  \defgroup SPI
  Communicate with peripheral devices via SPI.
  Many devices (sensors, SD cards, etc) use the \b SPI (Serial Peripheral Interface) to communicate
  with the outside world.  The Make Controller SPI interface has 4 channels, although 2 of these
  are not available since they're used internally.  Channels 2 and 3 can still be used, though.
  
  \b Note - the SPI routines are not thread-safe.  If you're going to be using them from different
  threads, use the spiLock() and spiUnlock() routines to get exclusive access.
  \ingroup interfacing
  @{
*/

static Mutex spiMutex;

/**
  Enable 
*/
bool spiEnableChannel(int channel)
{
  if (channel < 0 || channel > 3)
    return false;

  // configure as periph a or b
  // note - on CONTROLLER_VERSION 50, this would need to check which PIO port to use
  // but those boards don't exist, so just use port A
  pinSetMode(spiGetPin(channel), spiGetMode(channel));
  return true;
}

/**
  Initialize the SPI system.
  This is automatically done during system startup, but you can prevent it
  by defining \b NO_SPI_INIT in your config.h file.
*/
void spiInit(void)
{
  chMtxInit(&spiMutex);

  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SWRST;
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_SPI0;

  // DON'T USE FDIV FLAG - it makes the SPI unit fail!!
  AT91C_BASE_SPI0->SPI_MR = AT91C_SPI_MSTR |         // Select the master
                             AT91C_SPI_PS_VARIABLE |
                             AT91C_SPI_PCS |         // Variable Addressing - no address here
                             AT91C_SPI_MODFDIS;      // Disable fault detect

  AT91C_BASE_SPI0->SPI_IDR = 0x3FF; // All interrupts are off

  pinSetMode(PIN_PA16, INPUT); // this disables the pullup
  // Select the correct Devices
  pinGroupSetMode(GROUP_A, PIN_PA16_BIT | PIN_PA17_BIT | PIN_PA18_BIT, PERIPHERAL_A);

  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIEN;
}

/**
  Deinitialize the SPI system.
*/
void spiDeinit(void)
{
  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIDIS;
  AT91C_BASE_PMC->PMC_PCDR = 1 << AT91C_ID_SPI0;
}

/**
  Configure the SPI to communicate on a given channel.
  @param channel Which channel to communicate on.
  @param bits The number of transfer bits.
  @param clockDivider The divider of the SPI clock to use.
  @param delayBeforeSPCK The amount of time to delay before enabling the SPI clock.
  @param delayBetweenTransfers The amount of time to delay between transfers.
*/
int spiConfigure(int channel, int bits, int clockDivider, int delayBeforeSPCK, int delayBetweenTransfers)
{
  // Check parameters
  if (bits < 8 || bits > 16)
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  if (clockDivider < 0 || clockDivider > 255)
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  if (delayBeforeSPCK < 0 || delayBeforeSPCK > 255)
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  if (delayBetweenTransfers < 0 || delayBetweenTransfers > 255)
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  AT91C_BASE_SPI0->SPI_CSR[channel] =
        AT91C_SPI_NCPHA | // Clock Phase TRUE
        (((bits - 8) << 4) & AT91C_SPI_BITS) | // Transfer bits
        ((clockDivider << 8) & AT91C_SPI_SCBR) | // Serial Clock Baud Rate Divider (255 = slow)
        ((delayBeforeSPCK << 16) & AT91C_SPI_DLYBS) | // Delay before SPCK
        ((delayBetweenTransfers << 24) & AT91C_SPI_DLYBCT); // Delay between transfers
  
  return CONTROLLER_OK;
}

/**
  Exchange a block of data.
  SPI always involves a two-way transfer, so this writes the data originally
  contained in \b buffer and reads the response data back into \b buffer.
  @param channel Which channel to communicate on.
  @param buffer The buffer to read/write data from.
  @param count The number of bytes to exchange.
  @return 0 on success, non-zero on failure.
  
  \b Example
  \code
  #define MY_SPI_DEVICE_CHANNEL 0x5
  unsigned char mybuf[10];
  if (spiReadWriteBlock(MY_SPI_DEVICE_CHANNEL, mybuf, 10) == 0) {
    // success
  }
  \endcode
*/
int spiReadWriteBlock(int channel, unsigned char* buffer, int count)
{
  int i;
  // Make sure the unit is at rest before we re-begin
  while (!(AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_TXEMPTY))
    ;
  while ((AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_RDRF))
    i = AT91C_BASE_SPI0->SPI_RDR;

  // Make the CS line hang around
  AT91C_BASE_SPI0->SPI_CSR[channel] |= AT91C_SPI_CSAAT;

  i = 0;
  while (i++ < count) {
    AT91C_BASE_SPI0->SPI_TDR = (*buffer & 0xFF) |
                               ((~(1 << channel) << 16) &  AT91C_SPI_TPCS) |
                               (int)((i == count) ? AT91C_SPI_LASTXFER : 0);

    while (!(AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_RDRF))
      ;

    *buffer++ = (unsigned char)(AT91C_BASE_SPI0->SPI_RDR & 0xFF);
  }
  AT91C_BASE_SPI0->SPI_CSR[channel] &= ~AT91C_SPI_CSAAT;
  return 0;
}

/**
  Get exclusive access to the SPI system.
*/
void spiLock()
{
  chMtxLock(&spiMutex);
}

/**
  Release exclusive access to the SPI system.
*/
void spiUnlock()
{
  chMtxUnlock();
}

/** @} */

int spiGetPin(int channel)
{
  switch (channel) {
    case 0: return SPI_SEL0_IO;
    case 1: return SPI_SEL1_IO;
    case 2: return SPI_SEL2_IO;
    case 3: return SPI_SEL3_IO;
    default: return 0;
  }
}

bool spiGetMode(int channel)
{  
  switch (channel) {
    case 0: return SPI_SEL0_MODE;
    case 1: return SPI_SEL1_MODE;
    case 2: return SPI_SEL2_MODE;
    case 3: return SPI_SEL3_MODE;
    default: return false;
  }
}
