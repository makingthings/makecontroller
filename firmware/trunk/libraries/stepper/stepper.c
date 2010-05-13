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
#include "core.h"
#include "fasttimer.h"

#include "at91sam7.h"

#if ( APPBOARD_VERSION == 90 || APPBOARD_VERSION == 95 || APPBOARD_VERSION == 100 )
  #define STEPPER_0_IO_0 PIN_PA24
  #define STEPPER_0_IO_1 PIN_PA5
  #define STEPPER_0_IO_2 PIN_PA6
  #define STEPPER_0_IO_3 PIN_PA2
  #define STEPPER_1_IO_0 PIN_PB25
  #define STEPPER_1_IO_1 PIN_PA25
  #define STEPPER_1_IO_2 PIN_PA26
  #define STEPPER_1_IO_3 PIN_PB23
#endif

#define STEPPER_COUNT 2

#ifndef STEPPER_DEFAULT_SPEED
#define STEPPER_DEFAULT_SPEED 10
#endif

typedef struct Stepper_t {
  unsigned int bipolar : 1;
  unsigned int halfStep : 1;
  unsigned int timerRunning : 1;
  unsigned int speed;
  int duty;
  int acceleration;
  int destination;
  int position;
  int pins[4];
  FastTimer fastTimer;
} Stepper;

static void stepperIRQCallback(int id);

static int stepperGetIo(int stepper, int io);
static void stepperSetDetails(Stepper* s);
static void stepperSetUnipolarHalfStepOutput(Stepper *s, int position);
static void stepperSetUnipolarOutput(Stepper *s, int position);
static void stepperSetBipolarOutput(Stepper *s, int position);
static void stepperSetBipolarHalfStepOutput(Stepper *s, int position);

static void stepperSetOn(int stepper, int* portAOn, int* portBOn);
static void stepperSetOff(int stepper, int* portAOff, int* portBOff);
static void stepperSetAll(int portAOn, int portBOn, int portAOff, int portBOff);

static Stepper steppers[STEPPER_COUNT];

/** \defgroup Stepper Stepper
  The Stepper Motor subsystem provides speed and position control for one or two stepper motors.
  Up to 2 stepper motors can be controlled with the Make Application Board.
  Specify settings for your stepper motor by setting whether it's:
  - bipolar or unipolar
  - normal of half-stepping
  
  \section Usage
  There are 2 approaches to using the stepper motor - \b absolute positioning or \b relative positioning.

  \section abs Absolute Positioning
  For absolute positioning, call stepperSetDestination() with the desired position, and the motor will move there.
  You can read back the stepper's position at any point along the way, via stepperPosition(), to determine
  where it is at a given moment. The board keeps an internal count of how many steps the motor has taken in
  order to keep track of where it is.

  \section rel Relative Positioning
  For relative positioning, simply use stepperStep() to move a number of steps from the current position.
  
  See the <a href="http://www.makingthings.com/documentation/how-to/stepper-motor">Stepper Motor how-to</a>
  for more detailed info on hooking up a stepper motor to the Make Controller.
  \ingroup io
  @{
*/

/**
	Enable a stepper motor.
	@param stepper Which stepper (0 or 1).
	
	\b Example
	\code
	// enable stepper 1
	stepperEnable(1);
	\endcode
*/
void stepperEnable(int stepper)
{
  fasttimerInit(2);
  Stepper* s = &steppers[stepper];
  pwmEnableChannel(stepper * 2);
  pwmEnableChannel(stepper * 2 + 1);
  // Fire the PWM's up
  s->duty = 1023;

  int pwm = stepper * 2;
  pwmSetDuty(pwm, s->duty);
  pwmSetDuty(pwm + 1, s->duty);

  // Get IO's
  int i;
  for (i = 0; i < 4; i++) {
    s->pins[i] = stepperGetIo(stepper, i);
    pinSetMode(s->pins[i], OUTPUT);
    pinOn(s->pins[i]);
  }

  s->position = 0;
  s->destination = 0;
  s->speed = STEPPER_DEFAULT_SPEED;
  s->timerRunning = 0;
  s->halfStep = false;
  s->bipolar = true;

  s->fastTimer.handler = stepperIRQCallback;
  s->fastTimer.id = stepper;
  fasttimerStart(&s->fastTimer, s->speed * 1000, true);
}

