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

#include "stepper.h"
#include "io.h"
#include "fasttimer.h"
#include "config.h"
#include "error.h"

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
#if ( APPBOARD_VERSION >= 90 )
  #define STEPPER_0_IO_0 IO_PA24
  #define STEPPER_0_IO_1 IO_PA05
  #define STEPPER_0_IO_2 IO_PA06
  #define STEPPER_0_IO_3 IO_PA02
  #define STEPPER_1_IO_0 IO_PB25
  #define STEPPER_1_IO_1 IO_PA25
  #define STEPPER_1_IO_2 IO_PA26
  #define STEPPER_1_IO_3 IO_PB23
#endif

// static 
Stepper::StepperInternal* Stepper::steppers[STEPPER_COUNT] = {0, 0};

void Stepper_IRQCallback( int id );

/**
  Create a new stepper object.
  @param index An integer specifying which stepper (0 or 1).
  
  \b Example
  \code
  // stepper 1
  Stepper step(1);
  // that's it!
  \endcode
*/
Stepper::Stepper( int index )
{
  if ( index < 0 || index >= STEPPER_COUNT )
    return;
  _index = index;

  StepperInternal* s;
  if ( steppers[_index] == 0 )
  {
    steppers[_index] = new StepperInternal;
    s = steppers[_index];
    // Fire the PWM's up
    s->duty = 1023;
    int pwmIdx = _index * 2;
    s->pwmA = new Pwm( pwmIdx );
    s->pwmB = new Pwm( pwmIdx + 1 );
    s->pwmA->setDuty( s->duty );
    s->pwmB->setDuty( s->duty );

    // setup IO's
    for( int i = 0; i < 4; i++ )
    {
      s->ios[i] = new Io( getIo(i) );
      s->ios[i]->setValue(true);
    }

    DisableFIQFromThumb();
    s->position = 0;
    s->destination = 0;
    s->speed = 10;
    s->timerRunning = false;
    s->halfStep = false;
    s->bipolar = true;
    s->refcount = 0;
    EnableFIQFromThumb();
    
    s->fastTimer.setHandler( Stepper_IRQCallback, _index );
    s->fastTimer.start(s->speed * 1000);
  }
  s->refcount++;
}

Stepper::~Stepper( )
{
  StepperInternal* si = steppers[_index];
  if( --si->refcount <= 0 )
  {
    if( si->timerRunning )
    {
      DisableFIQFromThumb();
      si->fastTimer.stop();
      EnableFIQFromThumb();
    }

    for( int i = 0; i < 4; i++ )
    {
      si->ios[i]->off();
      delete si->ios[i];
    }
 
    delete si->pwmA;
    delete si->pwmB;
    delete si;
    steppers[_index] = 0;
  }
}

/** 
  Reset where the stepper motor thinks it is.
  
  Note that this will not ask the stepper to move.  It will simply update
  the position that the stepper thinks it's at.  To move the stepper, see
  stepTo() or step().
  
  If the stepper is currently moving it will stop, since it will now
  be at the newly specified destination.
  
  @param position An integer specifying the stepper position.
  @return True on success, false on failure.
  
  \b Example
  \code
  // reset stepper 1 to call its current position 0
  Stepper s(1);
  s.resetPosition(0);
  \endcode
*/
bool Stepper::resetPosition( int position )
{
  DisableFIQFromThumb();
  steppers[_index]->position = position;
  steppers[_index]->destination = position;
  EnableFIQFromThumb();

  setDetails( );
  return true;
}

/** 
  Step to a certain position.
  This will start the stepper moving the given number of
  steps at the current speed, as set by setSpeed().
  
  While it's moving, you can call position() to read its current position.
  @param position An integer specifying the desired destination.
  @return True on success, false on failure.
  
  \b Example
  \code
  // start moving stepper 0 to position 1500
  Stepper s(0);
  s.stepTo(1500);
  \endcode
*/
bool Stepper::stepTo( int position )
{
  DisableFIQFromThumb();
  steppers[_index]->destination = position;
  EnableFIQFromThumb();

  setDetails( );
  return true;
}

