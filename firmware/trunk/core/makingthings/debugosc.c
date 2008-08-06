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

/** \file debugosc.c	
	Controller Debug.
*/

#include "config.h"
#ifdef OSC

#include "debug.h"
#include <stdio.h>
#include "osc.h"
#include <stdarg.h>

#define DEBUG_MAX_MESSAGE 100

typedef struct Debug_
{
  int users;
  int level;
  int usb;
  int udp;
  char message[ DEBUG_MAX_MESSAGE ];
} DebugStruct;

DebugStruct* DebugData;

/** \defgroup Debug OSC Debug
	The Debug subsystem offers a simple way to send debug messages back from the MAKE Controller via OSC.
	Even without single-stepping through the code running on the MAKE Controller, it is still easy to get helpful
	debug information about the program through the Debug subsystem.
	
	Specify the level of each debug message and then, in a debug session, set the level you would like to see.  More 
	frequent and lower level messages can be filtered out in a debug session by setting the debug
	level a bit higher to let through only the messages you're interested in. 

  Possible levels are:
  - 0 - Always send
  - 1 - Error messages
  - 2 - Warning messages
  - 3 - Normal/test messages

	\ingroup Core
	@{
*/

/**
	Sets whether the Debug subsystem is active.
  The Debug system is automatically set to active by making a call to any Debug system function,
  so you don't typically have to worry about doing that.  But, you can set it to inactive
  to recover a bit of space on the heap.
	@param state An integer specifying the active state - 1 (active) or 0 (inactive).
	@return Zero on success.

  \par Example
  \code
  // Checking the debug level sets the system to active.
  int level = Debug_GetLevel();
  // then we can turn it off
  Debug_SetActive( 0 );
  \endcode
*/
int Debug_SetActive( int state )
{
  if( state && DebugData == NULL )
  {
		DebugData = MallocWait( sizeof( struct Debug_ ), 100 );
    DebugData->usb = 1;
    DebugData->udp = 1;
    DebugData->level = DEBUG_MESSAGE;
  }
  if( !state && DebugData != NULL )
  {
    Free( DebugData );
    DebugData = NULL;
  }
  return CONTROLLER_OK;
}

/**
	Returns the active state of the Debug subsystem.
	@return State - 1 (active) or 0 (inactive).

  \par Example
  \code
  // Check to see if we're active...
  int active = Debug_GetActive();
  if( active )
    // then do something...
  \endcode
*/
int Debug_GetActive( )
{
  return DebugData != NULL;
}

/**	
	Sets the debug level.
  Set to a lower number to get more messages.  Default level is 3.
	@param level An integer specifying the level (from 0-3).
  @return 0 on success

  \par Example
  \code
  // Set the level to just report error messages.
  Debug_SetLevel( 1 );
  \endcode
*/
int Debug_SetLevel( int level )
{
  if ( !Debug_GetActive() )
    Debug_SetActive( 1 );
  DebugData->level = level;
  return CONTROLLER_OK;
}

/**	
	Read the current debugger level.
  @return Level An integer from 0-3 specifying the level.

  \par Example
  \code
  // Check the current debug level
  int level = Debug_GetLevel( );
  switch( level )
  {
    case DEBUG_ALWAYS:
      // ...
      break;
    case DEBUG_ERROR:
      // ...
      break;
    case DEBUG_WARNING:
      // ...
      break;
    case DEBUG_MESSAGE:
      // ...
      break;
  }
  \endcode
*/
int Debug_GetLevel( )
{
  if ( !Debug_GetActive() )
    Debug_SetActive( 1 );
  return DebugData->level;
}

/**
	Sets whether the Debug subsystem outputs its OSC messages via USB.
  This is enabled by default.  
	@param state An integer specifying whether or not to send over USB - 1 (on) or 0 (off).
	@return Zero on success.

  \par Example
  \code
  // Turn debugging over USB off.
  Debug_SetUsb( 0 );
  \endcode
*/
int Debug_SetUsb( int state )
{
  if ( !Debug_GetActive() )
    Debug_SetActive( 1 );
  DebugData->usb = state;
  return CONTROLLER_OK;
}

/**
	Returns whether the Debug system is set to send its messages over USB.
  This is enabled by default.
	@return state An integer specifying whether messages will be sent over USB - 1 (enabled) or 0 (disabled).

  \par Example
  \code
  // Check whether we're sending Debug messages over USB
  int usb_debug = Debug_GetUsb( );
  \endcode
*/
int Debug_GetUsb( )
{
  if ( !Debug_GetActive() )
    Debug_SetActive( 1 );
  return DebugData->usb;
}

