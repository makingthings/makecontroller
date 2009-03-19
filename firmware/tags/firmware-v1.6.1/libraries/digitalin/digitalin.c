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

/** \file digitalin.c	
	Digital In.
	Library of functions for the Make Application Board's Digital Output section.
*/

#include "io.h"
#include "digitalin.h"
#include "analogin.h"
#include "config.h"

#include "AT91SAM7X256.h"

// This number is 4 because the a/d converter is sitting on the IO's 4 - 7
// this means that we'll need to use the A/d converter to get the digital value.
// Crazy, eh?  And slow.  Whew.
#define DIGITALIN_COUNT 8 

//#if ( APPBOARD_VERSION == 90 )
  #define DIGITALIN_0_IO IO_PB27
  #define DIGITALIN_1_IO IO_PB28
  #define DIGITALIN_2_IO IO_PB29
  #define DIGITALIN_3_IO IO_PB30
  #define DIGITALIN_4_IO -4
  #define DIGITALIN_5_IO -5
  #define DIGITALIN_6_IO -6
  #define DIGITALIN_7_IO -7
//#endif

static int DigitalIn_Start( int index );
static int DigitalIn_Stop( int index );

static int DigitalIn_GetIo( int index );

static int DigitalIn_users[ DIGITALIN_COUNT ];


/** \defgroup DigitalIn Digital Inputs
* Read the 8 inputs on the Make Controller as digital values - 1 (on) or 0 (off).  
  The 8 inputs can be either read as digital inputs by DigitalIn or as \ref AnalogIn.  
  Because inputs 4-7 are actually AnalogIn lines, there's no performance gain to reading
  those as DigitalIns.
* \ingroup Libraries
* @{
*/

/**
	Sets whether the specified Digital In is active.
	@param index An integer specifying which Digital In (0-7).
	@param state An integer specifying the state - 1 (active) or 0 (inactive).
	@return Zero on success.
	
	\b Example
	\code
	// Enable DigitalIn 3
	DigitalIn_SetActive(3, 1);
	\endcode
*/
int DigitalIn_SetActive( int index, int state )
{
  if ( index < 0 || index >= DIGITALIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( state )
    return DigitalIn_Start( index );
  else
    return DigitalIn_Stop( index );
}

/**
	Returns the active state of the Digital In.
	@param index An integer specifying which Digital In (0-7).
	@return Zero if inactive, non-zero if active.
	
	\b Example
	\code
	if( DigitalIn_GetActive(3) )
	{
	  // DigitalIn 3 is active
	}
	else
	{
	  // DigitalIn 3 is inactive
	}
	\endcode
*/
int DigitalIn_GetActive( int index )
{
  if ( index < 0 || index >= DIGITALIN_COUNT )
    return false;
  return DigitalIn_users[ index ] > 0;
}

/**	
	Read the value of a Digital Input on the MAKE Application Board.
	If the voltage on the input is greater than ~0.6V, the Digital In will read high.
	@param index An integer specifying which Digital In (0-7).
  @return Non-zero when high, 0 when low
  
  \b Example
	\code
	if( DigitalIn_GetValue(5) )
	{
	  // DigitalIn 5 is high
	}
	else
	{
	  // DigitalIn 5 is low
	}
	\endcode
*/
int DigitalIn_GetValue( int index )
{
  if ( index < 0 || index >= DIGITALIN_COUNT )
    return 0;

  if ( DigitalIn_users[ index ] < 1 )
  {
    int status = DigitalIn_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  int io = DigitalIn_GetIo( index );

  if ( io >= 0 )
    return Io_GetValue( io );
  else 
    return AnalogIn_GetValue( -io ) > 200;
}

/** @}
*/

int DigitalIn_Start( int index )
{
  int status;

  if ( index < 0 || index >= DIGITALIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( DigitalIn_users[ index ]++ == 0 )
  {
    int io = DigitalIn_GetIo( index );

    if ( io > 0 )
    {
      status = Io_Start( io, false );
      if ( status != CONTROLLER_OK )
      {
        DigitalIn_users[ index ]--;
        return CONTROLLER_ERROR_CANT_LOCK;
      }
  
      // Got it, now set the io up right
      Io_SetPio( io, true );
      Io_SetDirection( io, IO_INPUT );
    }
    else
      return AnalogIn_SetActive( -io, 1 );
  }

  return CONTROLLER_OK;
}

int DigitalIn_Stop( int index )
{
  if ( index < 0 || index >= DIGITALIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( DigitalIn_users[ index ] < 1 )
    return CONTROLLER_ERROR_TOO_MANY_STOPS;

  if ( --DigitalIn_users[ index ] == 0 )
  {
    int io = DigitalIn_GetIo( index );
    Io_Stop( io );
  }
  return CONTROLLER_OK;
}

int DigitalIn_GetIo( int index )
{
  int io = -1;
  switch ( index )
  {
    case 0:
      io = DIGITALIN_0_IO;
      break;
    case 1:
      io = DIGITALIN_1_IO;
      break;
    case 2:
      io = DIGITALIN_2_IO;
      break;
    case 3:
      io = DIGITALIN_3_IO;
      break;
    case 4:
      io = DIGITALIN_4_IO;
      break;
    case 5:
      io = DIGITALIN_5_IO;
      break;
    case 6:
      io = DIGITALIN_6_IO;
      break;
    case 7:
      io = DIGITALIN_7_IO;
      break;
  }
  return io;
}

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

#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* DigitalInOsc_Name = "digitalin";
static char* DigitalInOsc_PropertyNames[] = { "active", "value", 0 }; // must have a trailing 0

int DigitalInOsc_PropertySet( int index, int property, int value );
int DigitalInOsc_PropertyGet( int index, int property );

// Returns the name of the subsystem
const char* DigitalInOsc_GetName( )
{
  return DigitalInOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int DigitalInOsc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
                                     DIGITALIN_COUNT, DigitalInOsc_Name,
                                     DigitalInOsc_PropertySet, DigitalInOsc_PropertyGet, 
                                     DigitalInOsc_PropertyNames );

                                     
  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, DigitalInOsc_Name, status );
  return CONTROLLER_OK;
}

// Set the index LED, property with the value
int DigitalInOsc_PropertySet( int index, int property, int value )
{
  switch ( property )
  {
    case 0: 
      DigitalIn_SetActive( index, value );
      break;      
  }
  return CONTROLLER_OK;
}

// Get the index LED, property
int DigitalInOsc_PropertyGet( int index, int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      value = DigitalIn_GetActive( index );
      break;
    case 1:
      value = DigitalIn_GetValue( index );
      break;
  }
  
  return value;
}

#endif
