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

/** \file stepper.c	
	Stepper Motor Controls.
	Methods for controlling up to 2 stepper motors with the Make Application Board.
*/

#include "stepper.h"
#include "io.h"
#include "fasttimer.h"
#include "config.h"

#include "AT91SAM7X256.h"

#if ( APPBOARD_VERSION == 50 )
  #define STEPPER_0_IO_0 IO_PA02
  #define STEPPER_0_IO_1 IO_PA02
  #define STEPPER_0_IO_2 IO_PA02
  #define STEPPER_0_IO_3 IO_PA02
  #define STEPPER_1_IO_0 IO_PA02
  #define STEPPER_1_IO_1 IO_PA02
  #define STEPPER_1_IO_2 IO_PA02
  #define STEPPER_1_IO_3 IO_PA02
#endif
#if ( APPBOARD_VERSION == 90 || APPBOARD_VERSION == 95 || APPBOARD_VERSION == 100 )
  #define STEPPER_0_IO_0 IO_PA24
  #define STEPPER_0_IO_1 IO_PA05
  #define STEPPER_0_IO_2 IO_PA06
  #define STEPPER_0_IO_3 IO_PA02
  #define STEPPER_1_IO_0 IO_PB25
  #define STEPPER_1_IO_1 IO_PA25
  #define STEPPER_1_IO_2 IO_PA26
  #define STEPPER_1_IO_3 IO_PB23
#endif

#define STEPPER_COUNT 2

typedef struct StepperControlS
{
  int users;
  unsigned int bipolar : 1;
  unsigned int halfStep : 1;
  int speed;
  int duty;
  int acceleration;
  int positionRequested;
  int position;
  int io[ 4 ];  
  unsigned timerRunning : 1;
  FastTimerEntry fastTimerEntry;
} StepperControl;

typedef struct StepperS
{
  int users;
  StepperControl* control[ STEPPER_COUNT ];
} Stepper_;

void Stepper_IRQCallback( int id );

static int Stepper_Start( int index );
static int Stepper_Stop( int index );
static int Stepper_Init( void );
static int Stepper_GetIo( int index, int io );
static void Stepper_SetDetails( StepperControl* s );
static void Stepper_SetUnipolarHalfStepOutput( StepperControl *s, int position );
static void Stepper_SetUnipolarOutput( StepperControl *s, int position );
static void Stepper_SetBipolarOutput( StepperControl *s, int position );
static void Stepper_SetBipolarHalfStepOutput( StepperControl *s, int position );

void Stepper_SetOn( int index, int* portAOn, int* portBOn );
void Stepper_SetOff( int index, int* portAOff, int* portBOff );
void Stepper_SetAll( int portAOn, int portBOn, int portAOff, int portBOff );

Stepper_* Stepper;

/** \defgroup Stepper Stepper
  The Stepper Motor subsystem provides speed and position control for one or two stepper motors.
  Up to 2 stepper motors can be controlled with the Make Application Board.
  Specify settings for your stepper motor by setting whether it's:
  - bipolar or unipolar
  - normal of half-stepping
  
  \section Positioning
  You can generally use the stepper motor in 2 modes - \b absolute positioning or \b relative positioning.

  For absolute positioning, call Stepper_SetPositionRequested() with the desired position, and the motor will move there.
  You can read back the stepper's position at any point along the way to determine where it is at a given moment.  The board
  keeps an internal count of how many steps the motor has taken in order to keep track of where it is.

  For relative positioning, use Stepper_Step( ) to simply move a number of steps from the current position.
  
  See the <a href="http://www.makingthings.com/documentation/how-to/stepper-motor">Stepper Motor how-to</a>
  for more detailed info on hooking up a stepper motor to the Make Controller.
* \ingroup Libraries
* @{
*/

