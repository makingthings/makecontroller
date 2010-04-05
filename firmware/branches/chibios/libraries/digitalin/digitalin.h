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

#include "types.h"

/**
  Read the 8 inputs on the Application Board as digital values - on or off.
  
  \section Usage
  Because the digitalin system relies on some analogins, ainInit() must be called before
  you can use the digital ins.
  To get started reading a DigitalIn, create a DigitialIn object and call its value() method.
  \code
  DigitalIn din(3);
  bool din3_is_on = din.value();
  if(din3_is_on)
  {
    // then do this
  }
  else
  {
    // then do that
  }
  \endcode
  
  \section Notes
  Internally, the 8 inputs on the Application Board consist of 4 dedicated analog inputs, and 4 lines which can
  be configured either as digitial ins or outs. Because digital ins 4-7 are always AnalogIn lines, there's no 
  performance gain to reading those as DigitalIns as opposed to AnalogIns.
  
  \ingroup io
*/
#ifdef __cpluscplus
extern "C" {
#endif
bool digitalinValue(int channel);
/* OSC Interface */
const char* DigitalInOsc_GetName( void );
int DigitalInOsc_ReceiveMessage( int channel, char* message, int length );
#ifdef __cpluscplus
extern "C" {
#endif
#endif // DIGITALIN_H
