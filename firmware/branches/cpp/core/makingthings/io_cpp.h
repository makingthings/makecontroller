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

#ifndef IO_CPP_H
#define IO_CPP_H

extern "C" {
  #include "io.h"
}
// #define IO_OUTPUT true
// #define IO_INPUT false
#define INVALID_PIN 1024 // a value too large for any pin

enum IoPeripheral { IO_A, IO_B, GPIO };

class Io
{
public:
  Io( int pin = INVALID_PIN, IoPeripheral = GPIO, bool direction = IO_OUTPUT );
  ~Io( ) { releasePeripherals( ); }
  bool valid( ) { return io_pin != INVALID_PIN; }
  
  bool setPin( int pin );
  int getPin( );
  
  bool getValue( );
  bool setValue( bool onoff );
  bool on();
  bool off();
  
  bool getDirection( );
  bool setDirection( bool output );
  
  bool getPullup( );
  bool setPullup( bool enabled );
  
  bool getFilter( );
  bool setFilter( bool enabled );
  
  int getPeripheral( );
  bool setPeripheral( IoPeripheral periph, bool disableGpio = true );
  bool releasePeripherals( );
  
private:
  unsigned int io_pin;
};

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

#endif
