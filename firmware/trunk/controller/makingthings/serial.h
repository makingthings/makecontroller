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
	SERIAL.h
  MakingThings
*/

#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

int Serial_SetActive( int state );
int Serial_GetActive( void );

int Serial_SetBaud( int baud );
int Serial_SetBits( int bits );
int Serial_SetParity( int parity );
int Serial_SetStopBits( int stop );
int Serial_SetHardwareHandshake( int hardwareHandshake );
int Serial_SetChar( int character );

int Serial_GetBaud( void );
int Serial_GetBits( void );
int Serial_GetParity( void );
int Serial_GetStopBits( void );
int Serial_GetHardwareHandshake( void );
int Serial_GetReadable( void );
int Serial_GetChar( void );

int Serial_Write( uchar *buffer, int count, int timeout );
int Serial_Read( uchar* buffer, int count, int timeout );

/* OSC Interface */
const char* SerialOsc_GetName( void );
int SerialOsc_ReceiveMessage( int channel, char* message, int length );

#endif // SERIAL_H