/**
	Sets whether the specified Stepper is active.
	@param index An integer specifying which stepper (0 or 1).
	@param state An integer specifying the active state - 1 (active) or 0 (inactive).
	@return Zero on success.
	
	\b Example
	\code
	// enable stepper 1
	Stepper_SetActive(1, 1);
	\endcode
*/
int Stepper_SetActive( int index, int state )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( state )
  {
    if( Stepper == NULL ) // if our subsystem has not been fired up
    {
      int retVal = Stepper_Init( );
      if( retVal != CONTROLLER_OK )
        return retVal;
    }

    if( Stepper->control[ index ] == NULL ) // if this particular stepper has not been fired up
    {
      Stepper->control[ index ] = MallocWait( sizeof( StepperControl ), 100 );
      Stepper->control[ index ]->users = 0;
      if( Stepper_Start( index ) != CONTROLLER_OK )
        return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;
    }

    if( Stepper->control[ index ] != NULL )
      return CONTROLLER_OK;
    else
      return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;
  }
  else
    return Stepper_Stop( index );
}

/**
	Check whether the stepper is active or not
	@param index An integer specifying which stepper (0-1).
	@return State - 1 (active) or 0 (inactive).
	
	\b Example
	\code
	if( Stepper_GetActive(1) )
	{
	  // Stepper 1 is active
	}
	else
	{
	  // Stepper 1 is inactive
	}
	\endcode
*/
int Stepper_GetActive( int index )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if( Stepper == NULL )
    return 0;
  else
    return Stepper->control[ index ] != NULL;
}

/**	
	Set the position of the specified stepper motor.
	Note that this will not ask the stepper to move.  It will simply update
	the position that the stepper thinks its at.  To move the stepper, see
	Stepper_SetPositionRequested() or Stepper_Step().
	@param index An integer specifying which stepper (0 or 1).
	@param position An integer specifying the stepper position.
  @return status (0 = OK).
  
  \b Example
	\code
	// reset stepper 1 to call its current position 0
	Stepper_SetPosition(1, 0);
	\endcode
*/
int Stepper_SetPosition( int index, int position )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Stepper_SetActive( index, 1 ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;

  StepperControl* s = Stepper->control[ index ]; 
  
  DisableFIQFromThumb();
  s->position = position;
  s->positionRequested = position;
  EnableFIQFromThumb();

  Stepper_SetDetails( s );

  return CONTROLLER_OK;
}

/**	
	Set the destination position for a stepper motor.
	This will start the stepper moving the given number of
	steps at the current speed, as set by Stepper_SetSpeed().
	
	While it's moving, you can call Stepper_GetPosition() to read
	its current position.
	@param index An integer specifying which stepper (0 or 1).
	@param positionRequested An integer specifying the desired stepper position.
  @return status (0 = OK).
  
  \b Example
	\code
	// start moving stepper 0 1500 steps
	Stepper_SetPositionRequested(0, 1500);
	\endcode
*/
int Stepper_SetPositionRequested( int index, int positionRequested )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Stepper_SetActive( index, 1 ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;

  StepperControl* s = Stepper->control[ index ]; 
  DisableFIQFromThumb();
  s->positionRequested = positionRequested;
  EnableFIQFromThumb();

  Stepper_SetDetails( s );

  return CONTROLLER_OK;
}

/**	
	Set the speed at which a stepper will move.
  This is a number of ms per step, rather than the more common steps per second.  
  Arranging it this way makes it easier to express as an integer.  
  Fastest speed is 1ms / step (1000 steps per second) and slowest is many seconds.
	@param index An integer specifying which stepper (0 or 1).
	@param speed An integer specifying the stepper speed in ms per step
  @return status (0 = OK).
  
  \b Example
	\code
	// set the speed to 1ms / step (1000 steps per second)
	Stepper_SetSpeed(0, 1);
	\endcode
*/
int Stepper_SetSpeed( int index, int speed )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Stepper_SetActive( index, 1 ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;
  
  StepperControl* s = Stepper->control[ index ]; 

  s->speed = speed * 1000;

  DisableFIQFromThumb();
  FastTimer_SetTime( &s->fastTimerEntry, s->speed );
  EnableFIQFromThumb();

  Stepper_SetDetails( s );

  return CONTROLLER_OK;
}

/**	
	Get the speed at which a stepper will move.
	Read the value previously set for the speed parameter.
	@param index An integer specifying which stepper (0 or 1).
  @return The speed (0 - 1023), or 0 on error.
  
  \b Example
	\code
	int step0_speed = Stepper_GetSpeed(0);
	// now step0_speed has the speed of stepper 0
	\endcode
*/
int Stepper_GetSpeed( int index )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Stepper_SetActive( index, 1 ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;

  return Stepper->control[ index ]->speed;
}