/** 
  Set the speed at which a stepper will move.
  This is a number of ms per step, rather than the more common steps per second.  
  Arranging it this way makes it easier to express as an integer.  
  Fastest speed is 1ms / step (1000 steps per second) and slowest is many seconds.
  @param speed The speed in millis per step
  @return True on success, false on failure.
  
  \b Example
  \code
  // set the speed to 1ms / step (1000 steps per second)
  Stepper step(0);
  step.setSpeed(1);
  \endcode
*/
bool Stepper::setSpeed( int speed )
{ 
  StepperInternal* s = steppers[_index];
  s->speed = speed * 1000;
  DisableFIQFromThumb();
  s->fastTimer.start(s->speed);
  EnableFIQFromThumb();

  setDetails( );
  return true;
}

/** 
  Get the speed at which a stepper will move.
  @return The speed (0 - 1023)
  
  \b Example
  \code
  Stepper s(0);
  int step0_speed = s.speed(0);
  // now step0_speed has the speed of stepper 0
  \endcode
*/
int Stepper::speed( )
{
  return steppers[_index]->speed;
}

/** 
  Read the current position of a stepper motor.
  
  If you've told the motor to step to a certain position
  via stepTo(), you can check how far along it is with this method.
  
  @return The position
  
  \b Example
  \code
  Stepper s(0);
  int step0_pos = s.position(0);
  // now step0_pos has the current position of stepper 0
  \endcode
*/
int Stepper::position( )
{
  return steppers[_index]->position;
}

/** 
  Read the destination position of a stepper motor.
  
  When you use stepTo() you set the destination for the stepper.
  You can check position() along the way, or use this method destination()
  to determine where the stepper is headed.
  @return The destination.
  
  \b Example
  \code
  Stepper s(1);
  s.stepTo(5000);
  Task::sleep(10); // wait a moment while it steps
  int pos = s.position(); // how far along is it
  int dest = s.destination(); // destination should be 5000
  \endcode
*/
int Stepper::destination( )
{
  return steppers[_index]->destination;
}

/** 
  Simply take a number of steps from wherever the motor is currently positioned.
  This function will move the motor a given number of steps from the current position.
  
  @param steps How many steps to take.  Can be negative to go in reverse.
  @return True on success, false on failure.
  
  \b Example
  \code
  // take 1200 steps forward from our current position
  Stepper steppy(1);
  steppy.step(1200);
  \endcode
*/
bool Stepper::step( int steps )
{
  StepperInternal* s = steppers[_index]; 
  DisableFIQFromThumb();
  s->destination = (s->position + steps);
  EnableFIQFromThumb();

  setDetails( );
  return true;
}

/** 
  Set the duty - from 0 to 1023.  The default is for 100% power (1023).
  
  @param duty The duty 0 (off) - 1023 (full on).
  @return True on success, false on failure.
  
  \b Example
  \code
  // set stepper 0 to half power
  Stepper* s = new Stepper(0);
  s->setDuty(512);
  \endcode
*/
bool Stepper::setDuty( int duty )
{
  StepperInternal* s = steppers[_index];
  s->duty = duty;
  // Fire the PWM's up
  s->pwmA->setDuty( s->duty );
  s->pwmB->setDuty( s->duty );
  return true;
}

/** 
  Get the duty 
  Read the value previously set for the duty.
  @return The duty (0 - 1023).
  
  \b Example
  \code
  Stepper* s = new Stepper(1);
  int step1_duty = s->duty();
  // step1_duty has the current duty for stepper 1
  \endcode
*/
int Stepper::duty( )
{
  return steppers[_index]->duty;
}

/** 
  Declare whether the stepper is bipolar or not.  
  Default is bipolar.  If not biploar, then it's unipolar.
  @param bipolar True for bipolar, false for unipolar
  @return True on success, false on failure.
  
  \b Example
  \code
  // set stepper 1 to unipolar
  Stepper s(1);
  s.setBipolar(true);
  \endcode
*/
bool Stepper::setBipolar( bool bipolar )
{
  steppers[_index]->bipolar = bipolar;
  return true;
}

/** 
  Read whether this stepper is in bipolar mode.
  
  @return True if it's bipolar or false for unipolar.
  
  \b Example
  \code
  Stepper s(0);
  if( s.bipolar() )
  {
    // stepper 1 is bipolar
  }
  else
  {
    // stepper 1 is unipolar
  }
  \endcode
*/
bool Stepper::bipolar( )
{
  return steppers[_index]->bipolar;
}

