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

// MakingThings - Make Controller Board - 2006

/** \file servo.c	
	Servo Motor Controls.
	Methods for controlling up to 4 servo motors with the Make Application Board.
*/

#include "servo.h"
#include "io.h"
#include "fasttimer.h"
#include "config.h"

#include "AT91SAM7X256.h"

#if ( APPBOARD_VERSION == 50 )
  #define SERVO_0_IO IO_PA02
  #define SERVO_1_IO IO_PA02
  #define SERVO_2_IO IO_PA02
  #define SERVO_3_IO IO_PA02
#endif
#if ( APPBOARD_VERSION == 90 || APPBOARD_VERSION == 95 || APPBOARD_VERSION == 100 )
  #define SERVO_0_IO IO_PB24
  #define SERVO_1_IO IO_PA23
  #define SERVO_2_IO IO_PA21
  #define SERVO_3_IO IO_PA22
#endif

void DisableFIQFromThumb( void );
void EnableFIQFromThumb( void );


void Servo_IRQCallback( int id );

static int Servo_Start( int index );
static int Servo_Stop( int index );
static int Servo_Init( void );
static int Servo_Deinit( void );
static int Servo_GetIo( int index );

#define SERVO_COUNT 4

typedef struct ServoControlS
{
  int users;
  int speed;
  int positionRequested;
  int position;
  int pin;
  AT91S_PIO* pIoBase;
} ServoControl;

typedef struct ServoS
{
  int users;
  int gap;
  int index;
  int state;
  FastTimerEntry fastTimerEntry;
  ServoControl control[ SERVO_COUNT ];
} Servo_;


Servo_ Servo;

/** \defgroup Servo
* The Servo Motor subsystem controls speed and position control for up to 4 standard servo motors.
* Up to 4 standard servo motors can be controlled with the MAKE Application Board.\n\n
* Standard servos have a range of motion of approximately 180 degrees, although this varies from motor to motor.
* Be sure to plug in the connector with the correct orientation with regard to the GND/5V signals on the board.\n\n
* Set the position of each motor by specifying a value from 0-1023, which will be mapped to the full range of motion
* of the motor.  You can also specify the speed with which the motors will respond to new position commands - a high
* value will result in an immediate response, while a lower value can offer some smoothing when appropriate.
* \ingroup AppBoard
* @{
*/

