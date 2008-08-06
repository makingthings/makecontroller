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
	debug.h

*/

#ifndef DEBUG_H
#define DEBUG_H

/**
\ingroup Debug
@{
*/

#define DEBUG_ALWAYS  0 /**< The lowest debug level, used for messages that should go out no matter what.*/
#define DEBUG_ERROR   1 /**< Used for critical/fatal error messages. */
#define DEBUG_WARNING 2 /**< Used for warnings. */
#define DEBUG_MESSAGE 3 /**< Used for normal/test messages.  */

/* @} */

int Debug_SetActive( int state );
int Debug_GetActive( void );

int Debug_SetLevel( int level );
int Debug_GetLevel( void );

int Debug_SetUsb( int usb );
int Debug_GetUsb( void );

int Debug_SetUdp( int udp );
int Debug_GetUdp( void );

int Debug( int level, char* string, ... );

const char* DebugOsc_GetName( void );
int DebugOsc_ReceiveMessage( int channel, char* message, int length );

#endif


