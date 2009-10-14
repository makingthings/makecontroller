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

#include "spi.h"
#include "error.h"
#include "config.h"
#include <ch.h>
#include "pin.h"
#include "at91lib/AT91SAM7X256.h"

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

static int  spiGetPin( int channel );
static bool spiGetMode( int channel );

static Mutex spiMutex;

bool spiEnableChannel( int channel )
{
  if( channel < 0 || channel > 3 )
    return false;

  // configure as periph a or b
  // note - on CONTROLLER_VERSION 50, this would need to check which PIO port to use
  // but those boards don't exist, so just use port A
  pinSetMode(spiGetPin(channel), spiGetMode(channel));
  return true;
}

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

  // Set up the IO lines for the peripheral
  AT91C_BASE_PIOA->PIO_PDR = AT91C_PA16_SPI0_MISO |
                              AT91C_PA17_SPI0_MOSI |
                              AT91C_PA18_SPI0_SPCK;

  AT91C_BASE_PIOA->PIO_PPUDR = AT91C_PA16_SPI0_MISO;
  AT91C_BASE_PIOA->PIO_ODR = AT91C_PA16_SPI0_MISO;

  // Select the correct Devices
  AT91C_BASE_PIOA->PIO_ASR = AT91C_PA16_SPI0_MISO |
                              AT91C_PA17_SPI0_MOSI |
                              AT91C_PA18_SPI0_SPCK;

  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIEN;
}

void spiDeinit(void)
{
  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIDIS;
  AT91C_BASE_PMC->PMC_PCDR = 1 << AT91C_ID_SPI0;
}

int spiConfigure( int channel, int bits, int clockDivider, int delayBeforeSPCK, int delayBetweenTransfers )
{
  // Check parameters
  if ( bits < 8 || bits > 16 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  if ( clockDivider < 0 || clockDivider > 255 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  if ( delayBeforeSPCK < 0 || delayBeforeSPCK > 255 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  if ( delayBetweenTransfers < 0 || delayBetweenTransfers > 255 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  AT91C_BASE_SPI0->SPI_CSR[ channel ] =
        AT91C_SPI_NCPHA | // Clock Phase TRUE
        ( ( ( bits - 8 ) << 4 ) & AT91C_SPI_BITS ) | // Transfer bits
        ( ( clockDivider << 8 ) & AT91C_SPI_SCBR ) | // Serial Clock Baud Rate Divider (255 = slow)
        ( ( delayBeforeSPCK << 16 ) & AT91C_SPI_DLYBS ) | // Delay before SPCK
        ( ( delayBetweenTransfers << 24 ) & AT91C_SPI_DLYBCT ); // Delay between transfers
  
  return CONTROLLER_OK;
}


int spiReadWriteBlock( int channel, unsigned char* buffer, int count )
{
  int i;

  // Make sure the unit is at rest before we re-begin
  while( !( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_TXEMPTY ) )
    ;
  while( ( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_RDRF ) )
    i = AT91C_BASE_SPI0->SPI_RDR;

  // Make the CS line hang around
  AT91C_BASE_SPI0->SPI_CSR[ channel ] |= AT91C_SPI_CSAAT;

  i = 0;
  while ( i++ < count ) {
    AT91C_BASE_SPI0->SPI_TDR = ( *buffer & 0xFF ) |
                               ( ( ~( 1 << channel ) << 16 ) &  AT91C_SPI_TPCS ) |
                               (int)( ( i == count ) ? AT91C_SPI_LASTXFER : 0 );

    while ( !( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_RDRF ) )
      ;

    *buffer++ = (unsigned char)( AT91C_BASE_SPI0->SPI_RDR & 0xFF );
  }
  AT91C_BASE_SPI0->SPI_CSR[ channel ] &= ~AT91C_SPI_CSAAT;
  return 0;
}

void spiLock()
{
  chMtxLock(&spiMutex);
}

void spiUnlock()
{
  chMtxUnlock();
}

int spiGetPin( int channel )
{
  switch ( channel ) {
    case 0: return SPI_SEL0_IO;
    case 1: return SPI_SEL1_IO;
    case 2: return SPI_SEL2_IO;
    case 3: return SPI_SEL3_IO;
    default: return 0;
  }
}

bool spiGetMode( int channel )
{  
  switch ( channel ) {
    case 0: return SPI_SEL0_MODE;
    case 1: return SPI_SEL1_MODE;
    case 2: return SPI_SEL2_MODE;
    case 3: return SPI_SEL3_MODE;
    default: return false;
  }
}
