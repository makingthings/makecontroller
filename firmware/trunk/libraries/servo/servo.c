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

/** \file servo.c	
	Servo Motor Controls.
	Methods for controlling up to 4 servo motors with the Make Application Board.
*/

#include "servo.h"
#include "io.h"
#include "fasttimer.h"
#include "config.h"

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
  ServoControl* control[ SERVO_COUNT ];
} Servo_;


Servo_* Servo;

/** \defgroup Servo Servo
  The Servo Motor subsystem controls speed and position control for up to 4 standard servo motors.
  Standard servos have a range of motion of approximately 180 degrees, although this varies from motor to motor.
  Be sure to plug in the connector with the correct orientation with regard to the GND/5V signals on the board.

  Because not all servo motors are created equal, and not all of them can be safely driven across all 180 degrees,
  the default (safe) range of motion is from values 0 to 1023.  The full range of motion is from -512 to 1536, but use this
  extra range cautiously if you don't know how your motor can handle it.  A little gentle experimentation should do the trick.
  
  You can also specify the speed with which the motors will respond to new position commands - a high
  value will result in an immediate response, while a lower value can offer some smoothing when appropriate.
  
  See the servo section in the <a href="http://www.makingthings.com/documentation/tutorial/application-board-overview/servos">
  Application Board overview</a> for more detailed info.
* \ingroup Libraries
* @{
*/

