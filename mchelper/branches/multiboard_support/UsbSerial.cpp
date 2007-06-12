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

#include "UsbSerial.h"
#include <QMutexLocker> 

#ifdef Q_WS_WIN
#define _WIN32_WINDOWS 0x0501 // 0x0501 == WindowsXP...define this for MinGW
#include <dbt.h>

#endif 

UsbSerial::UsbSerial( )
{
	deviceOpen = false;
	readInProgress = false;
	#ifdef Q_WS_MAC
	kern_return_t err;
	masterPort = 0;
	if( ( err = IOMasterPort( MACH_PORT_NULL, &masterPort ) ) ) 
		printf( "could not create master port, err = %08x\n", err );
	#endif
}

UsbSerial::UsbStatus UsbSerial::usbOpen( )
{
  QMutexLocker locker( &usbOpenMutex );
  
  	if( deviceOpen )  //if it's already open, do nothing.
	  return ALREADY_OPEN;
  
  // Linux Only
  #if (defined(Q_WS_LINUX))
	  printf( "OSC over USB not implemented in Linux\n" );
    return NOT_OPEN;
  #endif

  //Mac-only
	#ifdef Q_WS_MAC
		kern_return_t kernResult;
		io_iterator_t iterator = 0;
		
		kernResult = getDevicePath( iterator, deviceFilePath, sizeof(deviceFilePath) );
		IOObjectRelease( iterator ); // clean up
		
		if (!deviceFilePath[0] )
    {
			//printf("Didn't find a Make Controller.\n");
			return NOT_OPEN;
    }
		
		// now try to actually do something
		deviceHandle = ::open( deviceFilePath, O_RDWR | O_NOCTTY | ( ( blocking ) ? 0 : O_NDELAY ) );
		if ( deviceHandle < 0 )
	  {
	    //printf( "Could not open the port (Error %d)\n", deviceHandle );
	    return NOT_OPEN;
	  } else
		{
	    deviceOpen = true;
			sleepMs( 10 ); //give it a moment after opening before trying to read/write
			// printf( "USB opened at %s\n", deviceFilePath );
			messageInterface->message( 1, "Usb> Make Controller connected.\n" );
	  }
	  return OK;
		
	#endif //Mac-only UsbSerial::open( )
	
  //-----------------------------------------------------------------
  //                  Windows-only usbOpen( )
  //-----------------------------------------------------------------
	#ifdef Q_WS_WIN
	UsbStatus result = openDevice( (TCHAR*)deviceHandle );
	if( result == OK )
	{
		messageInterface->message( 1, "Usb> Make Controller connected at %s", portName );
		Sleep( 10 );  // wait after opening it before trying to read/write
		deviceOpen = true;
		DoRegisterForNotification( ); // now set up to get called back when it's unplugged
		return OK;
	}
	if( result == ALREADY_OPEN )
		return result;

	return NOT_OPEN;
	#endif //Windows-only UsbSerial::open( )
}

void UsbSerial::usbClose( )
{
  if( deviceOpen )
	{
    // Linux Only
    #if (defined(Q_WS_LINUX))
    #endif
  	  //Mac-only
		#ifdef Q_WS_MAC
		::close( deviceHandle );
		deviceHandle = -1;
		deviceOpen = false;
		#endif
	  //-----------------------------------------------------------------
  	  //                  Windows-only usbClose( )
  	  //-----------------------------------------------------------------
	  #ifdef Q_WS_WIN
	  CloseHandle( deviceHandle );
	  deviceHandle = INVALID_HANDLE_VALUE;
	  deviceOpen = false;
	  UnregisterDeviceNotification( notificationHandle );
		#endif //Windows-only UsbSerial::close( )
		messageInterface->message( 1, "Usb> Make Controller disconnected.\n" );
	}
}

