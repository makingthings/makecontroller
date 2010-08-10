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

#include "core.h"

#ifdef OSC
#include "osc.h"
extern const OscNode appledOsc;
#endif

#ifdef __cplusplus
extern "C" {
#endif
void appledInit(void);
void appledSetValue(int led, bool onff);
bool appledValue(int led);
#ifdef __cplusplus
}
#endif
#endif // APPLED_H

