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

#ifndef LED_CPP_H
#define LED_CPP_H

#include "io.h"

/**
  Controls the single green LED on the MAKE Controller Board.
  There are two LEDs on the MAKE Controller Board - one green and one red.  The red LED is simply
  a power indicator and cannot be controlled by the Controller.  The green LED can be used for
  program feedback.  In many MakingThings applications, for example, it is set to blink once a
  second, showing the board's "heartbeat" and letting the user know that the board is running.
  
  If you're looking to control the LEDs on the Application Board, check \ref AppLed.
*/
class Led
{
public:
  Led( );
  ~Led( ) { }
  void setState( bool value );
  bool state( );
  
private:
  Io ledIo;
};

// /* OSC Interface */
// const char* LedOsc_GetName( void );
// int LedOsc_ReceiveMessage( int channel, char* message, int length );

#endif
