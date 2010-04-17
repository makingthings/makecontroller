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

#include "appled.h"
#include "pin.h"

#define APPLED_COUNT 4

#if ( APPBOARD_VERSION == 50 )
  #define APPLED_0 PIN_PB19
  #define APPLED_1 PIN_PB20
  #define APPLED_2 PIN_PB21
  #define APPLED_3 PIN_PB22
#elif ( APPBOARD_VERSION == 90 )
  #define APPLED_0 PIN_PA10
  #define APPLED_1 PIN_PA11
  #define APPLED_2 PIN_PA13
  #define APPLED_3 PIN_PA15
#elif ( APPBOARD_VERSION == 95 || APPBOARD_VERSION == 100 )
  #define APPLED_0 PIN_PA15
  #define APPLED_1 PIN_PA13
  #define APPLED_2 PIN_PA28
  #define APPLED_3 PIN_PA27
#elif ( APPBOARD_VERSION == 200 )
  #define APPLED_0 PIN_PA15
  #define APPLED_1 PIN_PA13
  #define APPLED_2 PIN_PA27
  #define APPLED_3 PIN_PA28
#endif

static int appledGetIo(int index)
{
  switch (index) {
    case 0: return APPLED_0;
    case 1: return APPLED_1;
    case 2: return APPLED_2;
    case 3: return APPLED_3;
    default: return -1;
  }
}

/**
  Configure an app LED.
  @param index Which app LED - valid options are 0-3

  \b Example
  \code
  appledEnable(1); // set it up
  \endcode
*/
void appledEnable(int channel)
{
  short io = appledGetIo(channel);
  pinSetMode(io, OUTPUT);
  pinOn(io); // inverted
}

/**
  Turn an AppLed on or off.
  @param channel Which app LED - valid options are 0-3.
  @param state True to turn it on, false to turn it off.

  \b Example
  \code
  appledSetValue(0, ON); // turn LED 0 on
  appledSetValue(1, OFF); // turn LED 1 off
  \endcode
*/
void appledSetValue(int channel, bool on)
{
  pinSetValue(appledGetIo(channel), !on); // inverted since it's tied to 3.3V
}

/**
  Read whether an AppLed is currently on or not.
  @return true if it's on, false if it's not.

  \b Example
  \code
  if (appledValue() == ON) {
    // then do this
  }
  else {
    // then do that
  }
  \endcode
*/
bool appledValue(int channel)
{
  return !pinValue(appledGetIo(channel));
}

#ifdef OSC

/**
  \defgroup AppLEDOSC App LED - OSC
  Control the Application Board's Status LEDs via OSC.
	
	\section devices Devices
	There are 4 LEDs on the Make Application Board, numbered 0 - 3.
	
	\section properties Properties
	The LEDs have the following properties:
  - state
  - active

	\par State
	The \b state property corresponds to the on/off state of a given LED.
	For example, to turn on the first LED, send a message like
	\code /appled/0/state 1 \endcode
	To turn it off, send the message \code /appled/0/state 0 \endcode
	
	\par Active
	The \b active property corresponds to the active state of an LED.
	If an LED is set to be active, no other tasks will be able to
	write to it.  If you're not seeing appropriate
	responses to your messages to the LED, check the whether it's 
	locked by sending a message like
	\code /appled/0/active \endcode
	\par
	You can set the active flag by sending
	\code /appled/0/active 1 \endcode
*/

#include "osc.h"
#include "string.h"
#include "stdio.h"

const char* AppLedOSC::propertyList[] = {"state", 0};

/*
  We expect to get a message like /appled/index/property (data)
*/
int AppLedOSC::onNewMsg( OscTransport t, OscMessage* msg, int src_addr, int src_port )
{
  (void)src_addr;
  (void)src_port;
  int replies = 0;
  char* property = msg->addressElementAsString( 2 ); // address element 2 should be the property
  if( !property )
    return replies;
  
  int prop_index = propertyLookup( propertyList, property );
  if( prop_index < 0 ) // not one of our properties
    return replies;
  
  int index;
  OscRangeHelper rangeHelper(msg, 1, APPLED_COUNT); // element 1 should be our index
  while( rangeHelper.hasNext() )
  {
    index = rangeHelper.next();
    if( index < 0 || index >= APPLED_COUNT )
      continue;
    AppLed led(index);
    if( msg->data_count ) // setter...write the value to the led
    {
      switch( prop_index )
      {
        case 0: // state
          led.setState(msg->dataItemAsInt(0));
          break;
      }
    }
    else // getter...return a message
    {
      switch( prop_index )
      {
        case 0: // state
          Osc::get()->createMessage(t, msg->address, ",i", led.getState());
          replies++;
          break;
      }
    }
  }
  return replies;
}

int AppLedOSC::onQuery( OscTransport t, char* address, int element )
{
  int replies = 0;
  switch( element )
  {
    case 1: // index
    {
      int i;
      for(i = 0; i < APPLED_COUNT; i++)
      {
        Osc::get()->createMessage(t, address, ",i", i);
        replies++;
      }
      break;
    }
    case 2: // property
    {
      const char** prop = propertyList; // create message for each item in our propertyList
      while(prop)
      {
        Osc::get()->createMessage(t, address, ",s", *prop);
        replies++;
        prop++;
      }
      break;
    }
  }
  return replies;
}

#endif // OSC
