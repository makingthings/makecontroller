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
	SERIAL2.h
  MakingThings
*/

#ifndef SERIAL2_H
#define SERIAL2_H

#include "types.h"

int Serial2_SetActive( int index, int state );
int Serial2_GetActive( int index );

int Serial2_SetBaud( int index, int baud );
int Serial2_SetBits( int index, int bits );
int Serial2_SetParity( int index, int parity );
int Serial2_SetStopBits( int index, int stop );
int Serial2_SetHardwareHandshake( int index, int hardwareHandshake );
int Serial2_SetChar( int index, int character );

int Serial2_GetBaud( int index );
int Serial2_GetBits( int index );
int Serial2_GetParity( int index );
int Serial2_GetStopBits( int index );
int Serial2_GetHardwareHandshake( int index );
int Serial2_GetReadable( int index );
int Serial2_GetChar( int index );

int Serial2_Write( int index, uchar *buffer, int count, int timeout );
int Serial2_Read( int index, uchar* buffer, int count, int timeout );

/* OSC Interface */
const char* Serial2Osc_GetName( void );
int Serial2Osc_ReceiveMessage( int channel, char* message, int length );

#endif // SERIAL2_H
