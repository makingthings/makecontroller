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

// MakingThings - Make Controller Board - 2006

/** \file dipswitch.c	
  DIP Switch.
	Functions for reading from the 8-position DIP switch on the Make Application Board.
*/

#include "io.h"
#include "dipswitch.h"
#include "spi.h"
#include "config.h"
#include "stdio.h"

#define DIPSWITCH_DEVICE 2

static int DipSwitch_Start( void );
static int DipSwitch_Stop( void );

typedef struct
{
  int users;
  int lastValue;
  #ifdef OSC
  int autosend;
  #endif
} DipSwitchSubsystem;

DipSwitchSubsystem* DipSwitch;

/** \defgroup DipSwitch DIP Switch
* The DIP Switch subsystem reads values in from the 8 position DIP Switch (0 - 255) on the Application Board.
  Mask off the appropriate bits in the value returned from the DIP switch to determine whether a particular channel is on or off.
  
  See the <a href="http://www.makingthings.com/documentation/tutorial/application-board-overview/user-interface">
  Application Board overview</a> for details.
* \ingroup Libraries
* @{
*/

/**
	Sets whether the DIP Switch is active.
	@param state An integer specifying the state of the DIP Switch - 1 (on) or 0 (off).
	@return Zero on success.
	
	\b Example
	\code
	// enable the DIP switch
	DipSwitch_SetActive(1);
	\endcode
*/
int DipSwitch_SetActive( int state )
{
  if ( state )
  {
    if( DipSwitch == NULL )
    {
      DipSwitch = MallocWait( sizeof( DipSwitchSubsystem ), 100 );
      DipSwitch->users = 0;
      DipSwitch->lastValue = 0;
      #ifdef OSC
      DipSwitch->autosend = DipSwitch_GetAutoSend( true );
      #endif
      return DipSwitch_Start(  );
    }
  }
  else
  {
    if( DipSwitch )
      Free( DipSwitch );
    return DipSwitch_Stop(  );
  }
  return CONTROLLER_OK;
}

/**
	Returns the active state of the DIP Switch.
	@return The active state of the DIP Switch - 1 (active) or 0 (inactive).
	
	\b Example
	\code
	if( DipSwitch_GetActive() )
	{
	  // DIP switch is active
	}
	else
	{
	  // DIP switch is not active
	}
	\endcode
*/
int DipSwitch_GetActive( )
{
  return DipSwitch->users > 0;
}

/**	
	Read the current configuration of the on-board DIP switch.
	@return An integer from 0-255 indicating the current configuration of the DIP switch.
	
	\b Example
	\code
	int dip_switch = DipSwitch_GetValue();
	// now dip_switch has a bitmask of all 8 channels of the DIP switch
	\endcode
*/
int DipSwitch_GetValue( )
{
  DipSwitch_SetActive( 1 );
  if ( DipSwitch->users == 0 )
  {
    int status = DipSwitch_Start();
    if ( status != CONTROLLER_OK )
      return status;
  }

  Spi_Lock();

  unsigned char c[ 2 ];

  c[ 0 ] = 0xFE;
  c[ 1 ] = 0xFF;
  
  Spi_ReadWriteBlock( DIPSWITCH_DEVICE, c, 2 );

  Spi_Unlock();

  int r = ( c[ 1 ] & 0x01 ) << 8 | 
          ( c[ 0 ] & 0x01 ) << 7 | 
            c[ 1 ] >> 1; 

  return r;
}

