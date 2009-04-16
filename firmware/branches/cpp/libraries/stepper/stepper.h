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
  \ingroup Libraries
*/
class Stepper
{
  public:
    Stepper(int index);
    ~Stepper();

    int position();
    int destination();
    int stepTo(int pos);
    bool step(int steps);
    bool resetPosition(int pos = 0);

    int duty();
    bool setDuty(int duty);

    bool bipolar();
    bool setBipolar(bool bipolar);

    bool halfStep();
    bool setHalfStep(bool halfstep);

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
