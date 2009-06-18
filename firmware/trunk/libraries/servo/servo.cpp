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

#include "servo.h"
#include "io.h"
#include "fasttimer.h"
#include "config.h"
#include "error.h"

#include "AT91SAM7X256.h"

// These constants govern how long the pulse preamble is 
// (SERVO_OFFSET) and how long the pulse can be (SERVO_MAX)
// So... if you want the servo pulse to be 1ms-2ms, you'd 
// set SERVO_MAX = 1000, SERVO_OFFEST = 1000
// So... if you want the servo pulse to be 0.3ms-2.3ms, you'd 
// set SERVO_MAX = 2000, SERVO_OFFEST = 300
//#define SERVO_MAX_PULSE     1000
#define SERVO_OFFSET  1000
#define SERVO_MIN_POSITION  -512
#define SERVO_MAX_POSITION  1536
#define SERVO_MID_POSITION 512
#define SERVO_CYCLE 2048
#define SERVO_SAFE_MIN 0
#define SERVO_SAFE_MAX 1023

#if ( APPBOARD_VERSION == 50 )
  #define SERVO_0_IO IO_PA02
  #define SERVO_1_IO IO_PA02
  #define SERVO_2_IO IO_PA02
  #define SERVO_3_IO IO_PA02
#endif
#if ( APPBOARD_VERSION >= 90 )
  #define SERVO_0_IO IO_PB24
  #define SERVO_1_IO IO_PA23
  #define SERVO_2_IO IO_PA21
  #define SERVO_3_IO IO_PA22
#endif

void DisableFIQFromThumb( void );
void EnableFIQFromThumb( void );
void Servo_IRQCallback( int id );

#define SERVO_COUNT 4

// static
Servo::ServoInternal* Servo::servos[] = {0, 0, 0, 0};
Servo::Base Servo::servoBase;

/**
  Create a new Servo object.
  @param index Which servo to control - valid options are 0, 1, 2, 3
  
  \b Example
  \code
  Servo srv(2);
  // or allocate from the heap...
  Servo* srv = new Servo(2);
  \endcode
*/
Servo::Servo( int index )
{
  if ( index < 0 || index >= SERVO_COUNT )
    return;
  _index = index;

  if( !servos[_index] )
  {
    if( !servoBase.activeDevices++ ) // has the system ever been fired up?
      baseInit();

    servos[_index] = new ServoInternal();
    ServoInternal* si = servos[_index];
    si->io = new Io(getIo(_index));
    si->io->on();

    si->position = (SERVO_MID_POSITION + SERVO_OFFSET) << 6;
    si->speed = 1023 << 6;
  }
  else
    servos[_index]->refcount++;
}

Servo::~Servo( )
{
  ServoInternal* si = servos[_index]; 
  if ( --si->refcount <= 0 )
  {
    delete si->io;
    delete si;
    servos[_index] = 0;
    if( --servoBase.activeDevices <= 0 ) // were we the last one?
      baseDeinit();
  }
}

/** 
  Set the position of the specified servo motor.
  
  Most servos like to be driven within a "safe range" which usually ends up being somewhere 
  around 110-120 degrees range of motion, or thereabouts.  Some servos don't mind being 
  driven all the way to their 180 degree range of motion limit.  With this in mind, the 
  range of values you can send to servos connected to the Make Controller Kit is as follows:
  - Values from 0-1023 correspond to the normal, or "safe", range of motion.  
  - You can also send values from -512 all the way up to 1536 to drive the servo through its full
  range of motion.
  
  Note that it is sometimes possible to damage your servo by driving it too far, so 
  proceed with a bit of caution when using the extended range until you know your servos 
  can handle it.

  @param position An integer specifying the servo position (0 - 1023).
  @return True on success, false on failure.
  
  \b Example
  \code
  // set servo 1 to midway through the "safe" range
  Servo srv(1);
  srv.setPosition(512);
  \endcode
*/
bool Servo::setPosition( int position )
{
  if ( position < SERVO_MIN_POSITION )
    position = SERVO_SAFE_MIN;
  if ( position > SERVO_MAX_POSITION )
    position = SERVO_SAFE_MAX;

  position += SERVO_OFFSET;

  DisableFIQFromThumb();
  servos[_index]->destination = position << 6;
  EnableFIQFromThumb();

  return true;
}

