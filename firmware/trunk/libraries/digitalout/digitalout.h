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
	digitalout.h

  MakingThings
*/

#ifndef DIGITALOUT_H
#define DIGITALOUT_H

int DigitalOut_SetActive( int index, int state );
int DigitalOut_GetActive( int index );

int DigitalOut_SetValue( int index, int value );
int DigitalOut_GetValue( int index );
int DigitalOut_SetAll( int value );

/* OSC Interface */
const char* DigitalOutOsc_GetName( void );
int DigitalOutOsc_ReceiveMessage( int channel, char* message, int length );


#endif
