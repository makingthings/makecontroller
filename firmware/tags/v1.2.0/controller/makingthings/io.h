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
	io.h

  MakingThings
*/

#ifndef IO_H
#define IO_H

#include "types.h"

int  Io_Start( int index, bool lock );
int  Io_Stop( int index );
int  Io_StartBits( longlong bits, bool lock );
int  Io_StopBits( longlong bits );

int Io_SetActive( int index, int active );
int Io_GetActive( int index );

int Io_SetDirection( int index, int output );
int Io_GetDirection( int index );

int Io_SetPortA( int value );
int Io_GetPortA( void );
int Io_SetPortB( int value );
int Io_GetPortB( void );
int Io_SetPortAMask( int value );
int Io_GetPortAMask( void );
int Io_SetPortBMask( int value );
int Io_GetPortBMask( void );

int  Io_SetOutput( int index );
int  Io_SetInput( int index );
int  Io_SetTrue( int index );
int  Io_SetFalse( int index );
int  Io_SetValue( int index, char value );
char Io_GetValue( int index );
int  Io_SetPeripheralA( int index );
int  Io_SetPeripheralB( int index );
int  Io_SetPio( int index, int enable );
int  Io_GetPio( int index );
int  Io_PioEnable( int index );
int  Io_PioDisable( int index );
int  Io_SetPullup( int index, int enable );
int  Io_GetPullup( int index );
int  Io_PullupEnable( int index );
int  Io_PullupDisable( int index );

void Io_SetOutputBits( longlong bits );
void Io_SetInputBits( longlong bits );
void Io_SetTrueBits( longlong bits );
void Io_SetFalseBits( longlong bits );
void Io_SetValueBits( longlong bits, longlong value );
int  Io_SetPeripheralABits( longlong bits );
int  Io_SetPeripheralBBits( longlong bits );
int  Io_PioEnableBits( longlong bits );
int  Io_PioDisableBits( longlong bits );
longlong Io_GetValueBits( void );

const char* IoOsc_GetName( void );
int IoOsc_ReceiveMessage( int channel, char* message, int length );

int  Io_Test( void );

/**
\defgroup IoIndices
  Indicies (0-63) for each of the processor's IO lines.  PA0-PA31 are represented
  by indicies 0 - 31, PB0-PB31 by indicies 32 - 63.
\ingroup Io

@{
*/

#define IO_PA00  0
#define IO_PA01  1
#define IO_PA02  2
#define IO_PA03  3
#define IO_PA04  4
#define IO_PA05  5
#define IO_PA06  6
#define IO_PA07  7
#define IO_PA08  8
#define IO_PA09  9
#define IO_PA10 10
#define IO_PA11 11
#define IO_PA12 12
#define IO_PA13 13
#define IO_PA14 14
#define IO_PA15 15
#define IO_PA16 16
#define IO_PA17 17
#define IO_PA18 18
#define IO_PA19 19
#define IO_PA20 20
#define IO_PA21 21
#define IO_PA22 22
#define IO_PA23 23
#define IO_PA24 24
#define IO_PA25 25
#define IO_PA26 26
#define IO_PA27 27
#define IO_PA28 28
#define IO_PA29 29
#define IO_PA30 30
#define IO_PA31 31

#define IO_PB00 ( 32 +  0 )
#define IO_PB01 ( 32 +  1 )
#define IO_PB02 ( 32 +  2 )
#define IO_PB03 ( 32 +  3 )
#define IO_PB04 ( 32 +  4 )
#define IO_PB05 ( 32 +  5 )
#define IO_PB06 ( 32 +  6 )
#define IO_PB07 ( 32 +  7 )
#define IO_PB08 ( 32 +  8 )
#define IO_PB09 ( 32 +  9 )
#define IO_PB10 ( 32 + 10 )
#define IO_PB11 ( 32 + 11 )
#define IO_PB12 ( 32 + 12 )
#define IO_PB13 ( 32 + 13 )
#define IO_PB14 ( 32 + 14 )
#define IO_PB15 ( 32 + 15 )
#define IO_PB16 ( 32 + 16 )
#define IO_PB17 ( 32 + 17 )
#define IO_PB18 ( 32 + 18 )
#define IO_PB19 ( 32 + 19 )
#define IO_PB20 ( 32 + 20 )
#define IO_PB21 ( 32 + 21 )
#define IO_PB22 ( 32 + 22 )
#define IO_PB23 ( 32 + 23 )
#define IO_PB24 ( 32 + 24 )
#define IO_PB25 ( 32 + 25 )
#define IO_PB26 ( 32 + 26 )
#define IO_PB27 ( 32 + 27 )
#define IO_PB28 ( 32 + 28 )
#define IO_PB29 ( 32 + 29 )
#define IO_PB30 ( 32 + 30 )
#define IO_PB31 ( 32 + 31 )

