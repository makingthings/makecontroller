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

#ifndef USB_SERIAL_H
#define USB_SERIAL_H

//--------------------------------------- Mac-only -------------------------------
#ifndef WIN32

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>

//IOKit biz for USB connection
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <CoreFoundation/CFNumber.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/IOMessage.h>
#endif // Mac-only includes

//--------------------------------------- Windows-only -------------------------------
#ifdef WIN32
typedef unsigned char bool;
#include <windows.h>
#include <tchar.h>
#include <ctype.h>
#include <dbt.h>
#endif // Win-only includes

//mcUsb structure
typedef struct
{
  bool deviceOpen;
  bool debug;
  char deviceLocation[ 512 ];
  
  #ifdef WIN32
  HANDLE deviceHandle;
  #endif
	
  #ifndef WIN32
  int deviceHandle;
  #endif
	
} t_usbInterface;

//function prototypes
t_usbInterface* usb_init( t_usbInterface** uip );
int usb_open( t_usbInterface* usbInt, int devicetype );
void usb_close( t_usbInterface* usbInt );
int usb_read( t_usbInterface* usbInt, char* buffer, int length );
int usb_write( t_usbInterface* usbInt, char* buffer, int length );
void usb_flush( t_usbInterface* usbInt );
int usb_writeChar( t_usbInterface* usbInt, char c );
int usb_numBytesAvailable( t_usbInterface* usbInt );

//Windows only
#ifdef WIN32
int openDevice( t_usbInterface* usbInt );
void* formatErrorMsg( );
#endif

typedef enum 
{
  USB_OK                  = 0,
	USB_E_ALREADY_OPEN      = -1,
	USB_E_NOT_OPEN          = -2,
  USB_E_IOERROR           = -3,
  USB_E_CLOSE             = -4,
	USB_E_NOTHING_AVAILABLE = -5
} MaxUsbError;

#endif // USB_SERIAL_H