/**	
	Read the current position of a stepper motor.
	@param index An integer specifying which stepper (0 or 1).
  @return The position, 0 on error.
  
  \b Example
	\code
	int step0_pos = Stepper_GetPosition(0);
	// now step0_pos has the current position of stepper 0
	\endcode
*/
int Stepper_GetPosition( int index )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Stepper_SetActive( index, 1 ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;

  return Stepper->control[ index ]->position;
}

/**	
	Read the destination position of a stepper motor.
	This indicates where the stepper is ultimately headed.  To see
	where it actually is, see Stepper_GetPosition().
	@param index An integer specifying which stepper (0 or 1).
  @return The position and 0 on error
  
  \b Example
	\code
	int step1_destination = Stepper_GetPositionRequested(1);
	// step1_destination has the requested position for stepper 1
	\endcode
*/
int Stepper_GetPositionRequested( int index )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Stepper_SetActive( index, 1 ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;

  return Stepper->control[ index ]->positionRequested;
}

/**	
	Simply take a number of steps from wherever the motor is currently positioned.
  This function will move the motor a given number of steps from the current position.
	@param index An integer specifying which stepper (0 or 1).
	@param steps An integer specifying the number of steps.  Can be negative to go in reverse.
  @return status (0 = OK).
  
  \b Example
	\code
	// take 1200 steps forward from our current position
	Stepper_Step(0, 1200);
	\endcode
*/
int Stepper_Step( int index, int steps )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Stepper_SetActive( index, 1 ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;

  StepperControl* s = Stepper->control[ index ]; 
  DisableFIQFromThumb();
  s->positionRequested = (s->position + steps);
  EnableFIQFromThumb();

  Stepper_SetDetails( s );
  return CONTROLLER_OK;
}

/**	
	Set the duty - from 0 to 1023.  The default is for 100% power (1023).
	@param index An integer specifying which stepper (0 or 1).
	@param duty An integer specifying the stepper duty (0 - 1023).
  @return status (0 = OK).
  
  \b Example
	\code
	// set stepper 0 to half power
	Stepper_SetDuty(0, 512);
	\endcode
*/
int Stepper_SetDuty( int index, int duty )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Stepper_SetActive( index, 1 ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;

  StepperControl* s = Stepper->control[ index ]; 
  s->duty = duty;

  // Fire the PWM's up
  int pwm = index * 2;
  Pwm_Set( pwm, duty );
  Pwm_Set( pwm + 1, duty );

  return CONTROLLER_OK;
}

/**	
	Get the duty 
  Read the value previously set for the duty.
	@param index An integer specifying which stepper (0 or 1).
  @return The duty (0 - 1023), or 0 on error.
  
  \b Example
	\code
	int step1_duty = Stepper_GetDuty(1);
	// step1_duty has the current duty for stepper 1
	\endcode
*/
int Stepper_GetDuty( int index )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Stepper_SetActive( index, 1 ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;

  return Stepper->control[ index ]->duty;
}

/**	
	Declare whether the stepper is bipolar or not.  
	Default is bipolar.
	@param index An integer specifying which stepper (0 or 1).
	@param bipolar An integer 1 for bipolar, 0 for unipolar
  @return status (0 = OK).
  
  \b Example
	\code
	// set stepper 1 to unipolar
	Stepper_SetBipolar(1, 0);
	\endcode
*/
int Stepper_SetBipolar( int index, int bipolar )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Stepper_SetActive( index, 1 ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;

  StepperControl* s = Stepper->control[ index ]; 
  s->bipolar = bipolar;

  return CONTROLLER_OK;
}

/**	
	Get the bipolar setting
  Read the value previously set for bipolar.
	@param index An integer specifying which stepper (0 or 1).
  @return 1 for bipolar or 0 for unipolar.
  
  \b Example
	\code
	if( Stepper_GetBipolar(1) )
	{
	  // stepper 1 is bipolar
	}
	else
	{
	  // stepper 1 is unipolar
	}
	\endcode
*/
int Stepper_GetBipolar( int index )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Stepper_SetActive( index, 1 ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;

  return Stepper->control[ index ]->bipolar;
}

