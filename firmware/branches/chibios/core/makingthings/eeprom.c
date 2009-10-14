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

#include "eeprom.h"
#include "error.h"
#include "pin.h"

#define EEPROM_INSTRUCTION_WREN 0x06
#define EEPROM_INSTRUCTION_WRDI 0x04
#define EEPROM_INSTRUCTION_RDSR 0x05
#define EEPROM_INSTRUCTION_WRSR 0x01
#define EEPROM_INSTRUCTION_READ 0x03
#define EEPROM_INSTRUCTION_WRITE 0x02

#define EEPROM_DEVICE 0x03
//#define EEPROM_NOTCS 0x0B  

static void eepromReady(void);
static void eepromWriteEnable(void);

void eepromInit()
{
  spiEnableChannel(EEPROM_DEVICE);
  spiConfigure(EEPROM_DEVICE, 8, 4, 0, 1);
}

void eepromWriteEnable()
{
  uchar c = EEPROM_INSTRUCTION_WREN;
  spiReadWriteBlock( EEPROM_DEVICE, &c, 1 );
}

void eepromReady( )
{
  unsigned char c[2];
  do
  {
    c[0] = EEPROM_INSTRUCTION_RDSR;
    c[1] = 0;
    spiReadWriteBlock( EEPROM_DEVICE, c, 2 );
  } while ( c[1] == 0xFF );
}

/**
  Read an individual character from EEPROM.
  @param address The address to read from.
  @return The value stored at that address.
  
  \b Example
  \code
  Eeprom* e = Eeprom::get(); // get a reference to the EEPROM
  int myvalue = e->read(32); // read from address 32
  \endcode
*/
int eepromRead( int address )
{
  int val;
  eepromReadBlock( address, (uchar*)&val, 4 );
  return val;
}

/**
  Write an individual character to EEPROM.
  @param address The address to write to.
  @param value The value to store
  
  \b Example
  \code
  Eeprom* e = Eeprom::get( ); // get a reference to the EEPROM
  int valueToStore = 95;
  e->write(32, valueToStore);
  \endcode
*/
void eepromWrite(int address, int value)
{
  eepromWriteBlock( address, (uchar*)&value, 4 );
}

/**
  Read a block of data from EEPROM.
  @param address The address to start reading from
  @param data Where to read the data into
  @param length How many bytes of data to read
  @return 0 on OK, otherwise less than 0
  
  \b Example
  \code
  uchar mydata[24];
  Eeprom* e = Eeprom::get( ); // get a reference to the EEPROM
  e->readBlock(32, mydata, 24);
  \endcode
*/
int eepromReadBlock(int address, uchar* data, int length)
{
  if ( address < 0 || address > EEPROM_SIZE )
    return CONTROLLER_ERROR_BAD_ADDRESS;

  spiLock();
  eepromReady( );

  unsigned char c[ length + 4 ];
  c[ 0 ] = EEPROM_INSTRUCTION_READ;
  c[ 1 ] = (unsigned char)( address >> 8 );
  c[ 2 ] = (unsigned char)( address & 0xFF );
  c[ 3 ] = 0;
  
  spiReadWriteBlock( EEPROM_DEVICE, c, length + 3 );

  int i;
  for ( i = 0; i < length; i++ )
    data[ i ] = c[ i + 3 ];

  spiUnlock();

  return CONTROLLER_OK;
}

/**
  Write a block of data to EEPROM.
  @param address The address to start writing at
  @param data The data to write
  @param length How many bytes of data to write
  
  \b Example
  \code
  uchar mydata[24];
  Eeprom* e = Eeprom::get( ); // get a reference to the EEPROM
  e->writeBlock(32, mydata, 24);
  \endcode
*/
int eepromWriteBlock(int address, uchar *data, int length)
{
  if ( address < 0 || address >= EEPROM_SIZE )
    return CONTROLLER_ERROR_BAD_ADDRESS;

  spiLock();
  eepromReady( );
  eepromWriteEnable();    

  uchar c[ length + 4 ];
  c[ 0 ] = EEPROM_INSTRUCTION_WRITE;
  c[ 1 ] = (unsigned char)( address >> 8 );
  c[ 2 ] = (unsigned char)( address & 0xFF );
  c[ 3 ] = 0;

  int i;
  for ( i = 0; i < length; i++ )
    c[ i + 3 ] = data[ i ];
  
  spiReadWriteBlock( EEPROM_DEVICE, c, 3 + length );
  spiUnlock();

  return CONTROLLER_OK;
}






