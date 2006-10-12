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
	stepper.h

  MakingThings
*/

#ifndef STEPPER_H
#define STEPPER_H

void Stepper_SetPosition( int index, int position );
int  Stepper_GetPosition( int index );
void Stepper_SetSpeed( int index, int speed );
int  Stepper_GetSpeed( int index );
void Stepper_SetAcceleration( int index, int acceleration );
int  Stepper_GetAcceleration( int index );

#endif
