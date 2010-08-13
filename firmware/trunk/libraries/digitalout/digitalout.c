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
#include "core.h"

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

/**
  \defgroup digitalout Digital Output
  Control the 8 high current outputs on the Application Board.
  
  \section Usage
  To turn a digital out on or off, call digitaloutSetValue().  If you need to check
  whether a digital out is on or off, use digitaloutValue().
  
  \section Notes
  If you're simultaneously using any of the other systems on the outputs (Stepper, Motor, etc.), results will
  be unpredictable since they're using the same signals.  Make sure to only use one system at a time on a given
  output signal.
  
  See the <a href="http://www.makingthings.com/documentation/tutorial/application-board-overview/digital-outputs">
  Digital Out section</a> of the Application Board overview for more details.
  
  \ingroup io
  @{
*/

static int digitaloutGetIo(int index)
{
  switch (index) {
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

/**
  Initialize the digital out system.
  This is done automatically during system startup, but if you don't want
  it initialized, define \b NO_DOUT_INIT in your config.h file.
*/
void digitaloutInit()
{ 
  // configure enable lines as outputs and turn them all on
  pinGroupSetMode(GROUP_B, DIGITALOUT_ENABLE_MASK, OUTPUT);
  pinGroupOn(GROUP_B, DIGITALOUT_ENABLE_MASK);
  
  // configure douts as outputs & turn off
  pinGroupSetMode(GROUP_A, DOUT_PIOA, OUTPUT);
  pinGroupSetMode(GROUP_B, DOUT_PIOB, OUTPUT);
  pinGroupOff(GROUP_A, DOUT_PIOA);
  pinGroupOff(GROUP_B, DOUT_PIOB);
}

/** 
  Turn a digital out on or off.
  @param channel Which digital out channel - valid options are 0-7.
  @param on True to turn it on, false to turn it off.
  @return True on success, false on failure.
  
  \b Example
  \code
  // Turn digital out 2 on
  digitaloutSetValue(2, ON);
  \endcode
*/
void digitaloutSetValue(int channel, bool on)
{
  pinSetValue(digitaloutGetIo(channel), on);
}

/** 
  Read whether a digital out is on or off.
  @return True if it's on, false if it's off.
  
  \b Example
  \code
  if (digitaloutValue() == ON) {
    // DigitalOut 2 is high
  }
  else {
    // DigitalOut 2 is low
  }
  \endcode
*/
bool digitaloutValue(int channel)
{
  return pinValue(digitaloutGetIo(channel));
}

/** @}
*/

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

#include "osc.h"

static void digitaloutOscHandler(OscChannel ch, char* address, int idx, OscData d[], int datalen)
{
  if (datalen == 1 && d[0].type == INT) {
    digitaloutSetValue(idx, d[0].value.i);
  }
  else if (datalen == 0) {
    OscData d = { .type = INT, .value.i = digitaloutValue(idx) };
    oscCreateMessage(ch, address, &d, 1);
  }
}

static const OscNode digitaloutVal = {
  .name = "value",
  .handler = digitaloutOscHandler
};

const OscNode digitaloutOsc = {
  .name = "digitalout",
  .range = DIGITALOUT_COUNT,
  .children = { &digitaloutVal, 0 }
};

#endif