/**
	Sets whether the Debug system is set to send its messages over UDP.
  This is enabled by default.
	@param state An integer specifying the state - 1 (on) or 0 (off).
	@return Zero on success.

  \par Example
  \code
  // Turn debugging over UDP off.
  Debug_SetUdp( 0 );
  \endcode
*/
int Debug_SetUdp( int state )
{
  if ( !Debug_GetActive() )
    Debug_SetActive( 1 );
  DebugData->udp = state;
  return CONTROLLER_OK;
}

/**
	Sets whether the Debug subsystem outputs its OSC messages via USB.
  This is enabled by default.
	@return state An integer specifying whether messages will be sent over USB - 1 (enabled) or 0 (disabled).

  \par Example
  \code
  // Check whether we're sending Debug messages over UDP
  int udp_debug = Debug_GetUdp( );
  \endcode
*/
int Debug_GetUdp( )
{
  if ( !Debug_GetActive() )
    Debug_SetActive( 1 );

  return DebugData->udp;
}

/**
  Create a debug message.
	@param level An integer specifying the debug level.  
	@param format printf()-style formatting. %d, %s, %x, etc.
	@param ... The contents of the debug message - this can look just like a call to printf().

  \par Example
  \code
  // send a simple debug message...
  Debug( DEBUG_MESSAGE, "%s", "Hello." );

  // or a slightly more interesting message
  SomeTask( void* p )
  {
    int counter = 0;
    while( 1 )
    {
      Debug( DEBUG_MESSAGE, "%s %d", "Current count is: ", counter++ );
      Sleep( 1000 );
    }
  }
  \endcode

	@return 
*/

int Debug( int level, char* format, ... )
{
  va_list args;

  if ( !Debug_GetActive() )
    Debug_SetActive( 1 );

  if ( Osc_GetActive() && Osc_GetRunning( ) && level <= DebugData->level )
  {
    va_start( args, format );
    vsnprintf( DebugData->message, DEBUG_MAX_MESSAGE, format, args ); 
    // va_end( args );
    #ifdef MAKE_CTRL_USB
    if ( DebugData->usb && Usb_GetActive() )
    {
      Osc_CreateMessage( OSC_CHANNEL_USB, "/debug/message", ",s", DebugData->message );
      Osc_SendPacket( OSC_CHANNEL_USB );
    }
    #endif
    #ifdef MAKE_CTRL_NETWORK
    if ( DebugData->udp && Network_GetActive() )
    {
      Osc_CreateMessage( OSC_CHANNEL_UDP, "/debug/message", ",s", DebugData->message );
      Osc_SendPacket( OSC_CHANNEL_UDP );
    }
    #endif
  }
  return CONTROLLER_OK;
}


/** @}
*/

/** \defgroup DebugOSC Debug - OSC
  Debug allows sending/receiving debug messages via OSC.
  By default debug messages are sent over both USB and UDP, but this can be modified.
  \ingroup OSC
   
    \section devices Devices
    There's only one Debug system, so a device index is not used in OSC messages to it.
   
    \section properties Properties
    Debug has six properties - \b active, \b level, \b usb, and \b udp.

    \par Active
    The \b 'active' property corresponds to the active state of the Debug system.
    If Debug is set to be inactive, it will not respond to any other OSC messages. 
    If you're not seeing appropriate
    responses to your messages to Debug, check the whether it's
    active by sending a message like
    \verbatim /debug/active \endverbatim
    \par
    You can set the active flag by sending
    \verbatim /debug/active 1 \endverbatim
    
    \par Level
    The \b level property corresponds to the level of detail of the debug messages you want to receive.
    Typically in a debug system, low-level messages are given a low number and higher-level messages
    get higher numbers.  So, if you don't need to see all the low level stuff, you can simply set the debug
    level to a higher number so you're not flooded with tons of nitty gritty messages.

    Values for the levels:
    - 0 - Always send
    - 1 - Error messages
    - 2 - Warning messages
    - 3 - Normal/test messages
    
    The default debug level is \b 3.  Set the level by sending a message like:
    \verbatim /debug/level 2 \endverbatim
   
    \par Usb
    The \b usb property determines whether debug messages will be sent over USB.  This is turned on
    by default.

    To set this to off, send a message like:
    \code /debug/usb 0 \endcode
   
    \par Udp
    The \b udp property determines whether debug messages will be sent over UDP.  This is turned on
    by default.

    To set this to off, send a message like:
    \code /debug/udp 0 \endcode
*/

#include "osc.h"
#include "string.h"
#include "stdio.h"

// OSC Interface 
//const char* DebugOsc_GetName( void );
//int DebugOsc_ReceiveMessage( int channel, char* message, int length );

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
  int value = 0;
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

#endif // OSC


