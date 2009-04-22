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

#ifndef DIGITALOUT_H
#define DIGITALOUT_H

#include "io.h"

/**
  Control the 8 high current outputs on the Application Board.
  
  \section Usage
  To get started using the DigitalOuts, create a DigitalOut object then set and read its value
  as desired.
  \code
  DigitalOut dout(4); // create a DigitalOut object, controlling channel 4
  dout.setValue(true); // turn it on
  bool is_it_on = dout.value(); // check if it's on or not
  \endcode
  
  \section Notes
  If you're simultaneously using any of the other systems on the outputs (Stepper, Motor, etc.), results will
  be unpredictable since they're using the same signals.  Make sure to only use one system at a time on a given
  output signal.
  
  See the <a href="http://www.makingthings.com/documentation/tutorial/application-board-overview/digital-outputs">
  Digital Out section</a> of the Application Board overview for more details.
  
  \ingroup io
*/
class DigitalOut
{
public:
  DigitalOut(int index);
  ~DigitalOut();

  bool setValue(bool on);
  bool value();

protected:
  short _index;
  int getIo( int index );
  int getEnableIo( int enableIndex );

  static Io* ios[];
  static short refcounts[];
};

/* OSC Interface */
const char* DigitalOutOsc_GetName( void );
int DigitalOutOsc_ReceiveMessage( int channel, char* message, int length );


#endif
