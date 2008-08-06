/*********************************************************************************

 Copyright 2006-2008 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

/*
	motor.h

  Includes, etc. so make users don't have to

  MakingThings
*/

#ifndef MOTOR_H
#define MOTOR_H

#include "controller.h"

int Motor_SetActive( int index, int state );
int Motor_GetActive( int index );

int Motor_SetSpeed( int motor, int duty );
int Motor_SetDirection( int motor, char forward );
int Motor_GetSpeed( int motor );
int Motor_GetDirection( int motor );

/* OSC Interface */
const char* MotorOsc_GetName( void );
int MotorOsc_ReceiveMessage( int channel, char* message, int length );

#endif
