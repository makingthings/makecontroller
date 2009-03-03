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

/** \file led.c	
	Controller LED.
	Functions for controlling the status LED on the Make Controller Board.
*/

#include "io.h"
#include "config.h"
#include "led.h"

static int Led_users;

#if ( CONTROLLER_VERSION == 50 )
  #define LED_IO IO_PB25
#elif ( CONTROLLER_VERSION == 90 )
  #define LED_IO IO_PB12
#elif ( CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 || CONTROLLER_VERSION == 200 )
  #define LED_IO IO_PA12
#endif

static int Led_Start( void );
static int Led_Stop( void );

/** \defgroup Led LED
	Controls the single green LED on the MAKE Controller Board.
	There are two LEDs on the MAKE Controller Board - one green and one red.  The red LED is simply
	a power indicator and cannot be controlled by the Controller.  The green LED can be used for
	program feedback.  In many MakingThings applications, for example, it is set to blink once a
	second, showing the board's "heartbeat" and letting the user know that the board is running.
* \ingroup Core
* @{
*/

/**
	Sets whether the Led subsystem is active.
	@param state An integer specifying the active state of the LED system - 1 (on) or 0 (off).
	@return Zero on success.
	
	\b Example
	\code
	// enable the LED system
	Led_SetActive(1);
	\endcode
*/
int Led_SetActive( int state )
{
  if ( state )
    return Led_Start( );
  else
    return Led_Stop( );
}

/**
	Read the active state of the LED system.
	@return State - 1/non-zero (on) or 0 (off).
	
	\b Example
	\code
	int enabled = Led_GetActive();
	if(enabled)
	{
	  // then we're enabled
	}
	else
	{
	  // not enabled
	}
	\endcode
*/
int Led_GetActive( )
{
  return Led_users > 0;
}

/**	
	Control the LED on the MAKE Controller Board.
	@param state An integer specifying the state - on (1) or off (0).
  @return 0 on success.
  
  \b Example
	\code
	// turn the LED on
	Led_SetState(1);
	\endcode
*/
int Led_SetState( int state )
{
  if ( Led_users == 0 )
  {
    int status = Led_Start();
    if ( status != CONTROLLER_OK )
      return status;
  }

  return Io_SetValue( LED_IO, !state );
}

/**	
	Read the state of the LED on the MAKE Controller Board.
  @return State - 1/non-zero (on) or 0 (off).
  
  \b Example
	\code
	int led_on = Led_GetState();
	if(led_on)
	{
	  // the LED is on
	}
	else
	{
	  // the LED is off
	}
	\endcode
*/
int Led_GetState( )
{
  if ( Led_users == 0 )
  {
    int status = Led_Start();
    if ( status != CONTROLLER_OK )
      return status;
  }

  return Io_GetValue( LED_IO ) ? 0 : 1;
}

/** @}
*/

int Led_Start()
{
  if ( Led_users == 0 )
  {
    int status;
    status = Io_Start( LED_IO, true );
    if ( status != CONTROLLER_OK )
      return status;
    Io_SetValue( LED_IO, false );
    Io_SetPio( LED_IO, true );
    Io_SetDirection( LED_IO, IO_OUTPUT );
  }
  Led_users++;
 
  return CONTROLLER_OK;
}

int Led_Stop()
{
  if ( Led_users == 1 )
  {
    Io_Stop( LED_IO );
    Io_SetValue( LED_IO, false );
    Io_SetPio( LED_IO, true );
    Io_SetDirection( LED_IO, IO_OUTPUT );
  }
  Led_users--;

  return CONTROLLER_OK;
}

#ifdef OSC

#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* LedOsc_Name = "led";
static char* LedOsc_PropertyNames[] = { "active", "state", 0 }; // must have a trailing 0

int LedOsc_PropertySet( int property, int value );
int LedOsc_PropertyGet( int property );

// Returns the name of the subsystem
const char* LedOsc_GetName( )
{
  return LedOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int LedOsc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_IntReceiverHelper( channel, message, length, 
                                LedOsc_Name,
                                LedOsc_PropertySet, LedOsc_PropertyGet, 
                                LedOsc_PropertyNames );

  if ( status != CONTROLLER_OK )
    Osc_SendError( channel, LedOsc_Name, status );
  return CONTROLLER_OK;
}

// Set the index LED, property with the value
int LedOsc_PropertySet( int property, int value )
{
  switch ( property )
  {
    case 0: 
      Led_SetActive( value );
      break;      
    case 1:
      Led_SetState( value );
      break;
  }
  return CONTROLLER_OK;
}

// Get the index LED, property
int LedOsc_PropertyGet( int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      value = Led_GetActive( );
      break;
    case 1:
      value = Led_GetState( );
      break;
  }
  
  return value;
}

#endif