/** 
  Set a servo's speed.
  
  After setting the speed, calls to setPosition() will react at the specified speed.
  
  Higher values will result in a more immediate response, while lower values will more 
  slowly step through all the intermediate values, achieving a smoother motion.
  
  @param speed An integer specifying the servo speed (0 - 1023).
  @return True on success, false on failure.
  
  \b Example
  \code
  // set servo 1 half speed
  Servo* srv = new Servo(1);
  srv->setSpeed(512);
  \endcode
*/
bool Servo::setSpeed( int speed )
{
  if ( speed < 1 )
    speed = 1;
  if( speed > 1023 )
    speed = 1023;
  DisableFIQFromThumb();
  servos[_index]->speed = speed << 6;
  EnableFIQFromThumb();
  return true;
}

/** 
  Read the current position of a servo motor.
  
  You can call this while a servo is moving towards its final
  destination to see where it is along the way.
  
  @return The position (0 - 1023), or 0 on error.
  
  \b Example
  \code
  Servo srv(3);
  int pos = srv.position(); // should be 0
  srv.step(100);
  pos = srv.position(); // should be 100
  \endcode
*/
int Servo::position( )
{
  return (servos[_index]->position >> 6) - SERVO_OFFSET;
}

/** 
  Read a servo's speed.
  This is 1023 by default.
  @return The speed (0 - 1023), or 0 on error.
  
  \b Example
  \code
  Servo srv(2);
  int speed = srv.speed(); // should be 1023
  \endcode
*/
int Servo::speed( )
{
  return servos[_index]->speed >> 6;
}

int Servo::getIo( int index )
{
  switch( index )
  {
    case 0: return SERVO_0_IO;
    case 1: return SERVO_1_IO;
    case 2: return SERVO_2_IO;
    case 3: return SERVO_3_IO;
    default: return -1;
  }
}

/*
  Fire up base resources for all servos.
*/
void Servo::baseInit()
{
  // Global init stuff
  servoBase.state = 0;
  servoBase.gap = 1882; // forces 64'ish cycles a second (1894 + 2012 = 3906, 3906 * 4 = 15624, 15624 * 64 = 999936)
  servoBase.index = 0;
  servoBase.fastTimer.setHandler( Servo_IRQCallback, 0 );
  servoBase.fastTimer.start( 2000 );
}

/*
  All the servo instances in the system have been deleted.
  Clean up.
*/
void Servo::baseDeinit()
{
  servoBase.fastTimer.stop();
}

void Servo_IRQCallback( int id )
{
  (void)id;

  Servo::Base* sb = &Servo::servoBase;
  if( sb->state )
  {
    if ( ++sb->index >= SERVO_COUNT || sb->index < 0 )
      sb->index = 0;
    Servo::ServoInternal* s = Servo::servos[ sb->index ];
    if( s )
    {
      if ( s->position != s->destination )
      {
        int diff = s->destination - s->position;
        if ( diff < 0 )
        {
          s->position -= s->speed;
          if ( s->position < s->destination )
            s->position = s->destination;
        }
        else
        {
          s->position += s->speed;
          if ( s->position > s->destination )
            s->position = s->destination;
        }
      }
    
      int period = s->position >> 6;
      if ( period >= (SERVO_MIN_POSITION + SERVO_OFFSET) && period <= (SERVO_MAX_POSITION + SERVO_OFFSET) )
        s->io->off();
      else
        period = SERVO_MAX_POSITION;
      sb->fastTimer.setPeriod( period );
    }
  }
  else
  {
    Servo::ServoInternal* s = Servo::servos[ sb->index ];
    if(s)
    {
      int period = s->position >> 6;
      s->io->on();
      sb->fastTimer.setPeriod( sb->gap + ( SERVO_CYCLE - period ) );
    }
  }
  sb->state = !sb->state;
}

#ifdef OSC // defined in config.h

