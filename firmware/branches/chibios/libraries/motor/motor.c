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

#include "motor.h"
#include "core.h"
#include "pwmout.h"

#define MOTOR_COUNT 4

struct Motor {
  int direction;
  int speed;
};

static struct Motor motors[MOTOR_COUNT];
static void motorFinalize( int motor );

/**
  \defgroup dcmotor DC Motor
  Forward/reverse and speed control for up to 4 DC motors.

  \b Note - this library is intended for use with the Make Application Board.

  \section Usage
  First enable the motor you want to use with motorEnable() and then use the other
  routines to control it.  Note that other output devices cannot be used simultaneously
  since they use the same output signals.  For example, a \ref digitalout cannot be used
  without first setting overlapping the motor I/O lines to inactive.

  Note - the symbols \b FORWARD and \b REVERSE are defined to \b true and \b false to make
  things a little clearer.

  \code
  motorEnable(3); // enable the motor on channel 3
  motorSetDirection(3, REVERSE); // set the motor to go backwards
  motorSetSpeed(3, 1023); // full steam ahead
  \endcode

  \section Setup
  Each motor controller is composed of 2 adjacent Digital Outs on the Make Application Board:
  - motor 0 - Digital Outs 0 and 1.
  - motor 1 - Digital Outs 2 and 3.
  - motor 2 - Digital Outs 4 and 5.
  - motor 3 - Digital Outs 6 and 7.

  See the digital out section of the
  <a href="http://www.makingthings.com/documentation/tutorial/application-board-overview/digital-outputs">
  Application Board overview</a> for more details.
  \ingroup io
  @{
*/

/**
  Enable a motor.
  @param motor Which motor - valid options are 0-3.
  
  \b Example
  \code
  motorEnable(1);
  \endcode
*/
void motorEnable(int motor)
{
  struct Motor* m = &(motors[motor]);
  m->direction = true;
  m->speed = 0;
  motorFinalize(motor);
}

void motorDisable(int channel)
{
  motorSetSpeed(channel, 0);
}

/** 
  Set the speed of a DC motor.
  @param motor Which motor - valid options are 0-3.
  @param duty An integer (0 - 1023) specifying the speed.
  @returns True on success, false on failure.
  
  \b Example
  \code
  // Set the speed of motor 3 to %75
  motorSetSpeed(3, 768);
  \endcode
*/
bool motorSetSpeed(int motor, int duty)
{ 
  motors[motor].speed = duty;
  motorFinalize(motor);
  return true;
}

/** 
  Set the direction of a DC motor.
  @param motor Which motor - valid options are 0-3.
  @param forward True for forward, false for reverse
  @return True on success, false on failure
  
  \b Example
  \code
  // Set the direction of motor 2 to reverse.
  motorSetDirection(2, REVERSE);
  \endcode
*/
bool motorSetDirection(int motor, bool forward)
{
  motors[motor].direction = forward;
  motorFinalize(motor);
  return true;
}

/** 
  Read the speed of a DC motor.
  @param motor Which motor - valid options are 0-3.
  @return the speed (0 - 1023)
  
  \b Example
  \code
  // check the current speed of motor 1
  int speed = motorSpeed(1);
  \endcode
*/
int motorSpeed(int motor)
{ 
  return motors[motor].speed;
}

/** 
  Read the direction of a DC motor.
  @param motor Which motor - valid options are 0-3.
  @return True for forward, false for reverse
  
  \b Example
  \code
  if (motorDirection(0) == FORWARD) {
    // Motor 0 is going forward
  }
  else {
    // Motor 0 is going in reverse
  }
  \endcode
*/
bool motorDirection(int motor)
{
  return motors[motor].direction;
}

/** @} */

void motorFinalize( int motor )
{
  struct Motor* m = &(motors[motor]);
  // possibly add a dead zone in between?
  if( m->direction )
    pwmoutSetAll( motor, m->speed, false, true );
  else
    pwmoutSetAll( motor, (m->speed * -1), true, false );
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

//#include "osc.h"
//#include "string.h"
//#include "stdio.h"
//
//// Need a list of property names
//// MUST end in zero
//static char* MotorOsc_Name = "motor";
//static char* MotorOsc_PropertyNames[] = { "active", "speed", "direction", 0 }; // must have a trailing 0
//
//int MotorOsc_PropertySet( int index, int property, int value );
//int MotorOsc_PropertyGet( int index, int property );
//
//// Returns the name of the subsystem
//const char* MotorOsc_GetName( )
//{
//  return MotorOsc_Name;
//}
//
//// Now getting a message.  This is actually a part message, with the first
//// part (the subsystem) already parsed off.
//int MotorOsc_ReceiveMessage( int channel, char* message, int length )
//{
//  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
//                                     MOTOR_COUNT, MotorOsc_Name,
//                                     MotorOsc_PropertySet, MotorOsc_PropertyGet, 
//                                     MotorOsc_PropertyNames );
//                                     
//  if ( status != CONTROLLER_OK )
//    return Osc_SendError( channel, MotorOsc_Name, status );
//  return CONTROLLER_OK;
//}
//
//// Set the index LED, property with the value
//int MotorOsc_PropertySet( int index, int property, int value )
//{
//  switch ( property )
//  {
//    case 0: 
//      Motor_SetActive( index, value );
//      break;      
//    case 1:
//      Motor_SetSpeed( index, value );
//      break;
//    case 2:
//      Motor_SetDirection( index, value );
//      break;
//  }
//  return CONTROLLER_OK;
//}
//
//// Get the index LED, property
//int MotorOsc_PropertyGet( int index, int property )
//{
//  int value = 0;
//  switch ( property )
//  {
//    case 0:
//      value = Motor_GetActive( index );
//      break;
//    case 1:
//      value = Motor_GetSpeed( index );
//      break;
//    case 2:
//      value = Motor_GetDirection( index );
//      break;
//  }
//  
//  return value;
//}

#endif
