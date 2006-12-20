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
	usb.h

*/

#ifndef USB_H
#define USB_H

#include "config.h"

int Usb_SetActive( int state );
int Usb_GetActive( void );

int Usb_Put( int c, int timeout );
int Usb_Get( int timeout );

/**
 */
int Usb_SlipSend( char* buffer, int length );

/* 
 */
int Usb_SlipReceive( char* buffer, int length );

#ifdef OSC

/* OSC Interface */
const char* UsbOsc_GetName( void );
int UsbOsc_ReceiveMessage( int channel, char* message, int length );

#endif // OSC

#endif
