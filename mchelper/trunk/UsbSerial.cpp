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

UsbSerial::UsbSerial( )
{
	deviceOpen = false;
	readInProgress = false;
	#ifdef Q_WS_MAC
	blocking = false;
	foundMakeController = false;
	kern_return_t err;
	masterPort = 0;
	if( ( err = IOMasterPort( MACH_PORT_NULL, &masterPort ) ) ) 
		printf( "could not create master port, err = %08x\n", err );
		
	notifyThread = new UsbNotify( );
	#endif
}

UsbSerial::UsbStatus UsbSerial::usbOpen( )
{
  QMutexLocker locker( &usbOpenMutex );
  
  // Linux Only
  #if (defined(Q_WS_LINUX))
	  printf( "OSC over USB not implemented in Linux\n" );
    return NOT_OPEN;
  #endif

  //Mac-only
	#ifdef Q_WS_MAC
		if( deviceOpen )  //if it's already open, do nothing.
		  return ALREADY_OPEN;

		kern_return_t kernResult;
		io_iterator_t serialPortIterator;
		
		kernResult = findMakeController( &serialPortIterator );
		kernResult = getDevicePath( serialPortIterator, deviceFilePath, sizeof(deviceFilePath) );
		IOObjectRelease(serialPortIterator);    // Release the iterator.
		
		if (!deviceFilePath[0] || !foundMakeController)
    {
			printf("Didn't find a Make Controller.\n");
			
			return NOT_OPEN;
    }
		
		// now try to actually do something
		deviceHandle = ::open( deviceFilePath, O_RDWR | O_NOCTTY | ( ( blocking ) ? 0 : O_NDELAY ) );
		if ( deviceHandle < 0 )
	  {
	    printf( "Could not open the port (Error %d)\n", deviceHandle );
	    return NOT_OPEN;
	  } else
		{
	    deviceOpen = true;
			sleepMs( 10 ); //give it a moment after opening before trying to read/write
	    printf( "USB opened at %s\n", deviceFilePath );
	  }
	  return OK;
		
	#endif //Mac-only UsbSerial::open( )
	
  //Windows-only
	#ifdef Q_WS_WIN
		if( deviceOpen )  //if it's already open, do nothing.
		  return ALREADY_OPEN;
		TCHAR* openPorts[32];
		int foundOpen = ScanEnumTree( TEXT("SYSTEM\\CURRENTCONTROLSET\\ENUM\\USB"), openPorts);
		//messageInterface->message( 1, "Found open: %d\n", foundOpen );
		int i;
		for( i = 0; i < foundOpen; i++ )
		{
			if( openPorts[i] != NULL )
			{
				if( openDevice( openPorts[i] ) == 0 )
				{
					messageInterface->message( 1, "Device name: %ls\n", openPorts[i] );
					Sleep( 10 );  // wait after opening it before trying to read/write
					deviceOpen = true;
					return OK;
				}
				//else
				  //messageInterface->message( 1, "Found device but could not open: %ls\n", openPorts[i] );
			}
			//else
			  //messageInterface->message( 1, "Did not find any potential ports\n" );
		}
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
		foundMakeController = false;
		deviceHandle = -1;
		deviceOpen = false;
		#endif
		//Windows-only
	  #ifdef Q_WS_WIN
	  CloseHandle( deviceHandle );
	  deviceHandle = INVALID_HANDLE_VALUE;
	  deviceOpen = false;
		#endif //Windows-only UsbSerial::close( )
	}
}

