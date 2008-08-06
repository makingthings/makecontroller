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

/** \file motor.c	
	DC_Motor.
	Functions for controlling DC Motors with the MAKE Application Board.
*/

#include "motor.h"
#include "pwmout.h"
#include "config.h"

#define MOTOR_COUNT 4

static int Motor_Start( int motor );
static int Motor_Stop( int motor );

struct Motor_
{
  int users;
  int direction;
  int speed;
} Motor[ MOTOR_COUNT ];

static int Motor_SetFinal( int index, struct Motor_ * mp );

/** \defgroup Motor Motor
The Motor subsystem provides forward/reverse and speed control for up to 4 DC motors across the 8 high current outputs.
Each motor controller is composed of 2 adjacent Digital Outs on the Make Application Board:
- motor 0 - Digital Outs 0 and 1.
- motor 1 - Digital Outs 2 and 3.
- motor 2 - Digital Outs 4 and 5.
- motor 3 - Digital Outs 6 and 7.

Other output devices cannot be used simultaneously - for example, the DigitalOuts cannot be called without
first setting overlapping the DC motor I/O lines to inactive.

See the digital out section of the 
<a href="http://www.makingthings.com/documentation/tutorial/application-board-overview/digital-outputs">
Application Board overview</a> for more details.
\ingroup Libraries
@{
*/

