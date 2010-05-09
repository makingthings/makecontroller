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

#ifndef STEPPER_H
#define STEPPER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void stepperEnable(int stepper);
void stepperDisable(int stepper);
int  stepperResetPosition(int stepper, int position);
int  stepperPosition(int stepper);
int  stepperSetDestination(int stepper, int destination);
int  stepperDestination(int stepper);
int  stepperSetDuty(int stepper, int duty);
int  stepperDuty(int stepper);
void stepperConfigure(int stepper, bool bipolar, bool halfstep);
int  stepperSetSpeed(int stepper, int speed);
int  stepperSpeed(int stepper);
int  stepperStep(int stepper, int steps);
#ifdef __cplusplus
}
#endif

#endif // STEPPER_H