/**	
	Declare whether the stepper is in half stepping mode or not.  
	Default is not - i.e. in full step mode.
	@param index An integer specifying which stepper (0 or 1).
	@param halfStep An integer specifying 1 for half step, 0 for full step
  @return status (0 = OK).
  
  \b Example
	\code
	// set stepper 1 to half step mode
	Stepper_SetHalfStep(1, 1);
	\endcode
*/
int Stepper_SetHalfStep( int index, int halfStep )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Stepper_SetActive( index, 1 ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;

  StepperControl* s = Stepper->control[ index ]; 
  s->halfStep = halfStep;

  return CONTROLLER_OK;
}

/**	
	Read whether the stepper is in half stepping mode or not.
	@param index An integer specifying which stepper (0 or 1).
  @return the HalfStep setting.
  
  \b Example
	\code
	if( Stepper_GetHalfStep(1) )
	{
	  // stepper 1 is in half step mode
	}
	else
	{
	  // stepper 1 is in full step mode
	}
	\endcode
*/
int Stepper_GetHalfStep( int index )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( Stepper_SetActive( index, 1 ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE;

  return Stepper->control[ index ]->halfStep;
}


/** @}
*/

int Stepper_Start( int index )
{
  static int status; // why does this not seem to work at -O2 when this is not declared as static?

  if ( index < 0 || index >= STEPPER_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  StepperControl* sc = Stepper->control[ index ];
  if ( sc->users++ == 0 )
  {
    status = Pwm_Start( index * 2 );
    if ( status != CONTROLLER_OK )
    {
      sc->users--;
      Stepper->users--;
      return status;
    }

    status = Pwm_Start( index * 2 + 1 );
    if ( status != CONTROLLER_OK )
    {
      Pwm_Stop( index * 2 );
      sc->users--;
      Stepper->users--;
      return status;
    }

    // Fire the PWM's up
    sc->duty = 1023;

    int pwm = index * 2;
    Pwm_Set( pwm, sc->duty );
    Pwm_Set( pwm + 1, sc->duty );

    // Get IO's
    int i;
    for ( i = 0; i < 4; i++ )
    {
      int io = Stepper_GetIo( index, i );      
      sc->io[ i ] = io;
        
      // Try to lock the stepper output
      status = Io_Start( io, true );
      if ( status != CONTROLLER_OK )
      {
        // Damn - unlock any that we did get
        int j;
        for ( j = 0; j < i; j++ )
          Io_Stop( sc->io[ j ] );

        Pwm_Stop( pwm );
        Pwm_Stop( pwm + 1 );

        Stepper->users--;
        sc->users--;
        return status;
      }
      Io_SetPio( io, true );
      Io_SetValue( io, true );
      Io_SetDirection( io, IO_OUTPUT );
    }

    DisableFIQFromThumb();
    sc->position = 0;
    sc->positionRequested = 0;
    sc->speed = 10;
    sc->timerRunning = 0;
    sc->halfStep = false;
    sc->bipolar = true;
    EnableFIQFromThumb();

    FastTimer_InitializeEntry( &sc->fastTimerEntry, Stepper_IRQCallback, index, sc->speed * 1000, true );
    Stepper->users++;
  }

  return CONTROLLER_OK;
}

int Stepper_Stop( int index )
{
  if ( index < 0 || index >= STEPPER_COUNT || Stepper == NULL )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if( Stepper->control[ index ] == NULL )
    return CONTROLLER_OK;

  StepperControl* s = Stepper->control[ index ]; 

  if ( s->users <= 0 )
    return CONTROLLER_ERROR_TOO_MANY_STOPS;

  if ( --s->users == 0 )
  {
    if ( s->timerRunning )
    {
      DisableFIQFromThumb();
      FastTimer_Cancel( &s->fastTimerEntry );
      EnableFIQFromThumb();
    }

    int i;
    for ( i = 0; i < 4; i++ )
    {
      int io = s->io[ i ];
      Io_SetValue( io, false );
      Io_Stop( io );
    }
 
    int pwm = index * 2;
    Pwm_Stop( pwm );
    Pwm_Stop( pwm + 1 );

    Free( s );
    s = NULL;

    if ( --Stepper->users == 0 )
    {
      Free( Stepper );
      Stepper = NULL;
    }
  }

  return CONTROLLER_OK;
}


int Stepper_GetIo( int stepperIndex, int ioIndex )
{
  int io = -1;
  switch ( stepperIndex )
  {
    case 0:
      switch ( ioIndex )
      {
        case 0:
          io = STEPPER_0_IO_0;
          break;
        case 1:
          io = STEPPER_0_IO_1;
          break;
        case 2:
          io = STEPPER_0_IO_2;
          break;
        case 3:
          io = STEPPER_0_IO_3;
          break;
      }
      break;
    case 1:
      switch ( ioIndex )
      {
        case 0:
          io = STEPPER_1_IO_0;
          break;
        case 1:
          io = STEPPER_1_IO_1;
          break;
        case 2:
          io = STEPPER_1_IO_2;
          break;
        case 3:
          io = STEPPER_1_IO_3;
          break;
      }
      break;
  }
  return io;
}

int Stepper_Init()
{
  if( Stepper == NULL )
  {
    Stepper = MallocWait( sizeof( Stepper_ ), 100 );
    Stepper->users = 0;
    int i;
    for( i = 0; i < STEPPER_COUNT; i++ )
      Stepper->control[ i ] = NULL;
  }
  // otherwise, we're all set.
  return CONTROLLER_OK;
}

void Stepper_IRQCallback( int id )
{
  if(id < 0 || id > 1)
    return;
  StepperControl* s = Stepper->control[ id ]; 

  if ( s->position < s->positionRequested )
    s->position++;
  if ( s->position > s->positionRequested )
    s->position--;

  if ( s->bipolar )
  {
    if ( s->halfStep )
      Stepper_SetBipolarHalfStepOutput( s, s->position );
    else
      Stepper_SetBipolarOutput( s, s->position );
  }
  else
  {
    if ( s->halfStep ) 
      Stepper_SetUnipolarHalfStepOutput( s, s->position );
    else
      Stepper_SetUnipolarOutput( s, s->position );
  }

  if ( s->position == s->positionRequested )
  {
    FastTimer_Cancel( &s->fastTimerEntry );
    s->timerRunning = false;
  }
}

void Stepper_SetDetails( StepperControl* s )
{
  if ( !s->timerRunning && ( s->position != s->positionRequested ) && ( s->speed != 0 ) )
  {
    s->timerRunning = true;
    DisableFIQFromThumb();
    FastTimer_Set( &s->fastTimerEntry );
    EnableFIQFromThumb();
  }
  else
  {
    if ( ( s->timerRunning ) && ( ( s->position == s->positionRequested ) || ( s->speed == 0 ) ) )
    {
      DisableFIQFromThumb();
      FastTimer_Cancel( &s->fastTimerEntry );
      EnableFIQFromThumb();
      s->timerRunning = false;
    }
  }
}

void Stepper_SetUnipolarHalfStepOutput( StepperControl *s, int position )
{
  //int output = position % 8;
  int output = position & 0x7;

  int* iop = s->io;

  int portAOn = 0;
  int portBOn = 0;
  int portAOff = 0;
  int portBOff = 0;

  switch ( output )
  {
    case -1:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 0:
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 1:
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 2:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 3:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 4:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 5:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );      
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      break;
    case 6:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );      
      Stepper_SetOff( *iop++, &portAOff, &portBOff );      
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      break;
    case 7:
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );      
      Stepper_SetOff( *iop++, &portAOff, &portBOff );      
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      break;
  }  

  Stepper_SetAll( portAOn, portBOn, portAOff, portBOff );
}

