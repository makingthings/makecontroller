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

#include "usb_enum.h"
#include "ext.h"
#include <stdio.h>

#ifdef WIN32
#include <initguid.h>
DEFINE_GUID( GUID_MAKE_CTRL_KIT, 0x4D36E978, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 );
#endif

bool findUsbDevice( t_usbInterface* usbInt, int devicetype )
{
  bool retval = false;
  
#ifdef WIN32 // Windows only

  SP_DEVICE_INTERFACE_DATA ifcData;
  DWORD detDataPredictedLength;
  HDEVINFO devInfo = INVALID_HANDLE_VALUE;
  GUID * guidDev = (GUID *) & GUID_MAKE_CTRL_KIT;
  DWORD i;
  
  bool ok = true;
  PSP_DEVICE_INTERFACE_DETAIL_DATA detData = NULL;
  ifcData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

  devInfo = SetupDiGetClassDevs(guidDev, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  if(devInfo == INVALID_HANDLE_VALUE) {
    error("SetupDiGetClassDevs failed. Error code: %d", GetLastError());
    return false;
  }

  for ( i = 0; ok; i++)
  {
    ok = SetupDiEnumDeviceInterfaces(devInfo, NULL, guidDev, i, &ifcData);
    if (ok)
    {
      SP_DEVINFO_DATA devData = {sizeof(SP_DEVINFO_DATA)};
      //check for required detData size
      SetupDiGetDeviceInterfaceDetail(devInfo, & ifcData, NULL, 0, &detDataPredictedLength, & devData);
      detData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(detDataPredictedLength);
      detData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

      //check the details
      if (SetupDiGetDeviceInterfaceDetail(devInfo, &ifcData, detData, detDataPredictedLength, NULL, & devData))
      {
        static char portName[512];
        static char friendName[512];
        HKEY devKey;
        bool gotdevice = false;
        if( !getDeviceProperty(friendName, 1024, devInfo, &devData, SPDRP_FRIENDLYNAME) )
          error("getDeviceProperty friendly name failed" );
        devKey = SetupDiOpenDevRegKey(devInfo, &devData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if( !getRegKeyValue(portName, 1024, devKey, TEXT("PortName")) )
          error("getRegKeyValue portName failed");

        if( devicetype == FIND_MAKE_CONTROLLER && !strncmp( friendName, "Make Controller Ki", 18) )
          gotdevice = true;
        else if( devicetype == FIND_TELEO && !strncmp( friendName, "USB <-> Serial", 14) )
          gotdevice = true;

        if( gotdevice)
        {
          if( getPortNumber(portName) > 9 )
            sprintf(usbInt->deviceLocation, "\\\\.\\%s", portName);
          else
            strcpy(usbInt->deviceLocation, portName);
          retval = true;
        }
      }
      else
        error("SetupDiGetDeviceInterfaceDetail failed. Error code: %d", GetLastError());

      free( detData );
    }
    else if (GetLastError() != ERROR_NO_MORE_ITEMS)
    {
      error("SetupDiEnumDeviceInterfaces failed. Error code: %d", GetLastError());
      return false;
    }
  }
  SetupDiDestroyDeviceInfoList(devInfo);

#else ifndef WIN32  // Windows-only FindUsbDevices( )
  //--------------------------------------- Mac-only -------------------------------
  io_object_t modemService;
  io_iterator_t iterator = 0;

  // create a dictionary that looks for all BSD modems
  CFMutableDictionaryRef matchingDictionary = IOServiceMatching( kIOSerialBSDServiceValue );
  if (matchingDictionary == NULL)
    return false;
  else
    CFDictionarySetValue(matchingDictionary, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDModemType));

  // then create the iterator with all the matching devices
  if( IOServiceGetMatchingServices( kIOMasterPortDefault, matchingDictionary, &iterator ) != KERN_SUCCESS )
    return false;

  // Iterate through all modems found. In this example, we bail after finding the first modem.
  while( (modemService = IOIteratorNext(iterator)) && !retval )
  {
    CFTypeRef bsdPathAsCFString = NULL;
    CFTypeRef productNameAsCFString = NULL;
    char devicePath[MAXPATHLEN];
    char productName[MAXPATHLEN];

    // check the name of the modem's callout device
    bsdPathAsCFString = IORegistryEntrySearchCFProperty(modemService, kIOServicePlane, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0);
    // then, because the callout device could be any old thing, and because the reference to the modem returned by the
    // iterator doesn't include much device specific info, look at its parent, and check the product name
    io_registry_entry_t parent;
    if( IORegistryEntryGetParentEntry( modemService,	kIOServicePlane, &parent ) == KERN_SUCCESS )
    {
      productNameAsCFString = IORegistryEntrySearchCFProperty(parent, kIOServicePlane, CFSTR("Product Name"), kCFAllocatorDefault, 0);
      IOObjectRelease(parent);
    }

    if( bsdPathAsCFString )
    {   
      CFStringGetCString((CFStringRef)bsdPathAsCFString, devicePath, PATH_MAX, kCFStringEncodingUTF8);
      CFRelease(bsdPathAsCFString);
    }

    if(productNameAsCFString)
    {
      bool gotdevice = false;
      CFStringGetCString((CFStringRef)productNameAsCFString, productName, PATH_MAX, kCFStringEncodingUTF8);

      if( devicetype == FIND_MAKE_CONTROLLER && !strncmp( productName, "Make Controller Ki", 18) )
        gotdevice = true;
      else if( devicetype == FIND_TELEO && !strncmp( productName, "USB <-> Serial", 14) )
        gotdevice = true;

      if( gotdevice )
      {
        strcpy( usbInt->deviceLocation, devicePath );
        retval = true;
      }
      CFRelease(productNameAsCFString);
    }
    IOObjectRelease(modemService);
  }
  IOObjectRelease(iterator);
#endif // WIN32
  return retval;
}
#ifdef WIN32

bool getRegKeyValue(char* buf, int len, HKEY key, LPCTSTR property)
{
	DWORD size = 0;
  bool retval = false;
	RegQueryValueEx(key, property, NULL, NULL, NULL, & size);
  if( (DWORD)len >= size )
  {
	  if (RegQueryValueEx(key, property, NULL, NULL, buf, & size) == ERROR_SUCCESS)
      retval = true;
	  else
      error("getRegKeyValue: can not obtain value from registry");
  }
	RegCloseKey(key);
  return retval;
}

int getPortNumber( char* portName )
{
  while( !isdigit(*portName) )
    portName++;
  return atoi(portName);
}

bool getDeviceProperty(char* buf, int len, HDEVINFO devInfo, PSP_DEVINFO_DATA devData, DWORD property)
{
	DWORD buffSize = 0;
  bool result = false;
	SetupDiGetDeviceRegistryProperty(devInfo, devData, property, NULL, NULL, 0, & buffSize);
  if( (DWORD)len >= buffSize )
  {
    if (SetupDiGetDeviceRegistryProperty(devInfo, devData, property, NULL, buf, buffSize, NULL))
      result = true;
    else
		  error("Can not obtain property: %d from registry", property); 
  }
	return result;
}

#endif // WIN32