int UsbSerial::usbRead( char* buffer, int length )
{
  // make sure we're open
  if( !deviceOpen )
  {
    UsbStatus portIsOpen = usbOpen( );
    if( portIsOpen != OK )
	{
	  usbClose( );
	  return NOT_OPEN;
	}
  }
  
  // Linux Only
  #if (defined(Q_WS_LINUX))
  // make sure we're open
  //if( !deviceOpen )
    return NOT_OPEN;
  #endif

  //Mac-only
	#ifdef Q_WS_MAC
	int count;
		
	count = ::read( deviceHandle, buffer, length );
	printf( "Reading...count = %d\n", count );
	switch ( count )
  {
    case 0:
		return ERROR_CLOSE; // EOF; possibly file was closed
      break;

    case 1:
      return GOT_CHAR; // got a character
      break;

    case -1:
      if ( errno == EAGAIN )
	      return NOTHING_AVAILABLE; // non-blocking but no data available
      else
	      return IO_ERROR; //printf( "Some other error...\n" );
      break;

    default:
		  //printf( "Unknown error.\n" );
      return IO_ERROR;
      break;
  }
	return OK;
	#endif //Mac-only UsbSerial::read( )
	
  //Windows-only///////////////////////////////////////////////////////////////////////
  #ifdef Q_WS_WIN
  DWORD count;
  int retval = -1;
  DWORD numTransferred;
    
  retval = OK;  
  readInProgress = false;
  overlappedRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  // reset the read overlapped structure
  overlappedRead.Offset = overlappedRead.OffsetHigh = 0;
  
  if ( !ReadFile( deviceHandle, buffer, length, &count, &overlappedRead ) )
  {
  	DWORD lastError = GetLastError();
		// messageInterface->message( 1, "USB Read Error: %d \n", lastError );
	  if ( lastError != ERROR_IO_PENDING)     // read not delayed?
	  {
	    usbClose( );
	    //messageInterface->message( 1, "Closed trying to read the file\n" );
	    retval = -1; //UNKNOWN_ERROR;
	  }
	  else
	    readInProgress = true;
  }
  else          	
  	retval = count;


  if( readInProgress )
  {
  	DWORD r;
	  do
	  {
	    r = WaitForSingleObject( overlappedRead.hEvent, 1000 );	 
	  } while ( r == WAIT_TIMEOUT );
	  switch( r )
		{
		  case WAIT_FAILED:
		    usbClose( );
		    retval = -1; //UNKNOWN_ERROR;
	      break;
		  case WAIT_TIMEOUT:
		    retval = -1; // NOTHING_AVAILABLE;
		    break;
		  case WAIT_OBJECT_0:
	  		// check to see if the pending operation completed
        	if( !GetOverlappedResult( deviceHandle, &overlappedRead, &numTransferred, FALSE )  ) // don't wait
			{
		      usbClose( );
		      SetEvent( overlappedRead.hEvent );
		      retval = -1; //IO_ERROR;
		      break;
			}
  			retval = numTransferred;
			  break; 
		  case WAIT_ABANDONED:
		  default:
		    usbClose( );
		    retval = -1; //IO_ERROR;
			  break;
		} // end of switch
  }
  
  CloseHandle( overlappedRead.hEvent );
  return retval;
  	
  #endif //Windows-only UsbSerial::read( )//////////////////////////////////////////////////////////////
}

UsbSerial::UsbStatus UsbSerial::usbWrite( char* buffer, int length )
{
  if( !deviceOpen )
  {
    UsbStatus portIsOpen = usbOpen( );
    if( portIsOpen != OK )
	{
      usbClose( );
	  return NOT_OPEN;
	}
  }
  
  // Linux Only
  #if (defined(Q_WS_LINUX))
    return NOT_OPEN;
  #endif

  //Mac-only
	#ifdef Q_WS_MAC
 
	int size = ::write( deviceHandle, buffer, length );
	if ( length == size )
    return OK;
	else if( errno == EAGAIN )
	  return NOTHING_AVAILABLE;
  else
  {
	  //printf("usbWrite: write failed, errno %d", errno);
    usbClose( );
    return IO_ERROR;
  }
	
	#endif //Mac-only UsbSerial::write( )

  //Windows-only
  #ifdef Q_WS_WIN
  DWORD cout;
  bool success;
  DWORD ret;
  UsbSerial::UsbStatus retval = OK;
  DWORD numWritten;
  
  int read = 0;
  
  overlappedWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  
	// reset the write overlapped structure
  overlappedWrite.Offset = overlappedWrite.OffsetHigh = 0; 
  // messageInterface->message( 1, "Writing...\n" );
  success = WriteFile( deviceHandle, buffer, length, &cout, &overlappedWrite );
  
  if( !success )
  {
  	if ( GetLastError() == ERROR_IO_PENDING)
	  {
	  	// messageInterface->message( 1, "Write: IO PENDING.\n" );
		  do
		  {
	  		//if( debug > 4 )TRACE_MESSAGE("TI_put(): Waiting for overlapped write to complete");
		  	ret = WaitForSingleObject( overlappedWrite.hEvent, 1000 );
		  }  while ( ret == WAIT_TIMEOUT );
	
	    if ( ret == WAIT_OBJECT_0 )
	    {
				// messageInterface->message( 1, "Write: IO PENDING.\n" );
			
			do
			{
				GetOverlappedResult( deviceHandle, &overlappedWrite, &numWritten, TRUE);
				read += numWritten;
			} while( read != length );
			if( read == length )
				retval = OK;
			else
			  retval = IO_ERROR;
	    }
	    else
	      retval = IO_ERROR;

	  }
	  else
	  {
	    usbClose( );
	    retval = IO_ERROR;
	  }
  }
  
  CloseHandle( overlappedWrite.hEvent );
  return retval;
  
  #endif //Windows-only UsbSerial::write( )
}

bool UsbSerial::isOpen( )
{
  return deviceOpen;
}

