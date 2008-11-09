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

/** \file appled.c	
	MAKE Application Board LED control.
	Library of functions for the Make Application Board's LED subsystem.
*/

#include "io_cpp.h"
#include "appled_cpp.h"

extern "C" {
  #include "config.h"
}

#include "AT91SAM7X256.h"

#define APPLED_COUNT 4 

#if ( APPBOARD_VERSION == 50 )
  #define APPLED_0_IO IO_PB19
  #define APPLED_1_IO IO_PB20
  #define APPLED_2_IO IO_PB21
  #define APPLED_3_IO IO_PB22
#endif
#if ( APPBOARD_VERSION == 90 )
  #define APPLED_0_IO IO_PA10
  #define APPLED_1_IO IO_PA11
  #define APPLED_2_IO IO_PA13
  #define APPLED_3_IO IO_PA15
#endif
#if ( APPBOARD_VERSION == 95 || APPBOARD_VERSION == 100 )
  #define APPLED_0_IO IO_PA15
  #define APPLED_1_IO IO_PA13
  #define APPLED_2_IO IO_PA28
  #define APPLED_3_IO IO_PA27
#endif

/** \defgroup AppLed Application Board LEDs
* The Application Board LED subsystem controls the 4 LEDs on the Application Board, for status and program feedback.
* \ingroup Libraries
* @{
*/

AppLed::AppLed( int index )
{
  if( index < 0 || index >= APPLED_COUNT )
    return;
  
  switch( index )
  {
    case 0:
      appledIo.setPin( APPLED_0_IO );
      break;
    case 1:
      appledIo.setPin( APPLED_1_IO );
      break;
    case 2:
      appledIo.setPin( APPLED_2_IO );
      break;
    case 3:
      appledIo.setPin( APPLED_3_IO );
      break;
  }
  appledIo.setDirection( IO_OUTPUT );
  appledIo.setPeripheral( GPIO );
}

void AppLed::setState( bool state )
{
  appledIo.setValue( state );
}

bool AppLed::getState( )
{
  return appledIo.getValue( );
}

#ifdef OSC

/** \defgroup AppLEDOSC App LED - OSC
  Control the Application Board's Status LEDs via OSC.
  \ingroup OSC
	
	\section devices Devices
	There are 4 LEDs on the Make Application Board, numbered 0 - 3.
	
	\section properties Properties
	The LEDs have two properties:
  - state
  - active

	\par State
	The \b state property corresponds to the on/off state of a given LED.
	For example, to turn on the first LED, send a message like
	\verbatim /appled/0/state 1\endverbatim
	To turn it off, send the message \verbatim /appled/0/state 0\endverbatim
	
	\par Active
	The \b active property corresponds to the active state of an LED.
	If an LED is set to be active, no other tasks will be able to
	write to it.  If you're not seeing appropriate
	responses to your messages to the LED, check the whether it's 
	locked by sending a message like
	\verbatim /appled/0/active \endverbatim
	\par
	You can set the active flag by sending
	\verbatim /appled/0/active 1 \endverbatim
*/

#include "osc.h"
#include "string.h"
#include "stdio.h"

/*
  We expect to get a message like /appled/index/property (data)
*/
void AppLedOSC::onNewMsg( OscMessage* msg, OscTransport t, int src_addr, int src_port )
{
  (void)t;
  (void)src_addr;
  (void)src_port;
  bool ok;
  int index = msg->addressElementAsInt( 1, &ok );
  if( !ok )
    return;
  char* property = msg->addressElementAsString( 2 );
  if( !property )
    return;
  AppLed led(index);
  int value;  
  if( msg->data_count )
    value = msg->dataItemAsInt(0);
  
  // now go through our possible properties
  if(strcmp(property, "state") == 0)
  {
    if( msg->data_count )
      led.setState( value );
    // else
    //   
  }
}

// Need a list of property names
// MUST end in zero
// static char* AppLedOsc_Name = "appled";
// static char* AppLedOsc_PropertyNames[] = { "active", "state", 0 }; // must have a trailing 0
// 
// int AppLedOsc_PropertySet( int index, int property, int value );
// int AppLedOsc_PropertyGet( int index, int property );
// 
// // Returns the name of the subsystem
// const char* AppLedOsc_GetName( )
// {
//   return AppLedOsc_Name;
// }
// 
// // Now getting a message.  This is actually a part message, with the first
// // part (the subsystem) already parsed off.
// int AppLedOsc_ReceiveMessage( int channel, char* message, int length )
// {
//   int status = Osc_IndexIntReceiverHelper( channel, message, length, 
//                                            APPLED_COUNT, AppLedOsc_Name,
//                                            AppLedOsc_PropertySet, AppLedOsc_PropertyGet, 
//                                            AppLedOsc_PropertyNames );
// 
//   if ( status != CONTROLLER_OK )
//     return Osc_SendError( channel, AppLedOsc_Name, status );
//   return CONTROLLER_OK;
// }
// 
// // Set the index LED, property with the value
// int AppLedOsc_PropertySet( int index, int property, int value )
// {
//   switch ( property )
//   {
//     case 0: 
//       AppLed_SetActive( index, value );
//       break;      
//     case 1:
//       AppLed_SetState( index, value );
//       break;
//   }
//   return CONTROLLER_OK;
// }
// 
// // Get the index LED, property
// int AppLedOsc_PropertyGet( int index, int property )
// {
//   int value = 0;
//   switch ( property )
//   {
//     case 0:
//       value = AppLed_GetActive( index );
//       break;
//     case 1:
//       value = AppLed_GetState( index );
//       break;
//   }
//   
//   return value;
// }

#endif // OSC
