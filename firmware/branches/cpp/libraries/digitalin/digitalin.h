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

#ifndef DIGITALIN_H
#define DIGITALIN_H

#include "io.h"
#include "analogin.h"

/**
  Read the 8 inputs on the Make Controller as digital values - 1 (on) or 0 (off).  
  The 8 inputs can be either read as digital inputs by DigitalIn or as \ref AnalogIn.  
  Because inputs 4-7 are actually AnalogIn lines, there's no performance gain to reading
  those as DigitalIns.
*/
class DigitalIn
{
public:
  DigitalIn(int index);
  ~DigitalIn();

  bool value();

protected:
  short _index;
  int getIo( int index );
  static AnalogIn* ains[];
  static Io* ios[];
  static short refcounts[];
};

/* OSC Interface */
const char* DigitalInOsc_GetName( void );
int DigitalInOsc_ReceiveMessage( int channel, char* message, int length );


#endif
