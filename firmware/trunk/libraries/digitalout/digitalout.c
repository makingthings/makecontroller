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

/** \file digitalout.c	
	Library of functions for the Make Application Board's Digital Outputs.
*/

#include "io.h"
#include "digitalout.h"
#include "config.h"

#include "AT91SAM7X256.h"

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
#endif

#if ( APPBOARD_VERSION == 90 )
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
#endif
#if ( APPBOARD_VERSION == 95 || APPBOARD_VERSION == 100 )

  #define DIGITALOUT_0_IO IO_PA24
  #define DIGITALOUT_1_IO IO_PA05
  #define DIGITALOUT_2_IO IO_PA06
  #define DIGITALOUT_3_IO IO_PA02
  #define DIGITALOUT_4_IO IO_PB25
  #define DIGITALOUT_5_IO IO_PA25
  #define DIGITALOUT_6_IO IO_PA26
  #define DIGITALOUT_7_IO IO_PB23
  #define DIGITALOUT_01_ENABLE IO_PB19
  #define DIGITALOUT_23_ENABLE IO_PB20
  #define DIGITALOUT_45_ENABLE IO_PB21
  #define DIGITALOUT_67_ENABLE IO_PB22
#endif

static int DigitalOut_Start( int index );
static int DigitalOut_Stop( int index );

int DigitalOut_GetIo( int index );
int DigitalOut_GetEnableIo( int enableIndex );

int DigitalOut_users[ DIGITALOUT_COUNT ];
int DigitalOut_enableUsers[ DIGITALOUT_COUNT >> 1 ];

/** \defgroup DigitalOut Digital Outputs
* The Digital Out subsystem sends digital values out the 8 high current outputs - 1 (on) or 0 (off).
  
  If you've previously used any of the other systems on the outputs (steppers, motors, etc.), you'll need
  to set them to \b inactive to unlock the IO lines and use the Digital Outs.
  
  See the <a href="http://www.makingthings.com/documentation/tutorial/application-board-overview/digital-outputs">
  Digital Out section</a> of the Application Board overview for more details.
* \ingroup Libraries
* @{
*/

