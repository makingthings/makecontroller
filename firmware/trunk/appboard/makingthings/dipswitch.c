/*********************************************************************************

 Copyright 2006 MakingThings

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

#define DIPSWITCH_DEVICE 2

int DipSwitch_Start( void );
int DipSwitch_Stop( void );

int DipSwitch_users;

/** \defgroup DipSwitch
* The DIP Switch subsystem reads values in from the 8 position DIP Switch (0 - 255).
* \ingroup AppBoard
* @{
*/

/**
	Sets whether the DIP Switch is active.
	@param state An integer specifying the state of the DIP Switch - 1 (on) or 0 (off).
	@return Zero on success.
*/
int DipSwitch_SetActive( int state )
{
  if ( state )
    return DipSwitch_Start(  );
  else
    return DipSwitch_Stop(  );
}

/**
	Returns the active state of the DIP Switch.
	@return The active state of the DIP Switch - 1 (active) or 0 (inactive).
*/
int DipSwitch_GetActive( )
{
  return DipSwitch_users > 0;
}

/**	
	Read the current configuration of the on-board DIP switch.
	@return An integer address corresponding to one of the 256 possible configurations.
	@see SPI
*/
int DipSwitch_GetValue( )
{
  if ( DipSwitch_users == 0 )
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

/** @}
*/

int DipSwitch_Start()
{
  int status;
  if ( DipSwitch_users++ == 0 )
  {
    // Start the SPI
    status = Spi_Start( DIPSWITCH_DEVICE );
    if ( status != CONTROLLER_OK )
    {
      DipSwitch_users--;
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
  if ( DipSwitch_users <= 0 )
    return CONTROLLER_ERROR_TOO_MANY_STOPS;
  
  if ( --DipSwitch_users == 0 )
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
	The DIP Switch has two properties - 'value' and 'active'.

	\par Value
	The 'value' property corresponds to current configuration of the DIP Switch.
	The DIP switch is 8-position, which means that it will send a value from 0-255
	depending on the orientation of each of the switches.  Convert the binary number
	represented by the switches to decimal to arrive at this value.
	\par
	Because you can only ever \em read the value of an input, you'll never
	want to include an argument at the end of your OSC message to read the value.\n
	To read from the DIP Switch, send the message
	\verbatim /dipswitch/value \endverbatim
	Note the lack of a device index, and the lack of an argument value.
	
	\par Active
	The 'active' property corresponds to the active state of the DIP Switch.
	If the DIP Switch is set to be active, no other tasks will be able to
	use its I/O lines.  This might be useful if the Controller is being used with
	another piece of hardware.  If you're not seeing appropriate
	responses to your messages to the DIP Switch, check the whether it's 
	locked by sending a message like
	\verbatim /dipswitch/active \endverbatim
	\par
	You can set the active flag by sending
	\verbatim /dipswitch/active 1 \endverbatim
*/

#include "osc.h"

static char* DipSwitchOsc_Name = "dipswitch";
static char* DipSwitchOsc_PropertyNames[] = { "active", "value", 0 }; // must have a trailing 0

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

int DipSwitchOsc_Poll( )
{
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
  }
  return CONTROLLER_OK;
}

// Get the property
int DipSwitchOsc_PropertyGet( int property )
{
  int value;
  switch ( property )
  {
    case 0:
      value = DipSwitch_GetActive( );
      break;
    case 1:
      value = DipSwitch_GetValue( );
      break;
  }
  
  return value;
}

#endif