void Stepper_SetUnipolarOutput( StepperControl *s, int position )
{
  //int output = position % 4;
  int output = position & 0x3;
  int* iop = s->io;

  int portAOn = 0;
  int portBOn = 0;
  int portAOff = 0;
  int portBOff = 0;

  switch ( output )
  {
    case -1:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 0:
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 1:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 2:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 3:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      break;
  }  
  Stepper_SetAll( portAOn, portBOn, portAOff, portBOff );
}

void Stepper_SetBipolarHalfStepOutput( StepperControl *s, int position )
{
  //int output = position % 8;
  int output = position & 0x7;
  int* iop = s->io;

  int portAOn = 0;
  int portBOn = 0;
  int portAOff = 0;
  int portBOff = 0;

  switch ( output )
  {
    case -1:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 0:
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 1:
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 2:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 3:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 4:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 5:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      break;
    case 6:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      break;
    case 7:
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );      
      Stepper_SetOff( *iop++, &portAOff, &portBOff );      
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      break;
  }  

  Stepper_SetAll( portAOn, portBOn, portAOff, portBOff );
}

void Stepper_SetBipolarOutput( StepperControl *s, int position )
{
  //int output = position % 4; // work around % bug - negative numbers not handled properly
  int output = position & 0x3;
  int* iop = s->io;

  int portAOn = 0;
  int portBOn = 0;
  int portAOff = 0;
  int portBOff = 0;

  // This may be the least efficient code I have ever written
  switch ( output )
  {
    case -1:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 0:
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 1:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      break;
    case 2:
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      break;
    case 3:
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOff( *iop++, &portAOff, &portBOff );
      Stepper_SetOn( *iop++, &portAOn, &portBOn );
      break;
  }  
  Stepper_SetAll( portAOn, portBOn, portAOff, portBOff );
}