/**
  Disable a stepper motor.
  @param stepper Which stepper (0 or 1).
*/
void stepperDisable(int stepper)
{
  Stepper* s = &steppers[stepper];
  if (s->timerRunning) {
    chSysDisable();
    fasttimerStop(&s->fastTimer);
    chSysEnable();
  }

  int i;
  for (i = 0; i < 4; i++)
    pinOff(s->pins[i]);

  int pwm = stepper * 2;
  pwmDisableChannel(pwm);
  pwmDisableChannel(pwm + 1);
}

/**	
	Reset the position of the specified stepper motor.
	Note that this will not ask the stepper to move.  It will simply set the
	stepper's current position as 0.  To move the stepper, see
	stepperSetDestination() or stepperStep().
	@param stepper An integer specifying which stepper (0 or 1).
	@param position An integer specifying the stepper position.
  @return status (0 = OK).
  
  \b Example
	\code
	// reset stepper 1 to call its current position 0
	stepperSetPosition(1, 0);
	\endcode
*/
int stepperResetPosition(int stepper, int position)
{
  Stepper* s = &steppers[stepper];
  
  chSysDisable();
  s->position = position;
  s->destination = position;
  chSysEnable();

  stepperSetDetails(s);
  return CONTROLLER_OK;
}

/**	
	Set the destination position for a stepper motor.
	This will start the stepper moving the given number of
	steps at the current speed, as set by Stepper_SetSpeed().
	
	While it's moving, you can call Stepper_GetPosition() to read
	its current position.
	@param stepper An integer specifying which stepper (0 or 1).
	@param positionRequested An integer specifying the desired stepper position.
  @return status (0 = OK).
  
  \b Example
	\code
	// start moving stepper 0 1500 steps
	stepperSetDestination(0, 1500);
	\endcode
*/
int stepperSetDestination(int stepper, int positionRequested)
{
  Stepper* s = &steppers[stepper];
  chSysDisable();
  s->destination = positionRequested;
  chSysEnable();

  stepperSetDetails(s);
  return CONTROLLER_OK;
}

/**	
	Set the speed at which a stepper will move.
  This is a number of ms per step, rather than the more common steps per second.  
  Arranging it this way makes it easier to express as an integer.  
  Fastest speed is 1ms / step (1000 steps per second) and slowest is many seconds.
	@param stepper An integer specifying which stepper (0 or 1).
	@param speed An integer specifying the stepper speed in ms per step
  @return status (0 = OK).
  
  \b Example
	\code
	// set the speed to 1ms / step (1000 steps per second)
	stepperSetSpeed(0, 1);
	\endcode
*/
int stepperSetSpeed(int stepper, int speed)
{
  Stepper* s = &steppers[stepper];
  s->speed = speed * 1000;

  chSysDisable();
  fasttimerStop(&s->fastTimer);
  fasttimerStart(&s->fastTimer, s->speed, true);
  chSysEnable();

  stepperSetDetails(s);
  return CONTROLLER_OK;
}

/**	
	Get the speed at which a stepper will move.
	Read the value previously set for the speed parameter.
	@param stepper An integer specifying which stepper (0 or 1).
  @return The speed (0 - 1023), or 0 on error.
  
  \b Example
	\code
	int step0_speed = stepperSpeed(0);
	// now step0_speed has the speed of stepper 0
	\endcode
*/
int stepperSpeed(int stepper)
{
  return steppers[stepper].speed;
}

/**	
	Read the current position of a stepper motor.
	@param index An integer specifying which stepper (0 or 1).
  @return The position, 0 on error.
  
  \b Example
	\code
	int step0_pos = stepperPosition(0);
	// now step0_pos has the current position of stepper 0
	\endcode
*/
int stepperPosition(int index)
{
  return steppers[index].position;
}

