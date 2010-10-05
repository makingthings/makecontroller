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

#if ((CONTROLLER_VERSION == 50) || (CONTROLLER_VERSION >= 95))
  #define SPI_SEL0_IO           PIN_PA12
  #define SPI_SEL1_IO           PIN_PA13
  #define SPI_SEL2_IO           PIN_PA8
  #define SPI_SEL3_IO           PIN_PA9
#elif (CONTROLLER_VERSION == 90)
  #define SPI_SEL0_IO           PIN_PA12
  #define SPI_SEL1_IO           PIN_PA13
  #define SPI_SEL2_IO           PIN_PB14
  #define SPI_SEL3_IO           PIN_PB17
#endif

#define SPI_SEL0_MODE PERIPHERAL_A
#define SPI_SEL1_MODE PERIPHERAL_A
#define SPI_SEL2_MODE PERIPHERAL_B
#define SPI_SEL3_MODE PERIPHERAL_B

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

static Mutex spi0Mutex;
#if USE_SPI1
static Mutex spi1Mutex;
#endif

static int spiGetPin(int channel)
{
  switch (channel) {
    case 0: return SPI_SEL0_IO;
    case 1: return SPI_SEL1_IO;
    case 2: return SPI_SEL2_IO;
    case 3: return SPI_SEL3_IO;
    default: return 0;
  }
}

static bool spiGetMode(int channel)
{
  switch (channel) {
    case 0: return SPI_SEL0_MODE;
    case 1: return SPI_SEL1_MODE;
    case 2: return SPI_SEL2_MODE;
    case 3: return SPI_SEL3_MODE;
    default: return false;
  }
}

/**
  Initialize the Spi system.
  This is automatically done during system startup, but you can prevent it
  by defining \b NO_SPI_INIT in your config.h file.
*/
void spiInit()
{
  // configure pins for Spi0, Spi1 must be configured separately
  pinSetMode(PIN_PA16, PULLUP_OFF);
  pinGroupSetMode(GROUP_A, PIN_PA16_BIT | PIN_PA17_BIT | PIN_PA18_BIT, PERIPHERAL_A);

  chMtxInit(&spi0Mutex);
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_SPI0;  // power on SPI device
  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SWRST;      // two resets to account for errata
  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SWRST;
  AT91C_BASE_SPI0->SPI_IDR = 0x3FF;               // All interrupts are off
  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIEN;      // enable the device

  #if USE_SPI1
  chMtxInit(&spi1Mutex);
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_SPI1;
  AT91C_BASE_SPI1->SPI_CR = AT91C_SPI_SWRST;
  AT91C_BASE_SPI1->SPI_CR = AT91C_SPI_SWRST;
  AT91C_BASE_SPI1->SPI_IDR = 0x3FF;
  AT91C_BASE_SPI1->SPI_CR = AT91C_SPI_SPIEN;
  #endif
}

/**
  Deinitialize the Spi system.
  @param spi Which SPI device - options are \b Spi0 or \b Spi1
*/
void spiDeinit(Spi spi)
{
  spi->SPI_CR = AT91C_SPI_SPIDIS;
  if (spi == Spi0)
    AT91C_BASE_PMC->PMC_PCDR = 1 << AT91C_ID_SPI0;
  #if USE_SPI1
  else if (spi == Spi1)
    AT91C_BASE_PMC->PMC_PCDR = 1 << AT91C_ID_SPI1;
  #endif
}

/**
  Configure the SPI to communicate on a given channel.
  @param spi Which SPI device - options are \b Spi0 or \b Spi1
  @param csn Which chip select line - options are 0-3.
  @param bits The number of transfer bits.
  @param clockDivider The divider of the SPI clock to use.
  @param delayBeforeSPCK The amount of time to delay before enabling the Spi clock.
  @param delayBetweenTransfers The amount of time to delay between transfers.
*/
int spiConfigure(Spi spi, int csn, int bits, int clockDivider, int delayBeforeSPCK, int delayBetweenTransfers)
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

  // configure chip select as periph a or b
  // note - on CONTROLLER_VERSION 50, this would need to check which PIO port to use
  // but those boards don't exist, so just use port A
  if (spi == Spi0)
    pinSetMode(spiGetPin(csn), spiGetMode(csn));

  // DON'T USE FDIV FLAG - it makes the SPI unit fail!!
  spi->SPI_MR = AT91C_SPI_MSTR |         // Select the master
                AT91C_SPI_PS_VARIABLE |
                AT91C_SPI_PCS |         // Variable Addressing - can address different chip select each transfer
                AT91C_SPI_MODFDIS;      // Disable fault detect

  spi->SPI_CSR[csn] =
        AT91C_SPI_NCPHA | // Clock Phase TRUE
        AT91C_SPI_CSAAT | // Chip Select Active After Transfer - make sure chip select stays low between bytes
        (((bits - 8) << 4) & AT91C_SPI_BITS) | // Transfer bits
        ((clockDivider << 8) & AT91C_SPI_SCBR) | // Serial Clock Baud Rate Divider (255 = slow)
        ((delayBeforeSPCK << 16) & AT91C_SPI_DLYBS) | // Delay before SPCK
        ((delayBetweenTransfers << 24) & AT91C_SPI_DLYBCT); // Delay between transfers
  
  return CONTROLLER_OK;
}

/**
  Exchange a block of data.
  Spi always involves a two-way transfer, so this writes the data originally
  contained in \b buffer and reads the response data back into \b buffer.

  The chip select is specified in spiConfigure() - if you need to communicate
  with another device on the same device, you need to reconfigure it via spiConfigure().
  @param spi Which SPI device - options are \b Spi0 or \b Spi1
  @param csn Which chip select line - options are 0-3.
  @param buffer The buffer to read/write data from.
  @param count The number of bytes to exchange.
  @return 0 on success, non-zero on failure.
  
  \b Example
  \code
  #define MY_SPI_DEVICE_CHANNEL 0x3
  unsigned char mybuf[10];
  if (spiReadWriteBlock(Spi0, mybuf, 10) == 0) {
    // success
  }
  \endcode
*/
int spiReadWriteBlock(Spi spi, int csn, unsigned char* buffer, int count)
{
  while (count--) {
    // write byte of data, if last one set AT91C_SPI_LASTXFER
    spi->SPI_TDR = (*buffer & 0xFF) |
                    ((~(1 << csn) << 16) &  AT91C_SPI_TPCS) |
                    ((0 == count) ? AT91C_SPI_LASTXFER : 0);

    // wait for byte of data, then read it
    while (!(spi->SPI_SR & AT91C_SPI_RDRF));
    *buffer++ = (unsigned char)(spi->SPI_RDR & 0xFF);

    // ensure previous byte has been completely written out
    while (!(spi->SPI_SR & AT91C_SPI_TDRE));
  }

  return 0;
}

/**
  Get exclusive access to the Spi system.
  @param spi Which SPI device - options are \b Spi0 or \b Spi1
*/
void spiLock(Spi spi)
{
  if (spi == Spi0)
    chMtxLock(&spi0Mutex);
  #if USE_SPI1
  else if (spi == Spi1)
    chMtxLock(&spi1Mutex);
  #endif
}

/**
  Release exclusive access to the Spi system.
*/
void spiUnlock()
{
  chMtxUnlock();
}

/** @} */