/**
	Lock or unlock the I/O lines used by the servos.  
	Sets whether the specified Servo I/O is active.
	@param index An integer specifying which servo (0 - 3).
	@param state An integer specifying the active state - 1 (active) or 0 (inactive).
	@return Zero on success.
	
	\b Example
	\code
	// enable servo 2
	Servo_SetActive(2, 1);
	\endcode
*/
int Servo_SetActive( int index, int state )
{
  if ( index < 0 || index >= SERVO_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( state )
  {
    if( Servo == NULL )
    {
      int retVal = Servo_Init( );
      if( retVal != CONTROLLER_OK )
        return retVal;
    }
    return Servo_Start( index );
  }
  else
  {
    if( Servo == NULL )
      return -1;

    int retVal = Servo_Stop( index );
    if( retVal != CONTROLLER_OK )
      return retVal;

    if( --Servo->users <= 0 )
      retVal = Servo_Deinit( );

    return retVal;
  }
}

/**
	Check whether the I/O lines used by the servos are locked.
	@param index An integer specifying which servo (0-3).
	@return State - 1 (active) or 0 (inactive).
	
	\b Example
	\code
	if( Servo_GetActive(2) )
	{
	  // Servo 2 is active
	}
	else
	{
	  // Servo 2 is inactive
	}
	\endcode
*/
int Servo_GetActive( int index )
{
  if ( index < 0 || index >= SERVO_COUNT || Servo == NULL )
    return false;
  return Servo->control[ index ]->users > 0;
}


/**	
	Set the position of the specified servo motor.
	Most servos like to be driven within a "safe range" which usually ends up being somewhere around 110-120
  degrees range of motion, or thereabouts.  Some servos don't mind being driven all the way to their 180
  degree range of motion limit.  With this in mind, the range of values you can send to servos connected
  to the Make Controller Kit is as follows:
  - Values from 0-1023 correspond to the normal, or "safe", range of motion.  
  - You can also send values from -512 all the way up to 1536 to drive the servo through its full
  range of motion.
  
  Note that it is sometimes possible to damage your servo by driving it too far, so proceed with a bit of
  caution when using the extended range until you know your servos can handle it.

	@param index An integer specifying which servo (0 - 3).
	@param position An integer specifying the servo position (0 - 1023).
  @return status (0 = OK).
  
  \b Example
	\code
	// set servo 1 to midway through the "safe" range
	Servo_SetPosition(1, 512);
	\endcode
*/
int Servo_SetPosition( int index, int position )
{
  if ( index < 0 || index >= SERVO_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Servo_SetActive( index, 1 );

  if ( position < SERVO_MIN_POSITION )
    position = SERVO_SAFE_MIN;
  if ( position > SERVO_MAX_POSITION )
    position = SERVO_SAFE_MAX;

  position += SERVO_OFFSET;

  DisableFIQFromThumb();
  Servo->control[ index ]->positionRequested = position << 6;
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
  
  \b Example
	\code
	// set servo 1 half speed
	Servo_SetSpeed(1, 512);
	\endcode
*/
int Servo_SetSpeed( int index, int speed )
{
  if ( index < 0 || index >= SERVO_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;
  
  Servo_SetActive( index, 1 );

  if ( speed < 1 )
    speed = 1;
  if( speed > 1023 )
    speed = 1023;
  DisableFIQFromThumb();
  Servo->control[ index ]->speed = speed << 6;
  EnableFIQFromThumb();

  return CONTROLLER_OK;
}

/**	
	Read the current position of a servo motor.
	@param index An integer specifying which servo (0 - 3).
  @return The position (0 - 1023), or 0 on error.
  
  \b Example
	\code
	int srv0_pos = Servo_GetPosition(0);
	// now srv0_pos is the current position
	\endcode
*/
int Servo_GetPosition( int index )
{
  if ( index < 0 || index >= SERVO_COUNT )
    return 0;
  
  Servo_SetActive( index, 1 );
  return (Servo->control[ index ]->position >> 6) - SERVO_OFFSET;
}

/**	
	Get the speed at which a servo will move in response to a call to Servo_SetPosition().
	Read the value previously set for the speed parameter.
	@param index An integer specifying which servo (0 - 3).
  @return The speed (0 - 1023), or 0 on error.
  
  \b Example
	\code
	int srv0_speed = Servo_GetSpeed(0);
	// now srv0_speed is the current speed
	\endcode
*/
int Servo_GetSpeed( int index )
{
  if ( index < 0 || index >= SERVO_COUNT )
    return 0;
  
  Servo_SetActive( index, 1 );

  return Servo->control[ index ]->speed >> 6;
}

/** @}
*/

int Servo_Start( int index )
{
  int status;

  if ( index < 0 || index >= SERVO_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  ServoControl* sc = Servo->control[ index ];  
  if ( sc->users == 0 )
  {
    int io = Servo_GetIo( index );

    // Try to lock the servo output
    status = Io_Start( io, true );
    if ( status != CONTROLLER_OK )
    {
      // Damn
      Servo->users--;
      sc->users--;
      return status;
    }

    Io_SetPio( io, true );
    Io_SetValue( io, true );
    Io_SetDirection( io, IO_OUTPUT );

    sc->position = (SERVO_MID_POSITION + SERVO_OFFSET) << 6;
    sc->speed = 1023 << 6;
    sc->users++;
  }

  return CONTROLLER_OK;
}

int Servo_Stop( int index )
{
  if ( index < 0 || index >= SERVO_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  ServoControl* sc = Servo->control[ index ]; 

  if ( sc->users <= 0 )
    return CONTROLLER_ERROR_TOO_MANY_STOPS;

  if ( --sc->users == 0 )
  {
    int io = Servo_GetIo( index );
    //Io_SetInput( io );
    Io_SetDirection( io, IO_INPUT );
    Io_Stop( io );
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
  Servo = MallocWait( sizeof( Servo_ ), 100 );

  Servo->users = 0;
  int i;
  for( i = 0; i < SERVO_COUNT; i++ )
  {
    Servo->control[ i ] = NULL;
    Servo->control[ i ] = MallocWait( sizeof( ServoControl ), 100 );

    ServoControl* s = Servo->control[ i ];
    s->users = 0;
    s->positionRequested = (SERVO_MID_POSITION + SERVO_OFFSET) << 6;
    switch( i )
    {
      case 0:
        s->pIoBase = AT91C_BASE_PIOB;
        s->pin = AT91C_PIO_PB24;
        break;
      case 1:
        s->pIoBase = AT91C_BASE_PIOA;
        s->pin = AT91C_PIO_PA23;
        break;
      case 2:
        s->pIoBase = AT91C_BASE_PIOA;
        s->pin = AT91C_PIO_PA21;
        break;
      case 3:
        s->pIoBase = AT91C_BASE_PIOA;
        s->pin = AT91C_PIO_PA22;
        break;
    }
  }

  // Global init stuff
  Servo->state = 0;
  Servo->gap = 1882; // forces 64'ish cycles a second (1894 + 2012 = 3906, 3906 * 4 = 15624, 15624 * 64 = 999936)
  Servo->index = 0;

  FastTimer_InitializeEntry( &Servo->fastTimerEntry, Servo_IRQCallback, 0, 2000, true );
  FastTimer_Set( &Servo->fastTimerEntry );

  return CONTROLLER_OK;
}

int Servo_Deinit()
{
  // Disable the device
  // AT91C_BASE_TC1->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
  FastTimer_Cancel( &Servo->fastTimerEntry );
  int i;
  for( i = 0; i < SERVO_COUNT; i++ )
    Free( Servo->control[i] );
  Free( Servo );
  Servo = NULL;
  return CONTROLLER_OK;
}

void Servo_IRQCallback( int id )
{
  (void)id;
  int period;

  switch ( Servo->state )
  {
    case 0:
    {
      if ( ++Servo->index >= SERVO_COUNT || Servo->index < 0 )
        Servo->index = 0;
      ServoControl* s = Servo->control[ Servo->index ];
      
      if ( s->position != s->positionRequested )
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

      period = s->position >> 6;
      if ( period >= (SERVO_MIN_POSITION + SERVO_OFFSET) && period <= (SERVO_MAX_POSITION + SERVO_OFFSET) )
        s->pIoBase->PIO_CODR = s->pin;
      else
        period = SERVO_MAX_POSITION;
      FastTimer_SetTime( &Servo->fastTimerEntry, period );
      Servo->state = 1;
      break;
    }
    case 1:
    {
      ServoControl* s = Servo->control[ Servo->index ];
      period = s->position >> 6;
      s->pIoBase->PIO_SODR = s->pin;
      FastTimer_SetTime( &Servo->fastTimerEntry, Servo->gap + ( SERVO_CYCLE - period ) );
      Servo->state = 0;
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
  int value = 0;
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
