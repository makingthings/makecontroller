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

#ifndef SERVO_H
#define SERVO_H

#ifdef __cplusplus
extern "C" {
#endif
void servoInit(void);
void servoDeinit(void);
void servoEnable(int index);
int servoSetPosition(int index, int position);
int servoPosition(int index);
int servoSetSpeed(int index, int speed);
int servoGetSpeed(int index);
#ifdef __cplusplus
}
#endif

#endif // SERVO_H

