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

/** \file spi.c	
	SPI - Serial Peripheral Interface Controller.
	Methods for communicating with devices via the SPI protocol.
  //ToDo: need to do so locking around the Start code to make it thread safe
	//ToDo: make the naming scheme consistent for the SetActive() business
*/

/* Library includes. */
#include <string.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Hardware specific headers. */
#include "Board.h"
#include "AT91SAM7X256.h"

#include "spi.h"
#include "io.h"
#include "config.h"

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
#elif ( CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 || CONTROLLER_VERSION == 200 )
  #define SPI_SEL0_IO           IO_PA12
  #define SPI_SEL0_PERIPHERAL_A 1 
  #define SPI_SEL1_IO           IO_PA13
  #define SPI_SEL1_PERIPHERAL_A 1 
  #define SPI_SEL2_IO           IO_PA08
  #define SPI_SEL2_PERIPHERAL_A 0 
  #define SPI_SEL3_IO           IO_PA09
  #define SPI_SEL3_PERIPHERAL_A 0
#endif
 
struct Spi_
{
  int users;
  int channels;
  xSemaphoreHandle semaphore;
} Spi;

static int  Spi_Init( void );
static void Spi_Deinit( void );
static int  Spi_GetChannelIo( int channel );
static int  Spi_GetChannelPeripheralA( int channel );

/** \defgroup Spi SPI
   The SPI subsystem allows the MAKE Controller to communicate with peripheral devices via SPI.  
   See \ref Eeprom module for an example use.
  
   \ingroup Core
* @{
*/

/**
	Configure the SPI channel.
  This function locks the I/O lines associated with the SPI channel so they cannot be
	used by another device on the same lines.
  @param channel An integer specifying the SPI channel to configure.
  @param bits An integer specifying the number of bits to transfer (8-16).
  @param clockDivider An integer specifying the desired divider from the serial clock (0 - 255).
  @param delayBeforeSPCK An integer specifying the delay, in ms, before the clock starts (0 - 255).
  @param delayBetweenTransfers An integer specifying how long, in ms, to wait between transfers (0 - 255).
	@return 0 on success.
*/
int Spi_Configure( int channel, int bits, int clockDivider, int delayBeforeSPCK, int delayBetweenTransfers )
{
  // Make sure the channel is locked (by someone!)
  int c = 1 << channel;
  if ( !( c & Spi.channels ) )
    return CONTROLLER_ERROR_NOT_LOCKED;

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
  AT91C_BASE_SPI0->SPI_CSR[ channel ] = 
      AT91C_SPI_NCPHA | // Clock Phase TRUE
      ( ( ( bits - 8 ) << 4 ) & AT91C_SPI_BITS ) | // Transfer bits
      ( ( clockDivider << 8 ) & AT91C_SPI_SCBR ) | // Serial Clock Baud Rate Divider (255 = slow)
      ( ( delayBeforeSPCK << 16 ) & AT91C_SPI_DLYBS ) | // Delay before SPCK
      ( ( delayBetweenTransfers << 24 ) & AT91C_SPI_DLYBCT ); // Delay between transfers
  
  return CONTROLLER_OK;
}

/**	
	Read/Write a block of data via SPI.
	@param channel An integer specifying the device to communicate with.
	@param buffer A pointer to the buffer to read to, or write from.
	@param count An integer specifying the length in bytes of the buffer to read/write.
	@return 0 on success.
*/
int Spi_ReadWriteBlock( int channel, unsigned char* buffer, int count )
{
  int r;

  int address = ~( 1 << channel );

  // Make sure the unit is at rest before we re-begin
  if ( !( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_TXEMPTY ) )
  {
    while( !( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_TXEMPTY ) )
      ;
    while( !( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_RDRF ) )
      r = AT91C_BASE_SPI0->SPI_SR;
    r = AT91C_BASE_SPI0->SPI_RDR;
  }

  if ( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_RDRF )
    r = AT91C_BASE_SPI0->SPI_RDR;

  // Make the CS line hang around
  AT91C_BASE_SPI0->SPI_CSR[ channel ] |= AT91C_SPI_CSAAT;

  int writeIndex = 0;

  unsigned char* writeP = buffer;
  unsigned char* readP = buffer;

  while ( writeIndex < count )
  {
    // Do the read write
    writeIndex++;
    AT91C_BASE_SPI0->SPI_TDR = ( *writeP++ & 0xFF ) | 
                               ( ( address << 16 ) &  AT91C_SPI_TPCS ) | 
                               (int)( ( writeIndex == count ) ? AT91C_SPI_LASTXFER : 0 );

    while ( !( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_RDRF ) )
    ;

    *readP++ = (unsigned char)( AT91C_BASE_SPI0->SPI_RDR & 0xFF );
  }

  AT91C_BASE_SPI0->SPI_CSR[ channel ] &= ~AT91C_SPI_CSAAT;

  return 0;
}

