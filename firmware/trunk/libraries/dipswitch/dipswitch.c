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


#include "dipswitch.h"
#include "core.h"

#if (APPBOARD_VERSION >= 200)
#warning dipswitch is not available on Application Board v2.0 or later
#else

#define DIPSWITCH_DEVICE 2

/**
  \defgroup dipswitch DIP Switch
  Reads values in from the 8 position DIP Switch (0 - 255) on the Application Board.

  \section Usage
  First, initialize the DIP switch with dipswitchInit().  If you only want to read the value
  of a single channel on the switch, use dipswitchSingleValue(), otherwise use dipswitchValue()
  to read all the channels at once.

  Note - this is only appropriate when using the Make Application Board.

  See the <a href="http://www.makingthings.com/documentation/tutorial/application-board-overview/user-interface">
  Application Board overview</a> for details.
  \ingroup io
  @{
*/

/**
  Enable the DIP switch.

  \b Example
  \code
  dipswitchInit();
  \endcode
*/
void dipswitchInit()
{
  spiEnableChannel(DIPSWITCH_DEVICE);
  spiConfigure(DIPSWITCH_DEVICE, 8, 4, 0, 1);
}

/** 
  Read the current configuration of the on-board DIP switch.
  @return An integer from 0-255 indicating the current configuration of the DIP switch.
  
  \b Example
  \code
  int dip_switch = dipswitchValue();
  // now dip_switch has a bitmask of all 8 channels of the DIP switch
  \endcode
*/
int dipswitchValue()
{
  spiLock();
  unsigned char c[2] = { 0xFE, 0xFF };
  spiReadWriteBlock(DIPSWITCH_DEVICE, c, 2);
  spiUnlock();

  return (c[1] & 0x01) << 8 |
         (c[0] & 0x01) << 7 |
          c[1] >> 1;
}

/**
  Read a single channel's value.
  @param channel The channel (0-7) you'd like to read.
  @return true if the channel is on, false if it's off.
  
  \b Example
  \code
  if (dipswitchSingleValue(4) == ON) {
    // DIP switch channel 4 is on
  }
  else {
    // DIP switch channel 4 is off
  }
  \endcode
*/
bool dipswitchSingleValue(int channel)
{
  if (channel < 0 || channel > 7)
    return false;

  int val = dipswitchValue();
  return ( val < 0 ) ? false : ((val >> channel) & 0x1);
}

/** @} */

#ifdef OSC

/** \defgroup DIPSwitchOSC DIP Switch - OSC
  Read the Application Board's DIP Switch via OSC.
  \ingroup OSC
  
  \section devices Devices
  There's a single DIP Switch the Make Application Board. Because there's
  only one device, a device index is not included in any OSC messages to the
  DIP Switch.
  
  \section properties Properties
  The DIP Switch has the following properties
  - value
  - autosend

  \par Value
  The \b value property corresponds to current configuration of the DIP Switch.
  The DIP switch is 8-position, which means that it will send a value from 0-255
  depending on the orientation of each of the switches. Mask off the appropriate bits to 
  determine the value of a particular channel.
  \par
  Because you can only ever \em read the value of an input, you'll never
  want to include an argument at the end of your OSC message to read the value.\n
  To read from the DIP Switch, send the message
  \verbatim /dipswitch/value \endverbatim

  \par Autosend
  The \b autosend property corresponds to whether the DIP Switch will automatically send a message
  when its value changes.
  To tell the DIP Swtich to automatically send messages, send the message
  \verbatim /dipswitch/autosend 1 \endverbatim
  To have the DIP Switch stop sending messages automatically, send the message
  \verbatim /dipswitch/autosend 0 \endverbatim
  All autosend messages send at the same interval.  You can set this interval, in 
  milliseconds, by sending the message
  \verbatim /system/autosend-interval 10 \endverbatim
  so that messages will be sent every 10 milliseconds.  This can be anywhere from 1 to 5000 milliseconds.
  You also need to select whether the board should send to you over USB or Ethernet.  Send
  \verbatim /system/autosend-usb 1 \endverbatim
  to send via USB, and 
  \verbatim /system/autosend-udp 1 \endverbatim
  to send via Ethernet.  Via Ethernet, the board will send messages to the last address it received a message from.
*/

//#include "osc.h"
//
//bool DipSwitch_GetAutoSend( bool init )
//{
//  DipSwitch_SetActive( 1 );
//  if( init )
//  {
//    int autosend;
//    Eeprom_Read( EEPROM_DIPSWITCH_AUTOSEND, (uchar*)&autosend, 4 );
//    DipSwitch->autosend = (autosend == 1 ) ? 1 : 0;
//  }
//  return DipSwitch->autosend;
//}
//
//void DipSwitch_SetAutoSend( int onoff )
//{
//  DipSwitch_SetActive( 1 );
//  if( DipSwitch->autosend != onoff )
//  {
//    DipSwitch->autosend = onoff;
//    Eeprom_Write( EEPROM_DIPSWITCH_AUTOSEND, (uchar*)&onoff, 4 );
//  }
//}

//int DipSwitchOsc_Async( int channel )
//{
//  int newMsgs = 0;
//  if( !DipSwitch_GetAutoSend( false ) )
//    return newMsgs;
//  char address[ OSC_SCRATCH_SIZE ];
//  int value = DipSwitch_GetValue( );
//  
//  if( value != DipSwitch->lastValue )
//  {
//    DipSwitch->lastValue = value;
//    snprintf( address, OSC_SCRATCH_SIZE, "/%s/value", DipSwitchOsc_Name );
//    Osc_CreateMessage( channel, address, ",i", value );
//    newMsgs++;
//  }
//
//  return newMsgs;
//}
static bool dipswitchOscHandler(OscChannel ch, char* address, int idx, OscData d[], int datalen)
{
  UNUSED(d);
  UNUSED(idx);
  if (datalen == 0) {
    OscData d = {
      .type = INT,
      .value.i = dipswitchValue()
    };
    oscCreateMessage(ch, address, &d, 1);
    return true;
  }
  return false;
}

static const OscNode dipswitchValueNode = { .name = "value", .handler = dipswitchOscHandler };
const OscNode dipswitchOsc = {
  .name = "dipswitch",
  .children = {
    &dipswitchValueNode,
    0
  }
};

#endif // OSC

#endif // (APPBOARD_VERSION >= 200)
