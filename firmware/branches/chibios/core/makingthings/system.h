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

#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

/** 
  Monitors and controls several aspects of the system. 
*/

#ifdef __cplusplus
extern "C" {
#endif
const char* systemName(void);
int  systemSetName(const char* name);
void systemReset(bool sure);
void systemSamba(bool sure);
int  systemSerialNumber(void);
int  systemSetSerialNumber(int serial);
int  systemFreeMemory(void);
#ifdef __cplusplus
}
#endif

#ifdef OSC
#include "osc.h"
extern const OscNode systemOsc;
#endif

#endif
