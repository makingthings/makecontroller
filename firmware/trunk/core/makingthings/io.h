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
	io.h

  MakingThings
*/

#ifndef IO_H
#define IO_H

#include "types.h"

#define IO_OUTPUT 1
#define IO_INPUT 0

int Io_Start( int index, bool lock );
int Io_Stop( int index );
bool Io_GetActive( int index );
int Io_StartBits( longlong bits, bool lock );
int Io_StopBits( longlong bits );

int Io_SetDirection( int index, bool output );
bool Io_GetDirection( int index );

int Io_SetPortA( int value );
int Io_GetPortA( void );
int Io_SetPortB( int value );
int Io_GetPortB( void );
int Io_SetPortAMask( int value );
int Io_GetPortAMask( void );
int Io_SetPortBMask( int value );
int Io_GetPortBMask( void );

int Io_SetValue( int index, bool value );
bool Io_GetValue( int index );
int Io_SetPeripheralA( int index );
int Io_SetPeripheralB( int index );
int Io_SetPio( int index, bool enable );
bool Io_GetPio( int index );
int Io_SetPullup( int index, bool enable );
bool Io_GetPullup( int index );

void Io_SetDirectionBits( longlong bits, bool output );
void Io_SetValueBits( longlong bits, longlong value );
int Io_SetPeripheralABits( longlong bits );
int Io_SetPeripheralBBits( longlong bits );
void Io_SetPioBits( longlong bits, bool enable );
void Io_SetPullupBits( longlong bits, bool enable );

longlong Io_GetValueBits( void );
longlong Io_GetPullupBits( void );
longlong Io_GetPioBits( void );
longlong Io_GetDirectionBits( void );

const char* IoOsc_GetName( void );
int IoOsc_ReceiveMessage( int channel, char* message, int length );

/**
	\defgroup IoIndices IO Indices
  Indices (0-63) for each of the processor's IO lines.  PA0-PA31 are represented
  by indices 0 - 31, PB0-PB31 by indices 32 - 63.
	\ingroup Io
	@{
*/

/** IO 0, Port A */
#define IO_PA00  0
/** IO 1, Port A */
#define IO_PA01  1
/** IO 2, Port A */
#define IO_PA02  2
/** IO 3, Port A */
#define IO_PA03  3
/** IO 4, Port A */
#define IO_PA04  4
/** IO 5, Port A */
#define IO_PA05  5
/** IO 6, Port A */
#define IO_PA06  6
/** IO 7, Port A */
#define IO_PA07  7
/** IO 8, Port A */
#define IO_PA08  8
/** IO 9, Port A */
#define IO_PA09  9
/** IO 10, Port A */
#define IO_PA10 10
/** IO 11, Port A */
#define IO_PA11 11
/** IO 12, Port A */
#define IO_PA12 12
/** IO 113, Port A */
#define IO_PA13 13
/** IO 14, Port A */
#define IO_PA14 14
/** IO 15, Port A */
#define IO_PA15 15
/** IO 16, Port A */
#define IO_PA16 16
/** IO 17, Port A */
#define IO_PA17 17
/** IO 18, Port A */
#define IO_PA18 18
/** IO 19, Port A */
#define IO_PA19 19
/** IO 20, Port A */
#define IO_PA20 20
/** IO 21, Port A */
#define IO_PA21 21
/** IO 22, Port A */
#define IO_PA22 22
/** IO 23, Port A */
#define IO_PA23 23
/** IO 24, Port A */
#define IO_PA24 24
/** IO 25, Port A */
#define IO_PA25 25
/** IO 26, Port A */
#define IO_PA26 26
/** IO 27, Port A */
#define IO_PA27 27
/** IO 28, Port A */
#define IO_PA28 28
/** IO 29, Port A */
#define IO_PA29 29
/** IO 30, Port A */
#define IO_PA30 30
/** IO 31, Port A */
#define IO_PA31 31