UsbSerial::UsbStatus UsbSerial::usbRead( char* buffer, int length )
{
  // Linux Only
  #if (defined(Q_WS_LINUX))
  // make sure we're open
  //if( !deviceOpen )
    return NOT_OPEN;
  #endif

  //Mac-only
	#ifdef Q_WS_MAC
	int count;
	
	if( !deviceOpen )
	{
	  UsbStatus portIsOpen = usbOpen( );
		if( portIsOpen == OK )
		  deviceOpen = true;
		else 
		  return NOT_OPEN;
	}
		
	count = ::read( deviceHandle, buffer, length );
	//printf( "Reading...\n" );
	switch ( count )
  {
    case 0:
      // EOF; possibly file was closed
			printf( "End of file.\n" );
			return ERROR_CLOSE;
      break;

    case 1:
      // got a character
			printf( "Got a character: %s\n", buffer );
      return GOT_CHAR;
      break;

    case -1:
      if ( errno == EAGAIN )
			{
	      // non-blocking but no data available
				//printf( "No data available.\n" );
	      return NOTHING_AVAILABLE;
			}
      else
			  printf( "Some other error...\n" );
	      return IO_ERROR;
      break;

    default:
		  printf( "Unknown error.\n" );
      return IO_ERROR;
      break;
  }
	return OK;
	#endif //Mac-only UsbSerial::read( )
	
  //Windows-only
  #ifdef Q_WS_WIN
  DWORD count;
  UsbStatus retval;
  DWORD numTransferred;
  //int length = 0;
  
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
	    usbClose();
	    //messageInterface->message( 1, "Closed trying to read the file\n" );
	    retval = UNKNOWN_ERROR;
	  }
	  else
	    readInProgress = true;
  }
  else          	
  {
		// messageInterface->message( 1, "USB read D: %c \n", *buffer );
  	retval = GOT_CHAR;
  }

  if( readInProgress )
  {
  	DWORD r;
	  do
	  {
	    r = WaitForSingleObject( overlappedRead.hEvent, 10000 );	 
	  } while ( r == WAIT_TIMEOUT );
	  switch( r )
		{
		  case WAIT_FAILED:
		    //FIXME is there anything else i need to clean up?
		    // reset events? terminate anything?
		    usbClose(  );
		    //messageInterface->message( 1, "Closed after wait failed\n" );
		    retval = UNKNOWN_ERROR; //( 100 );
	      break;
		  case WAIT_TIMEOUT:
			  //if( debug > 1 ) TRACE_MESSAGE( "timeout occurred while waiting for event\n" );
		    retval = NOTHING_AVAILABLE; //( TELEO_E_NOTHING );
		    //messageInterface->message( 1, "Nothing available\n" );
		    break;
		  case WAIT_OBJECT_0:
	  		// check to see if the pending operation completed
        if( !GetOverlappedResult( deviceHandle, &overlappedRead, &numTransferred, FALSE )  ) // don't wait
				{
	        ///FIXME is there anything else i need to clean up?
		      // reset events? terminate anything?
		      usbClose( );
		      SetEvent( overlappedRead.hEvent );
		      //messageInterface->message( 1, "Closed because error getting overlapped result\n" );
		      retval = IO_ERROR;
		      break;
				}
		    //messageInterface->message( 1, "Successfully got overlapped result\n" );
  			retval = GOT_CHAR;
			  break; 
		  case WAIT_ABANDONED:
		  default:
		    usbClose( );
		    //messageInterface->message( 1, "Closed - wait abandoned or default\n" );
		    retval = IO_ERROR;
			  break;
		} // end of switch
  }
  
  CloseHandle( overlappedRead.hEvent );
  return retval;
  	
	#endif //Windows-only UsbSerial::read( )
}

UsbSerial::UsbStatus UsbSerial::usbWrite( char* buffer, int length )
{
  // Linux Only
  #if (defined(Q_WS_LINUX))
  // make sure we're open
  //if( !deviceOpen )
    return NOT_OPEN;
  #endif

  //Mac-only
	#ifdef Q_WS_MAC
	
	if( !deviceOpen )  //then try to open it
	{
	  UsbStatus portIsOpen = usbOpen( );
		if( portIsOpen != OK )
		  return NOT_OPEN;
	}
 
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
  int count = 1;
  bool success;
  DWORD ret;
  UsbSerial::UsbStatus retval = OK;
  DWORD numWritten;
    
  if( !deviceOpen )
  {
    UsbStatus portIsOpen = usbOpen( );
    if( portIsOpen != OK )
	{
      usbClose( );
	  return NOT_OPEN;
	}
  }
  
  overlappedWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  
	// reset the write overlapped structure
  overlappedWrite.Offset = overlappedWrite.OffsetHigh = 0; 
  // messageInterface->message( 1, "Writing...\n" );
  success = WriteFile( deviceHandle, buffer, count, &cout, &overlappedWrite ) ;
  
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
			  GetOverlappedResult( deviceHandle, &overlappedWrite, &numWritten, TRUE);	
			  if( count == numWritten )
			  {
			  	// messageInterface->message( 1, "Write: COMPLETE.\n" );
			    retval = OK; 
			  }
			  else
			  {
			  	// messageInterface->message( 1, "write event, but only wrote %d\n", numWritten );
		      usbClose( );
			    retval = IO_ERROR;
			  }
	    }
	    else
	    {
	      usbClose( );
	      retval = IO_ERROR;
	    }
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

