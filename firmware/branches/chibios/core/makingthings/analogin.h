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

#ifndef ANALOGIN_H
#define ANALOGIN_H

#define ANALOGIN_CHANNELS 8

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void ainInit(void);
void ainDeinit(void);
int  ainValue(int channel);
int  ainValueWait(int channel);
bool ainMulti(int values[]);
#ifdef __cplusplus
}
#endif

///* OSC Interface */
//const char* AnalogInOsc_GetName( void );
//int AnalogInOsc_ReceiveMessage( int channel, char* message, int length );
//int AnalogInOsc_Async( int channel );

#endif
