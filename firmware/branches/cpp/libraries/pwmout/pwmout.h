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

#ifndef PWMOUT_H
#define PWMOUT_H

#include "io.h"
#include "pwm.h"

/**
	The PWM Out subsystem underlies the DC Motor subsystem and controls the 4 PWM signals on the SAM7X.
	Each PWM device controls a pair of Digital Outs - an A and a B channel:
	- PwmOut 0 - Digital Outs 0 (A) and 1 (B).
	- PwmOut 1 - Digital Outs 2 (A) and 3 (B).
	- PwmOut 2 - Digital Outs 4 (A) and 5 (B).
	- PwmOut 3 - Digital Outs 6 (A) and 7 (B).
	
	The A and B channels of a PWM device can be set independently to be inverted, or not, from one another
	in order to control motors, lights, etc.
	
	The simplest way to get started is simply with a call to PwmOut_SetActive() and then to 
	PwmOut_SetDuty() - this will allow you to control simple dimming and motors.  If you need to adjust timing, 
	inversion or other parameters, delve a bit deeper into the API above.
	
	PwmOut relies on the internal \ref Pwm system.  See that page for more info on the timing issues 
	involved with the PWM Out system.
	\ingroup Libraries
*/
class PwmOut
{
  public:
    PwmOut(int index);
    ~PwmOut();

    int duty();
    bool setDuty(int duty);

    bool invertedA();
    bool setInvertedA(bool invert);

    bool invertedB();
    bool setInvertedB(bool invert);

    bool setAll( int duty, bool invertA, bool invertB );

  protected:
    int _index;
    typedef struct {
      Io* ioA;
      Io* ioB;
      Pwm* pwm;
      short count;
    } PwmOutInternal;
    static PwmOutInternal* pwmouts[]; // only ever want to make 4 of these
    
    void getIos( int* ioA, int* ioB );
};

/* OSC Interface */
const char* PwmOutOsc_GetName( void );
int PwmOutOsc_ReceiveMessage( int channel, char* message, int length );

#endif