/**	
	Read the destination position of a stepper motor.
	This indicates where the stepper is ultimately headed.  To see
	where it actually is, see Stepper_GetPosition().
	@param stepper An integer specifying which stepper (0 or 1).
  @return The position and 0 on error
  
  \b Example
	\code
	int step1_destination = stepperDestination(1);
	// step1_destination has the requested position for stepper 1
	\endcode
*/
int stepperDestination(int stepper)
{
  return steppers[stepper].destination;
}

/**	
	Simply take a number of steps from wherever the motor is currently positioned.
  This function will move the motor a given number of steps from the current position.
	@param stepper Which stepper (0 or 1).
	@param steps An integer specifying the number of steps.  Can be negative to go in reverse.
  @return status (0 = OK).
  
  \b Example
	\code
	// take 1200 steps forward from our current position
	stepperStep(0, 1200);
	\endcode
*/
int stepperStep(int stepper, int steps)
{
  Stepper* s = &steppers[stepper];
  chSysDisable();
  s->destination = (s->position + steps);
  chSysEnable();

  stepperSetDetails(s);
  return CONTROLLER_OK;
}

/**	
	Set the duty - from 0 to 1023.  The default is for 100% power (1023).
	@param stepper Which stepper (0 or 1).
	@param duty An integer specifying the stepper duty (0 - 1023).
  @return status (0 = OK).
  
  \b Example
	\code
	// set stepper 0 to half power
	stepperSetDuty(0, 512);
	\endcode
*/
int stepperSetDuty(int stepper, int duty)
{
  Stepper* s = &steppers[stepper];
  s->duty = duty;

  // Fire the PWM's up
  int pwm = stepper * 2;
  pwmSetDuty(pwm, duty);
  pwmSetDuty(pwm + 1, duty);
  return CONTROLLER_OK;
}

/**	
	Get the duty 
  Read the value previously set for the duty.
	@param stepper Which stepper (0 or 1).
  @return The duty (0 - 1023), or 0 on error.
  
  \b Example
	\code
	int step1_duty = stepperDuty(1);
	// step1_duty has the current duty for stepper 1
	\endcode
*/
int stepperDuty(int stepper)
{
  return steppers[stepper].duty;
}

/**
  Configure whether the motor is in bipolar mode, and whether it's in half-step mode.
  By default it's in unipolar mode, and in full step mode.
  @param stepper An integer specifying which stepper (0 or 1).
  @param bipolar True for bipolar, false for unipolar.
  @param halfstep True for half step, false for full step

  \b Example
  \code
  // set stepper 1 to half step mode
  stepperConfigure(1, NO, YES);
  \endcode
*/
void stepperConfigure(int stepper, bool bipolar, bool halfstep)
{
  steppers[stepper].bipolar = bipolar;
  steppers[stepper].halfStep = halfstep;
}

/** @}
*/

int stepperGetIo(int stepper, int ioIndex)
{
  int io = -1;
  switch (stepper) {
    case 0:
      switch (ioIndex) {
        case 0: io = STEPPER_0_IO_0; break;
        case 1: io = STEPPER_0_IO_1; break;
        case 2: io = STEPPER_0_IO_2; break;
        case 3: io = STEPPER_0_IO_3; break;
      }
      break;
    case 1:
      switch (ioIndex) {
        case 0: io = STEPPER_1_IO_0; break;
        case 1: io = STEPPER_1_IO_1; break;
        case 2: io = STEPPER_1_IO_2; break;
        case 3: io = STEPPER_1_IO_3; break;
      }
      break;
  }
  return io;
}

void stepperIRQCallback(int id)
{
  Stepper* s = &steppers[id];

  if (s->position < s->destination)
    s->position++;
  if (s->position > s->destination)
    s->position--;

  if (s->bipolar) {
    if (s->halfStep)
      stepperSetBipolarHalfStepOutput(s, s->position);
    else
      stepperSetBipolarOutput(s, s->position);
  }
  else {
    if (s->halfStep) 
      stepperSetUnipolarHalfStepOutput(s, s->position);
    else
      stepperSetUnipolarOutput(s, s->position);
  }

  if (s->position == s->destination) {
    fasttimerStop(&s->fastTimer);
    s->timerRunning = false;
  }
}