void Stepper_SetOn( int index, int* portAOn, int* portBOn )
{
  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
    *portAOn |= mask;
  else
    *portBOn |= mask;
}

void Stepper_SetOff( int index, int* portAOff, int* portBOff )
{
  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
    *portAOff |= mask;
  else
    *portBOff |= mask;
}

void Stepper_SetAll( int portAOn, int portBOn, int portAOff, int portBOff )
{
  AT91C_BASE_PIOA->PIO_SODR = portAOn;
  AT91C_BASE_PIOB->PIO_SODR = portBOn;
  AT91C_BASE_PIOA->PIO_CODR = portAOff;
  AT91C_BASE_PIOB->PIO_CODR = portBOff;
}

#ifdef OSC // defined in config.h

/** \defgroup StepperOSC Stepper - OSC
  \ingroup OSC
  Control Stepper motors with the Application Board via OSC.
  Specify settings for your stepper motor by setting whether it's:
  - bipolar or unipolar
  - normal of half-stepping

  You can generally use the stepper motor in 2 modes - \b absolute positioning or \b relative positioning.

  For absolute positioning, set \b positionrequested with the desired position, and the motor will move there.
  You can read back the stepper's \b position property at any point along the way to determine where it is at a given moment.  The board
  keeps an internal count of how many steps the motor has taken in order to keep track of where it is.

  For relative positioning, use the \b step property to simply move a number of steps from the current position.
	
	\section devices Devices
	There are 2 Stepper controllers available on the Application Board, numbered 0 & 1.
	See the Stepper section in the Application Board user's guide for more information
	on hooking steppers up to the board.
	
	\section properties Properties
	Each stepper controller has eight properties:
  - position
  - positionrequested
  - speed
  - duty
  - bipolar
  - halfstep
  - step
  - active

	\par Step
	The \b step property simply tells the motor to take a certain number of steps.
	This is a write-only value.
	\par
	To take 1000 steps with the first stepper, send the message
	\verbatim /stepper/0/step 1000\endverbatim
  
  \par Position
	The \b position property corresponds to the current step position of the stepper motor
	This value can be both read and written.  Writing this value changes where the motor thinks it is.
  The initial value of this parameter is 0.
	\par
	To set the first stepper to step position 10000, send the message
	\verbatim /stepper/0/position 10000\endverbatim
	Leave the argument value off to read the position of the stepper:
	\verbatim /stepper/0/position \endverbatim

  \par PositionRequested
	The \b positionrequested property describes the desired step position of the stepper motor
	This value can be both read and written.  Writing this value changes the motor's destination.
	\par
	To set the first stepper to go to position 10000, send the message
	\verbatim /stepper/0/positionrequested 10000\endverbatim
	Leave the argument value off to read the last requested position of the stepper:
	\verbatim /stepper/0/positionrequested \endverbatim

	\par Speed
	The \b speed property corresponds to the speed with which the stepper responds to changes 
	of position.  This value is the number of milliseconds between each step.  So, a speed of one
  would be a step every millisecond, or 1000 steps a second.  
  \par
	Note that not all stepper motors can be stepped quite that fast.  If you find your stepper motor acting strangely, 
  experiment with slowing down the speed a bit.
	\par
	To set the speed of the first stepper to step at 100ms per step, send a message like
	\verbatim /stepper/0/speed 100 \endverbatim
	Adjust the argument value to one that suits your application.\n
	Leave the argument value off to read the speed of the stepper:
	\verbatim /stepper/0/speed \endverbatim
	
  \par Duty
	The \b duty property corresponds to the how much of the power supply is to be sent to the
  stepper.  This is handy for when the stepper is static and not being required to perform too
  much work and reducing its power helps reduce heat dissipation.
	This value can be both read and written, and the range of values is 0 - 1023.  A duty of 0
	means the stepper gets no power, and the value of 1023 means the stepper gets full power.  
  \par
	To set the duty of the first stepper to 500, send a message like
	\verbatim /stepper/0/duty 500 \endverbatim
	Adjust the argument value to one that suits your application.\n
	Leave the argument value off to read the duty of the stepper:
	\verbatim /stepper/0/duty \endverbatim

  \par Bipolar
	The \b bipolar property is set to the style of stepper being used.  A value of 1 specifies bipolar
  (the default) and 0 specifies a unipolar stepper.
	This value can be both read and written.

  \par HalfStep
	The \b halfstep property controls whether the stepper is being half stepped or not.  A 0 here implies full stepping
  (the default) and 1 implies a half stepping.
	This value can be both read and written.

	\par Active
	The \b active property corresponds to the active state of the stepper.
	If the stepper is set to be active, no other tasks will be able to
	write to the same I/O lines.  If you're not seeing appropriate
	responses to your messages to a stepper, check the whether it's 
	locked by sending a message like
	\verbatim /stepper/1/active \endverbatim
*/

