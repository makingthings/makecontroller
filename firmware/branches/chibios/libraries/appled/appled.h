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

#ifndef APPLED_H
#define APPLED_H

#include "config.h"
#include "types.h"

#ifdef OSC
#include "osc_cpp.h"

class AppLedOSC : public OscHandler
{
public:
  AppLedOSC( ) { }
  int onNewMsg( OscTransport t, OscMessage* msg, int src_addr, int src_port );
  int onQuery( OscTransport t, char* address, int element );
  const char* name( ) { return "appled"; }
  static const char* propertyList[];
};
#endif // OSC

/**
  Status LEDs for program feedback.
  App LEDs (Application Board LED) are great for providing some information about how your
  program is running.  
*/

void appledEnable(int channel);
void appledSetValue(int channel, bool onff);
bool appledValue(int channel);

#endif // APPLED_H