/**
	Sets whether the specified motor is active.
	@param index An integer specifying which Motor (0-3).
	@param state An integer specifying the availability of the motor's I/O lines - 1 (active) or 0 (inactive).
	@return Zero on success.
	
	\b Example
	\code
	// Enable motor 0
	Motor_SetActive(0, 1);
	\endcode
*/
int Motor_SetActive( int index, int state )
{
  if ( index < 0 || index >= MOTOR_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( state )
    return Motor_Start( index );
  else
    return Motor_Stop( index );
}

/**
	Returns the active state of the DC Motor I/O lines.
	@param index An integer specifying which DC Motor (0-3).
	@return The availability of the motor's I/O lines - 1 (active) or 0 (inactive).
	
	\b Example
	\code
	if( Motor_GetActive(0) )
	{
	  // Motor 0 is active
	}
	else
	{
	  // Motor 0 is inactive
	}
	\endcode
*/
int Motor_GetActive( int index )
{
  if ( index < 0 || index >= MOTOR_COUNT )
    return false;
  return Motor[ index ].users > 0;
}

/**	
	Set the speed of a DC motor.
	@param index An integer specifying which DC Motor (0-3).
	@param duty An integer (0 - 1023) specifying the speed.
  @returns Zero on success.
  
  \b Example
	\code
	// Set the speed of motor 3 to %75
	Motor_SetSpeed(3, 768);
	\endcode
*/
int Motor_SetSpeed( int index, int duty )
{
  if ( index < 0 || index >= MOTOR_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  struct Motor_* mp = &Motor[ index ];

  if ( mp->users == 0 )
  {
    int status = Motor_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }
  
  mp->speed = duty;
  
  return Motor_SetFinal( index, mp );
}

/**	
	Set the direction of a DC motor.
	@param index An integer specifying which DC Motor (0-3).
	@param forward A character specifying direction - 1/non-zero (forward) or 0 (reverse).
  @return Zero on success.
  
  \b Example
	\code
	// Set the direction of motor 2 to reverse.
	Motor_SetDirection(2, 0);
	\endcode
*/
int Motor_SetDirection( int index, char forward )
{
  if ( index < 0 || index >= MOTOR_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  struct Motor_* mp = &Motor[ index ];

  if ( mp->users == 0 )
  {
    int status = Motor_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  mp->direction = forward;

  return Motor_SetFinal( index, mp );
}

/**	
	Read the speed of a DC motor.
	@param index An integer specifying which DC Motor (0-3).
  @return the speed (0 - 1023)
  
  \b Example
	\code
	// check the current speed of motor 1
	int motor1_speed = Motor_GetSpeed(1);
	\endcode
*/
int Motor_GetSpeed( int index )
{
  if ( index < 0 || index >= MOTOR_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  struct Motor_* mp = &Motor[ index ];

  if ( mp->users == 0 )
  {
    int status = Motor_Start( index );
    if ( status != CONTROLLER_OK )
      return 0;
  }
  
  return mp->speed;
}

/**	
	Read the direction of a DC motor.
	@param index An integer specifying which DC Motor (0-3).
  @return Direction - non-zero (forward) or 0 (reverse).
  
  \b Example
	\code
	if( Motor_GetDirection(0) )
	{
	  // Motor 0 is going forward
	}
	else
	{
	  // Motor 0 is going in reverse
	}
	\endcode
*/
int Motor_GetDirection( int index )
{
  if ( index < 0 || index >= MOTOR_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  struct Motor_* mp = &Motor[ index ];

  if ( mp->users == 0 )
  {
    int status = Motor_Start( index );
    if ( status != CONTROLLER_OK )
      return 0;
  }

  return mp->direction;
}

/** @}
*/

int Motor_Start( int index )
{
  if ( index < 0 || index >= MOTOR_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  struct Motor_* mp = &Motor[ index ];
  if ( mp->users++ == 0 )
  {
    mp->direction = 1;
    mp->speed = 0; 

    int status = PwmOut_SetActive( index, true );
    if ( status != CONTROLLER_OK )
    {
      mp->users--;
      return status;
    }
  }

  return Motor_SetFinal( index, mp );
}

int Motor_Stop( int index )
{
  if ( index < 0 || index >= MOTOR_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  struct Motor_* mp = &Motor[ index ];
  if ( --mp->users == 0 )
  {  
    return PwmOut_SetActive( index, false );
  }

  return CONTROLLER_OK;
}


static int Motor_SetFinal( int index, struct Motor_ * mp )
{
  int speed = mp->speed;
  // Work out the speed
  if ( !mp->direction )
    speed *= -1;

  // possibly add a dead zone inbetween?
  int status;
  if ( speed >= 0 )
    status = PwmOut_SetAll( index, speed, 0, 1 );
  else
    status = PwmOut_SetAll( index, -speed, 1, 0 );
  return status;
}

#ifdef OSC

/** \defgroup MotorOSC Motor - OSC
  Control DC motors with the Application Board via OSC.
  \ingroup OSC
	
	\section devices Devices
	There are 4 DC Motor controllers available on the Application Board, numbered 0 - 3.
	
	Each motor controller is composed of 2 adjacent Digital Outs on the Make Application Board:
  - motor 0 - Digital Outs 0 and 1.
  - motor 1 - Digital Outs 2 and 3.
  - motor 2 - Digital Outs 4 and 5.
  - motor 3 - Digital Outs 6 and 7.
	
	\section properties Properties
	Each motor controller has three properties:
  - speed
  - direction
  - active

	\par Speed
	The \b speed property corresponds to the speed at which a motor is being driven.
	This value can be both read and written.  The range of values expected by the board
	is from 0 - 1023.  A speed of 0 means the motor is stopped, and 1023 means it is
	being driven at full speed.
	\par
	To set the second motor (connected to Digital Outs 2 and 3) at half speed, send the message
	\verbatim /motor/1/speed 512 \endverbatim
	Leave the argument value off to read the speed of the motor:
	\verbatim /motor/1/speed \endverbatim
	
	\par Direction
	The \b direction property corresponds to the forward/reverse direction of the motor.
	This value can be both read and written, and the range of values expected is simply 
	0 or 1.  1 means forward and 0 means reverse.
	\par
	For example, to set the first motor to move in reverse, send the message
	\verbatim /motor/0/direction 0 \endverbatim
	Simply change the argument of 0 to a 1 in order to set the motor's direction to forward.
	
	\par Active
	The \b active property corresponds to the active state of the motor.
	If the motor is set to be active, no other tasks will be able to
	use its 2 digital out lines.  If you're not seeing appropriate
	responses to your messages to the motor, check the whether it's 
	locked by sending a message like
	\verbatim /motor/0/active \endverbatim
	\par
	If you're no longer using the motor, you can free the 2 Digital Outs by 
	sending the message
	\verbatim /motor/0/active 0 \endverbatim
*/

#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* MotorOsc_Name = "motor";
static char* MotorOsc_PropertyNames[] = { "active", "speed", "direction", 0 }; // must have a trailing 0

int MotorOsc_PropertySet( int index, int property, int value );
int MotorOsc_PropertyGet( int index, int property );

// Returns the name of the subsystem
const char* MotorOsc_GetName( )
{
  return MotorOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int MotorOsc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
                                     MOTOR_COUNT, MotorOsc_Name,
                                     MotorOsc_PropertySet, MotorOsc_PropertyGet, 
                                     MotorOsc_PropertyNames );
                                     
  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, MotorOsc_Name, status );
  return CONTROLLER_OK;
}

// Set the index LED, property with the value
int MotorOsc_PropertySet( int index, int property, int value )
{
  switch ( property )
  {
    case 0: 
      Motor_SetActive( index, value );
      break;      
    case 1:
      Motor_SetSpeed( index, value );
      break;
    case 2:
      Motor_SetDirection( index, value );
      break;
  }
  return CONTROLLER_OK;
}

// Get the index LED, property
int MotorOsc_PropertyGet( int index, int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      value = Motor_GetActive( index );
      break;
    case 1:
      value = Motor_GetSpeed( index );
      break;
    case 2:
      value = Motor_GetDirection( index );
      break;
  }
  
  return value;
}

#endif
