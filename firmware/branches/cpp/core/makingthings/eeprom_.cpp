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

#include "eeprom_.h"

#define EEPROM_INSTRUCTION_WREN 0x06
#define EEPROM_INSTRUCTION_WRDI 0x04
#define EEPROM_INSTRUCTION_RDSR 0x05
#define EEPROM_INSTRUCTION_WRSR 0x01
#define EEPROM_INSTRUCTION_READ 0x03
#define EEPROM_INSTRUCTION_WRITE 0x02

#define EEPROM_DEVICE 0x03
//#define EEPROM_NOTCS 0x0B  

Eeprom* Eeprom::_instance = 0;

Eeprom::Eeprom( )
{
  spi = new SPI( EEPROM_DEVICE );
  spi->configure( 8, 4, 0, 1 );
}

void Eeprom::writeEnable( )
{
  uchar c = EEPROM_INSTRUCTION_WREN;
  spi->readWriteBlock( &c, 1 );
}

void Eeprom::ready( )
{
  int status;
  unsigned char c[ 2 ];
  do
  {
    c[ 0 ] = EEPROM_INSTRUCTION_RDSR;
    c[ 1 ] = 0;
    spi->readWriteBlock( c, 2 );
    status = c[ 1 ] != 0xFF;
  } while ( !status );
}

int Eeprom::read( int address )
{
  int val;
  readBlock( address, (uchar*)&val, 4 );
  return val;
}

void Eeprom::write(int address, int value)
{
  writeBlock( address, (uchar*)&value, 4 );
}

int Eeprom::readBlock(int address, uchar* buffer, int count)
{
  if ( address < 0 || address > EEPROM_SIZE )
    return CONTROLLER_ERROR_BAD_ADDRESS;

  spi->lock();
  ready( );

  unsigned char c[ count + 4 ];
  c[ 0 ] = EEPROM_INSTRUCTION_READ;
  c[ 1 ] = (unsigned char)( address >> 8 );
  c[ 2 ] = (unsigned char)( address & 0xFF );
  c[ 3 ] = 0;
  
  spi->readWriteBlock( c, count + 3 );

  int i;
  for ( i = 0; i < count; i++ )
    buffer[ i ] = c[ i + 3 ];

  spi->unlock();

  return CONTROLLER_OK;
}

int Eeprom::writeBlock(int address, uchar *buffer, int count)
{
  if ( address < 0 || address >= EEPROM_SIZE )
    return CONTROLLER_ERROR_BAD_ADDRESS;

  spi->lock();
  ready( );
  writeEnable();    

  uchar c[ count + 4 ];
  c[ 0 ] = EEPROM_INSTRUCTION_WRITE;
  c[ 1 ] = (unsigned char)( address >> 8 );
  c[ 2 ] = (unsigned char)( address & 0xFF );
  c[ 3 ] = 0;

  int i;
  for ( i = 0; i < count; i++ )
    c[ i + 3 ] = buffer[ i ];
  
  spi->readWriteBlock( c, 3 + count );
  spi->unlock();

  return CONTROLLER_OK;
}






