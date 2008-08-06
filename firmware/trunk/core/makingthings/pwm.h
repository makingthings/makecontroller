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
	PWM.h

  MakingThings
*/

#ifndef PWM_H
#define PWM_H

int Pwm_Start( int channel );
int Pwm_Stop( int channel );
int Pwm_Set( int index, int duty );
int Pwm_Get( int index );

int Pwm_SetDividerA(int val);
int Pwm_GetDividerA( void );

int Pwm_SetDividerB(int val);
int Pwm_GetDividerB( void );

int Pwm_SetClockSource(int channel, int val);
int Pwm_GetClockSource(int channel);

int Pwm_SetWaveformProperties(int channel, int val);
int Pwm_GetWaveformProperties(int channel);


#endif