/** 
  Declare whether the stepper is in half stepping mode or not.  
  Default is not - i.e. in full step mode.
  @param halfStep True for half step, false for full step
  @return status True on success, false on failure.
  
  \b Example
  \code
  // set stepper 1 to half step mode
  Stepper s(1);
  s.setHalfStep(true);
  \endcode
*/
bool Stepper::setHalfStep( bool halfStep )
{
  steppers[_index]->halfStep = halfStep;
  return true;
}

/** 
  Read whether the stepper is in half stepping mode or not.
  @return the HalfStep setting.
  
  \b Example
  \code
  Stepper s(1);
  if( s.halfStep() )
  {
    // stepper 1 is in half step mode
  }
  else
  {
    // stepper 1 is in full step mode
  }
  \endcode
*/
bool Stepper::halfStep( )
{
  return steppers[_index]->halfStep;
}

int Stepper::getIo( int ioIndex )
{
  switch ( _index )
  {
    case 0:
      switch ( ioIndex )
      {
        case 0: return STEPPER_0_IO_0;
        case 1: return STEPPER_0_IO_1;
        case 2: return STEPPER_0_IO_2;
        case 3: return STEPPER_0_IO_3;
      }
      break;
    case 1:
      switch ( ioIndex )
      {
        case 0: return STEPPER_1_IO_0;
        case 1: return STEPPER_1_IO_1;
        case 2: return STEPPER_1_IO_2;
        case 3: return STEPPER_1_IO_3;
      }
      break;
  }
  return 0;
}

void Stepper_IRQCallback( int id )
{
  if(id < 0 || id > 1)
    return;
  Stepper::StepperInternal* s = Stepper::steppers[id]; 

  if ( s->position < s->destination )
    s->position++;
  if ( s->position > s->destination )
    s->position--;

  if ( s->bipolar )
  {
    if ( s->halfStep )
      s->setBipolarHalfStepOutput( s->position );
    else
      s->setBipolarOutput( s->position );
  }
  else
  {
    if ( s->halfStep ) 
      s->setUnipolarHalfStepOutput( s->position );
    else
      s->setUnipolarOutput( s->position );
  }

  if ( s->position == s->destination )
  {
    s->fastTimer.stop();
    s->timerRunning = false;
  }
}

void Stepper::setDetails( )
{
  StepperInternal* s = steppers[_index];
  if ( !s->timerRunning && ( s->position != s->destination ) && ( s->speed != 0 ) )
  {
    s->timerRunning = true;
    DisableFIQFromThumb();
    s->fastTimer.start(s->speed);
    EnableFIQFromThumb();
  }
  else
  {
    if ( ( s->timerRunning ) && ( ( s->position == s->destination ) || ( s->speed == 0 ) ) )
    {
      DisableFIQFromThumb();
      s->fastTimer.stop();
      EnableFIQFromThumb();
      s->timerRunning = false;
    }
  }
}

void Stepper::StepperInternal::setUnipolarHalfStepOutput( int position )
{
  //int output = position % 8;
  int output = position & 0x7;

  Io* iop = ios[0];

  int portAOn = 0;
  int portBOn = 0;
  int portAOff = 0;
  int portBOff = 0;

  switch ( output )
  {
    case -1:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 0:
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 1:
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 2:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 3:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 4:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 5:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );      
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      break;
    case 6:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );      
      setOff( (iop++)->pin(), &portAOff, &portBOff );      
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      break;
    case 7:
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );      
      setOff( (iop++)->pin(), &portAOff, &portBOff );      
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      break;
  }  

  setAll( portAOn, portBOn, portAOff, portBOff );
}

void Stepper::StepperInternal::setUnipolarOutput( int position )
{
  //int output = position % 4;
  int output = position & 0x3;
  Io* iop = ios[0];

  int portAOn = 0;
  int portBOn = 0;
  int portAOff = 0;
  int portBOff = 0;

  switch ( output )
  {
    case -1:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 0:
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 1:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 2:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 3:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      break;
  }  
  setAll( portAOn, portBOn, portAOff, portBOff );
}

