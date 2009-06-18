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
#include "error.h"
#include "stdio.h"
#include "core.h"

#if (APPBOARD_VERSION >= 200)
#warning dipswitch is not avaialable on Application Board v2.0 or later
#else

#define DIPSWITCH_DEVICE 2
// static
Spi* DipSwitch::spi = 0;
int DipSwitch::refcount = 0;

/**
  Create a new DipSwitch object.
  
  \b Example
  \code
  DipSwitch dip;
  // that's all there is to it.
  \endcode
*/
DipSwitch::DipSwitch( )
{
  if( refcount++ == 0 )
  {
    spi = new Spi( DIPSWITCH_DEVICE );
    spi->configure( 8, 4, 0, 1 );
  }
}

DipSwitch::~DipSwitch()
{
  if ( --refcount == 0 )
    delete spi;
}

/** 
  Read the current configuration of the on-board DIP switch.
  @return An integer from 0-255 indicating the current configuration of the DIP switch.
  
  \b Example
  \code
  DipSwitch dip;
  int dip_switch = dip.value();
  // now dip_switch has a bitmask of all 8 channels of the DIP switch
  \endcode
*/
int DipSwitch::value( )
{
  spi->lock();
  unsigned char c[ 2 ];
  c[ 0 ] = 0xFE;
  c[ 1 ] = 0xFF;
  
  spi->readWriteBlock( c, 2 );
  spi->unlock();

  int r = ( c[ 1 ] & 0x01 ) << 8 | 
          ( c[ 0 ] & 0x01 ) << 7 | 
            c[ 1 ] >> 1; 
  return r;
}

/**
  Read a single channel's value.
  
  If you pass a channel number into value() it will
  return the state of just that channel.
  
  @param channel The channel (0-7) you'd like to read.
  return true if the channel is on, false if it's off.
  
  \b Example
  \code
  DipSwitch dip;
  if(dip.value(4))
  {
    // DIP switch channel 4 is on
  }
  else
  {
    // DIP switch channel 4 is off
  }
  \endcode
*/
bool DipSwitch::value( int channel )
{
  if( channel < 0 || channel > 7 )
    return false;

  int val = value();
  return ( val < 0 ) ? false : ((val >> channel) & 0x1);
}

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
  - active

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
  
  \par Active
  The \b active property corresponds to the active state of the DIP Switch.
  If the DIP Switch is set to be active, no other tasks will be able to
  use its I/O lines.  If you're not seeing appropriate
  responses to your messages to the DIP Switch, check whether it's 
  locked by sending the message
  \verbatim /dipswitch/active \endverbatim
  \par
  You can set the active flag by sending
  \verbatim /dipswitch/active 1 \endverbatim
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
//
//static char* DipSwitchOsc_Name = "dipswitch";
//static char* DipSwitchOsc_PropertyNames[] = { "active", "value", "autosend",  0 }; // must have a trailing 0
//
//int DipSwitchOsc_PropertySet( int property, int value );
//int DipSwitchOsc_PropertyGet( int property );
//
//const char* DipSwitchOsc_GetName( void )
//{
//  return DipSwitchOsc_Name;
//}
//
//int DipSwitchOsc_ReceiveMessage( int channel, char* message, int length )
//{
//  int status = Osc_IntReceiverHelper( channel, message, length, 
//                                      DipSwitchOsc_Name,
//                                      DipSwitchOsc_PropertySet, DipSwitchOsc_PropertyGet, 
//                                      DipSwitchOsc_PropertyNames );
//
//  if ( status != CONTROLLER_OK )
//    return Osc_SendError( channel, DipSwitchOsc_Name, status );
//  return CONTROLLER_OK;
//}
//
//// Set the index LED, property with the value
//int DipSwitchOsc_PropertySet( int property, int value )
//{
//  switch ( property )
//  {
//    case 0: 
//      DipSwitch_SetActive( value );
//      break;      
//    case 2: 
//      DipSwitch_SetAutoSend( value );
//      break;  
//  }
//  return CONTROLLER_OK;
//}
//
//// Get the property
//int DipSwitchOsc_PropertyGet( int property )
//{
//  int value = 0;
//  switch ( property )
//  {
//    case 0:
//      value = DipSwitch_GetActive( );
//      break;
//    case 1:
//      value = DipSwitch_GetValue( );
//      break;
//    case 2:
//      value = DipSwitch_GetAutoSend( false );
//      break;
//  }
//  
//  return value;
//}
//
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

#endif

#endif // (APPBOARD_VERSION >= 200)
