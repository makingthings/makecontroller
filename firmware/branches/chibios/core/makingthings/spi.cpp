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
#include "AT91SAM7X256.h"


// Define the SPI select lines 
// and which peripheral they are on that line
#if ( CONTROLLER_VERSION == 50 )
  #define SPI_SEL0_IO           IO_PA12
  #define SPI_SEL0_PERIPHERAL_A 1 
  #define SPI_SEL1_IO           IO_PA13
  #define SPI_SEL1_PERIPHERAL_A 1 
  #define SPI_SEL2_IO           IO_PA08
  #define SPI_SEL2_PERIPHERAL_A 0 
  #define SPI_SEL3_IO           IO_PA09
  #define SPI_SEL3_PERIPHERAL_A 0
#elif ( CONTROLLER_VERSION == 90 )
  #define SPI_SEL0_IO           IO_PA12
  #define SPI_SEL0_PERIPHERAL_A 1 
  #define SPI_SEL1_IO           IO_PA13
  #define SPI_SEL1_PERIPHERAL_A 1 
  #define SPI_SEL2_IO           IO_PB14
  #define SPI_SEL2_PERIPHERAL_A 0 
  #define SPI_SEL3_IO           IO_PB17
  #define SPI_SEL3_PERIPHERAL_A 0
#elif ( CONTROLLER_VERSION >= 95 )
  #define SPI_SEL0_IO           IO_PA12
  #define SPI_SEL0_PERIPHERAL_A 1 
  #define SPI_SEL1_IO           IO_PA13
  #define SPI_SEL1_PERIPHERAL_A 1 
  #define SPI_SEL2_IO           IO_PA08
  #define SPI_SEL2_PERIPHERAL_A 0 
  #define SPI_SEL3_IO           IO_PA09
  #define SPI_SEL3_PERIPHERAL_A 0
#endif

// static
int Spi::refcount = 0;

Spi::Spi( int channel )
{
  if( channel < 0 || channel > 3 )
    return;

  _channel = channel;
  if(refcount++ == 0)
    init();
  Io::Peripheral io_type = getChannelPeripheralA( channel ) ? Io::A : Io::B;
  chan = new Io( getIO(channel), io_type );
}

void Spi::init( )
{
  // Reset it
  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SWRST;
    
    // Must confirm the peripheral clock is running
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_SPI0;

  // DON'T USE FDIV FLAG - it makes the SPI unit fail!!

  AT91C_BASE_SPI0->SPI_MR = 
       AT91C_SPI_MSTR | // Select the master
       AT91C_SPI_PS_VARIABLE |
       AT91C_SPI_PCS | // Variable Addressing - no address here
    // AT91C_SPI_PCSDEC | // Select address decode
    // AT91C_SPI_FDIV | // Select Master Clock / 32 - PS DON'T EVER SET THIS>>>>  SAM7 BUG
       AT91C_SPI_MODFDIS | // Disable fault detect
    // AT91C_SPI_LLB | // Enable loop back test
       ( ( 0x0 << 24 ) & AT91C_SPI_DLYBCS ) ;  // Delay between chip selects

  AT91C_BASE_SPI0->SPI_IDR = 0x3FF; // All interupts are off

  // Set up the IO lines for the peripheral
  // Disable their peripherality
  AT91C_BASE_PIOA->PIO_PDR = 
      AT91C_PA16_SPI0_MISO | 
      AT91C_PA17_SPI0_MOSI | 
      AT91C_PA18_SPI0_SPCK;

  // Kill the pull up on the Input
  AT91C_BASE_PIOA->PIO_PPUDR = AT91C_PA16_SPI0_MISO;

  // Make sure the input isn't an output
  AT91C_BASE_PIOA->PIO_ODR = AT91C_PA16_SPI0_MISO;

  // Select the correct Devices
  AT91C_BASE_PIOA->PIO_ASR = 
      AT91C_PA16_SPI0_MISO | 
      AT91C_PA17_SPI0_MOSI | 
      AT91C_PA18_SPI0_SPCK;

  // Elsewhere need to do this for the select lines
  // AT91C_BASE_PIOB->PIO_BSR = 
  //    AT91C_PB17_NPCS03; 

  // Fire it up
  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIEN;
  return;
}

Spi::~Spi( )
{
  if(--refcount <= 0)
    AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIDIS;
}

int Spi::configure( int bits, int clockDivider, int delayBeforeSPCK, int delayBetweenTransfers )
{
  if( !valid())
    return 0;
  // Check parameters
  if ( bits < 8 || bits > 16 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  if ( clockDivider < 0 || clockDivider > 255 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  if ( delayBeforeSPCK < 0 || delayBeforeSPCK > 255 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  if ( delayBetweenTransfers < 0 || delayBetweenTransfers > 255 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  // Set the values
  AT91C_BASE_SPI0->SPI_CSR[ _channel ] = 
      AT91C_SPI_NCPHA | // Clock Phase TRUE
      ( ( ( bits - 8 ) << 4 ) & AT91C_SPI_BITS ) | // Transfer bits
      ( ( clockDivider << 8 ) & AT91C_SPI_SCBR ) | // Serial Clock Baud Rate Divider (255 = slow)
      ( ( delayBeforeSPCK << 16 ) & AT91C_SPI_DLYBS ) | // Delay before SPCK
      ( ( delayBetweenTransfers << 24 ) & AT91C_SPI_DLYBCT ); // Delay between transfers
  
  return CONTROLLER_OK;
}


int Spi::readWriteBlock( unsigned char* buffer, int count )
{
  if( !valid())
    return 0;
  int r;
  int address = ~( 1 << _channel );

  if ( !( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_TXEMPTY ) ) // Make sure the unit is at rest before we re-begin
  {
    while( !( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_TXEMPTY ) );
    while( !( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_RDRF ) )
      r = AT91C_BASE_SPI0->SPI_SR;
    r = AT91C_BASE_SPI0->SPI_RDR;
  }

  if ( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_RDRF )
    r = AT91C_BASE_SPI0->SPI_RDR;
  
  AT91C_BASE_SPI0->SPI_CSR[ _channel ] |= AT91C_SPI_CSAAT; // Make the CS line hang around

  int writeIndex = 0;
  unsigned char* writeP = buffer;
  unsigned char* readP = buffer;

  while ( writeIndex < count ) // Do the read write
  {
    writeIndex++;
    AT91C_BASE_SPI0->SPI_TDR = ( *writeP++ & 0xFF ) | 
                               ( ( address << 16 ) &  AT91C_SPI_TPCS ) | 
                               (int)( ( writeIndex == count ) ? AT91C_SPI_LASTXFER : 0 );

    while ( !( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_RDRF ) );
    *readP++ = (unsigned char)( AT91C_BASE_SPI0->SPI_RDR & 0xFF );
  }

  AT91C_BASE_SPI0->SPI_CSR[ _channel ] &= ~AT91C_SPI_CSAAT;

  return 0;
}

int Spi::getIO( int channel )
{
  switch ( channel )
  {
    case 0: return SPI_SEL0_IO;
    case 1: return SPI_SEL1_IO;
    case 2: return SPI_SEL2_IO;
    case 3: return SPI_SEL3_IO;
    default: return 0;
  }
}

int Spi::getChannelPeripheralA( int channel )
{  
  switch ( channel )
  {
    case 0: return SPI_SEL0_PERIPHERAL_A;
    case 1: return SPI_SEL1_PERIPHERAL_A;
    case 2: return SPI_SEL2_PERIPHERAL_A;
    case 3: return SPI_SEL3_PERIPHERAL_A;
    default: return -1;
  }
}
