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

#include "digitalout.h"
#include "config.h"
#include "error.h"
#include "pin.h"

#define DIGITALOUT_COUNT 8 

#if ( APPBOARD_VERSION == 50 )
  #define DIGITALOUT_0_IO IO_PA02
  #define DIGITALOUT_1_IO IO_PA02
  #define DIGITALOUT_2_IO IO_PA02
  #define DIGITALOUT_3_IO IO_PA02
  #define DIGITALOUT_4_IO IO_PA02
  #define DIGITALOUT_5_IO IO_PA02
  #define DIGITALOUT_6_IO IO_PA02
  #define DIGITALOUT_7_IO IO_PA02
  #define DIGITALOUT_01_ENABLE IO_PA02
  #define DIGITALOUT_23_ENABLE IO_PA02
  #define DIGITALOUT_45_ENABLE IO_PA02
  #define DIGITALOUT_67_ENABLE IO_PA02
#elif ( APPBOARD_VERSION == 90 )
  #define DIGITALOUT_0_IO IO_PB23
  #define DIGITALOUT_1_IO IO_PA26
  #define DIGITALOUT_2_IO IO_PA25
  #define DIGITALOUT_3_IO IO_PB25
  #define DIGITALOUT_4_IO IO_PA02
  #define DIGITALOUT_5_IO IO_PA06
  #define DIGITALOUT_6_IO IO_PA05
  #define DIGITALOUT_7_IO IO_PA24
  #define DIGITALOUT_01_ENABLE IO_PB19
  #define DIGITALOUT_23_ENABLE IO_PB20
  #define DIGITALOUT_45_ENABLE IO_PB21
  #define DIGITALOUT_67_ENABLE IO_PB22
#elif ( APPBOARD_VERSION >= 95 )
  #define DIGITALOUT_0 PIN_PA24
  #define DIGITALOUT_1 PIN_PA5
  #define DIGITALOUT_2 PIN_PA6
  #define DIGITALOUT_3 PIN_PA2
  #define DIGITALOUT_4 PIN_PB25
  #define DIGITALOUT_5 PIN_PA25
  #define DIGITALOUT_6 PIN_PA26
  #define DIGITALOUT_7 PIN_PB23
  #define DOUT_PIOA (PIN_PA24 | PIN_PA5 | PIN_PA6 | PIN_PA2 | PIN_PA25 | PIN_PA26)
  #define DOUT_PIOB (PIN_PB25 | PIN_PB23)
  #define DIGITALOUT_ENABLE_MASK (PIN_PB19 | PIN_PB20 | PIN_PB21 | PIN_PB22)
#endif

static int digitaloutGetIo( int index );

/**
  Create a new DigitalOut object.
  This is automatically called, setting the channel active, the first time DigitalOut_SetValue() is called.
  However, the channel must be explicitly set to inactive in order for any other devices to access the I/O lines. 
  @param index Which DigitalOut to control (0-7)
  
  \b Example
  \code
  DigitalOut dout(6);
  // or allocate one...
  DigitalOut* dout = new DigitalOut(6);
  \endcode
*/
void digitaloutInit( )
{ 
  // configure enable lines as outputs and turn them all on
  pinGroupSetMode(PORT_B, DIGITALOUT_ENABLE_MASK, OUTPUT);
  pinGroupOn(PORT_B, DIGITALOUT_ENABLE_MASK);
  
  // configure douts as outputs & turn off
  pinGroupSetMode(PORT_A, DOUT_PIOA, OUTPUT);
  pinGroupSetMode(PORT_B, DOUT_PIOB, OUTPUT);
  pinGroupOff(PORT_A, DOUT_PIOA);
  pinGroupOff(PORT_B, DOUT_PIOB);
}

/** 
  Turn a DigitalOut on or off.
  @param on True to turn it on, false to turn it off.
  @return True on success, false on failure.
  
  \b Example
  \code
  // Turn digital out 2 on
  DigitalOut dout(2);
  dout.setValue( true );
  \endcode
*/
void digitaloutSetValue( int channel, bool on )
{
  pinSetValue(digitaloutGetIo(channel), on);
}