int UsbSerial::numberOfAvailableBytes( )
{
    COMSTAT status;
    int             n = 0;
    unsigned long   state;

    if (deviceHandle != INVALID_HANDLE_VALUE)
    {
        bool success = false;
        success = ClearCommError( deviceHandle, &state, &status);
        n = status.cbInQue;
    }

    return(n);
}

// do the real opening of the device
//Windows-only
#ifdef Q_WS_WIN
UsbSerial::UsbStatus UsbSerial::openDevice( TCHAR* deviceName )
{
  DCB dcb;
  COMMTIMEOUTS timeouts;

  // if it's already open, do nothing
  if( deviceOpen )
    return ALREADY_OPEN;

  // Open the port
  deviceHandle = CreateFile( deviceName, 
			GENERIC_READ | GENERIC_WRITE, 
			0, 
			0, 
			OPEN_EXISTING, 
			FILE_FLAG_OVERLAPPED, 
			0 );
  
  if ( deviceHandle == INVALID_HANDLE_VALUE )
    return ERROR_CLOSE; 

  // initialize the overlapped structures
  overlappedRead.Offset  = overlappedWrite.Offset = 0; 
  overlappedRead.OffsetHigh = overlappedWrite.OffsetHigh = 0; 
  overlappedRead.hEvent  = CreateEvent(0, TRUE, FALSE, 0);
  overlappedWrite.hEvent  = CreateEvent(0, TRUE, FALSE, 0);

  if (!overlappedRead.hEvent || !overlappedWrite.hEvent )
    return ERROR_CLOSE; 

  GetCommState( deviceHandle, &dcb );
  dcb.BaudRate = CBR_115200;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;
  dcb.fOutxCtsFlow = TRUE;
  dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
  dcb.fAbortOnError = TRUE;
  //dcb.fDtrControl = DTR_CONTROL_ENABLE; // magic testing...
  if( !SetCommState( deviceHandle, &dcb ) )
	 return ERROR_CLOSE;

/*
  From MSDN:
  If an application sets ReadIntervalTimeout and ReadTotalTimeoutMultiplier to MAXDWORD and sets 
  ReadTotalTimeoutConstant to a value greater than zero and less than MAXDWORD, one of the following 
  occurs when the ReadFile function is called:

  * If there are any bytes in the input buffer, ReadFile returns immediately with the bytes in the buffer.
  * If there are no bytes in the input buffer, ReadFile waits until a byte arrives and then returns immediately.
  * If no bytes arrive within the time specified by ReadTotalTimeoutConstant, ReadFile times out.
*/
  timeouts.ReadIntervalTimeout = MAXDWORD;
  timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
  timeouts.ReadTotalTimeoutConstant = 500;
  timeouts.WriteTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = 0;   
  if( ! SetCommTimeouts( deviceHandle, &timeouts ) )
  	return ERROR_CLOSE;

  EscapeCommFunction( deviceHandle, SETDTR );

  return OK;
}
#endif //Windows-only UsbSerial::openDevice( )


//Windows-only
#ifdef Q_WS_WIN
 // make sure to do this only once the device has been opened,
 // since it relies on the deviceHandle to register for the call.
bool UsbSerial::DoRegisterForNotification( )
{
	DEV_BROADCAST_HANDLE NotificationFilter;
	
	ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
	NotificationFilter.dbch_size = sizeof( DEV_BROADCAST_HANDLE );
	NotificationFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
	NotificationFilter.dbch_handle = deviceHandle;  // class variable
	HWND winId = mainWindow->winId( );

    notificationHandle = RegisterDeviceNotification( winId, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE );

    if(!notificationHandle) 
    {
        qDebug( "RegisterDeviceNotification failed: %ld\n", GetLastError());
        return false;
    }
    qDebug( "RegisterDeviceNotification success\n" );
    return true;
}

#endif 

//Mac-only
#ifdef Q_WS_MAC

kern_return_t UsbSerial::getDevicePath(io_iterator_t serialPortIterator, char *path, CFIndex maxPathSize)
{
    io_object_t modemService;
		char productName[50] = "";
    kern_return_t kernResult = KERN_FAILURE;
    Boolean deviceFound = false;
    // Initialize the returned path
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
		return kernResult;
	}
	
	// Iterate through all modems found. In this example, we bail after finding the first modem.
	while ((modemService = IOIteratorNext(serialPortIterator)) && !deviceFound)
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
			}
			if (result)
			{
				//printf("Modem found with BSD path: %s", path);
				if( (strcmp( productName, "Make Controller Ki") == 0) )
				{
					CFRelease(bsdPathAsCFString);
					IOObjectRelease(parent);
					deviceFound = true;
					kernResult = KERN_SUCCESS;
				}
				else
					*path = '\0';  // clear this, since this is checked above.
			}
			printf("\n");
			(void) IOObjectRelease(modemService);
		}
	}
	return kernResult;
}

int UsbSerial::sleepMs( long ms )
{
	  usleep( 1000*ms );
		return 0; //TELEO_OK;
}
#endif