UsbSerial::UsbStatus UsbSerial::usbWriteChar( char c )
{
	usbWrite( &c, 1 );
	return OK;
}

bool UsbSerial::usbIsOpen( )
{
  return deviceOpen;
}

// try to open, and then close, a given port just to see if there's a device there
//Windows-only
#ifdef Q_WS_WIN
int UsbSerial::testOpen( TCHAR* deviceName )
{
  deviceHandle = CreateFile( deviceName, 
			GENERIC_READ | GENERIC_WRITE, 
			0, 
			0, 
			OPEN_EXISTING, 
			FILE_FLAG_OVERLAPPED, 0 );
  
  if ( deviceHandle == INVALID_HANDLE_VALUE )
  {
    CloseHandle( deviceHandle );
    return -1;
  }

  // otherwise, we found one.
  // close it again immediately - we were just checking to see if anything was there...
  CloseHandle( deviceHandle );
  deviceHandle = INVALID_HANDLE_VALUE;
  return 0;
}
#endif //Windows-only UsbSerial::testOpen( )

// do the real opening of the device
//Windows-only
#ifdef Q_WS_WIN
int UsbSerial::openDevice( TCHAR* deviceName )
{
  DCB dcb;
  COMMTIMEOUTS timeouts;

  // if it's already open, do nothing
  if( deviceOpen )
    return 0;

  // Open the port
  deviceHandle = CreateFile( deviceName, 
			GENERIC_READ | GENERIC_WRITE, 
			0, 
			0, 
			OPEN_EXISTING, 
			FILE_FLAG_OVERLAPPED, 
			0 );
  
  if ( deviceHandle == INVALID_HANDLE_VALUE )
  {
  	//messageInterface->message( 1, "Got invalid handle back\n");
    return -1; 
  }

  // initialize the overlapped structures
  overlappedRead.Offset  = overlappedWrite.Offset = 0; 
  overlappedRead.OffsetHigh = overlappedWrite.OffsetHigh = 0; 
  overlappedRead.hEvent  = CreateEvent(0, TRUE, FALSE, 0);
  overlappedWrite.hEvent  = CreateEvent(0, TRUE, FALSE, 0);

  if (!overlappedRead.hEvent || !overlappedWrite.hEvent )
  {
	  //messageInterface->message( 1, "Could Not create overlapped events\n");
    return -1;
  }

  GetCommState( deviceHandle, &dcb );
  dcb.BaudRate = CBR_115200;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;
  dcb.fOutxCtsFlow = TRUE;
  dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
  dcb.fAbortOnError = TRUE;
  if( !SetCommState( deviceHandle, &dcb ) )
  {
	  //messageInterface->message( 1, "SetCommState failed\n");
	  return -1;
  }
/*
  if ( blocking )
  {
      timeouts.ReadIntervalTimeout = 5000; 
	  // we don't really want to wait forever since we'll miss errors
	  // due to closing of the port.
  }
  else
  {
	  if( debug ) printf
		  ( "non-blocking desired, setting timeout to MAXDWORD\n");
      timeouts.ReadIntervalTimeout = MAXDWORD; // return immediately
  }
*/
  timeouts.ReadTotalTimeoutMultiplier = 0;
  timeouts.ReadTotalTimeoutConstant = 0;
  timeouts.WriteTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = 0;   
  if( ! SetCommTimeouts( deviceHandle, &timeouts ) )
  {
	  //messageInterface->message( 1, "SetCommTimeouts failed\n");
	  return -1;
  }

  EscapeCommFunction( deviceHandle, SETDTR );

  //deviceOpen = true;

  return 0;
}
#endif //Windows-only UsbSerial::openDevice( )

