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

// MakingThings - Make Controller Kit - 2006

/** \file debugosc.c	
	Controller Debug.
*/

#include "debug.h"
#include <stdio.h>
#include "osc.h"
#include "config.h"
#include <stdarg.h>

static int Debug_Start( void );
static int Debug_Stop( void );

#define DEBUG_MAX_MESSAGE 200

struct Debug_
{
  int users;
  int level;
  int usb;
  int udp;
  char message[ DEBUG_MAX_MESSAGE ];
} DebugData;

/** \defgroup Debug
	The Debug subsystem offers a simple way to send debug messages back from the MAKE Controller via OSC.
	Even without single-stepping through the code running on the MAKE Controller, it is still easy to get helpful
	debug information about the program through the Debug subsystem.\n
	
	Specify the level of each debug message and then, in a debug session, set the level you would like to see.  More 
	frequent and lower level messages can be filtered out in a debug session by setting the debug
	level a bit higher to let through only the messages you're interested in. 
	\ingroup Controller
	@{
*/

/**
	Sets whether the Debug subsystem is active.
	@param state An integer specifying the active state - 1 (active) or 0 (inactive).
	@return Zero on success.
*/
int Debug_SetActive( int state )
{
  if ( state )
    return Debug_Start( );
  else
    return Debug_Stop( );
}

/**
	Returns the active state of the Debug subsystem.
	@return State - 1 (active) or 0 (inactive).
*/
int Debug_GetActive( )
{
  return DebugData.users > 0;
}

/**	
	Controls the controller debug level.
	@param level An integer specifying the level.
  @return none.
*/
int Debug_SetLevel( int level )
{
  if ( DebugData.users == 0 )
  {
    int status = Debug_Start();
    if ( status != CONTROLLER_OK )
      return status;
  }

  DebugData.level = level;
  return CONTROLLER_OK;
}

/**	
	Read the current debugger level.
  @return Level.
*/
int Debug_GetLevel( )
{
  if ( DebugData.users == 0 )
  {
    int status = Debug_Start();
    if ( status != CONTROLLER_OK )
      return status;
  }

  return DebugData.level;
}

/**
	Sets whether the Debug subsystem outputs to USB OSC.
	@param state An integer specifying the state - 1 (on) or 0 (off).
	@return Zero on success.
*/
int Debug_SetUsb( int state )
{
  DebugData.usb = state;
  return CONTROLLER_OK;
}

/**
	Returns the active state of the Debug subsystem.
	@return State - 1 (active) or 0 (inactive).
*/
int Debug_GetUsb( )
{
  return DebugData.usb;
}

/**
	Sets whether the Debug subsystem outputs to UDP OSC.
	@param state An integer specifying the state - 1 (on) or 0 (off).
	@return Zero on success.
*/
int Debug_SetUdp( int state )
{
  DebugData.udp = state;
  return CONTROLLER_OK;
}

/**
	Returns the active state of the Debug subsystem.
	@return State - 1 (active) or 0 (inactive).
*/
int Debug_GetUdp( )
{
  return DebugData.udp;
}

/**
  Create a debug message.
	@param level An integer specifying the debug level.  
	@param format The kind of message to send - most often "DEBUG_MESSAGE".
	@param ... The contents of the debug message - this can look just like a call to printf().
	@return 
*/

int Debug( int level, char* format, ... )
{
  va_list args;

  if ( !Debug_GetActive() )
    Debug_SetActive( 1 );

  if ( Osc_GetActive() && Osc_GetRunning( ) && level <= DebugData.level )
  {
    va_start( args, format );
    vsnprintf( DebugData.message, DEBUG_MAX_MESSAGE, format, args ); 
    // va_end( args );
  
    if ( DebugData.usb && Usb_GetActive() )
    {
      Osc_CreateMessage( OSC_CHANNEL_USB, "/debug/message", ",s", DebugData.message );
      Osc_SendPacket( OSC_CHANNEL_USB );
    }
    if ( DebugData.udp && Network_GetActive() )
    {
      Osc_CreateMessage( OSC_CHANNEL_UDP, "/debug/message", ",s", DebugData.message );
      Osc_SendPacket( OSC_CHANNEL_UDP );
    }
  }

  return CONTROLLER_OK;
}


/** @}
*/

int Debug_Start()
{
  if ( DebugData.users++ == 0 )
  {
    DebugData.usb = 1;
    DebugData.udp = 1;
    DebugData.level = DEBUG_MESSAGE;
  }
  return CONTROLLER_OK;
}

int Debug_Stop()
{
  if ( --DebugData.users == 0 )
    Osc_SetActive( false );

  return CONTROLLER_OK;
}


#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* DebugOsc_Name = "debug";
static char* DebugOsc_PropertyNames[] = { "active", "level", "usb", "udp", 0 }; // must have a trailing 0

int DebugOsc_PropertySet( int property, int value );
int DebugOsc_PropertyGet( int property );

// Returns the name of the subsystem
const char* DebugOsc_GetName( )
{
  return DebugOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int DebugOsc_ReceiveMessage( int channel, char* message, int length )
{
  return Osc_IntReceiverHelper( channel, message, length, 
                                DebugOsc_Name,
                                DebugOsc_PropertySet, DebugOsc_PropertyGet, 
                                DebugOsc_PropertyNames );
}

// Set the index LED, property with the value
int DebugOsc_PropertySet( int property, int value )
{
  switch ( property )
  {
    case 0: 
      Debug_SetActive( value );
      break;      
    case 1:
      Debug_SetLevel( value );
      break;
    case 2:
      Debug_SetUsb( value );
      break;
    case 3:
      Debug_SetUdp( value );
      break;
  }
  return CONTROLLER_OK;
}

// Get the index LED, property
int DebugOsc_PropertyGet( int property )
{
  int value;
  switch ( property )
  {
    case 0:
      value = Debug_GetActive( );
      break;
    case 1:
      value = Debug_GetLevel( );
      break;
    case 2:
      value = Debug_GetUsb( );
      break;  
    case 3:
      value = Debug_GetUdp( );
      break;  
  }
  
  return value;
}