/** IO 0, Port B */
#define IO_PB00 ( 32 +  0 )
/** IO 1, Port B */
#define IO_PB01 ( 32 +  1 )
/** IO 2, Port B */
#define IO_PB02 ( 32 +  2 )
/** IO 3, Port B */
#define IO_PB03 ( 32 +  3 )
/** IO 4, Port B */
#define IO_PB04 ( 32 +  4 )
/** IO 5, Port B */
#define IO_PB05 ( 32 +  5 )
/** IO 6, Port B */
#define IO_PB06 ( 32 +  6 )
/** IO 7, Port B */
#define IO_PB07 ( 32 +  7 )
/** IO 8, Port B */
#define IO_PB08 ( 32 +  8 )
/** IO 9, Port B */
#define IO_PB09 ( 32 +  9 )
/** IO 10, Port B */
#define IO_PB10 ( 32 + 10 )
/** IO 11, Port B */
#define IO_PB11 ( 32 + 11 )
/** IO 12, Port B */
#define IO_PB12 ( 32 + 12 )
/** IO 13, Port B */
#define IO_PB13 ( 32 + 13 )
/** IO 14, Port B */
#define IO_PB14 ( 32 + 14 )
/** IO 15, Port B */
#define IO_PB15 ( 32 + 15 )
/** IO 16, Port B */
#define IO_PB16 ( 32 + 16 )
/** IO 17, Port B */
#define IO_PB17 ( 32 + 17 )
/** IO 18, Port B */
#define IO_PB18 ( 32 + 18 )
/** IO 19, Port B */
#define IO_PB19 ( 32 + 19 )
/** IO 20, Port B */
#define IO_PB20 ( 32 + 20 )
/** IO 21, Port B */
#define IO_PB21 ( 32 + 21 )
/** IO 22, Port B */
#define IO_PB22 ( 32 + 22 )
/** IO 23, Port B */
#define IO_PB23 ( 32 + 23 )
/** IO 24, Port B */
#define IO_PB24 ( 32 + 24 )
/** IO 25, Port B */
#define IO_PB25 ( 32 + 25 )
/** IO 26, Port B */
#define IO_PB26 ( 32 + 26 )
/** IO 27, Port B */
#define IO_PB27 ( 32 + 27 )
/** IO 28, Port B */
#define IO_PB28 ( 32 + 28 )
/** IO 29, Port B */
#define IO_PB29 ( 32 + 29 )
/** IO 30, Port B */
#define IO_PB30 ( 32 + 30 )
/** IO 31, Port B */
#define IO_PB31 ( 32 + 31 )

/* @} */

/**
\defgroup IoBits IO Bits
  The values to use to create a mask to pass into any of the \b Bits style functions.  
  Your mask values need to be of type longlong since it needs to represent 64 bits, 
  for the 64 IO lines.

  \b Example

  \code
  longlong mymask = 0;
  mymask |= (IO_PA00_BIT | IO_PA03_BIT | IO_PA11_BIT);
  // now disable the pullups for lines PA00, PA03, and PA11
  Io_PullupDisableBits( mymask );
  \endcode
\ingroup Io
@{
*/

