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

/*
	EEPROM.h
  MakingThings
*/

#ifndef EEPROM_H
#define EEPROM_H

#include "types.h"

int Eeprom_SetActive( int state );
int Eeprom_GetActive( void );

int Eeprom_GetLastAddress( void );

int Eeprom_Write( int address, uchar *buffer, int count );
int Eeprom_Read( int address, uchar* buffer, int count );

/**
\ingroup Eeprom
@{

\def EEPROM_RESERVE_SIZE
Sets the amount of the EEPROM reserved for system information.

\def EEPROM_SIZE
The size of the EEPROM on the MAKE Controller Board.

\def EEPROM_SYSTEM_BASE
The location in Eeprom that the "System" information begins.

\def EEPROM_SYSTEM_SERIAL_NUMBER
The location in Eeprom where the Controller's serial number is stored.

\def EEPROM_SYSTEM_NET_CHECK
The location in Eeprom of the checksum that validates the IP settings.

\def EEPROM_SYSTEM_NET_ADDRESS
The location in Eeprom of the Controller's IP address.

\def EEPROM_SYSTEM_NET_MASK
The location in Eeprom where the IP address mask is stored.

\def EEPROM_SYSTEM_NET_GATEWAY
The location in Eeprom where the IP address of the gateway is stored.

\def EEPROM_SYSTEM_END
The location of the end of the "System" information in Eeprom.

\def EEPROM_UDP_PORT
The location in EEPROM where the local UDP port is stored.

\def EEPROM_TCP_CONNECT_ON_REBOOT
The location in EEPROM where the flag to automatically make a connection 
to a remote TCP server is stored.

\def EEPROM_TCP_REMOTE_ADDRESS
The location in EEPROM where the address for the remote TCP server to connect to 
is stored.

\def EEPROM_TCP_REMOTE_PORT
The location in EEPROM where the port of the remote TCP server to connect to 
is stored.

\def EEPROM_DHCP_ENABLED
The location in EEPROM where the flag of whether or not to use DHCP is stored.

\def EEPROM_SYSTEM_NAME
The location in EEPROM where the user assignable name of the board is stored.
This name can only be a max of 100 characters.

@}
*/

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
#define EEPROM_OSC_UDP_PORT               EEPROM_SYSTEM_BASE + 76
#define EEPROM_TCP_AUTOCONNECT     EEPROM_SYSTEM_BASE + 80
#define EEPROM_TCP_OUT_ADDRESS     EEPROM_SYSTEM_BASE + 84
#define EEPROM_TCP_OUT_PORT        EEPROM_SYSTEM_BASE + 88
#define EEPROM_DHCP_ENABLED           EEPROM_SYSTEM_BASE + 92
#define EEPROM_SYSTEM_NAME            EEPROM_SYSTEM_BASE + 96 // this is 100 bytes long
#define EEPROM_WEBSERVER_ENABLED      EEPROM_SYSTEM_BASE + 196
#endif
