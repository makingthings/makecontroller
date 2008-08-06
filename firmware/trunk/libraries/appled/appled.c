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

// MakingThings - Make Controller Kit - 2006

/** \file appled.c	
	MAKE Application Board LED control.
	Library of functions for the Make Application Board's LED subsystem.
*/

#include "io.h"
#include "appled.h"
#include "config.h"

#include "AT91SAM7X256.h"

#define APPLED_COUNT 4 

#if ( APPBOARD_VERSION == 50 )
  #define APPLED_0_IO IO_PB19
  #define APPLED_1_IO IO_PB20
  #define APPLED_2_IO IO_PB21
  #define APPLED_3_IO IO_PB22
#endif
#if ( APPBOARD_VERSION == 90 )
  #define APPLED_0_IO IO_PA10
  #define APPLED_1_IO IO_PA11
  #define APPLED_2_IO IO_PA13
  #define APPLED_3_IO IO_PA15
#endif
#if ( APPBOARD_VERSION == 95 || APPBOARD_VERSION == 100 )
  #define APPLED_0_IO IO_PA15
  #define APPLED_1_IO IO_PA13
  #define APPLED_2_IO IO_PA28
  #define APPLED_3_IO IO_PA27
#endif

static int AppLed_GetIo( int index );
static int AppLed_Start( int index );
static int AppLed_Stop( int index );

static int AppLed_users[ APPLED_COUNT ];

/** \defgroup AppLed Application Board LEDs
* The Application Board LED subsystem controls the 4 LEDs on the Application Board, for status and program feedback.
* \ingroup Libraries
* @{
*/

/**
	Enable an LED in the AppLed subsystem.
	@param index An integer specifying which LED (0-3).
	@param state An integer specifying locked/active (1) or unlocked/inactive (0).
	@return Zero on success.
	
	\b Example
	\code
	// enable LED 0
	AppLed_SetActive(0, 1);
	\endcode
*/
int AppLed_SetActive( int index, int state )
{
  if ( index < 0 || index >= APPLED_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( state )
    return AppLed_Start( index );
  else
    return AppLed_Stop( index );
}

/**
	Read whether an AppLed is enabled.
	@param index An integer specifying which LED (0-3).
	@return Zero if available, otherwise non-zero.
	
	\b Example
	\code
	if(AppLed_GetActive(1))
	{
	  // LED 1 is enabled
	}
	else
	{
	  // LED 1 is not enabled
	}
	\endcode
*/
int AppLed_GetActive( int index )
{
  if ( index < 0 || index >= APPLED_COUNT )
    return false;
  return AppLed_users[ index ] > 0;
}

/**	
	Turn an LED on or off.
	Sets whether the specified LED on the Application Board is on or off.
	@param index an integer specifying which LED (0-3).
	@param state an integer specifying on (1) or off (0).
	@return Zero on success, otherwise error code.

  \b Example
  \code
  // turn on LED 2
  AppLed_SetState( 2, 1 );
  \endcode
*/
int AppLed_SetState( int index, int state )
{
  if ( index < 0 || index >= APPLED_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( AppLed_users[ index ] < 1 )
  {
    int status = AppLed_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  int io = AppLed_GetIo( index );
  // remember the LED's are tied to +3.3, so they're
  // on when the output is off
  if ( state )
    Io_SetValue( io, false );
  else 
    Io_SetValue( io, true );

  return CONTROLLER_OK;
}

/**	
	Read whether an LED is on or off.
	@param index An integer specifying which LED (0-3).
  @return the LED state, or zero on error.
  
  \b Example
	\code
	if(AppLed_GetState(2))
	{
	  // LED 2 is currently on
	}
	else
	{
	  // LED 2 is currently off
	}
	\endcode
*/
int AppLed_GetState( int index )
{
  if ( index < 0 || index >= APPLED_COUNT )
    return 0;

  if ( AppLed_users[ index ] < 1 )
  {
    int status = AppLed_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  int io = AppLed_GetIo( index );
  return Io_GetValue( io ) ? 0 : 1;
}

/** @}
*/

int AppLed_Start( int index )
{
  int status;

  if ( AppLed_users[ index ]++ == 0 )
  {
    int io = AppLed_GetIo( index );

    status = Io_Start( io, true );
    if ( status != CONTROLLER_OK )
    {
      AppLed_users[ index ]--;
      return CONTROLLER_ERROR_CANT_LOCK;
    }

    // Got it, now set the io up right
    Io_SetPio( io, true );
    Io_SetValue( io, true );
    Io_SetDirection( io, IO_OUTPUT );
  }

  return CONTROLLER_OK;
}

int AppLed_Stop( int index )
{
  if ( AppLed_users[ index ] < 1 )
    return CONTROLLER_ERROR_TOO_MANY_STOPS;

  if ( --AppLed_users[ index ] == 0 )
  {
    int io = AppLed_GetIo( index );
    Io_Stop( io );
  }
  return CONTROLLER_OK;
}

int AppLed_GetIo( int index )
{
  int io = -1;
  switch ( index )
  {
    case 0:
      io = APPLED_0_IO;
      break;
    case 1:
      io = APPLED_1_IO;
      break;
    case 2:
      io = APPLED_2_IO;
      break;
    case 3:
      io = APPLED_3_IO;
      break;
  }
  return io;
}

#ifdef OSC

/** \defgroup AppLEDOSC App LED - OSC
  Control the Application Board's Status LEDs via OSC.
  \ingroup OSC
	
	\section devices Devices
	There are 4 LEDs on the Make Application Board, numbered 0 - 3.
	
	\section properties Properties
	The LEDs have two properties:
  - state
  - active

	\par State
	The \b state property corresponds to the on/off state of a given LED.
	For example, to turn on the first LED, send a message like
	\verbatim /appled/0/state 1\endverbatim
	To turn it off, send the message \verbatim /appled/0/state 0\endverbatim
	
	\par Active
	The \b active property corresponds to the active state of an LED.
	If an LED is set to be active, no other tasks will be able to
	write to it.  If you're not seeing appropriate
	responses to your messages to the LED, check the whether it's 
	locked by sending a message like
	\verbatim /appled/0/active \endverbatim
	\par
	You can set the active flag by sending
	\verbatim /appled/0/active 1 \endverbatim
*/

#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* AppLedOsc_Name = "appled";
static char* AppLedOsc_PropertyNames[] = { "active", "state", 0 }; // must have a trailing 0

int AppLedOsc_PropertySet( int index, int property, int value );
int AppLedOsc_PropertyGet( int index, int property );

// Returns the name of the subsystem
const char* AppLedOsc_GetName( )
{
  return AppLedOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int AppLedOsc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
                                           APPLED_COUNT, AppLedOsc_Name,
                                           AppLedOsc_PropertySet, AppLedOsc_PropertyGet, 
                                           AppLedOsc_PropertyNames );

  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, AppLedOsc_Name, status );
  return CONTROLLER_OK;
}

// Set the index LED, property with the value
int AppLedOsc_PropertySet( int index, int property, int value )
{
  switch ( property )
  {
    case 0: 
      AppLed_SetActive( index, value );
      break;      
    case 1:
      AppLed_SetState( index, value );
      break;
  }
  return CONTROLLER_OK;
}

// Get the index LED, property
int AppLedOsc_PropertyGet( int index, int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      value = AppLed_GetActive( index );
      break;
    case 1:
      value = AppLed_GetState( index );
      break;
  }
  
  return value;
}

#endif