/**
  Read a single channel's value.
  This is a convenience function that relies on DipSwitch_GetValue()
  internally, but extracts the value for a given channel.
  @param channel The channel (0-7) you'd like to read.
  return true if the channel is on, false if it's off.
  @see DipSwitch_GetValue( )
  
  \b Example
	\code
	if(DipSwitch_GetValueChannel(4) )
	{
	  // DIP switch channel 4 is on
	}
	else
	{
	  // DIP switch channel 4 is off
	}
	// now dip_switch has a bitmask of all 8 channels of the DIP switch
	\endcode
*/
bool DipSwitch_GetValueChannel( int channel )
{
  if( channel < 0 || channel > 7 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  int val = DipSwitch_GetValue();
  if( val < 0 )
    return false;
  else
    return ((val >> channel) & 0x1);
}

/** @}
*/

int DipSwitch_Start()
{
  int status;
  if ( DipSwitch->users++ == 0 )
  {
    // Start the SPI
    status = Spi_Start( DIPSWITCH_DEVICE );
    if ( status != CONTROLLER_OK )
    {
      DipSwitch->users--;
      return status;
    }
    
    // Configure the channel
    status = Spi_Configure( DIPSWITCH_DEVICE, 8, 4, 0, 1 );
    if ( status != CONTROLLER_OK )
    {
      // Undo all the setup.  Sigh.
      DipSwitch_Stop();
      return status;
    }
  }
  return CONTROLLER_OK;
}

int DipSwitch_Stop()
{
  if ( DipSwitch->users <= 0 )
    return CONTROLLER_ERROR_TOO_MANY_STOPS;
  
  if ( --DipSwitch->users == 0 )
    Spi_Stop( DIPSWITCH_DEVICE );

  return CONTROLLER_OK;
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

#include "osc.h"

bool DipSwitch_GetAutoSend( bool init )
{
  DipSwitch_SetActive( 1 );
  if( init )
  {
    int autosend;
    Eeprom_Read( EEPROM_DIPSWITCH_AUTOSEND, (uchar*)&autosend, 4 );
    DipSwitch->autosend = (autosend == 1 ) ? 1 : 0;
  }
  return DipSwitch->autosend;
}

void DipSwitch_SetAutoSend( int onoff )
{
  DipSwitch_SetActive( 1 );
  if( DipSwitch->autosend != onoff )
  {
    DipSwitch->autosend = onoff;
    Eeprom_Write( EEPROM_DIPSWITCH_AUTOSEND, (uchar*)&onoff, 4 );
  }
}

static char* DipSwitchOsc_Name = "dipswitch";
static char* DipSwitchOsc_PropertyNames[] = { "active", "value", "autosend",  0 }; // must have a trailing 0

int DipSwitchOsc_PropertySet( int property, int value );
int DipSwitchOsc_PropertyGet( int property );

const char* DipSwitchOsc_GetName( void )
{
  return DipSwitchOsc_Name;
}

int DipSwitchOsc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_IntReceiverHelper( channel, message, length, 
                                      DipSwitchOsc_Name,
                                      DipSwitchOsc_PropertySet, DipSwitchOsc_PropertyGet, 
                                      DipSwitchOsc_PropertyNames );

  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, DipSwitchOsc_Name, status );
  return CONTROLLER_OK;
}

// Set the index LED, property with the value
int DipSwitchOsc_PropertySet( int property, int value )
{
  switch ( property )
  {
    case 0: 
      DipSwitch_SetActive( value );
      break;      
    case 2: 
      DipSwitch_SetAutoSend( value );
      break;  
  }
  return CONTROLLER_OK;
}

// Get the property
int DipSwitchOsc_PropertyGet( int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      value = DipSwitch_GetActive( );
      break;
    case 1:
      value = DipSwitch_GetValue( );
      break;
    case 2:
      value = DipSwitch_GetAutoSend( false );
      break;
  }
  
  return value;
}

int DipSwitchOsc_Async( int channel )
{
  int newMsgs = 0;
  if( !DipSwitch_GetAutoSend( false ) )
    return newMsgs;
  char address[ OSC_SCRATCH_SIZE ];
  int value = DipSwitch_GetValue( );
  
  if( value != DipSwitch->lastValue )
  {
    DipSwitch->lastValue = value;
    snprintf( address, OSC_SCRATCH_SIZE, "/%s/value", DipSwitchOsc_Name );
    Osc_CreateMessage( channel, address, ",i", value );
    newMsgs++;
  }

  return newMsgs;
}

#endif
