/*********************************************************************************

 Copyright 2006 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

/** \file eeprom.c	
	EEPROM.
	Functions for working with the EEPROM on the Make Controller Board.
*/

/* Library includes. */
#include <string.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Hardware specific headers. */
#include "Board.h"
#include "AT91SAM7X256.h"

#include "eeprom.h"
#include "spi.h"

#include "config.h"
#include "io.h"

#define EEPROM_INSTRUCTION_WREN 0x06
#define EEPROM_INSTRUCTION_WRDI 0x04
#define EEPROM_INSTRUCTION_RDSR 0x05
#define EEPROM_INSTRUCTION_WRSR 0x01
#define EEPROM_INSTRUCTION_READ 0x03
#define EEPROM_INSTRUCTION_WRITE 0x02

#define EEPROM_DEVICE 0x03
//#define EEPROM_NOTCS 0x0B  

static int  Eeprom_Start( void );
static int  Eeprom_Stop( void );

static void Eeprom_WriteEnable( void );
static void Eeprom_WriteDisable( void );

static void Eeprom_Ready( void );

static int Eeprom_users;

/** \defgroup Eeprom
	Eeprom allows for the persistent storage of 32k bytes data. 
  The last 1k (1024) bytes of this space are reserved for system
  use storing things like the board's IP address, serial number, build version, etc.  Get the last
  EEPROM address available before this reserved section with a call to Eeprom_GetLastAddress( ).

  Use Eeprom_Write() and Eeprom_Read() to store and retrieve characters.  These both internally set
  the EEPROM system to active automatically.  

  Internally, Eeprom relies on \ref Spi, so activating Eeprom also activates \ref Spi.

	\ingroup Controller
	@{
*/

/**
	Set the active state of the EEPROM subsystem.  This is automatically set to 
  true by any call to Eeprom_Write or Eeprom_Read.
	@param state An integer specifying the active state - 1 (on) or 0 (off).
	@return CONTROLLER_OK (=0) on success.
*/
int Eeprom_SetActive( int state )
{
  if ( state )
    return Eeprom_Start();
  else
    return Eeprom_Stop();
}

/**
	Read the active state of the EEPROM.
	@return State - 1/non-zero (on) or 0 (off).
*/
int Eeprom_GetActive( )
{
  return Eeprom_users > 0;
}

/**	
	Write data to the EEPROM.
	@param address An integer specifying the address.
	@param buffer A pointer to the buffer to write from.
	@param count An integer specifying the number of bytes to write.
  @return status.
*/
int Eeprom_Write( int address, uchar* buffer, int count )
{
  if ( address < 0 || address >= EEPROM_SIZE )
    return CONTROLLER_ERROR_BAD_ADDRESS;

  if ( Eeprom_users == 0 )
  {
    int status = Eeprom_Start();
    if ( status != CONTROLLER_OK )
      return status;
  }

  Spi_Lock();

  Eeprom_Ready( );

  Eeprom_WriteEnable();    

  uchar c[ count + 4 ];

  c[ 0 ] = EEPROM_INSTRUCTION_WRITE;
  c[ 1 ] = (unsigned char)( address >> 8 );
  c[ 2 ] = (unsigned char)( address & 0xFF );
  c[ 3 ] = 0;

  int i;
  for ( i = 0; i < count; i++ )
  {
    c[ i + 3 ] = buffer[ i ];
  }
  
  Spi_ReadWriteBlock( EEPROM_DEVICE, c, 3 + count );

  Spi_Unlock();

  return CONTROLLER_OK;
}

/**	
	Read data from the EEPROM.
	@param address An integer specifying the address to read from.
	@param buffer A pointer to the buffer to read into.
	@param count An integer specifying the number of bytes to read.
  @return status.
*/
int Eeprom_Read( int address, uchar* buffer, int count )
{
  if ( address < 0 || address > EEPROM_SIZE )
    return CONTROLLER_ERROR_BAD_ADDRESS;

  if ( Eeprom_users == 0 )
  {
    int status = Eeprom_Start();
    if ( status != CONTROLLER_OK )
      return status;
  }

  Spi_Lock();

  Eeprom_Ready( );

  unsigned char c[ count + 4 ];

  c[ 0 ] = EEPROM_INSTRUCTION_READ;
  c[ 1 ] = (unsigned char)( address >> 8 );
  c[ 2 ] = (unsigned char)( address & 0xFF );
  c[ 3 ] = 0;
  
  Spi_ReadWriteBlock( EEPROM_DEVICE, c, count + 3 );

  int i;
  for ( i = 0; i < count; i++ )
  {
    buffer[ i ] = c[ i + 3 ];
  }

  Spi_Unlock();

  return CONTROLLER_OK;
}

/** 
  Returns the last usable address in the EEPROM
  @return address
*/
int Eeprom_GetLastAddress( )
{
  return EEPROM_SIZE - EEPROM_RESERVE_SIZE;
}

/** @}
*/

int Eeprom_Start()
{
  int status;

  if ( Eeprom_users++ == 0 )
  {    
    // Attempt to open the SPI device
    status = Spi_Start( EEPROM_DEVICE );
    if ( status != CONTROLLER_OK )
    {
      Eeprom_users--;
      return status;
    }
  
    // We're here means that we got the Spi open, now set it up
    status = Spi_Configure( EEPROM_DEVICE, 8, 4, 0, 1 );
    if ( status != CONTROLLER_OK )
    {
      // Undo all the setup.  Sigh.
      Eeprom_Stop();
      return status;
    }
  }
  
  return CONTROLLER_OK;
}

int Eeprom_Stop()
{
  if ( Eeprom_users <= 0 )
    return CONTROLLER_ERROR_NOT_LOCKED;

  if ( --Eeprom_users == 0 )
    Spi_Stop( EEPROM_DEVICE );

  return CONTROLLER_OK;
}

/**	
	Eeprom_WriteEnable.
	Enable writing from the EEPROM on the Make Controller Board. 
  This function should be called before making calls to Eeprom_Write().
	@param none.
  @return none.
*/
void Eeprom_WriteEnable()
{
  uchar c;
  c = EEPROM_INSTRUCTION_WREN;
  
  Spi_ReadWriteBlock( EEPROM_DEVICE, &c, 1 );
}

/**	
	Disable writing from the EEPROM on the Make Controller Board.
	@param none.
  @return none.
*/
void Eeprom_WriteDisable()
{
  uchar c;
  c = EEPROM_INSTRUCTION_WRDI;
  
  Spi_ReadWriteBlock( EEPROM_DEVICE, &c, 1 );
}

void Eeprom_Ready( void )
{
  int status;
  do
  {
    unsigned char c[ 2 ];

    c[ 0 ] = EEPROM_INSTRUCTION_RDSR;
    c[ 1 ] = 0;
  
    Spi_ReadWriteBlock( EEPROM_DEVICE, c, 2 );

    status = c[ 1 ] != 0xFF;

  } while ( !status );
}