// Scan the registry for Make Controller Kit USB entries
// If one is found, see if it can be opened and stick it back into
// openPots.  Returns the number of openable ports found.
//Windows-only
#ifdef Q_WS_WIN
int UsbSerial::ScanEnumTree( LPCTSTR lpEnumPath, TCHAR** openPorts )
{
  static const TCHAR lpstrPortsClass[] = TEXT("PORTS");
  static const TCHAR lpstrPortsClassGUID[] = TEXT("{4D36E978-E325-11CE-BFC1-08002BE10318}");

  DWORD  dwError=0;
  HKEY   hkEnum=NULL;
  DWORD  dwIndex1;
  HKEY   hkLevel1=NULL;
  DWORD  dwIndex2;
  HKEY   hkLevel2=NULL;
  HKEY   hkDeviceParameters=NULL;
  TCHAR  lpClass[sizeof(lpstrPortsClass)/sizeof(lpstrPortsClass[0])];
  DWORD  cbClass;
  TCHAR  lpClassGUID[sizeof(lpstrPortsClassGUID)/sizeof(lpstrPortsClassGUID[0])];
  DWORD  cbClassGUID;
  LPTSTR lpPortName=NULL;
  LPTSTR lpFriendlyName=NULL;
  
  int openCount = 0;

  typedef struct
  {
    LPCTSTR lpPortName;     /* "COM1", etc. */
    LPCTSTR lpFriendlyName; /* Suitable to describe the port, as for  */
                          /* instance "Infrared serial port (COM4)" */
  }LISTPORTS_PORTINFO;

  //printf( "ScanEnumTree(): top\n" );

  if(dwError=RegOpenKeyEx(HKEY_LOCAL_MACHINE,lpEnumPath,0,KEY_ENUMERATE_SUB_KEYS,&hkEnum)){
    goto end;
  }

  for(dwIndex1=0;;++dwIndex1)
  {
	//printf("finding loop; dwIndex1 = %d\n", dwIndex1);

    if(hkLevel1!=NULL){
      RegCloseKey(hkLevel1);
      hkLevel1=NULL;
    }
    if(dwError=OpenSubKeyByIndex(hkEnum,dwIndex1,KEY_ENUMERATE_SUB_KEYS,&hkLevel1)){
      if(dwError==ERROR_NO_MORE_ITEMS){
        dwError=0;
        break;
      }
      else goto end;
    }

      for(dwIndex2=0;;++dwIndex2){
        BOOL               bFriendlyNameNotFound=FALSE;
        LISTPORTS_PORTINFO portinfo;

        if(hkLevel2!=NULL){
          RegCloseKey(hkLevel2);
          hkLevel2=NULL;
        }
        if(dwError=OpenSubKeyByIndex(hkLevel1,dwIndex2,KEY_READ,&hkLevel2)){
          if(dwError==ERROR_NO_MORE_ITEMS){
            dwError=0;
            break;
          }
          else goto end;
        }

        /* Look if the driver class is the one we're looking for.
         * We accept either "CLASS" or "CLASSGUID" as identifiers.
         * No need to dynamically arrange for space to retrieve the values,
         * they must have the same length as the strings they're compared to
         * if the comparison is to be succesful.
         */

        cbClass=sizeof(lpClass);
        if(RegQueryValueEx(hkLevel2,TEXT("CLASS"),NULL,NULL,
                           (LPBYTE)lpClass,&cbClass)==ERROR_SUCCESS&&
           _tcsicmp(lpClass,lpstrPortsClass)==0){
          /* ok */
        }
        else{
          cbClassGUID=sizeof(lpClassGUID);
          if(RegQueryValueEx(hkLevel2,TEXT("CLASSGUID"),NULL,NULL,
                             (LPBYTE)lpClassGUID,&cbClassGUID)==ERROR_SUCCESS&&
             _tcsicmp(lpClassGUID,lpstrPortsClassGUID)==0){
            /* ok */
          }
          else continue;
        }

        /* get "PORTNAME" */

        dwError=QueryStringValue(hkLevel2,TEXT("PORTNAME"),&lpPortName);
        if(dwError==ERROR_FILE_NOT_FOUND){
          /* In Win200, "PORTNAME" is located under the subkey "DEVICE PARAMETERS".
           * Try and look there.
           */

          if(hkDeviceParameters!=NULL){
            RegCloseKey(hkDeviceParameters);
            hkDeviceParameters=NULL;
          }
          if(RegOpenKeyEx(hkLevel2,TEXT("DEVICE PARAMETERS"),0,KEY_READ,
                          &hkDeviceParameters)==ERROR_SUCCESS){
             dwError=QueryStringValue(hkDeviceParameters,TEXT("PORTNAME"),&lpPortName);
          }
        }
        if(dwError){
          if(dwError==ERROR_FILE_NOT_FOUND){ 
            /* boy that was strange, we better skip this device */
            dwError=0;
            continue;
          }
          else goto end;
        }
         //printf("found port, name %ls\n", lpPortName);

        /* check if it is a serial port (instead of, say, a parallel port) */

        if(_tcsncmp(lpPortName,TEXT("COM"),3)!=0)continue;

        /* now go for "FRIENDLYNAME" */

        if(dwError=QueryStringValue(hkLevel2,TEXT("FRIENDLYNAME"),&lpFriendlyName)){
          if(dwError==ERROR_FILE_NOT_FOUND){
            bFriendlyNameNotFound=TRUE;
            dwError=0;
          }
          else goto end;
        }
        
        /* Assemble the information and pass it on to the callback.
         * In the unlikely case there's no friendly name available,
         * use port name instead.
         */
        portinfo.lpPortName=lpPortName;
        portinfo.lpFriendlyName=bFriendlyNameNotFound?lpPortName:lpFriendlyName;
		{
			//printf( "Friendly name: %ls\n", portinfo.lpFriendlyName );
			if (!_tcsncmp(TEXT("Make Controller Kit"), portinfo.lpFriendlyName, 19))
			{
				TCHAR* pname;
				pname = _tcsdup(portinfo.lpPortName);
				//messageInterface->message( 1, "Found matching registry entry...\n" );
				// We've found a matching entry in the registry...
				// Now see if it's actually there by trying to open it
				int result = testOpen( pname );
				// if it is, store it
				if( result == 0 )
					openPorts[ openCount++ ] = pname; 
			}
        }
      }
  }
  goto end;

end:
  free(lpFriendlyName);
  free(lpPortName);
  if(hkDeviceParameters!=NULL)RegCloseKey(hkDeviceParameters);
  if(hkLevel2!=NULL)          RegCloseKey(hkLevel2);
  if(hkLevel1!=NULL)          RegCloseKey(hkLevel1);
  if(hkEnum!=NULL)            RegCloseKey(hkEnum);
  if(dwError!=0)
  {
    SetLastError(dwError);
    return 0; //TELEO_E_UNKNOWN;
  }
  else return openCount; //TELEO_OK;
}
#endif //Windows-only UsbSerial::ScanEnumTree( )

