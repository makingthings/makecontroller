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
	led.h

*/

#ifndef LED_H
#define LED_H

int Led_SetActive( int state );
int Led_GetActive( void );

int Led_SetState( int state );
int Led_GetState( void );

/* OSC Interface */
const char* LedOsc_GetName( void );
int LedOsc_ReceiveMessage( int channel, char* message, int length );

#endif