#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* StepperOsc_Name = "stepper";
static char* StepperOsc_PropertyNames[] = { "active", "position", "positionrequested", 
                                            "speed", "duty", "halfstep", 
                                            "bipolar", "step", 0 }; // must have a trailing 0

int StepperOsc_PropertySet( int index, int property, int value );
int StepperOsc_PropertyGet( int index, int property );

// Returns the name of the subsystem
const char* StepperOsc_GetName( )
{
  return StepperOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int StepperOsc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
                                           STEPPER_COUNT, StepperOsc_Name,
                                           StepperOsc_PropertySet, StepperOsc_PropertyGet, 
                                           StepperOsc_PropertyNames );

  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, StepperOsc_Name, status );
  return CONTROLLER_OK;
}

// Set the index LED, property with the value
int StepperOsc_PropertySet( int index, int property, int value )
{
  switch ( property )
  {
    case 0:
      Stepper_SetActive( index, value );
      break;
    case 1:
      Stepper_SetPosition( index, value );
      break;
    case 2:
      Stepper_SetPositionRequested( index, value );
      break;
    case 3:
      Stepper_SetSpeed( index, value );
      break;
    case 4:
      Stepper_SetDuty( index, value );
      break;
    case 5:
      Stepper_SetHalfStep( index, value );
      break;
    case 6:
      Stepper_SetBipolar( index, value );
      break;
    case 7: // step
      Stepper_Step( index, value );
      break;
  }
  return CONTROLLER_OK;
}

// Get the index LED, property
int StepperOsc_PropertyGet( int index, int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      value = Stepper_GetActive( index );
      break;
    case 1:
      value = Stepper_GetPosition( index );
      break;
    case 2:
      value = Stepper_GetPositionRequested( index );
      break;
    case 3:
      value = Stepper_GetSpeed( index );
      break;
    case 4:
      value = Stepper_GetDuty( index );
      break;
    case 5:
      value = Stepper_GetHalfStep( index );
      break;
    case 6:
      value = Stepper_GetBipolar( index );
      break;
  }
  return value;
}

#endif


