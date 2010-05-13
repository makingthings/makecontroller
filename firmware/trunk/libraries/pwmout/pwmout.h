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

#include "pwm.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void pwmoutEnable(int channel);
void pwmoutDisable(int channel);
void pwmoutSetDuty(int channel, int duty);
bool pwmoutInvertedA(int channel);
bool pwmoutSetInvertedA(int channel, bool invert);
bool pwmoutInvertedB(int channel);
bool pwmoutSetInvertedB(int channel, bool invert);
bool pwmoutSetAll(int channel, int duty, bool invertA, bool invertB);
#ifdef __cplusplus
}
#endif
/* OSC Interface */
// const char* PwmOutOsc_GetName( void );
// int PwmOutOsc_ReceiveMessage( int channel, char* message, int length );

#endif