/**
	Enable or disable a DigitalOut.
	This is automatically called, setting the channel active, the first time DigitalOut_SetValue() is called.
	However, the channel must be explicitly set to inactive in order for any other devices to access the I/O lines. 
	@param index An integer specifying which Digital Out (0-7).
	@param state An integer specifying the state - on (1) or off (0).
	@return Zero on success.
	
	\b Example
	\code
	// Enable DigitalOut 6
	DigitalOut_SetActive( 6, 1 );
	\endcode
*/
int DigitalOut_SetActive( int index, int state )
{
  if ( index < 0 || index >= DIGITALOUT_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( state )
    return DigitalOut_Start( index );
  else
    return DigitalOut_Stop( index );
}

/**
	Read whether a DigitalOut is active.
	@param index An integer specifying which Digital Out (0-7).
	@return Nonzero when active, 0 when inactive
	
	\b Example
	\code
	if( DigitalOut_GetActive( 5 ) )
	{
	  // DigitalOut 5 is active
	}
	else
	{
	  // DigitalOut 5 is inactive
	}
	\endcode
*/
int DigitalOut_GetActive( int index )
{
  if ( index < 0 || index >= DIGITALOUT_COUNT )
    return false;
  return DigitalOut_users[ index ] > 0;
}

/**	
	Turn a Digital Out on or off.
	@param index An integer specifying which Digital Out (0-7).
	@param state An integer specifying the state - on (1) or off (0).
  @return Zero on success.
  
  \b Example
	\code
	// Turn digital out 2 on
	DigitalOut_SetValue( 2, 1 );
	\endcode
*/
int DigitalOut_SetValue( int index, int state )
{
  if ( index < 0 || index >= DIGITALOUT_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( DigitalOut_users[ index ] < 1 )
  {
    int status = DigitalOut_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  int io = DigitalOut_GetIo( index );
  if ( state )
    Io_SetValue( io, true );
  else 
    Io_SetValue( io, false );

  return CONTROLLER_OK;
}

/**	
	Read whether a DigitalOut is on or off.
	@param index An integer specifying which Digital Out (0-7).
  @return The value - on (1) or off (0).
  
  \b Example
	\code
	if( DigitalOut_GetValue( 2 ) )
	{
	  // DigitalOut 2 is high
	}
	else
	{
	  // DigitalOut 2 is low
	}
	\endcode
*/
int DigitalOut_GetValue( int index )
{
  if ( index < 0 || index >= DIGITALOUT_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( DigitalOut_users[ index ] < 1 )
  {
    int status = DigitalOut_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  int io = DigitalOut_GetIo( index );

  return Io_GetValue( io );

  return CONTROLLER_OK;
}


/** @}
*/

int DigitalOut_Start( int index )
{
  int status;
  int enableIndex = index >> 1;

  if ( index < 0 || index >= DIGITALOUT_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( DigitalOut_users[ index ]++ == 0 )
  {
    int enableIo = DigitalOut_GetEnableIo( enableIndex );
    if ( DigitalOut_enableUsers[ enableIndex ]++ == 0 )
    {
      status = Io_Start( enableIo, true );
      if ( status != CONTROLLER_OK )
      {
        DigitalOut_enableUsers[ enableIndex ]--;
        DigitalOut_users[ index ]--;
        return CONTROLLER_ERROR_CANT_LOCK;
      }
      Io_SetValue( enableIo, true );
    }

    int io = DigitalOut_GetIo( index );

    status = Io_Start( io, true );
    if ( status != CONTROLLER_OK )
    {
      DigitalOut_users[ index ]--;
      return CONTROLLER_ERROR_CANT_LOCK;
    }

    // Got it, now set the io up right
    Io_SetPio( io, true );
    Io_SetValue( io, false );

    Io_SetDirection( io, IO_OUTPUT );
  }

  return CONTROLLER_OK;
}

int DigitalOut_Stop( int index )
{
  if ( index < 0 || index >= DIGITALOUT_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( DigitalOut_users[ index ] < 1 )
    return CONTROLLER_ERROR_TOO_MANY_STOPS;

  if ( --DigitalOut_users[ index ] == 0 )
  {
    int io = DigitalOut_GetIo( index );
    Io_Stop( io );

    int enableIndex = index >> 1;
    if ( --DigitalOut_enableUsers[ enableIndex ] == 0 )
    {
      int enableIo = DigitalOut_GetEnableIo( enableIndex );
      Io_Stop( enableIo );
    }

  }
  return CONTROLLER_OK;
}

int DigitalOut_GetIo( int index )
{
  int io = -1;
  switch ( index )
  {
    case 0:
      io = DIGITALOUT_0_IO;
      break;
    case 1:
      io = DIGITALOUT_1_IO;
      break;
    case 2:
      io = DIGITALOUT_2_IO;
      break;
    case 3:
      io = DIGITALOUT_3_IO;
      break;
    case 4:
      io = DIGITALOUT_4_IO;
      break;
    case 5:
      io = DIGITALOUT_5_IO;
      break;
    case 6:
      io = DIGITALOUT_6_IO;
      break;
    case 7:
      io = DIGITALOUT_7_IO;
      break;
  }
  return io;
}

int DigitalOut_GetEnableIo( int enableIndex )
{
  int enableIo = -1;
  switch ( enableIndex )
  {
    case 0:
      enableIo = DIGITALOUT_01_ENABLE;
      break;
    case 1:
      enableIo = DIGITALOUT_23_ENABLE;
      break;
    case 2:
      enableIo = DIGITALOUT_45_ENABLE;
      break;
    case 3:
      enableIo = DIGITALOUT_67_ENABLE;
      break;
  }
  return enableIo;
}

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
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* DigitalOutOsc_Name = "digitalout";
static char* DigitalOutOsc_PropertyNames[] = { "active", "value", 0 }; // must have a trailing 0

int DigitalOutOsc_PropertySet( int index, int property, int value );
int DigitalOutOsc_PropertyGet( int index, int property );

// Returns the name of the subsystem
const char* DigitalOutOsc_GetName( )
{
  return DigitalOutOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int DigitalOutOsc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
                                           DIGITALOUT_COUNT, DigitalOutOsc_Name,
                                           DigitalOutOsc_PropertySet, DigitalOutOsc_PropertyGet, 
                                           DigitalOutOsc_PropertyNames );
                                     
  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, DigitalOutOsc_Name, status );
  return CONTROLLER_OK;
}

// Set the index LED, property with the value
int DigitalOutOsc_PropertySet( int index, int property, int value )
{
  switch ( property )
  {
    case 0: 
      DigitalOut_SetActive( index, value );
      break;      
    case 1: 
      DigitalOut_SetValue( index, value );
      break;
  }
  return CONTROLLER_OK;
}

// Get the index LED, property
int DigitalOutOsc_PropertyGet( int index, int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      value = DigitalOut_GetActive( index );
      break;
    case 1:
      value = DigitalOut_GetValue( index );
      break;
  }
  
  return value;
}

#endif
