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

#ifndef PWM_H
#define PWM_H

#include "types.h"

#define PWM_COUNT 4

#ifdef __cplusplus
extern "C" {
#endif
void pwmInit(void);
void pwmDeinit(void);
bool pwmSetFrequency(int freq);
bool pwmEnableChannel( int channel );
void pwmDisableChannel( int channel );
void pwmSetDuty( int channel, int duty );
void pwmSetWaveform( int channel, bool left_aligned, bool starts_low );
bool pwmSetPeriod( int channel, int period );
#ifdef __cplusplus
}
#endif
#endif // PWM_H