/** @}
*/

int Spi_Lock( void )
{
  return xSemaphoreTake( Spi.semaphore, 1000 );
}

void Spi_Unlock( void )
{
  xSemaphoreGive( Spi.semaphore );
}

int Spi_Start( int channel )
{
  int status;

  if ( channel < 0 || channel > 3 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  // Make sure the channel isn't already being used
  int c = 1 << channel;
  if ( c & Spi.channels )
    return CONTROLLER_ERROR_CANT_LOCK;

  // lock the correct select line 
  int io = Spi_GetChannelIo( channel );
  // Try to lock the pin
  status = Io_Start( io, true );
  if ( status != CONTROLLER_OK )
    return status;

  // Disable the PIO for the IO Line
  Io_SetPio( io, false );

  // Select the correct peripheral for the IO line
  int peripheralA = Spi_GetChannelPeripheralA( channel );
  if ( peripheralA )
    Io_SetPeripheralA( io );
  else
    Io_SetPeripheralB( io );

  if ( Spi.users == 0 )
  {
    status = Spi_Init();
    if ( status != CONTROLLER_OK )
    {
      // Couldn't init for some reason, need to undo the IO lock
      Io_Stop( io );
      // now return with the reason for the failure
      return status;
    }
    Spi.users++;
  }

  // Set the channel to locked
  Spi.channels |= c;

  return CONTROLLER_OK;
}

int Spi_Stop( int channel )
{
  if ( channel < 0 || channel > 3 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;
      
  if ( Spi.users <= 0 )
    return CONTROLLER_ERROR_TOO_MANY_STOPS;

  int c = 1 << channel;
  if ( !( c & Spi.channels ) )
    return CONTROLLER_ERROR_NOT_LOCKED;

  // release the IO
  int io = Spi_GetChannelIo( channel );
  // release the pin
  Io_Stop( io );

  Spi.channels &= ~c;

  if ( --Spi.users == 0 )
    Spi_Deinit();

  return CONTROLLER_OK;
}

int Spi_Init()
{
    int status;
    status = Io_StartBits( IO_PA16_BIT | IO_PA17_BIT | IO_PA18_BIT, true );
    if ( status != CONTROLLER_OK )
      return status;
  
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
        AT91C_PA16_MISO0 | 
        AT91C_PA17_MOSI0 | 
        AT91C_PA18_SPCK0; 
  
    // Kill the pull up on the Input
    AT91C_BASE_PIOA->PIO_PPUDR = AT91C_PA16_MISO0;
  
    // Make sure the input isn't an output
    AT91C_BASE_PIOA->PIO_ODR = AT91C_PA16_MISO0;
  
    // Select the correct Devices
    AT91C_BASE_PIOA->PIO_ASR = 
        AT91C_PA16_MISO0 | 
        AT91C_PA17_MOSI0 | 
        AT91C_PA18_SPCK0; 
  
    // Elsewhere need to do this for the select lines
    // AT91C_BASE_PIOB->PIO_BSR = 
    //    AT91C_PB17_NPCS03; 
  
    // Fire it up
    AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIEN;
  
    if ( !Spi.semaphore )
      vSemaphoreCreateBinary( Spi.semaphore );

    return CONTROLLER_OK;
}

void Spi_Deinit()
{
  // Shut down the SPI
  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIDIS;
 
  // Kill the semaphore
  // vSemaphoreDestroyBinary( Spi.semaphore );
  // Spi.semaphore = null;
  
  Io_StopBits( IO_PA16_BIT | IO_PA17_BIT | IO_PA18_BIT );
}

int Spi_GetChannelIo( int channel )
{
  int io;
  switch ( channel )
  {
    case 0:
      io = SPI_SEL0_IO;
      break;
    case 1:
      io = SPI_SEL1_IO;
      break;
    case 2:
      io = SPI_SEL2_IO;
      break;
    case 3:
      io = SPI_SEL3_IO;
      break;
    default:
      io = 0;
      break;
  }
  return io;
}

int Spi_GetChannelPeripheralA( int channel )
{  
  int peripheralA;
  switch ( channel )
  {
    case 0:
      peripheralA = SPI_SEL0_PERIPHERAL_A;
      break;
    case 1:
      peripheralA = SPI_SEL1_PERIPHERAL_A;
      break;
    case 2:
      peripheralA = SPI_SEL2_PERIPHERAL_A;
      break;
    case 3:
      peripheralA = SPI_SEL3_PERIPHERAL_A;
      break;
    default:
      peripheralA = -1;
      break;
  }
  return peripheralA;
}
