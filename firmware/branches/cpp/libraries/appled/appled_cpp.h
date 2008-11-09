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
	appled.h

*/

#ifndef APPLED_CPP_H
#define APPLED_CPP_H

extern "C" {
  #include "config.h"
}

/* AppLed Functions */

class AppLed
{
public:
  AppLed( int index );
  ~AppLed( ) { }
  bool valid( ) { return appledIo.valid(); }
  void setState( bool state );
  bool getState( );
  
private:
  Io appledIo;
};

#ifdef OSC
#include "osc_cpp.h"

class AppLedOSC : public OscHandler
{
public:
  AppLedOSC( ) { }
  void onNewMsg( OscMessage* msg, OscTransport t, int src_addr, int src_port );
  void onQuery( char* address );
  const char* name( ) { return "appled"; }
  // const char* properties( ) { return propertyList; }
  // static const char* propertyList[];
};

#endif // OSC

// int AppLed_SetActive( int index, int state );
// int AppLed_GetActive( int index );
// 
// int AppLed_SetState( int index, int state );
// int AppLed_GetState( int index );

/* OSC Interface */
// const char* AppLedOsc_GetName( void );
// int AppLedOsc_ReceiveMessage( int channel, char* message, int length );

#endif // APPLED_CPP_H
