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

#ifndef USB_ENUM_H_
#define USB_ENUM_H_

#include "usb_serial.h"

#ifdef WIN32
typedef unsigned char bool;
#include <windows.h>
#include <tchar.h>
#include <ctype.h>
#include <dbt.h>
#include <setupapi.h>
#else ifndef WIN32
#include <sys/param.h>
#endif

bool findUsbDevice( t_usbInterface* usbInt );

#ifdef WIN32
HANDLE GetDeviceInfo( HDEVINFO HardwareDeviceInfo, PSP_INTERFACE_DEVICE_DATA DeviceInfoData, char* portName );
bool checkFriendlyName( HDEVINFO HardwareDeviceInfo, PSP_DEVINFO_DATA deviceSpecificInfo, char* portName );
#endif

#endif // usb_enum_H_