void stepperSetDetails(Stepper* s)
{
  if (!s->timerRunning && (s->position != s->destination) && (s->speed != 0)) {
    s->timerRunning = true;
    chSysDisable();
    fasttimerStart(&s->fastTimer, s->speed, true);
    chSysEnable();
  }
  else {
    if ((s->timerRunning) && ((s->position == s->destination) || (s->speed == 0))) {
      chSysDisable();
      fasttimerStop(&s->fastTimer);
      chSysEnable();
      s->timerRunning = false;
    }
  }
}

void stepperSetUnipolarHalfStepOutput(Stepper *s, int position)
{
  //int output = position % 8;
  int output = position & 0x7;

  int* iop = s->pins;

  int portAOn = 0;
  int portBOn = 0;
  int portAOff = 0;
  int portBOff = 0;

  switch (output) {
    case -1:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 0:
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 1:
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 2:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 3:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 4:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 5:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);      
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      break;
    case 6:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);      
      stepperSetOff(*iop++, &portAOff, &portBOff);      
      stepperSetOn(*iop++, &portAOn, &portBOn);
      break;
    case 7:
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);      
      stepperSetOff(*iop++, &portAOff, &portBOff);      
      stepperSetOn(*iop++, &portAOn, &portBOn);
      break;
  }  

  stepperSetAll( portAOn, portBOn, portAOff, portBOff );
}

void stepperSetUnipolarOutput(Stepper *s, int position)
{
  //int output = position % 4;
  int output = position & 0x3;
  int* iop = s->pins;

  int portAOn = 0;
  int portBOn = 0;
  int portAOff = 0;
  int portBOff = 0;

  switch (output) {
    case -1:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 0:
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 1:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 2:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 3:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      break;
  }  
  stepperSetAll(portAOn, portBOn, portAOff, portBOff);
}

void stepperSetBipolarHalfStepOutput(Stepper *s, int position)
{
  //int output = position % 8;
  int output = position & 0x7;
  int* iop = s->pins;

  int portAOn = 0;
  int portBOn = 0;
  int portAOff = 0;
  int portBOff = 0;

  switch (output) {
    case -1:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 0:
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 1:
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 2:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 3:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 4:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 5:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      break;
    case 6:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      break;
    case 7:
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);      
      stepperSetOff(*iop++, &portAOff, &portBOff);      
      stepperSetOn(*iop++, &portAOn, &portBOn);
      break;
  }  

  stepperSetAll(portAOn, portBOn, portAOff, portBOff);
}

void stepperSetBipolarOutput(Stepper *s, int position)
{
  //int output = position % 4; // work around % bug - negative numbers not handled properly
  int output = position & 0x3;
  int* iop = s->pins;

  int portAOn = 0;
  int portBOn = 0;
  int portAOff = 0;
  int portBOff = 0;

  // This may be the least efficient code I have ever written
  switch (output) {
    case -1:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 0:
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 1:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      break;
    case 2:
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      break;
    case 3:
      stepperSetOn(*iop++, &portAOn, &portBOn);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOff(*iop++, &portAOff, &portBOff);
      stepperSetOn(*iop++, &portAOn, &portBOn);
      break;
  }  
  stepperSetAll(portAOn, portBOn, portAOff, portBOff);
}

void stepperSetOn(int stepper, int* portAOn, int* portBOn)
{
  if (stepper < 32)
    *portAOn |= (1 << (stepper & 0x1F));
  else
    *portBOn |= (1 << (stepper & 0x1F));
}

void stepperSetOff(int stepper, int* portAOff, int* portBOff)
{
  if (stepper < 32)
    *portAOff |= (1 << (stepper & 0x1F));
  else
    *portBOff |= (1 << (stepper & 0x1F));
}

void stepperSetAll(int portAOn, int portBOn, int portAOff, int portBOff)
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


