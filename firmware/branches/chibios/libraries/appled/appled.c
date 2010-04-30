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

/**
  \defgroup appled Application Board LEDs
  Status LEDs for program feedback.
  App LEDs (Application Board LED) are great for providing some information about how your
  program is running.
  \ingroup io
  @{
*/

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
  @param led Which app LED - valid options are 0-3

  \b Example
  \code
  appledEnable(1); // set up app led 1
  \endcode
*/
void appledEnable(int led)
{
  short io = appledGetIo(led);
  pinSetMode(io, OUTPUT);
  pinOn(io); // inverted
}

/**
  Turn an AppLed on or off.
  @param led Which app LED - valid options are 0-3.
  @param on True to turn it on, false to turn it off.

  \b Example
  \code
  appledSetValue(0, ON); // turn LED 0 on
  appledSetValue(1, OFF); // turn LED 1 off
  \endcode
*/
void appledSetValue(int led, bool on)
{
  pinSetValue(appledGetIo(led), !on); // inverted since it's tied to 3.3V
}

/**
  Read whether an AppLed is currently on or not.
  @param led Which app LED - valid options are 0-3.
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
bool appledValue(int led)
{
  return !pinValue(appledGetIo(led));
}

/** @} */

#ifdef OSC

#include "osc.h"

/**
  \defgroup AppLEDOSC OSC - App LED
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

static bool appledOscHandler(OscChannel ch, char* address, int idx, OscData d[], int datalen)
{
  if (datalen == 1) {
    appledSetValue(idx, d[0].value.i);
    return true;
  }
  else if (datalen == 0) {
    OscData d;
    d.value.i = appledValue(idx);
    oscCreateMessage(ch, address, &d, 1);
    return true;
  }
  return false;
}

static const OscNode appledVal = {
  .name = "value",
  .handler = appledOscHandler
};
static const OscNode appledRange = {
  .range = 4,
  .children = { &appledVal, 0 }
};
const OscNode appledOsc = {
  .name = "appled",
  .children = { &appledRange, 0 }
};

#endif // OSC