/** IO 0, Port A */
#define IO_PA00_BIT 1LL<<0x00
/** IO 1, Port A */
#define IO_PA01_BIT 1LL<<0x01
/** IO 2, Port A */
#define IO_PA02_BIT 1LL<<0x02
/** IO 3, Port A */
#define IO_PA03_BIT 1LL<<0x03
/** IO 4, Port A */
#define IO_PA04_BIT 1LL<<0x04
/** IO 5, Port A */
#define IO_PA05_BIT 1LL<<0x05
/** IO 6, Port A */
#define IO_PA06_BIT 1LL<<0x06
/** IO 7, Port A */
#define IO_PA07_BIT 1LL<<0x07
/** IO 8, Port A */
#define IO_PA08_BIT 1LL<<0x08
/** IO 9, Port A */
#define IO_PA09_BIT 1LL<<0x09
/** IO 10, Port A */
#define IO_PA10_BIT 1LL<<0x0A
/** IO 11, Port A */
#define IO_PA11_BIT 1LL<<0x0B
/** IO 12, Port A */
#define IO_PA12_BIT 1LL<<0x0C
/** IO 13, Port A */
#define IO_PA13_BIT 1LL<<0x0D
/** IO 14, Port A */
#define IO_PA14_BIT 1LL<<0x0E
/** IO 15, Port A */
#define IO_PA15_BIT 1LL<<0x0F
/** IO 16, Port A */
#define IO_PA16_BIT 1LL<<0x10
/** IO 17, Port A */
#define IO_PA17_BIT 1LL<<0x11
/** IO 18, Port A */
#define IO_PA18_BIT 1LL<<0x12
/** IO 19, Port A */
#define IO_PA19_BIT 1LL<<0x13
/** IO 20, Port A */
#define IO_PA20_BIT 1LL<<0x14
/** IO 21, Port A */
#define IO_PA21_BIT 1LL<<0x15
/** IO 22, Port A */
#define IO_PA22_BIT 1LL<<0x16
/** IO 23, Port A */
#define IO_PA23_BIT 1LL<<0x17
/** IO 24, Port A */
#define IO_PA24_BIT 1LL<<0x18
/** IO 25, Port A */
#define IO_PA25_BIT 1LL<<0x19
/** IO 26, Port A */
#define IO_PA26_BIT 1LL<<0x1A
/** IO 27, Port A */
#define IO_PA27_BIT 1LL<<0x1B
/** IO 28, Port A */
#define IO_PA28_BIT 1LL<<0x1C
/** IO 29, Port A */
#define IO_PA29_BIT 1LL<<0x1D
/** IO 30, Port A */
#define IO_PA30_BIT 1LL<<0x1E
/** IO 31, Port A */
#define IO_PA31_BIT 1LL<<0x1F

/** IO 0, Port B */
#define IO_PB00_BIT 1LL<<0x20
/** IO 1, Port B */
#define IO_PB01_BIT 1LL<<0x21
/** IO 2, Port B */
#define IO_PB02_BIT 1LL<<0x22
/** IO 3, Port B */
#define IO_PB03_BIT 1LL<<0x23
/** IO 4, Port B */
#define IO_PB04_BIT 1LL<<0x24
/** IO 5, Port B */
#define IO_PB05_BIT 1LL<<0x25
/** IO 6, Port B */
#define IO_PB06_BIT 1LL<<0x26
/** IO 7, Port B */
#define IO_PB07_BIT 1LL<<0x27
/** IO 8, Port B */
#define IO_PB08_BIT 1LL<<0x28
/** IO 9, Port B */
#define IO_PB09_BIT 1LL<<0x29
/** IO 10, Port B */
#define IO_PB10_BIT 1LL<<0x2A
/** IO 11, Port B */
#define IO_PB11_BIT 1LL<<0x2B
/** IO 12, Port B */
#define IO_PB12_BIT 1LL<<0x2C
/** IO 13, Port B */
#define IO_PB13_BIT 1LL<<0x2D
/** IO 14, Port B */
#define IO_PB14_BIT 1LL<<0x2E
/** IO 15, Port B */
#define IO_PB15_BIT 1LL<<0x2F
/** IO 16, Port B */
#define IO_PB16_BIT 1LL<<0x30
/** IO 17, Port B */
#define IO_PB17_BIT 1LL<<0x31
/** IO 18, Port B */
#define IO_PB18_BIT 1LL<<0x32
/** IO 19, Port B */
#define IO_PB19_BIT 1LL<<0x33
/** IO 20, Port B */
#define IO_PB20_BIT 1LL<<0x34
/** IO 21, Port B */
#define IO_PB21_BIT 1LL<<0x35
/** IO 22, Port B */
#define IO_PB22_BIT 1LL<<0x36
/** IO 23, Port B */
#define IO_PB23_BIT 1LL<<0x37
/** IO 24, Port B */
#define IO_PB24_BIT 1LL<<0x38
/** IO 25, Port B */
#define IO_PB25_BIT 1LL<<0x39
/** IO 26, Port B */
#define IO_PB26_BIT 1LL<<0x3A
/** IO 27, Port B */
#define IO_PB27_BIT 1LL<<0x3B
/** IO 28, Port B */
#define IO_PB28_BIT 1LL<<0x3C
/** IO 29, Port B */
#define IO_PB29_BIT 1LL<<0x3D
/** IO 30, Port B */
#define IO_PB30_BIT 1LL<<0x3E
/** IO 31, Port B */
#define IO_PB31_BIT 1LL<<0x3F


/**
@}
*/

#endif
