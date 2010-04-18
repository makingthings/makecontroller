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

#include "digitalin.h"
#include "analogin.h"
#include "config.h"
#include "ch.h"
#include "hal.h"

// This number is 4 because the a/d converter is sitting on the IO's 4 - 7
// this means that we'll need to use the A/d converter to get the digital value.
// Crazy, eh?  And slow.  Whew.
#define DIGITALIN_COUNT 8

#ifndef DIGITALIN_THRESHOLD
#define DIGITALIN_THRESHOLD 200
#endif

// only need symbols for the first 4 since the others are ains
#define DIGITALIN_0 AT91C_PIO_PB27
#define DIGITALIN_1 AT91C_PIO_PB28
#define DIGITALIN_2 AT91C_PIO_PB29
#define DIGITALIN_3 AT91C_PIO_PB30

/**
  \defgroup digitalin Digital Input
  Read the 8 inputs on the Application Board as digital values - on or off.
  
  \section Usage
  Use digitalinValue() to read digital in values.
  
  \section Notes
  Internally, the 8 inputs on the Application Board consist of 4 dedicated analog inputs, 
  and 4 lines which can be configured either as digitial ins or outs. Because digital 
  ins 4-7 are always \ref analogin lines, there's no performance gain to reading those as DigitalIns 
  as opposed to AnalogIns.
  
  \ingroup io
  @{
*/

static int digitalinGetIo(int index)
{
  switch (index) {
    case 0: return DIGITALIN_0;
    case 1: return DIGITALIN_1;
    case 2: return DIGITALIN_2;
    case 3: return DIGITALIN_3;
    default: return 0;
  }
}

/** 
  Read the value of a Digital Input on the MAKE Application Board.
  If the voltage on the input is greater than ~0.6V, the Digital In will read high.
  @param channel The digital in channel to read - valid options are 0-7.
  @return True when high, false when low.
  
  \b Example
  \code
  if (digitalinValue(5) == ON) {
    // DigitalIn 5 is high
  }
  else {
    // DigitalIn 5 is low
  }
  \endcode
*/
bool digitalinValue(int channel)
{
  if (channel > 3)
    return ainValue(channel) > DIGITALIN_THRESHOLD;
  else 
    return palReadPad(IOPORT2, digitalinGetIo(channel));
}

/** @}
*/

#ifdef OSC

/** \defgroup DigitalInOSC Digital In - OSC
  Read the Application Board's Digital Inputs via OSC.
  \ingroup OSC
  
  \section devices Devices
  There are 8 Digital Inputs on the Make Application Board, numbered <b>0 - 7</b>.

  The Digital Ins are physically the same as the Analog Ins - they're on the same 
  signal lines and the same connectors - but reading them as Digital Ins is 
  slightly more efficient/quicker.

  If the voltage on the input is greater than <b>~0.6V</b>, the Digital In will read high.
  
  \section properties Properties
  The Digital Ins have two properties
  - value
  - active

  \par Value
  The \b value property corresponds to the on/off value of a Digital In.
  Because you can only ever \b read the value of an input, you'll never
  want to include an argument at the end of your OSC message to read the value.
  To read the third Digital In, send the message
  \verbatim /digitalin/2/value \endverbatim
  
  \par Active
  The \b active property corresponds to the active state of a Digital In.
  If a Digital In is set to be active, no other tasks will be able to
  read from it as an Analog In.  If you're not seeing appropriate
  responses to your messages to the Digital In, check the whether it's 
  locked by sending a message like
  \verbatim /digitalin/0/active \endverbatim
  \par
  You can set the active flag by sending
  \verbatim /digitalin/0/active 0 \endverbatim
*/

//#include "osc.h"
//#include "string.h"
//#include "stdio.h"
//
//// Need a list of property names
//// MUST end in zero
//static char* DigitalInOsc_Name = "digitalin";
//static char* DigitalInOsc_PropertyNames[] = { "active", "value", 0 }; // must have a trailing 0
//
//int DigitalInOsc_PropertySet( int index, int property, int value );
//int DigitalInOsc_PropertyGet( int index, int property );
//
//// Returns the name of the subsystem
//const char* DigitalInOsc_GetName( )
//{
//  return DigitalInOsc_Name;
//}
//
//// Now getting a message.  This is actually a part message, with the first
//// part (the subsystem) already parsed off.
//int DigitalInOsc_ReceiveMessage( int channel, char* message, int length )
//{
//  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
//                                     DIGITALIN_COUNT, DigitalInOsc_Name,
//                                     DigitalInOsc_PropertySet, DigitalInOsc_PropertyGet, 
//                                     DigitalInOsc_PropertyNames );
//
//                                     
//  if ( status != CONTROLLER_OK )
//    return Osc_SendError( channel, DigitalInOsc_Name, status );
//  return CONTROLLER_OK;
//}
//
//// Set the index LED, property with the value
//int DigitalInOsc_PropertySet( int index, int property, int value )
//{
//  switch ( property )
//  {
//    case 0: 
//      DigitalIn_SetActive( index, value );
//      break;      
//  }
//  return CONTROLLER_OK;
//}
//
//// Get the index LED, property
//int DigitalInOsc_PropertyGet( int index, int property )
//{
//  int value = 0;
//  switch ( property )
//  {
//    case 0:
//      value = DigitalIn_GetActive( index );
//      break;
//    case 1:
//      value = DigitalIn_GetValue( index );
//      break;
//  }
//  
//  return value;
//}

#endif