//Windows-only
#ifdef Q_WS_WIN
LONG UsbSerial::OpenSubKeyByIndex(HKEY hKey,DWORD dwIndex,REGSAM samDesired,PHKEY phkResult)
{
  DWORD              dwError=0;
  LPTSTR             lpSubkeyName=NULL;
  DWORD              cbSubkeyName=128*sizeof(TCHAR); /* an initial guess */
  FILETIME           filetime;

  /* loop asking for the subkey name til we allocated enough memory */

  for(;;){
    free(lpSubkeyName);
    if(!(lpSubkeyName=(LPTSTR)malloc(cbSubkeyName))){
       dwError=ERROR_NOT_ENOUGH_MEMORY;
       goto end;
    }
    if(!(dwError=RegEnumKeyEx(hKey,dwIndex,lpSubkeyName,&cbSubkeyName,
                              0,NULL,NULL,&filetime))){
      break; /* we did it */
    }
    else if(dwError==ERROR_MORE_DATA){ /* not enough space */
      dwError=0;
      /* no indication of space required, we try doubling */
      cbSubkeyName=(cbSubkeyName+sizeof(TCHAR))*2;
    }
    else goto end;
  }

  dwError=RegOpenKeyEx(hKey,lpSubkeyName,0,samDesired,phkResult);

end:
  free(lpSubkeyName);
  return dwError;
}
#endif //Windows-only UsbSerial::OpenSubKeyByIndex( )

//Windows-only
#ifdef Q_WS_WIN
LONG UsbSerial::QueryStringValue(HKEY hKey,LPCTSTR lpValueName,LPTSTR* lppStringValue)
{
  DWORD cbStringValue=128*sizeof(TCHAR); /* an initial guess */

  for(;;){
    DWORD dwError;

    free(*lppStringValue);
    if(!(*lppStringValue=(LPTSTR)malloc(cbStringValue))){
      return ERROR_NOT_ENOUGH_MEMORY;
    }
    if(!(dwError=RegQueryValueEx(hKey,lpValueName,NULL,NULL,
                                 (LPBYTE)*lppStringValue,&cbStringValue))){
      return ERROR_SUCCESS;
    }
    else if(dwError==ERROR_MORE_DATA){
      /* not enough space, keep looping */
    }
    else return dwError;
  }
}
#endif //Windows-only UsbSerial::QueryStringValue( )

