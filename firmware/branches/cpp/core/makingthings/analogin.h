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

/*
	adc.h

  MakingThings
*/

#ifndef ANALOGIN_H
#define ANALOGIN_H

#include "rtos_.h"
#include "AT91SAM7X256.h"

class AnalogIn
{
public:
  AnalogIn(int channel);
  ~AnalogIn();
  int value();
  int valueWait( );
  static bool multi(int values[]);

protected:
  int index;
  int getIo( int index );
  int managerInit();

  friend void AnalogIn_Isr();

  typedef struct {
    Semaphore semaphore;
    Semaphore doneSemaphore;
    bool initialized;
  } Manager;
  static Manager manager;
};

///* OSC Interface */
//const char* AnalogInOsc_GetName( void );
//int AnalogInOsc_ReceiveMessage( int channel, char* message, int length );
//int AnalogInOsc_Async( int channel );

#endif
