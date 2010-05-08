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

#ifndef MOTOR_H
#define MOTOR_H

#include "types.h"

#define FORWARD true   /**< */
#define REVERSE false

#ifdef __cplusplus
extern "C" {
#endif
void motorEnable(int motor);
void motorDisable(int motor);
int  motorSpeed(int motor);
bool motorSetSpeed(int motor, int duty);
bool motorDirection(int motor);
bool motorSetDirection( int motor, bool forward );
#ifdef __cplusplus
}
#endif

#ifdef OSC
#include "osc.h"
extern const OscNode motorOsc;
#endif

#endif