/**
@}
*/

/**
\defgroup IoBits
  Bit values (1 << 0-63) for each of the processor's IO lines.  Since there are 
  so many, the data type is a long long - (a 64 bit number).
\ingroup Io
@{
*/

#define IO_PA00_BIT 1LL<<0x00
#define IO_PA01_BIT 1LL<<0x01
#define IO_PA02_BIT 1LL<<0x02
#define IO_PA03_BIT 1LL<<0x03
#define IO_PA04_BIT 1LL<<0x04
#define IO_PA05_BIT 1LL<<0x05
#define IO_PA06_BIT 1LL<<0x06
#define IO_PA07_BIT 1LL<<0x07
#define IO_PA08_BIT 1LL<<0x08
#define IO_PA09_BIT 1LL<<0x09
#define IO_PA10_BIT 1LL<<0x0A
#define IO_PA11_BIT 1LL<<0x0B
#define IO_PA12_BIT 1LL<<0x0C
#define IO_PA13_BIT 1LL<<0x0D
#define IO_PA14_BIT 1LL<<0x0E
#define IO_PA15_BIT 1LL<<0x0F
#define IO_PA16_BIT 1LL<<0x10
#define IO_PA17_BIT 1LL<<0x11
#define IO_PA18_BIT 1LL<<0x12
#define IO_PA19_BIT 1LL<<0x13
#define IO_PA20_BIT 1LL<<0x14
#define IO_PA21_BIT 1LL<<0x15
#define IO_PA22_BIT 1LL<<0x16
#define IO_PA23_BIT 1LL<<0x17
#define IO_PA24_BIT 1LL<<0x18
#define IO_PA25_BIT 1LL<<0x19
#define IO_PA26_BIT 1LL<<0x1A
#define IO_PA27_BIT 1LL<<0x1B
#define IO_PA28_BIT 1LL<<0x1C
#define IO_PA29_BIT 1LL<<0x1D
#define IO_PA30_BIT 1LL<<0x1E
#define IO_PA31_BIT 1LL<<0x1F

#define IO_PB00_BIT 1LL<<0x20
#define IO_PB01_BIT 1LL<<0x21
#define IO_PB02_BIT 1LL<<0x22
#define IO_PB03_BIT 1LL<<0x23
#define IO_PB04_BIT 1LL<<0x24
#define IO_PB05_BIT 1LL<<0x25
#define IO_PB06_BIT 1LL<<0x26
#define IO_PB07_BIT 1LL<<0x27
#define IO_PB08_BIT 1LL<<0x28
#define IO_PB09_BIT 1LL<<0x29
#define IO_PB10_BIT 1LL<<0x2A
#define IO_PB11_BIT 1LL<<0x2B
#define IO_PB12_BIT 1LL<<0x2C
#define IO_PB13_BIT 1LL<<0x2D
#define IO_PB14_BIT 1LL<<0x2E
#define IO_PB15_BIT 1LL<<0x2F
#define IO_PB16_BIT 1LL<<0x30
#define IO_PB17_BIT 1LL<<0x31
#define IO_PB18_BIT 1LL<<0x32
#define IO_PB19_BIT 1LL<<0x33
#define IO_PB20_BIT 1LL<<0x34
#define IO_PB21_BIT 1LL<<0x35
#define IO_PB22_BIT 1LL<<0x36
#define IO_PB23_BIT 1LL<<0x37
#define IO_PB24_BIT 1LL<<0x38
#define IO_PB25_BIT 1LL<<0x39
#define IO_PB26_BIT 1LL<<0x3A
#define IO_PB27_BIT 1LL<<0x3B
#define IO_PB28_BIT 1LL<<0x3C
#define IO_PB29_BIT 1LL<<0x3D
#define IO_PB30_BIT 1LL<<0x3E
#define IO_PB31_BIT 1LL<<0x3F

/**
@}
*/

#endif
