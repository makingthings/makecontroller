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

#define MOTOR_COUNT 4
// static 
Motor::MotorInternal* Motor::motors[] = {0, 0, 0, 0};

/**
  Create a new motor.
  
  @param index Which motor to control - valid options are 0, 1, 2, 3
  
  \b Example
  \code
  Motor m(1);
  // or...
  Motor* m = new Motor(1);
  \endcode
*/
Motor::Motor( int index )
{
  if ( index < 0 || index >= MOTOR_COUNT )
    return;
  _index = index;
  if( !motors[_index] )
  {
    motors[_index] = new MotorInternal();
    MotorInternal* internal = motors[_index];
    internal->refcount = 1;
    internal->direction = true;
    internal->speed = 0;
    internal->pwmout = new PwmOut(_index);
    finalize( internal );
  }
  else
    motors[_index]->refcount++;  
}

Motor::~Motor()
{
  if( --(motors[_index]->refcount) <= 0 )
  {
    MotorInternal* internal = motors[_index];
    setSpeed(0);
    delete internal->pwmout;
    delete internal;
    motors[_index] = 0;
  }
}

/** 
  Set the speed of a DC motor.
  @param duty An integer (0 - 1023) specifying the speed.
  @returns True on success, false on failure.
  
  \b Example
  \code
  // Set the speed of motor 3 to %75
  Motor m(3);
  m.setSpeed(768);
  \endcode
*/
bool Motor::setSpeed( int duty )
{ 
  MotorInternal* internal = motors[_index];
  internal->speed = duty;
  finalize( internal );
  return true;
}

/** 
  Set the direction of a DC motor.
  @param forward True for forward, false for reverse
  @return True on success, false on failure
  
  \b Example
  \code
  // Set the direction of motor 2 to reverse.
  Motor* m = new Motor(2);
  m->setDirection(false);
  \endcode
*/
bool Motor::setDirection( bool forward )
{
  MotorInternal* internal = motors[_index];
  internal->direction = forward;
  finalize( internal );
  return true;
}

/** 
  Read the speed of a DC motor.
  @return the speed (0 - 1023)
  
  \b Example
  \code
  // check the current speed of motor 1
  Motor m(1);
  int motor1_speed = m.speed();
  \endcode
*/
int Motor::speed( )
{ 
  return motors[_index]->speed;
}

/** 
  Read the direction of a DC motor.
  @return True for forward, false for reverse
  
  \b Example
  \code
  Motor m(0);
  if( m.direction() )
  {
    // Motor 0 is going forward
  }
  else
  {
    // Motor 0 is going in reverse
  }
  \endcode
*/
bool Motor::direction( )
{
  return motors[_index]->direction;
}

void Motor::finalize( MotorInternal* m )
{
  // possibly add a dead zone inbetween?
  if( m->direction )
    m->pwmout->setAll( m->speed, false, true );
  else
    m->pwmout->setAll( (m->speed * -1), true, false );
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