/** 
  Read whether a DigitalOut is on or off.
  @return True if it's on, false if it's off.
  
  \b Example
  \code
  DigitalOut dout(2);
  if( dout.value() )
  {
    // DigitalOut 2 is high
  }
  else
  {
    // DigitalOut 2 is low
  }
  \endcode
*/
bool digitaloutValue(int channel)
{
  return pinValue(digitaloutGetIo(channel));
}

int digitaloutGetIo( int index )
{
  switch ( index )
  {
    case 0: return DIGITALOUT_0;
    case 1: return DIGITALOUT_1;
    case 2: return DIGITALOUT_2;
    case 3: return DIGITALOUT_3;
    case 4: return DIGITALOUT_4;
    case 5: return DIGITALOUT_5;
    case 6: return DIGITALOUT_6;
    case 7: return DIGITALOUT_7;
    default: return 0;
  }
}

#ifdef OSC

/** \defgroup DigitalOutOSC Digital Out - OSC
  Control the Application Board's Digital Outs via OSC.
  \ingroup OSC
  
  \section devices Devices
  There are 8 Digital Outs on the Make Application Board, numbered <b>0 - 7</b>.
  
  \section properties Properties
  The Digital Outs have two properties:
  - value
  - active

  \par Value
  The \b value property corresponds to the on/off value of a given Digital Out.
  For example, to turn on the fifth Digital Out, send a message like
  \verbatim /digitalout/6/value 1\endverbatim
  Turn it off by sending the message \verbatim /digitalout/6/value 0\endverbatim
  
  \par Active
  The \b active property corresponds to the active state of a Digital Out.
  If a Digital Out is set to be active, no other tasks will be able to
  write to that Digital Out.  If you're not seeing appropriate
  responses to your messages to the Digital Out, check the whether a Digital Out is 
  locked by sending a message like
  \verbatim /digitalout/0/active \endverbatim
  \par
  You can set the active flag by sending
  \verbatim /digitalout/0/active 1 \endverbatim
*/

//#include "osc.h"
//#include "string.h"
//#include "stdio.h"
//
//// Need a list of property names
//// MUST end in zero
//static char* DigitalOutOsc_Name = "digitalout";
//static char* DigitalOutOsc_PropertyNames[] = { "active", "value", 0 }; // must have a trailing 0
//
//int DigitalOutOsc_PropertySet( int index, int property, int value );
//int DigitalOutOsc_PropertyGet( int index, int property );
//
//// Returns the name of the subsystem
//const char* DigitalOutOsc_GetName( )
//{
//  return DigitalOutOsc_Name;
//}
//
//// Now getting a message.  This is actually a part message, with the first
//// part (the subsystem) already parsed off.
//int DigitalOutOsc_ReceiveMessage( int channel, char* message, int length )
//{
//  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
//                                           DIGITALOUT_COUNT, DigitalOutOsc_Name,
//                                           DigitalOutOsc_PropertySet, DigitalOutOsc_PropertyGet, 
//                                           DigitalOutOsc_PropertyNames );
//                                     
//  if ( status != CONTROLLER_OK )
//    return Osc_SendError( channel, DigitalOutOsc_Name, status );
//  return CONTROLLER_OK;
//}
//
//// Set the index LED, property with the value
//int DigitalOutOsc_PropertySet( int index, int property, int value )
//{
//  switch ( property )
//  {
//    case 0: 
//      DigitalOut_SetActive( index, value );
//      break;      
//    case 1: 
//      DigitalOut_SetValue( index, value );
//      break;
//  }
//  return CONTROLLER_OK;
//}
//
//// Get the index LED, property
//int DigitalOutOsc_PropertyGet( int index, int property )
//{
//  int value = 0;
//  switch ( property )
//  {
//    case 0:
//      value = DigitalOut_GetActive( index );
//      break;
//    case 1:
//      value = DigitalOut_GetValue( index );
//      break;
//  }
//  
//  return value;
//}

#endif