/** \defgroup ServoOSC Servo - OSC
  Control Servo motors with the Application Board via OSC.
  \ingroup OSC
  
  \section devices Devices
  There are 4 Servo controllers available on the Application Board, numbered 0 - 3.\n
  See the servo section in the Application Board user's guide for more information
  on hooking up servos to the board.
  
  \section properties Properties
  Each servo controller has three properties:
  - position
  - speed
  - active

  \par Position
  The \b position property corresponds to the position of the servo motor within its range of motion.
  This value can be both read and written.  
  \par 
  Most servos like to be driven within a "safe range" which usually ends up being somewhere around 110-120
  degrees range of motion, or thereabouts.  Some servos don't mind being driven all the way to their 180
  degree range of motion limit.  With this in mind, the range of values you can send to servos connected
  to the Make Controller Kit is as follows:
  - Values from 0-1023 correspond to the normal, or "safe", range of motion.  
  - You can also send values from -512 to 1536 to drive the servo through its full
  range of motion.
  \par
  Note that it is sometimes possible to damage your servo by driving it too far, so proceed with a bit of
  caution when using the extended range until you know your servos can handle it.
  \par
  To set the first servo to one quarter its position, send the message
  \verbatim /servo/0/position 256 \endverbatim
  Leave the argument value off to read the position of the servo:
  \verbatim /servo/0/position \endverbatim
  
  \par Speed
  The \b speed property corresponds to the speed with which the servo responds to changes 
  of position.
  This value can be both read and written, and the range of values is 0 - 1023.  A speed of 1023
  means the servo responds immediately, and lower values will result in slower, and smoother, responses.  
  \par
  To set the speed of the first servo to just under full speed, send a message like
  \verbatim /servo/0/speed 975 \endverbatim
  Adjust the argument value to one that suits your application.\n
  Leave the argument value off to read the position of the servo:
  \verbatim /servo/0/speed \endverbatim
  
  \par Active
  The \b active property corresponds to the active state of the servo.
  If the servo is set to be active, no other tasks will be able to
  write to the same I/O lines.  If you're not seeing appropriate
  responses to your messages to a servo, check the whether it's 
  locked by sending a message like
  \verbatim /servo/3/active \endverbatim
*/

//#include "osc.h"
//#include "string.h"
//#include "stdio.h"
//
//// Need a list of property names
//// MUST end in zero
//static char* ServoOsc_Name = "servo";
//static char* ServoOsc_PropertyNames[] = { "active", "position", "speed", 0 }; // must have a trailing 0
//
//int ServoOsc_PropertySet( int index, int property, int value );
//int ServoOsc_PropertyGet( int index, int property );
//
//// Returns the name of the subsystem
//const char* ServoOsc_GetName( )
//{
//  return ServoOsc_Name;
//}
//
//// Now getting a message.  This is actually a part message, with the first
//// part (the subsystem) already parsed off.
//int ServoOsc_ReceiveMessage( int channel, char* message, int length )
//{
//  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
//                                           SERVO_COUNT, ServoOsc_Name,
//                                           ServoOsc_PropertySet, ServoOsc_PropertyGet, 
//                                           ServoOsc_PropertyNames );
//
//  if ( status != CONTROLLER_OK )
//    return Osc_SendError( channel, ServoOsc_Name, status );
//  return CONTROLLER_OK;
//}
//
//// Set the index LED, property with the value
//int ServoOsc_PropertySet( int index, int property, int value )
//{
//  switch ( property )
//  {
//    case 0:
//      Servo_SetActive( index, value );
//      break;
//    case 1:
//      Servo_SetPosition( index, value );
//      break;
//    case 2:
//      Servo_SetSpeed( index, value );
//      break;
//  }
//  return CONTROLLER_OK;
//}
//
//// Get the index LED, property
//int ServoOsc_PropertyGet( int index, int property )
//{
//  int value = 0;
//  switch ( property )
//  {
//    case 0:
//      value = Servo_GetActive( index );
//      break;
//    case 1:
//      value = Servo_GetPosition( index );
//      break;
//    case 2:
//      value = Servo_GetSpeed( index );
//      break;
//  }
//  
//  return value;
//}

#endif