//Mac-only
#ifdef Q_WS_MAC

/*static*/ kern_return_t UsbSerial::getDevicePath(io_iterator_t serialPortIterator, char *path, CFIndex maxPathSize)
{
    io_object_t modemService;
    kern_return_t kernResult = KERN_FAILURE;
    Boolean modemFound = false;
    // Initialize the returned path
    *path = '\0';
    
    // Iterate across all modems found. In this example, we bail after finding the first modem.
    while ((modemService = IOIteratorNext(serialPortIterator)) && !modemFound)
    {
			CFTypeRef bsdPathAsCFString;
			bsdPathAsCFString = IORegistryEntryCreateCFProperty(modemService,
                                                            CFSTR(kIOCalloutDeviceKey),
                                                            kCFAllocatorDefault,
                                                            0);
			
			if (bsdPathAsCFString)
			{
				Boolean result;      
				result = CFStringGetCString( (CFStringRef)bsdPathAsCFString,
																		path,
																		maxPathSize, 
																		kCFStringEncodingUTF8);
				CFRelease(bsdPathAsCFString);
				
				if (result)
				{
					printf("Modem found with BSD path: %s", path);
					modemFound = true;
					kernResult = KERN_SUCCESS;
					ourDevice = modemService;
				}
				printf("\n");

				// Release the io_service_t now that we are done with it.
				(void) IOObjectRelease(modemService);
			}
		}
    return kernResult;
}

kern_return_t UsbSerial::findMakeController( io_iterator_t *matchingServices )
{
  // there's probably a good way to combine these steps into one dictionary, but...
	// first, try to find a BSD Serial Device
	kern_return_t err; 
	CFMutableDictionaryRef bsdMatchingDictionary;
	
	bsdMatchingDictionary = IOServiceMatching(kIOSerialBSDServiceValue);
	if (bsdMatchingDictionary == NULL)
		printf("IOServiceMatching returned a NULL dictionary.\n");
	else
		CFDictionarySetValue(bsdMatchingDictionary, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDModemType));
	
	err = IOServiceGetMatchingServices( kIOMasterPortDefault, bsdMatchingDictionary, matchingServices );    
	if ( KERN_SUCCESS != err )
	{
		printf("IOServiceGetMatchingServices returned %d\n", err);
		return err;
	}
	
	// then, try to match it against the vendor and product IDs of the Make Controller Kit
	io_iterator_t iterator = 0;
	//io_service_t usbDeviceReference;
	createMatchingDictionary( );
	
	err = IOServiceGetMatchingServices( masterPort, matchingDictionary, &iterator );
	matchingDictionary = 0;  // consumed by the above call

	if( ( usbDeviceReference = IOIteratorNext( iterator ) ) )
	{
		//messageInterface->message( 2, "usb> Found boot agent.\n" );
		printf( "usb> Found boot agent\n" );
		foundMakeController = true;
		
		setWarningFlag( usbDeviceReference, false );
		//(void) IOObjectRelease( ourDevice );
		
		IOObjectRelease(usbDeviceReference);
		IOObjectRelease(iterator);
		return 0;
	} else
	{
		usbClose( );
		IOObjectRelease(usbDeviceReference);
		IOObjectRelease(iterator);
		return -1;
	}
}

void UsbSerial::createMatchingDictionary( )
{
	SInt32 idVendor = 0xeb03;
	SInt32 idProduct = 0x920;
	CFNumberRef numberRef;
	
	if( !(matchingDictionary = IOServiceMatching( kIOUSBDeviceClassName )) )
	{
		//messageInterface->message( 1, "usb> could not create matching dictionary.\n" );
		printf( "could not create matching dictionary\n" );
		return;
	}
	
	if( !(numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idVendor)) )
	{
		//messageInterface->message( 1, "usb> could not create CFNumberRef for vendor.\n" );
		printf( "could not create CFNumberRef for vendor\n" );
		return;
	}
	CFDictionaryAddValue( matchingDictionary, CFSTR(kUSBVendorID), numberRef );
	CFRelease( numberRef );
	numberRef = 0;
	
	if( !(numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idProduct)) )
	{
		//messageInterface->message( 1, "usb> could not create CFNumberRef for product.\n" );
		printf( "could not create CFNumberRef for product\n" );
		return;
	}
	CFDictionaryAddValue( matchingDictionary, CFSTR(kUSBProductID), numberRef);
	CFRelease( numberRef );
	numberRef = 0;
}

