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

#define EEPROM_INSTRUCTION_WREN 0x06
#define EEPROM_INSTRUCTION_WRDI 0x04
#define EEPROM_INSTRUCTION_RDSR 0x05
#define EEPROM_INSTRUCTION_WRSR 0x01
#define EEPROM_INSTRUCTION_READ 0x03
#define EEPROM_INSTRUCTION_WRITE 0x02

#define EEPROM_DEVICE 0x03
//#define EEPROM_NOTCS 0x0B

/**
  \defgroup eeprom EEPROM
  Persistent storage of 32k bytes of data.
  If you want to store a little information that can be retrieved even after the board has
  been powered down, then Eeprom is for you.  You don't have a TON of space, but 32K is a good amount
  for configuration info and even a tiny bit of logging.

  \section Usage
  eepromInit() must be called before using the EEPROM - this is done automatically during system
  startup, but you can disable it by defining \b NO_EEPROM_INIT in your config.h file.  Once it's
  initialized, read and write single int values with eepromRead() and eepromWrite(), or write blocks
  of data with eepromReadBlock() and eepromWriteBlock().

  The last 1k (1024) bytes of this space are reserved for Make Controller system
  use, storing things like the board's IP address, serial number, build version, etc.  The symbol
  \b EEPROM_SYSTEM_BASE provides the last available address before the reserved section, so
  make sure that none of the addresses that you're writing to are greater than \b EEPROM_SYSTEM_BASE.

  Internally, Eeprom relies on \ref SPI, so activating Eeprom also activates \ref SPI.
  \ingroup Core
  @{
*/

/**
  Initialize the EEPROM system.
*/
void eepromInit()
{
  spiConfigure(Spi0, EEPROM_DEVICE, 8, 16, 0, 1);
}

static void eepromWriteEnable(void)
{
  uint8_t c = EEPROM_INSTRUCTION_WREN;
  spiReadWriteBlock(Spi0, EEPROM_DEVICE, &c, 1);
}

static void eepromReady(void)
{
  uint8_t c[2];
  do {
    c[0] = EEPROM_INSTRUCTION_RDSR;
    c[1] = 0;
    spiReadWriteBlock(Spi0, EEPROM_DEVICE, c, 2);
  } while ((c[1] & 1) != 0);
}

/**
  Read an int from EEPROM.
  @param address The address to read from.
  @return The value stored at that address.
  
  \b Example
  \code
  int myvalue = eepromRead(32); // read from address 32
  \endcode
*/
int eepromRead(int address)
{
  int val;
  eepromReadBlock(address, (uint8_t*)&val, 4);
  return val;
}

/**
  Write an int to EEPROM.
  @param address The address to write to.
  @param value The value to store
  
  \b Example
  \code
  int valueToStore = 95;
  eepromWrite(32, valueToStore);
  \endcode
*/
void eepromWrite(int address, int value)
{
  eepromWriteBlock(address, (uint8_t*)&value, 4);
}

/**
  Read a block of data from EEPROM.
  @param address The address to start reading from
  @param data Where to read the data into
  @param length How many bytes of data to read
  @return 0 on OK, otherwise less than 0
  
  \b Example
  \code
  uint8_t mydata[24];
  eepromReadBlock(32, mydata, 24);
  \endcode
*/
int eepromReadBlock(int address, uint8_t* data, int length)
{
  if (address < 0 || address > EEPROM_SIZE)
    return CONTROLLER_ERROR_BAD_ADDRESS;

  spiLock(Spi0);
  eepromReady();

  unsigned char c[length + 4];
  c[0] = EEPROM_INSTRUCTION_READ;
  c[1] = (unsigned char)(address >> 8);
  c[2] = (unsigned char)(address & 0xFF);
  c[3] = 0;
  
  spiReadWriteBlock(Spi0, EEPROM_DEVICE, c, length + 3);

  int i;
  for (i = 0; i < length; i++)
    data[i] = c[i + 3];

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
  uint8_t mydata[24];
  eepromWriteBlock(32, mydata, 24);
  \endcode
*/
int eepromWriteBlock(int address, uint8_t *data, int length)
{
  if (address < 0 || address >= EEPROM_SIZE)
    return CONTROLLER_ERROR_BAD_ADDRESS;

  spiLock(Spi0);
  eepromReady();
  eepromWriteEnable();    

  uint8_t c[length + 4];
  c[0] = EEPROM_INSTRUCTION_WRITE;
  c[1] = (uint8_t)(address >> 8);
  c[2] = (uint8_t)(address & 0xFF);
  c[3] = 0;

  int i;
  for (i = 0; i < length; i++)
    c[i + 3] = data[i];
  
  spiReadWriteBlock(Spi0, EEPROM_DEVICE, c, 3 + length);
  spiUnlock();

  return CONTROLLER_OK;
}

/** @} */





