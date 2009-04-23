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


#ifndef STEPPER_H
#define STEPPER_H

#include "fasttimer.h"
#include "pwm.h"
#include "io.h"

#define STEPPER_COUNT 2

/**
  Provides speed and position control for one or two stepper motors.
  Up to 2 stepper motors can be controlled with the Make Application Board.  Stepper 0 uses
  digital outs 0-3 and stepper 1 uses digital outs 4-7.
  
  \section Usage
  To get started using the Stepper system, create a stepper object specifying which stepper you'd
  like to control, 0 or 1.  You can then drive it around using the step() and stepTo() methods
  as described below.  You can also specify whether the motor is bipolar() or should be in
  halfStep() mode.
  
  \code
  Stepper s(0); // create an object for Stepper 0
  
  s.step(100); // take 100 steps
  int pos = s.position(); // should be 100
  s.stepTo(150); // should only take 50 more steps
  \endcode
  
  \section Positioning
  You can generally use the stepper motor in 2 modes - \b absolute positioning or \b relative positioning.

  For absolute positioning, call stepTo( ) with the desired position, and the motor will move there.
  You can read back the stepper's current position at any point along the way with the position() method.
  The board keeps an internal count of how many steps the motor has taken in order to keep track of 
  where it is.

  For relative positioning, use step( ) to simply move a number of steps from the current position.
  
  \section moreinfo More Info
  See the <a href="http://www.makingthings.com/documentation/how-to/stepper-motor">Stepper Motor how-to</a>
  for more detailed info on hooking up a stepper motor to the Make Controller.
  \ingroup io
*/
class Stepper
{
  public:
    Stepper(int index);
    ~Stepper();

    int position();
    int destination();
    bool stepTo(int pos);
    bool step(int steps);
    bool resetPosition(int pos = 0);

    int duty();
    bool setDuty(int duty);

    bool bipolar();
    bool setBipolar(bool bipolar);

    bool halfStep();
    bool setHalfStep(bool halfStep);

    int speed();
    bool setSpeed(int speed);

  protected:
    int _index;
    typedef struct
    {
      bool bipolar;
      bool halfStep;
      int speed;
      int duty;
      int destination;
      int position;
      Io* ios[ 4 ];
      bool timerRunning;
      int refcount;
      FastTimer fastTimer;
      Pwm* pwmA;
      Pwm* pwmB;

      void setUnipolarHalfStepOutput( int position );
      void setUnipolarOutput( int position );

      void setBipolarHalfStepOutput( int position );
      void setBipolarOutput( int position );

      void setOn( int index, int* portAOn, int* portBOn );
      void setOff( int index, int* portAOff, int* portBOff );
      void setAll( int portAOn, int portBOn, int portAOff, int portBOff );
    } StepperInternal;
    
    friend void Stepper_IRQCallback( int id );
    static StepperInternal* steppers[];

    int getIo( int io );
    void setDetails( );
};

/* OSC Interface */
const char* StepperOsc_GetName( void );
int StepperOsc_ReceiveMessage( int channel, char* message, int length );


#endif