int UsbSerial::sleepMs( long ms )
{
	  usleep( 1000*ms );
		return 0; //TELEO_OK;
}




#define kPathToSystemLog 		"/var/log/system.log"
#define	kwarn					"Warn"
#define knoWarn					"NoWarn"
#define kDriver					"AppleUSBCDCACMData"

/****************************************************************************************************/
//
//		Function:	getInterfaceWithName
//
//		Inputs:		masterPort - the mach master port
//					className - name of the driver
//					state - true(warn), false(nowarn)
//					vendor - Vendor ID
//					product - Product ID
//
//		Outputs:	Return value - kIOReturnSuccess or kIOReturnError
//
//		Desc:		Look through the registry and search for an object with the given
//					name.
//
/****************************************************************************************************/

kern_return_t UsbSerial::getInterfaceWithName( bool state )
{	
	kern_return_t	kr = kIOReturnSuccess;
	bool			done = FALSE;
    io_iterator_t	ite;
    io_object_t		obj = 0;
    io_name_t 		name;
	
	kr = IORegistryCreateIterator(masterPort, kIOServicePlane, true, &ite);
    if (kr != kIOReturnSuccess)
    {
        printf("ACMUCTool: getInterfaceWithName - IORegistryCreateIterator failed %08lx\n", (unsigned long)kr);
        return kIOReturnError;
    }

    while ((obj = IOIteratorNext(ite)))
    {
        if (IOObjectConformsTo(obj, (char *)kDriver))
        {
            printf("ACMUCTool: getInterfaceWithName - Found a driver\n");
			
				// Try and set the flag
			
			if (setWarningFlag( obj, state ))
			{
				done = TRUE;
				break;
			}
        } else {
            kr = IOObjectGetClass( obj, name);
            if (kr == kIOReturnSuccess)
            {
                printf("ACMUCTool: getInterfaceWithName - Skipping class %s\n", name);
            }
        }
        IOObjectRelease(obj);
    }
	
	IORegistryDisposeEnumerator(ite);
    
	if (!done)
	{
		return kIOReturnError;
	}
}



/****************************************************************************************************/
//
//		Function:	setWarningFlag
//
//		Inputs:		port - the port
//					state - true (warning) or false (noWarning)
//
//		Outputs:	Return code - Various
//
//		Desc:		Set the connection state in the driver
//
/****************************************************************************************************/
bool UsbSerial::setWarningFlag( io_object_t		intf, bool state )
{
	UInt16 vendor = 0xeb03;
	UInt16 product = 0x920;
	
	bool			res = FALSE;
	io_connect_t	dataPort;
    dataParms		input;
    statusData		output;
    IOByteCount		insize = sizeof(dataParms);
    IOByteCount		outsize = sizeof(statusData);
    kern_return_t 	kernResult;
	
		// Open and instantiate the user client

	kernResult = IOServiceOpen( intf, mach_task_self(), ACMData_Magic_Key, &dataPort);
	if (kernResult != KERN_SUCCESS)
	{
		printf("ACMUCTool: IOServiceOpen failed.\n");
		return res;
	}
    
	input.command = cmdACMData_Message;
	input.vendor = vendor;
	input.product = product;
    if (state)
    {
        input.message = warning;
    } else {
        input.message = noWarning;
    }
	
    kernResult = IOConnectMethodStructureIStructureO( dataPort, kUserClientdoRequest, insize, &outsize, &input, &output );
    if (kernResult != KERN_SUCCESS)
    {
        printf("ACMUCTool: setWarningFlag failed - %x\n", kernResult);
    } else {
		if (output.status != kSuccess)
		{
			printf("ACMUCTool: setWarningFlag not successful\n");
		} else {
			printf("ACMUCTool: setWarningFlag successful\n");
			res = TRUE;
		}
	}
    
    return res;
    
} /* end setWarningFlag */
#endif


