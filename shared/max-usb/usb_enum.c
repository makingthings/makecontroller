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

bool findUsbDevice( t_usbInterface* usbInt )
{
	bool retval = false;
	#ifdef WIN32 // Windows only
	HANDLE hOut;
  HDEVINFO                 hardwareDeviceInfo;
  SP_INTERFACE_DEVICE_DATA deviceInfoData;
  ULONG                    i = 0;
  
  // Open a handle to the plug and play dev node.
  // SetupDiGetClassDevs() returns a device information set that contains info on all
  // installed devices of a specified class.
  hardwareDeviceInfo = SetupDiGetClassDevs (
                         (LPGUID)&GUID_MAKE_CTRL_KIT,
                         NULL,            // Define no enumerator (global)
                         NULL,            // Define no
                         (DIGCF_PRESENT | // Only Devices present
                         DIGCF_INTERFACEDEVICE)); // Function class devices.

  deviceInfoData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
  
  while( true ) 
  {
	  if(SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
	                                      0, // We don't care about specific PDOs
	                                      (LPGUID)&GUID_MAKE_CTRL_KIT,
	                                      i++,
	                                      &deviceInfoData))
	  {
	  	char portName[ 8 ];
	  	hOut = GetDeviceInfo( hardwareDeviceInfo, &deviceInfoData, portName );
	    if(hOut != INVALID_HANDLE_VALUE && portName != NULL )
	    {
				usbInt->deviceHandle = hOut;
        sprintf( usbInt->deviceLocation, portName );
				retval = true;
        break;
	    }
	  }
	  else 
	  {
	    if(ERROR_NO_MORE_ITEMS == GetLastError()) 
	       break;
	  }
  }
  // destroy the device information set and free all associated memory.
  SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
  
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
    
    if(productNameAsCFString && bsdPathAsCFString)
    {
      char productName[MAXPATHLEN];
      char devicePath[MAXPATHLEN];
      CFStringGetCString((CFStringRef)bsdPathAsCFString, devicePath, MAXPATHLEN, kCFStringEncodingUTF8);
      CFStringGetCString((CFStringRef)productNameAsCFString, productName, MAXPATHLEN, kCFStringEncodingUTF8);
      if( !strncmp( productName, "Make Controller Ki", 18) )
      {
        strcpy( usbInt->deviceLocation, devicePath );
        retval = true;
      }
      CFRelease(productNameAsCFString);
      CFRelease(bsdPathAsCFString);
    }
    IOObjectRelease(modemService);
  }
  IOObjectRelease(iterator);
  #endif // WIN32
  return retval;
}
#ifdef WIN32
HANDLE GetDeviceInfo(HDEVINFO HardwareDeviceInfo, PSP_INTERFACE_DEVICE_DATA DeviceInfoData, char* portName  )
{
  PSP_INTERFACE_DEVICE_DETAIL_DATA functionClassDeviceData = NULL;
  ULONG                            predictedLength = 0;
  ULONG                            requiredLength = 0;
	SP_DEVINFO_DATA deviceSpecificInfo;
  
  // allocate a function class device data structure to receive the
  // goods about this particular device.
  SetupDiGetInterfaceDeviceDetail(HardwareDeviceInfo,
                                  DeviceInfoData,
                                  NULL,  // probing so no output buffer yet
                                  0,     // probing so output buffer length of zero
                                  &requiredLength,
                                  NULL); // not interested in the specific dev-node

  predictedLength = requiredLength;
  
  deviceSpecificInfo.cbSize = sizeof(SP_DEVINFO_DATA);

  functionClassDeviceData = (PSP_INTERFACE_DEVICE_DETAIL_DATA) malloc (predictedLength);
  functionClassDeviceData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);

  // Retrieve the information from Plug and Play.
  if (! SetupDiGetInterfaceDeviceDetail(HardwareDeviceInfo,
                                        DeviceInfoData,
                                        functionClassDeviceData,
                                        predictedLength,
                                        &requiredLength,
                                        &deviceSpecificInfo))
  {
    free(functionClassDeviceData);
    return INVALID_HANDLE_VALUE;
  }
  
  if( checkFriendlyName( HardwareDeviceInfo, &deviceSpecificInfo, portName ) )
  	return functionClassDeviceData->DevicePath;
  else
  {
		free( functionClassDeviceData );
		return INVALID_HANDLE_VALUE;
  }
}

//-----------------------------------------------------------------
//                  Windows-only checkFriendlyName( )
//-----------------------------------------------------------------
bool checkFriendlyName( HDEVINFO HardwareDeviceInfo, PSP_DEVINFO_DATA deviceSpecificInfo, char* portName )
{
	DWORD DataT;
    LPTSTR buffer = NULL;
    DWORD buffersize = 0;
		TCHAR *ptr;
		char* namePtr;
    
    while (!SetupDiGetDeviceRegistryProperty(
               HardwareDeviceInfo,
               deviceSpecificInfo,
               SPDRP_FRIENDLYNAME,
               &DataT,
               (PBYTE)buffer,
               buffersize,
               &buffersize))
   {
       if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) // then change the buffer size.
       {
           if (buffer) LocalFree(buffer);
           // Double the size to avoid problems on W2k MBCS systems per KB 888609.
           buffer = (TCHAR*)LocalAlloc(LPTR, buffersize * 2);
       }
       else
           break;
   }  
	if (buffer)
	{	// if the friendly name is Make Controller Kit, then that's us.
		if(!_tcsncmp(TEXT("Make Controller Kit"), buffer, 19))
		{
			// zip through the buffer to find the COM port number - between parentheses
			ptr = buffer;
			namePtr = portName;
			while( *ptr++ != '(' ) {};
			while( *ptr != ')' )
				*namePtr++ = *ptr++;
			*namePtr = '\0'; // null terminate the string
			return true;
		}
			
		LocalFree(buffer);
	}
		
	return false;
}

#endif // WIN32


