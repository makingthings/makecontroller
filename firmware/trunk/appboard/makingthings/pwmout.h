/*********************************************************************************

 Copyright 2006 MakingThings

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
	pwmout.h

  Includes, etc. so make users don't have to

  MakingThings
*/

#ifndef PWMOUT_H
#define PWMOUT_H

#include "controller.h"


int PwmOut_SetActive( int index, int state );
int PwmOut_GetActive( int index );

int PwmOut_SetDuty( int motor, int duty );
int PwmOut_GetDuty( int motor );

int PwmOut_SetInvertA( int motor, char invertA);
int PwmOut_GetInvertA( int motor );

int PwmOut_SetInvertB( int motor, char invertB );
int PwmOut_GetInvertB( int motor );

int PwmOut_SetInvert( int index, char invertA, char invertB );
int PwmOut_SetAll( int motor, int duty, char invertA, char invertB );

/* OSC Interface */
const char* PwmOutOsc_GetName( void );
int PwmOutOsc_ReceiveMessage( int channel, char* message, int length );

#endif
