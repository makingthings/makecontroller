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

#ifndef USB_SERIAL_H
#define USB_SERIAL_H

typedef const char cchar;

//--------------------------------------- Mac-only -------------------------------
#ifndef WIN32

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
// for scandir
#include <sys/types.h>
#include <dirent.h>

#endif

//--------------------------------------- Windows-only -------------------------------
#ifdef WIN32
typedef unsigned char bool;
#include <windows.h>
#include <tchar.h>
#include <ctype.h>
#endif

//mcUsb structure
typedef struct
{
  bool deviceOpen;
  bool readInProgress;
  
  #ifdef WIN32
  HANDLE deviceHandle;
  OVERLAPPED overlappedRead;
  char readBuffer[512];
  OVERLAPPED overlappedWrite;
  #endif
	
  #ifndef WIN32
  bool blocking;
  int deviceHandle;
  cchar* deviceName;
  #endif
	
} t_usbInterface;

//function prototypes
t_usbInterface* usb_init( cchar* name, t_usbInterface** uip );
int usb_open( t_usbInterface* usbInt );
void usb_close( t_usbInterface* usbInt );
int usb_read( t_usbInterface* usbInt, char* buffer, int length );
int usb_write( t_usbInterface* usbInt, char* buffer, int length );
int usb_writeChar( t_usbInterface* usbInt, char c );

// Mac-only
#ifndef WIN32
// usb function prototypes
int FindUsbSerialDevice( cchar** dest, int index );
static int matchUsbSerialDevice( struct dirent * tryThis );
#endif

//Windows only
#ifdef WIN32
LONG QueryStringValue(HKEY hKey,LPCTSTR lpValueName,LPTSTR* lppStringValue);
LONG OpenSubKeyByIndex(HKEY hKey,DWORD dwIndex,REGSAM samDesired,PHKEY phkResult);
int ScanEnumTree( t_usbInterface* usbInt, LPCTSTR lpEnumPath, TCHAR** openPorts );
int testOpen( t_usbInterface* usbInt, TCHAR* deviceName );
int openDevice( t_usbInterface* usbInt, TCHAR* deviceName );
#endif

#endif
