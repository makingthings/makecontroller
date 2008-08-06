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
	servo.h

  MakingThings
*/

#ifndef SERVO_H
#define SERVO_H

int Servo_SetActive( int index, int state );
int Servo_GetActive( int index );

int Servo_SetPosition( int index, int position );
int Servo_GetPosition( int index );

int Servo_SetSpeed( int index, int speed );
int Servo_GetSpeed( int index );

/* OSC Interface */
const char* ServoOsc_GetName( void );
int ServoOsc_ReceiveMessage( int channel, char* message, int length );

#endif
