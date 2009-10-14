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

#ifndef EEPROM_H
#define EEPROM_H

#include "types.h"
#include "spi.h"

/**
  Persistent storage of 32k bytes of data.
  If you want to store a little information that can be retrieved even after the board has 
  been powered down, then Eeprom is for you.  You don't have a TON of space, but 32K is a good amount
  for configuration info and even a tiny bit of logging.
  
  \section Usage
  Since there's only one internal EEPROM system, access is granted through the Eeprom::get()
  method.  So before any operations you'll need to use that to get a reference to it first.  See the
  example for get() for a sample.
  
  The last 1k (1024) bytes of this space are reserved for Make Controller system
  use, storing things like the board's IP address, serial number, build version, etc.  The symbol
   \b EEPROM_SYSTEM_BASE provides the last available address before the reserved section, so
  make sure that none of the addresses that you're writing to are greater than \b EEPROM_SYSTEM_BASE.
  
  Internally, Eeprom relies on \ref Spi, so activating Eeprom also activates \ref Spi.
*/

void eepromInit(void);
int  eepromRead(int address);
void eepromWrite(int address, int value);
int  eepromReadBlock(int address, uchar* data, int length);
int  eepromWriteBlock(int address, uchar *data, int length);

#define EEPROM_RESERVE_SIZE 1024
#define EEPROM_SIZE ( 32 * 1024 )
#define EEPROM_SYSTEM_BASE          ( EEPROM_SIZE - EEPROM_RESERVE_SIZE )
#define EEPROM_SYSTEM_SERIAL_NUMBER   EEPROM_SYSTEM_BASE + 0
#define EEPROM_SYSTEM_NOTUSED         EEPROM_SYSTEM_BASE + 4
#define EEPROM_SYSTEM_NET_CHECK       EEPROM_SYSTEM_BASE + 8
#define EEPROM_SYSTEM_NET_ADDRESS     EEPROM_SYSTEM_BASE + 12
#define EEPROM_SYSTEM_NET_MASK        EEPROM_SYSTEM_BASE + 16
#define EEPROM_SYSTEM_NET_GATEWAY     EEPROM_SYSTEM_BASE + 20
#define EEPROM_SYSTEM_END             EEPROM_SYSTEM_BASE + 24
#define EEPROM_POLY_0_TIMER_DURATION  EEPROM_SYSTEM_BASE + 28
#define EEPROM_POLY_0_TIMER_SUSTAIN   EEPROM_SYSTEM_BASE + 32
#define EEPROM_POLY_1_TIMER_DURATION  EEPROM_SYSTEM_BASE + 36
#define EEPROM_POLY_1_TIMER_SUSTAIN   EEPROM_SYSTEM_BASE + 40
#define EEPROM_POLY_0_FOLLOWER_RATE   EEPROM_SYSTEM_BASE + 44
#define EEPROM_POLY_0_FOLLOWER_PEAK   EEPROM_SYSTEM_BASE + 48
#define EEPROM_POLY_1_FOLLOWER_RATE   EEPROM_SYSTEM_BASE + 52
#define EEPROM_POLY_1_FOLLOWER_PEAK   EEPROM_SYSTEM_BASE + 56
#define EEPROM_POLY_0_OSCILLATOR_PERIODON  EEPROM_SYSTEM_BASE + 60
#define EEPROM_POLY_0_OSCILLATOR_PERIODOFF EEPROM_SYSTEM_BASE + 64
#define EEPROM_POLY_1_OSCILLATOR_PERIODON  EEPROM_SYSTEM_BASE + 68
#define EEPROM_POLY_1_OSCILLATOR_PERIODOFF EEPROM_SYSTEM_BASE + 72
#define EEPROM_OSC_UDP_LISTEN_PORT         EEPROM_SYSTEM_BASE + 76
#define EEPROM_TCP_AUTOCONNECT     EEPROM_SYSTEM_BASE + 80
#define EEPROM_TCP_OUT_ADDRESS     EEPROM_SYSTEM_BASE + 84
#define EEPROM_TCP_OUT_PORT        EEPROM_SYSTEM_BASE + 88
#define EEPROM_DHCP_ENABLED           EEPROM_SYSTEM_BASE + 92
#define EEPROM_SYSTEM_NAME            EEPROM_SYSTEM_BASE + 96 // this is 100 bytes long
#define EEPROM_WEBSERVER_ENABLED      EEPROM_SYSTEM_BASE + 196
#define EEPROM_OSC_UDP_SEND_PORT      EEPROM_SYSTEM_BASE + 200
#define EEPROM_OSC_ASYNC_DEST      EEPROM_SYSTEM_BASE + 204
#define EEPROM_XBEE_AUTOSEND      EEPROM_SYSTEM_BASE + 208
#define EEPROM_DIPSWITCH_AUTOSEND      EEPROM_SYSTEM_BASE + 212
#define EEPROM_ANALOGIN_AUTOSEND      EEPROM_SYSTEM_BASE + 216
#define EEPROM_OSC_ASYNC_INTERVAL      EEPROM_SYSTEM_BASE + 220

#endif
