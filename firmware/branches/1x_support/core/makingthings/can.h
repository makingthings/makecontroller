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
	can.h

  MakingThings
*/

#ifndef CAN_H
#define CAN_H

int Can_SetActive( int state );
int Can_GetActive( void );

int Can_SendMessage( int id, char* message, int length );
int Can_GetMessage( int* id, char* message, int* length );

/* OSC Interface */
const char* CanOsc_GetName( void );
int CanOsc_ReceiveMessage( int channel, char* message, int length );

#endif
