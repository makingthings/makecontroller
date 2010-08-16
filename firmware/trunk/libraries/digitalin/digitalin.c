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
#include "core.h"

// This number is 4 because the a/d converter is sitting on the IO's 4 - 7
// this means that we'll need to use the A/d converter to get the digital value.
// Crazy, eh?  And slow.  Whew.
#define DIGITALIN_COUNT 8

#ifndef DIGITALIN_THRESHOLD
#define DIGITALIN_THRESHOLD 200
#endif

// only need symbols for the first 4 since the others are ains
#define DIGITALIN_0 PIN_PB27
#define DIGITALIN_1 PIN_PB28
#define DIGITALIN_2 PIN_PB29
#define DIGITALIN_3 PIN_PB30

static void digitalinAutoSendInit(void);

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

void digitalinInit()
{

  #ifdef OSC
  digitalinAutoSendInit();
  #endif
}

static int digitalinGetPin(int index)
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
    return analoginValue(channel) > DIGITALIN_THRESHOLD;
  else 
    return pinValue(digitalinGetPin(channel));
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
  The Digital Ins have the following properties
  - value

  \par Value
  The \b value property corresponds to the on/off value of a Digital In.
  Because you can only ever \b read the value of an input, you'll never
  want to include an argument at the end of your OSC message to read the value.
  To read the third Digital In, send the message
  \verbatim /digitalin/2/value \endverbatim
*/

static void digitalinOscHandler(OscChannel ch, char* address, int idx, OscData d[], int datalen)
{
  UNUSED(d);
  if (datalen == 0) {
    OscData d = { .type = INT, .value.i = digitalinValue(idx) };
    oscCreateMessage(ch, address, &d, 1);
  }
}

// sort of a checksum to verify whether a previous save was legit
#define DIN_AUTOSEND_SAVED 0xDF

static uint8_t digitalinAutosendVals[DIGITALIN_COUNT];
static uint16_t digitalinAutosendChannels;

void digitalinAutoSendInit()
{
  digitalinAutosendChannels = eepromRead(EEPROM_DIGITALIN_AUTOSEND);
  if (((digitalinAutosendChannels >> 8) & 0xFF) != DIN_AUTOSEND_SAVED)
    digitalinAutosendChannels = DIN_AUTOSEND_SAVED << 8;
}

static void digitalinOscAutosender(OscChannel ch)
{
  uint8_t i;
  OscData d = { .type = INT };
  char addr[20];
  for (i = 0; i < ANALOGIN_CHANNELS; i++) {
    if (digitalinAutosendChannels & (1 << i)) {
      d.value.i = digitalinValue(i);
      if (digitalinAutosendVals[i] != d.value.i) {
        digitalinAutosendVals[i] = d.value.i;
        sniprintf(addr, sizeof(addr), "/digitalin/%d/value", i);
        oscCreateMessage(ch, addr, &d, 1);
      }
    }
  }
}

static void digitalinAutosendHandler(OscChannel ch, char* address, int idx, OscData d[], int datalen)
{
  UNUSED(d);
  UNUSED(address);
  if (datalen == 0) {
    OscData d;
    d.type = INT;
    d.value.i = (digitalinAutosendChannels & (1 << idx)) ? 1 : 0;
    oscCreateMessage(ch, address, &d, 1);
  }
  else if (datalen == 1) {
    if (d[0].value.i)
      digitalinAutosendChannels |= (1 << idx);
    else
      digitalinAutosendChannels &= ~(1 << idx);

    eepromWrite(EEPROM_DIGITALIN_AUTOSEND, digitalinAutosendChannels);
  }
}

static const OscNode digitalinAutosendNode = { .name = "autosend", .handler = digitalinAutosendHandler };
static const OscNode digitalinValueNode = { .name = "value", .handler = digitalinOscHandler };

const OscNode digitalinOsc = {
  .name = "digitalin",
  .range = DIGITALIN_COUNT,
  .autosender = digitalinOscAutosender,
  .children = {
    &digitalinValueNode,
    &digitalinAutosendNode, 0
  }
};

#endif // OSC
