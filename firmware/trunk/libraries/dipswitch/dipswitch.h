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
	dipswitch.h

  MakingThings
*/

#ifndef DIPSWITCH_H
#define DIPSWITCH_H

/* DipSwitch Interface */
#include "types.h"

int DipSwitch_SetActive( int state );
int DipSwitch_GetActive( void );

int DipSwitch_GetValue( void );
bool DipSwitch_GetValueChannel( int channel );

bool DipSwitch_GetAutoSend( bool init );
void DipSwitch_SetAutoSend( int onoff );

/* DipSwitchOsc Interface */

const char* DipSwitchOsc_GetName( void );
int DipSwitchOsc_ReceiveMessage( int channel, char* message, int length );
int DipSwitchOsc_Async( int channel );

#endif