void Stepper::StepperInternal::setBipolarHalfStepOutput( int position )
{
  //int output = position % 8;
  int output = position & 0x7;
  Io* iop = ios[0];

  int portAOn = 0;
  int portBOn = 0;
  int portAOff = 0;
  int portBOff = 0;

  switch ( output )
  {
    case -1:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 0:
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 1:
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 2:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 3:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 4:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 5:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      break;
    case 6:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      break;
    case 7:
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );      
      setOff( (iop++)->pin(), &portAOff, &portBOff );      
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      break;
  }  

  setAll( portAOn, portBOn, portAOff, portBOff );
}

void Stepper::StepperInternal::setBipolarOutput( int position )
{
  //int output = position % 4; // work around % bug - negative numbers not handled properly
  int output = position & 0x3;
  Io* iop = ios[0];

  int portAOn = 0;
  int portBOn = 0;
  int portAOff = 0;
  int portBOff = 0;

  // This may be the least efficient code I have ever written
  switch ( output )
  {
    case -1:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 0:
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 1:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      break;
    case 2:
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      break;
    case 3:
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOff( (iop++)->pin(), &portAOff, &portBOff );
      setOn( (iop++)->pin(), &portAOn, &portBOn );
      break;
  }  
  setAll( portAOn, portBOn, portAOff, portBOff );
}

void Stepper::StepperInternal::setOn( int index, int* portAOn, int* portBOn )
{
  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
    *portAOn |= mask;
  else
    *portBOn |= mask;
}

void Stepper::StepperInternal::setOff( int index, int* portAOff, int* portBOff )
{
  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
    *portAOff |= mask;
  else
    *portBOff |= mask;
}

void Stepper::StepperInternal::setAll( int portAOn, int portBOn, int portAOff, int portBOff )
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
  Each stepper has the following properties:
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

//#include "osc.h"
//#include "string.h"
//#include "stdio.h"
//
//// Need a list of property names
//// MUST end in zero
//static char* StepperOsc_Name = "stepper";
//static char* StepperOsc_PropertyNames[] = { "active", "position", "positionrequested", 
//                                            "speed", "duty", "halfstep", 
//                                            "bipolar", "step", 0 }; // must have a trailing 0
//
//int StepperOsc_PropertySet( int index, int property, int value );
//int StepperOsc_PropertyGet( int index, int property );
//
//// Returns the name of the subsystem
//const char* StepperOsc_GetName( )
//{
//  return StepperOsc_Name;
//}
//
//// Now getting a message.  This is actually a part message, with the first
//// part (the subsystem) already parsed off.
//int StepperOsc_ReceiveMessage( int channel, char* message, int length )
//{
//  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
//                                           STEPPER_COUNT, StepperOsc_Name,
//                                           StepperOsc_PropertySet, StepperOsc_PropertyGet, 
//                                           StepperOsc_PropertyNames );
//
//  if ( status != CONTROLLER_OK )
//    return Osc_SendError( channel, StepperOsc_Name, status );
//  return CONTROLLER_OK;
//}
//
//// Set the index LED, property with the value
//int StepperOsc_PropertySet( int index, int property, int value )
//{
//  switch ( property )
//  {
//    case 0:
//      Stepper_SetActive( index, value );
//      break;
//    case 1:
//      Stepper_SetPosition( index, value );
//      break;
//    case 2:
//      Stepper_SetPositionRequested( index, value );
//      break;
//    case 3:
//      Stepper_SetSpeed( index, value );
//      break;
//    case 4:
//      Stepper_SetDuty( index, value );
//      break;
//    case 5:
//      Stepper_SetHalfStep( index, value );
//      break;
//    case 6:
//      Stepper_SetBipolar( index, value );
//      break;
//    case 7: // step
//      Stepper_Step( index, value );
//      break;
//  }
//  return CONTROLLER_OK;
//}
//
//// Get the index LED, property
//int StepperOsc_PropertyGet( int index, int property )
//{
//  int value = 0;
//  switch ( property )
//  {
//    case 0:
//      value = Stepper_GetActive( index );
//      break;
//    case 1:
//      value = Stepper_GetPosition( index );
//      break;
//    case 2:
//      value = Stepper_GetPositionRequested( index );
//      break;
//    case 3:
//      value = Stepper_GetSpeed( index );
//      break;
//    case 4:
//      value = Stepper_GetDuty( index );
//      break;
//    case 5:
//      value = Stepper_GetHalfStep( index );
//      break;
//    case 6:
//      value = Stepper_GetBipolar( index );
//      break;
//  }
//  return value;
//}

#endif


