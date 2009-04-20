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
  
  If you've previously used any of the other systems on the outputs (steppers, motors, etc.), you'll need
  to set them to \b inactive to unlock the IO lines and use the Digital Outs.
  
  See the <a href="http://www.makingthings.com/documentation/tutorial/application-board-overview/digital-outputs">
  Digital Out section</a> of the Application Board overview for more details.
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
