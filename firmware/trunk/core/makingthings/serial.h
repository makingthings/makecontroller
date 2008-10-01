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

/*
	serial.h
  MakingThings
*/

#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"
/** Index to be used to specify serial port 0 \ingroup serial */
#define SERIAL_0 0
/** Index to be used to specify serial port 1 \ingroup serial */
#define SERIAL_1 1


int Serial_SetActive( int index, int state );
bool Serial_GetActive( int index );

int Serial_SetBaud( int index, int baud );
int Serial_SetBits( int index, int bits );
int Serial_SetParity( int index, int parity );
int Serial_SetStopBits( int index, int stop );
int Serial_SetHardwareHandshake( int index, int hardwareHandshake );
int Serial_SetChar( int index, int character );

int Serial_GetBaud( int index );
int Serial_GetBits( int index );
int Serial_GetParity( int index );
int Serial_GetStopBits( int index );
int Serial_GetHardwareHandshake( int index );
int Serial_GetReadable( int index );
int Serial_GetChar( int index );

int Serial_Write( int index, uchar *buffer, int count, int timeout );
int Serial_Read( int index, uchar* buffer, int count, int timeout );

void Serial_Flush( int index );
void Serial_ClearErrors( int index );
bool Serial_GetErrors(int index,  bool* overrun, bool* frame, bool* parity );
void Serial_StartBreak( int index );
void Serial_StopBreak( int index );

/* OSC Interface */
const char* SerialOsc_GetName( void );
int SerialOsc_ReceiveMessage( int channel, char* message, int length );

#endif // SERIAL_H
