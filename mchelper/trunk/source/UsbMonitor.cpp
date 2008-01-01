/*********************************************************************************

 Copyright 2006-2007 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#include "UsbMonitor.h"
#include "BoardArrivalEvent.h"

#ifdef Q_WS_WIN // Windows-only
#include <initguid.h>
DEFINE_GUID( GUID_MAKE_CTRL_KIT, 0x4D36E978, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 );
#endif

UsbMonitor::UsbMonitor( ) : QThread( )
{
	
}

void UsbMonitor::run( )
{
	while( 1 )
	{
		QList<PacketInterface*> newBoards;
		scan( &newBoards );
		if( newBoards.count( ) > 0 )
		{
			BoardArrivalEvent* event = new BoardArrivalEvent( Board::UsbSerial );
			event->pInt += newBoards;
			application->postEvent( mainWindow, event );
		}
		sleep( 1 ); // check once a second
	}
}

UsbMonitor::Status UsbMonitor::scan( QList<PacketInterface*>* arrived )
{
	FindUsbDevices( arrived );  // fill up our list of connectedDevices, if there are any out there.
	return OK;
}

void UsbMonitor::closeAll( )
{	// app is shut down - close everything out.
	QHash<QString, PacketUsbCdc*>::iterator i  = connectedDevices.begin( );
	while( i != connectedDevices.end( ) )
	{
		i.value( )->close( );
		++i;
	}
}

void UsbMonitor::setInterfaces( MessageInterface* messageInterface, QApplication* application, McHelperWindow* mainWindow )
{
	this->messageInterface = messageInterface;
	this->application = application;
	this->mainWindow = mainWindow;
}

void UsbMonitor::FindUsbDevices( QList<PacketInterface*>* arrived )
{
  #ifdef Q_WS_MAC
	io_iterator_t serialPortIterator = 0;
	
	io_object_t modemService;
		char productName[50] = "";
    kern_return_t kernResult = KERN_FAILURE;
    // Initialize the returned path
		int maxPathSize = sizeof(portName);
    char* path = portName;
		*path = '\0';
	
	CFMutableDictionaryRef bsdMatchingDictionary;
	
	// create a dictionary that looks for all BSD modems
	bsdMatchingDictionary = IOServiceMatching( kIOSerialBSDServiceValue );
	if (bsdMatchingDictionary == NULL)
		printf("IOServiceMatching returned a NULL dictionary.\n");
	else
		CFDictionarySetValue(bsdMatchingDictionary, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDModemType));
	
	// then create the iterator with all the matching devices
	kernResult = IOServiceGetMatchingServices( kIOMasterPortDefault, bsdMatchingDictionary, &serialPortIterator );    
	if ( KERN_SUCCESS != kernResult )
	{
		printf("IOServiceGetMatchingServices returned %d\n", kernResult);
		return;
	}
	
	// Iterate through all modems found. In this example, we bail after finding the first modem.
	while( (modemService = IOIteratorNext(serialPortIterator) ) )
	{
		CFTypeRef bsdPathAsCFString;
		CFTypeRef productNameAsCFString;
		// check the name of the modem's callout device
		bsdPathAsCFString = IORegistryEntrySearchCFProperty(modemService,
																													kIOServicePlane,
																													CFSTR(kIOCalloutDeviceKey),
																													kCFAllocatorDefault,
																													0);
		// then, because the callout device could be any old thing, and because the reference to the modem returned by the
		// iterator doesn't include much device specific info, look at its parent, and check the product name
		io_registry_entry_t parent;  
		kernResult = IORegistryEntryGetParentEntry( modemService,	kIOServicePlane, &parent );																										
		productNameAsCFString = IORegistryEntrySearchCFProperty(parent,
																													kIOServicePlane,
																													CFSTR("Product Name"),
																													kCFAllocatorDefault,
																													0);
		
		if( bsdPathAsCFString )
		{
			Boolean result;      
			result = CFStringGetCString( (CFStringRef)bsdPathAsCFString,
																	path,
																	maxPathSize, 
																	kCFStringEncodingUTF8);
			
			if( productNameAsCFString )
			{
				result = CFStringGetCString( (CFStringRef)productNameAsCFString,
																	productName,
																	maxPathSize, 
																	kCFStringEncodingUTF8);
				CFRelease( productNameAsCFString );
			}
			if (result)
			{
				if( (strcmp( productName, "Make Controller Ki") == 0) )
				{
					QString portNameKey( path );
					if( !connectedDevices.contains( portNameKey ) ) // make sure we don't already have this board in our list
					{
						PacketUsbCdc* device = new PacketUsbCdc( );
						device->setportName( path );
						device->setInterfaces( messageInterface, application, this );
						if( PacketInterface::OK == device->open( ) )
						{
							connectedDevices.insert( portNameKey, device );  // stick it in our own list of boards we know about
							arrived->append( device ); // then stick it on the list of new boards that's been requested
							msleep( 10 );
							device->start( );
						}
						else
							delete device;
					}
					productName[0] = 0; // clear this out for the next time around
					IOObjectRelease(parent);
				}
				else
					*path = '\0';  // clear this, since this is checked above.
			}
			(void) IOObjectRelease(modemService);
			CFRelease(bsdPathAsCFString);
		}
	}
	return;
	#endif // Mac-only FindUsbDevices( )
	
	
	#ifdef Q_WS_WIN // Windows only
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
	      QString portNameKey( portName );
	      if( !connectedDevices.contains( portNameKey ) ) // make sure we don't already have this board in our list
	      {
	      	PacketUsbCdc* device = new PacketUsbCdc( );
	     		device->deviceHandle = hOut;
	     		device->setportName( portName );
	     		device->setInterfaces( messageInterface, application, this );
					device->setWidget( mainWindow );
	     		if( PacketInterface::OK == device->open( ) )
	     		{
	     			connectedDevices.insert( portNameKey, device );  // stick it in our own list of boards we know about
		      	arrived->append( device ); // then stick it on the list of new boards that's been requested
		      	device->start( );
	     		}
	     		else
	     			delete device;
	      }
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
  return;
	#endif // Windows-only FindUsbDevices( )
}

#ifdef Q_WS_WIN
//-----------------------------------------------------------------
//                  Windows-only GetDevicePath( )
//-----------------------------------------------------------------
HANDLE UsbMonitor::GetDeviceInfo(HDEVINFO HardwareDeviceInfo, 
								PSP_INTERFACE_DEVICE_DATA DeviceInfoData, 
								char* portName  )
{
  PSP_INTERFACE_DEVICE_DETAIL_DATA functionClassDeviceData = NULL;
  ULONG                            predictedLength = 0;
  ULONG                            requiredLength = 0;
  
  // allocate a function class device data structure to receive the
  // goods about this particular device.
  SetupDiGetInterfaceDeviceDetail(HardwareDeviceInfo,
                                  DeviceInfoData,
                                  NULL,  // probing so no output buffer yet
                                  0,     // probing so output buffer length of zero
                                  &requiredLength,
                                  NULL); // not interested in the specific dev-node

  predictedLength = requiredLength;
  
  SP_DEVINFO_DATA deviceSpecificInfo;
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
bool UsbMonitor::checkFriendlyName( HDEVINFO HardwareDeviceInfo, 
									PSP_DEVINFO_DATA deviceSpecificInfo, 
									char* portName )
{
	DWORD DataT;
    LPTSTR buffer = NULL;
    DWORD buffersize = 0;
    
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
			TCHAR *ptr = buffer;
			char* namePtr = portName;
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

// zip through our list of devices, and check if the HANDLE from the DEVICEREMOVED broadcast matches any of them
void UsbMonitor::removalNotification( HANDLE handle )
{  
	QHash<QString, PacketUsbCdc*>::iterator i = connectedDevices.begin( );
	while( i != connectedDevices.end( ) )
	{
		if( i.value( )->deviceHandle == handle )
		{
			i.value( )->close( );
			mainWindow->removeDeviceThreadSafe( i.key() );
			i = connectedDevices.erase( i );
		}
		else
			++i;
	}
}

#endif // Windows-only stuff

void UsbMonitor::deviceRemoved( QString key )
{
	if( connectedDevices.contains( key) )
	{
		PacketUsbCdc* usb = connectedDevices.value( key );
		connectedDevices.remove( key );
		if( usb->isOpen() )
			usb->close( );
		mainWindow->removeDeviceThreadSafe( key );
	}
}