/**
	Lock or unlock the I/O lines used by the servos.  
	Sets whether the specified Servo I/O is active.
	@param index An integer specifying which servo (0 - 3).
	@param state An integer specifying the active state - 1 (active) or 0 (inactive).
	@return Zero on success.
*/
int Servo_SetActive( int index, int state )
{
  if ( index < 0 || index >= SERVO_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( state )
    return Servo_Start( index );
  else
    return Servo_Stop( index );
}

/**
	Check whether the I/O lines used by the servos are locked.
	@param index An integer specifying which servo (0-3).
	@return State - 1 (active) or 0 (inactive).
*/
int Servo_GetActive( int index )
{
  if ( index < 0 || index >= SERVO_COUNT )
    return false;
  return Servo.control[ index ].users > 0;
}


/**	
	Set the position of the specified servo motor.
	Most servos move within a 180 degree range.  The 0 - 1023 range of the position parameter represents
	the full range of motion of the motor.
	@param index An integer specifying which servo (0 - 3).
	@param position An integer specifying the servo position (0 - 1023).
  @return status (0 = OK).
*/
int Servo_SetPosition( int index, int position )
{
  if ( index < 0 || index >= SERVO_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Servo.control[ index ].users < 1 )
  {
    int status = Servo_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  if ( position < 0 )
    position = 0;
  if ( position > 1023 )
    position = 1023;

  DisableFIQFromThumb();
  Servo.control[ index ].positionRequested = position << 6;
  EnableFIQFromThumb();

  return CONTROLLER_OK;
}

/**	
	Set the speed at which a servo will move in response to a call to Servo_SetPosition().
	Higher values will result in a more immediate response, while lower values will more slowly step through
	all the intermediate values, achieving a smoother motion.  
	@param index An integer specifying which servo (0 - 3).
	@param speed An integer specifying the servo speed (0 - 1023).
  @return status (0 = OK).
*/
int Servo_SetSpeed( int index, int speed )
{
  if ( index < 0 || index >= SERVO_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Servo.control[ index ].users < 1 )
  {
    int status = Servo_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  if ( speed < -1 )
    speed = -1;
  DisableFIQFromThumb();
  Servo.control[ index ].speed = speed;
  EnableFIQFromThumb();

  return CONTROLLER_OK;
}

/**	
	Read the current position of a servo motor.
	@param index An integer specifying which servo (0 - 3).
  @return The position (0 - 1023), or 0 on error.
*/
int Servo_GetPosition( int index )
{
  if ( index < 0 || index >= SERVO_COUNT )
    return 0;

  if ( Servo.control[ index ].users < 1 )
  {
    int status = Servo_Start( index );
    if ( status != CONTROLLER_OK )
      return 0;
  }

  return Servo.control[ index ].position >> 6;
}

/**	
	Get the speed at which a servo will move in response to a call to Servo_SetPosition().
	Read the value previously set for the speed parameter.
	@param index An integer specifying which servo (0 - 3).
  @return The speed (0 - 1023), or 0 on error.
*/
int Servo_GetSpeed( int index )
{
  if ( index < 0 || index >= SERVO_COUNT )
    return 0;

  if ( Servo.control[ index ].users < 1 )
  {
    int status = Servo_Start( index );
    if ( status != CONTROLLER_OK )
      return 0;
  }

  return Servo.control[ index ].speed;
}

/** @}
*/

int Servo_Start( int index )
{
  int status;

  if ( index < 0 || index >= SERVO_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  ServoControl* sc = &Servo.control[ index ]; 
  if ( sc->users++ == 0 )
  {
    if ( Servo.users++ == 0 )
    {
      int status = Servo_Init();
      if ( status != CONTROLLER_OK )
      {
        // I guess not then
        sc->users--;
        Servo.users--;
        return status;
      }   
    }

    int io = Servo_GetIo( index );

    // Try to lock the servo output
    status = Io_Start( io, true );
    if ( status != CONTROLLER_OK )
    {
      // Damn
      Servo.users--;
      sc->users--;
      return status;
    }

    Io_PioEnable( io );
    Io_SetTrue( io );
    Io_SetOutput( io );

    ServoControl* s = &Servo.control[ index ]; 
    s->position = 512 << 6;
    s->speed = -1;
  }

  return CONTROLLER_OK;
}

int Servo_Stop( int index )
{
  if ( index < 0 || index >= SERVO_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  ServoControl* sc = &Servo.control[ index ]; 

  if ( sc->users <= 0 )
    return CONTROLLER_ERROR_TOO_MANY_STOPS;

  if ( --sc->users == 0 )
  {
    int io = Servo_GetIo( index );
    Io_SetInput( io );
    Io_Stop( io );

    if ( --Servo.users == 0 )
    {
      Servo_Deinit();
    }
  }

  return CONTROLLER_OK;
}


int Servo_GetIo( int index )
{
  int io = -1;
  switch ( index )
  {
    case 0:
      io = SERVO_0_IO;
      break;
    case 1:
      io = SERVO_1_IO;
      break;
    case 2:
      io = SERVO_2_IO;
      break;
    case 3:
      io = SERVO_3_IO;
      break;
  }
  return io;
}

int Servo_Init()
{
  ServoControl* s = &Servo.control[ 0 ]; 
  s->pIoBase = AT91C_BASE_PIOB;
  s->pin = AT91C_PIO_PB24;
  s++;
  s->pIoBase = AT91C_BASE_PIOA;
  s->pin = AT91C_PIO_PA23;
  s++;
  s->pIoBase = AT91C_BASE_PIOA;
  s->pin = AT91C_PIO_PA21;
  s++;
  s->pIoBase = AT91C_BASE_PIOA;
  s->pin = AT91C_PIO_PA22;
  s++;

  // Global init stuff
  Servo.state = 0;
  Servo.gap = 1882; // forces 64'ish cycles a second (1894 + 2012 = 3906, 3906 * 4 = 15624, 15624 * 64 = 999936)
  Servo.index = 0;

  FastTimer_InitializeEntry( &Servo.fastTimerEntry, Servo_IRQCallback, 0, 2000, true );
  FastTimer_Set( &Servo.fastTimerEntry );

  return CONTROLLER_OK;
}

int Servo_Deinit()
{
  // Disable the device
  // AT91C_BASE_TC1->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
  return CONTROLLER_OK;
}

void Servo_IRQCallback( int id )
{
  int period;

  switch ( Servo.state )
  {
    case 0:
    {
      if ( ++Servo.index >= SERVO_COUNT || Servo.index < 0 )
        Servo.index = 0;
      ServoControl* s = &Servo.control[ Servo.index ];
      
      if ( s->position != s->positionRequested )
      {
        if ( s->speed == -1 )
          s->position = s->positionRequested;
        else
        {
          int diff = s->positionRequested - s->position;
          if ( diff < 0 )
          {
            s->position -= s->speed;
            if ( s->position < s->positionRequested )
              s->position = s->positionRequested;
          }
          else
          {
            s->position += s->speed;
            if ( s->position > s->positionRequested )
              s->position = s->positionRequested;
          }
        }
      }

      period = s->position >> 6;
      if ( period >= 0 && period <= 1023 )
      {
        s->pIoBase->PIO_CODR = s->pin;
      }
      else
        period = 1023;
      FastTimer_SetTime( &Servo.fastTimerEntry, period + 988 );
      Servo.state = 1;
      break;
    }
    case 1:
    {
      ServoControl* s = &Servo.control[ Servo.index ];
      period = s->position >> 6;
      s->pIoBase->PIO_SODR = s->pin;
      FastTimer_SetTime( &Servo.fastTimerEntry, Servo.gap + ( 1023 - period ) );
      Servo.state = 0;
      break;
    }
  }
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
	Each servo controller has three properties - 'position', 'speed', and 'active'.

	\par Position
	The 'position' property corresponds to the position of the servo motor within its range of motion.
	This value can be both read and written.  The relevant range of values is from 0 - 1023.  
	A position of 0 sets the servo to one extreme of its range of motion and 1023 to its opposite extreme.
	\par
	To set the first servo to one quarter its position, send the message
	\verbatim /servo/0/position 256 \endverbatim
	Leave the argument value off to read the position of the servo:
	\verbatim /servo/0/position \endverbatim
	
	\par Speed
	The 'speed' property corresponds to the speed with which the servo responds to changes 
	of position.
	This value can be both read and written, and the range of values is 0 - 1023.  A speed of 1023
	means the servo responds immediately, and lower values will result in slower, and often smoother, 
	responses.  
	\par
	To set the speed of the first servo to just under full speed, send a message like
	\verbatim /servo/0/speed 975 \endverbatim
	Adjust the argument value to one that suits your application.\n
	Leave the argument value off to read the position of the servo:
	\verbatim /servo/0/speed \endverbatim
	
	\par Active
	The 'active' property corresponds to the active state of the servo.
	If the servo is set to be active, no other tasks will be able to
	write to the same I/O lines.  If you're not seeing appropriate
	responses to your messages to a servo, check the whether it's 
	locked by sending a message like
	\verbatim /servo/3/state \endverbatim
*/

#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* ServoOsc_Name = "servo";
static char* ServoOsc_PropertyNames[] = { "active", "position", "speed", 0 }; // must have a trailing 0

int ServoOsc_PropertySet( int index, int property, int value );
int ServoOsc_PropertyGet( int index, int property );

// Returns the name of the subsystem
const char* ServoOsc_GetName( )
{
  return ServoOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int ServoOsc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
                                           SERVO_COUNT, ServoOsc_Name,
                                           ServoOsc_PropertySet, ServoOsc_PropertyGet, 
                                           ServoOsc_PropertyNames );

  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, ServoOsc_Name, status );
  return CONTROLLER_OK;
}

// Set the index LED, property with the value
int ServoOsc_PropertySet( int index, int property, int value )
{
  switch ( property )
  {
    case 0:
      Servo_SetActive( index, value );
      break;
    case 1:
      Servo_SetPosition( index, value );
      break;
    case 2:
      Servo_SetSpeed( index, value );
      break;
  }
  return CONTROLLER_OK;
}

// Get the index LED, property
int ServoOsc_PropertyGet( int index, int property )
{
  int value;
  switch ( property )
  {
    case 0:
      value = Servo_GetActive( index );
      break;
    case 1:
      value = Servo_GetPosition( index );
      break;
    case 2:
      value = Servo_GetSpeed( index );
      break;
  }
  
  return value;
}

#endif
