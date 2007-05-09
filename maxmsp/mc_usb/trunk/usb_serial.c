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

#include "mcError.h"
#include "usb_serial.h"
#include "ext.h" //for calling post() to the Max window.


t_usbInterface* usb_init( cchar* name, t_usbInterface** uip )
{
  t_usbInterface* usbInt;
  usbInt = (t_usbInterface*)malloc( sizeof( t_usbInterface ) );
	
  usbInt->deviceOpen = false;
  usbInt->readInProgress = false;
  #ifndef WIN32
  usbInt->blocking = false;
  #endif
		
  return usbInt;
}

int usb_open( t_usbInterface* usbInt )
{		
  //--------------------------------------- Mac-only -------------------------------
  #ifndef WIN32
  
  if( usbInt->deviceOpen )  //if it's already open, do nothing.
    return MC_ALREADY_OPEN;

  kern_return_t kernResult;
	io_iterator_t iterator = 0;
	
	kernResult = getDevicePath( iterator, usbInt->deviceLocation, sizeof(usbInt->deviceLocation) );
	IOObjectRelease( iterator ); // clean up
	
	if (!usbInt->deviceLocation[0] )
	{
		//post("Didn't find a Make Controller!.\n");
		return MC_NOT_OPEN;
	}
		
  // now try to actually do something
  usbInt->deviceHandle = open( usbInt->deviceLocation, O_RDWR | O_NOCTTY | ( ( usbInt->blocking ) ? 0 : O_NDELAY ) );
  if ( usbInt->deviceHandle < 0 )
  {
    //post( "Could not open the port (Error %d)\n", usbInt->deviceHandle );
    return MC_NOT_OPEN;
  } else
  {
    usbInt->deviceOpen = true;
		usleep( 10000 ); //give it a moment after opening before trying to read/write
		//post( "USB opened at %s, deviceHandle = %d", usbInt->deviceName, usbInt->deviceHandle);
		post( "mc.usb connected to a Make Controller." );
  }
  return MC_OK;
  #endif
		
  //--------------------------------------- Windows-only -------------------------------
  
#ifdef WIN32
  TCHAR* openPorts[32];
  int i;
  int foundOpen;
  bool result;

  if( usbInt->deviceOpen )  //if it's already open, do nothing.
    return MC_ALREADY_OPEN;

  foundOpen = ScanEnumTree( usbInt, TEXT("SYSTEM\\CURRENTCONTROLSET\\ENUM\\USB"), openPorts);
  //post( "Found %d ports open", foundOpen );

  for( i = 0; i < foundOpen; i++ )
  {
    if( openPorts[i] != NULL )
	{
	  if( openDevice( usbInt, openPorts[i] ) == 0 )
	  {
		sprintf( usbInt->deviceLocation, "%s", openPorts[i] );
		post( "mc.usb connected to a Make Controller Kit at: %s", usbInt->deviceLocation );

		// now set up to get called back when it's unplugged
		result = DoRegisterForNotification( usbInt );

		Sleep( 10 );  // wait after opening it before trying to read/write
		usbInt->deviceOpen = true;
		return MC_OK;
	  }
	}
  }
  //post( "mc.usb did not open." );
  return MC_NOT_OPEN;
  #endif
}

void usb_close( t_usbInterface* usbInt )
{
  if( usbInt->deviceOpen )
  {
    //Mac-only
    #ifndef WIN32
    close( usbInt->deviceHandle );
    usbInt->deviceHandle = -1;
    usbInt->deviceOpen = false;
    #endif
	//Windows only
    #ifdef WIN32
    CloseHandle( usbInt->deviceHandle );
	UnregisterDeviceNotification( usbInt->deviceNotificationHandle );
    usbInt->deviceHandle = INVALID_HANDLE_VALUE;
    usbInt->deviceOpen = false;
    #endif
	post( "mc.usb closed the Make Controller Kit USB connection." );
  }
}

int usb_read( t_usbInterface* usbInt, char* buffer, int length )
{
  //--------------------------------------- Mac-only -------------------------------
  #ifndef WIN32
  int count;
	
  if( !usbInt->deviceOpen )
  {
    //post( "Didn't think the port was open." );
		int portIsOpen = usb_open( usbInt );
		if( portIsOpen != MC_OK )
	  return MC_NOT_OPEN;
  }
		
  count = read( usbInt->deviceHandle, buffer, length );
  //post( "Reading..." );
  switch ( count )
  {
    case 0:
      // EOF; possibly file was closed
			//post( "End of file." );
			return MC_ERROR_CLOSE;
      break;

    case 1:
      // got a character
			//post( "Got a character: %s", buffer );
      return MC_GOT_CHAR;
      break;

    case -1:
      if ( errno == EAGAIN )
			{
	      // non-blocking but no data available
				//post( "No data available." );
	      return MC_NOTHING_AVAILABLE;
			}
      else
			  //post( "Some other error...errno = %d", errno );
	      return MC_IO_ERROR;
      break;

    default:
		  //post( "Unknown error.\n" );
      return MC_UNKNOWN_ERROR;
      break;
  }
	#endif
	
  //--------------------------------------- Windows-only -------------------------------
  #ifdef WIN32
  //Windows-only
  DWORD count;
  int retval;
  DWORD numTransferred;
  DWORD lastError;

  DWORD waitState;
  //int length = 0;
  
  // make sure we're open
  if( !usbInt->deviceOpen )
  {
    //post( "Didn't think the port was open." );
	int portIsOpen = usb_open( usbInt );
	if( portIsOpen != MC_OK )
	  return MC_NOT_OPEN;
  }
    
  retval = MC_OK;  
  //usbInt->readInProgress = false;
  usbInt->overlappedRead.hEvent = NULL; //CreateEvent(NULL, TRUE, FALSE, NULL);

  // reset the read overlapped structure
  usbInt->overlappedRead.Offset = usbInt->overlappedRead.OffsetHigh = 0;
  
  if( !usbInt->readInProgress )
  {
	if ( !ReadFile( usbInt->deviceHandle, buffer, length, &count, &usbInt->overlappedRead ) )
    {	  
	  lastError = GetLastError();
	  //post( "USB Read Error: %d", lastError );
	  if ( GetLastError() != ERROR_IO_PENDING)     // read not delayed?
	    retval = MC_UNKNOWN_ERROR;
	  else
	    return usbInt->readInProgress = true;
    }
    else          	
    {
      //post( "USB read D: %c", *buffer );
  	  retval = MC_GOT_CHAR;
    }
  }

  if( usbInt->readInProgress )
  {
    if( HasOverlappedIoCompleted( &usbInt->overlappedRead ) )
	{
      if(!GetOverlappedResult( usbInt->deviceHandle, &usbInt->overlappedRead, &numTransferred, FALSE ) ) // don't wait
	  {
          ///FIXME is there anything else i need to clean up?
	      // reset events? terminate anything?
          usbInt->readInProgress = false;
	      usb_close( usbInt );
	      return MC_IO_ERROR;
	  }
		// read completed, no errors
      usbInt->readInProgress = false;
	  //*cp = usbInt->readBuffer;
	  return MC_GOT_CHAR;
	}
	else
	{
	  //post( "Nothing Available." );
	  return MC_NOTHING_AVAILABLE;
	}
  }
/*
  if( usbInt->readInProgress )
  {
  	DWORD r;
	  do
	  {
	    post( "Got in here - waiting for single object." );
		r = WaitForSingleObject( usbInt->overlappedRead.hEvent, 1 );	 
	  } while ( r == WAIT_TIMEOUT );
	  switch( r )
		{
		  case WAIT_FAILED:
		    //FIXME is there anything else i need to clean up?
		    // reset events? terminate anything?
			post( "Wait failed." );
		    usb_close( usbInt );
		    retval = MC_UNKNOWN_ERROR; //( 100 );
	        break;
		  case WAIT_TIMEOUT:
			//if( debug > 1 ) TRACE_MESSAGE( "timeout occurred while waiting for event\n" );
			post( "Wait timeout." );
		    retval = MC_NOTHING_AVAILABLE; //( TELEO_E_NOTHING );
		    break;
		  case WAIT_OBJECT_0:
	  		// check to see if the pending operation completed
			post( "Checking overlapped result." );
            if( !GetOverlappedResult( usbInt->deviceHandle, &usbInt->overlappedRead, &numTransferred, FALSE ) ) // don't wait
			{
	          //FIXME is there anything else i need to clean up?
		      // reset events? terminate anything?
		      usb_close( usbInt );
		      retval = MC_IO_ERROR;
			}
		    post( "USB read I: %c numx %d", *buffer, numTransferred );
  			retval = MC_GOT_CHAR;
			break; 
		  case WAIT_ABANDONED:
		  default:
			post( "Wait abandoned, or something else." );
		    usb_close( usbInt );
		    retval = MC_IO_ERROR;
			break;
		} // end of switch
  }
  */

  CloseHandle( usbInt->overlappedRead.hEvent );
  return retval;
  	
  #endif //Windows-only usbRead( )	
}

int usb_write( t_usbInterface* usbInt, char* buffer, int length )
{ 
  //--------------------------------------- Mac-only -------------------------------
  #ifndef WIN32
	if( !usbInt->deviceOpen )  //then try to open it
	{
	  int portIsOpen = usb_open( usbInt );
      if( portIsOpen != MC_OK )
	    return MC_NOT_OPEN;
	}
	int size = write( usbInt->deviceHandle, buffer, length );
	if ( length == size )
		return MC_OK;
	else if( errno == EAGAIN )
	{
	  //post( "Nothing available. ");
		return MC_NOTHING_AVAILABLE;
	}
    else
    {
	  //post("usbWrite: write failed, errno %d", errno);
      usb_close( usbInt );
      return MC_IO_ERROR;
    }
  #endif
	
  //--------------------------------------- Windows-only -------------------------------
  #ifdef WIN32
  DWORD cout;
  int read = 0;
  bool success;
  DWORD ret;
  int retval = MC_OK;
  DWORD numWritten;
  int portIsOpen;

  if( !usbInt->deviceOpen )  //then try to open it
	{
	  portIsOpen = usb_open( usbInt );
      if( portIsOpen != MC_OK )
	    return MC_NOT_OPEN;
	}
  
  usbInt->overlappedWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  
	// reset the write overlapped structure
  usbInt->overlappedWrite.Offset = usbInt->overlappedWrite.OffsetHigh = 0; 
  // messageInterface->message( 1, "Writing...\n" );
  success = WriteFile( usbInt->deviceHandle, buffer, length, &cout, &usbInt->overlappedWrite ) ;
  
  if( !success )
  {
  	if ( GetLastError() == ERROR_IO_PENDING)
	  {
	  	//post( 1, "Write: IO PENDING.\n" );
		  do
		  {
	  		//if( debug > 4 )TRACE_MESSAGE("TI_put(): Waiting for overlapped write to complete");
		  	ret = WaitForSingleObject( usbInt->overlappedWrite.hEvent, 1000 );
		  }  while ( ret == WAIT_TIMEOUT );
	
	    if ( ret == WAIT_OBJECT_0 )
	    {
			do
			{
				GetOverlappedResult( usbInt->deviceHandle, &usbInt->overlappedWrite, &numWritten, TRUE);
				read += numWritten;
			} while( read != length );

			if( read == length )
				retval = MC_OK;
			else
			  retval = MC_IO_ERROR;
	    }
	    else
	    {
	    	retval = MC_IO_ERROR;
	    }
	  }
	  else
	    retval = MC_IO_ERROR;
  }
  
  CloseHandle( usbInt->overlappedWrite.hEvent );
  return retval;
  
  #endif //Windows-only usb_write( )
}

int usb_writeChar( t_usbInterface* usbInt, char c )
{
	return usb_write( usbInt, &c, 1 );
}


//--------------------------------------- Mac-only -------------------------------
#ifndef WIN32

kern_return_t getDevicePath(io_iterator_t serialPortIterator, char *path, CFIndex maxPathSize)
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
		//post("IOServiceGetMatchingServices returned %d\n", kernResult);
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

#endif

// Windows specific functions
#ifdef WIN32

LONG QueryStringValue(HKEY hKey,LPCTSTR lpValueName,LPTSTR* lppStringValue)
{
  DWORD cbStringValue=128*sizeof(TCHAR); /* an initial guess */

  for(;;)
  {
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

LONG OpenSubKeyByIndex(HKEY hKey,DWORD dwIndex,REGSAM samDesired,PHKEY phkResult)
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

int ScanEnumTree( t_usbInterface* usbInt, LPCTSTR lpEnumPath, TCHAR** openPorts )
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
  int result;
  int openCount = 0;

  typedef struct
  {
    LPCTSTR lpPortName;     /* "COM1", etc. */
    LPCTSTR lpFriendlyName; /* Suitable to describe the port, as for  */
                          /* instance "Infrared serial port (COM4)" */
  }LISTPORTS_PORTINFO;

  if(dwError=RegOpenKeyEx(HKEY_LOCAL_MACHINE,lpEnumPath,0,KEY_ENUMERATE_SUB_KEYS,&hkEnum)){
    goto end;
  }

  for(dwIndex1=0;;++dwIndex1)
  {
	//post("finding loop; dwIndex1 = %d\n", dwIndex1);

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
         //post("found port, name %ls\n", lpPortName);

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
			//post( "Friendly name: %s\n", portinfo.lpFriendlyName );
			if (!_tcsncmp(TEXT("Make Controller Kit"), portinfo.lpFriendlyName, 19))
			{
				TCHAR* pname;
				pname = _tcsdup(portinfo.lpPortName);
				// We've found a matching entry in the registry...
				// Now see if it's actually there by trying to open it
				result = testOpen( usbInt, pname );
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
    return 0;
  }
  else return openCount;
}

int testOpen( t_usbInterface* usbInt, TCHAR* deviceName )
{
  usbInt->deviceHandle = CreateFile( deviceName, 
			GENERIC_READ | GENERIC_WRITE, 
			0, 
			0, 
			OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, 0 );
  
  if ( usbInt->deviceHandle == INVALID_HANDLE_VALUE )
    return -1;

  // otherwise, we found one.
  // close it again immediately - we were just checking to see if anything was there...
  CloseHandle( usbInt->deviceHandle );
  usbInt->deviceHandle = INVALID_HANDLE_VALUE;
  return 0;
}

int openDevice( t_usbInterface* usbInt, TCHAR* deviceName )
{
  DCB dcb;
  COMMTIMEOUTS timeouts;

  // if it's already open, do nothing
  if( usbInt->deviceOpen )
    return 0;

  // Open the port
  usbInt->deviceHandle = CreateFile( deviceName, 
			GENERIC_READ | GENERIC_WRITE, 
			0, 
			0, 
			OPEN_EXISTING, 
			FILE_FLAG_OVERLAPPED, 
			0 );
  
  if ( usbInt->deviceHandle == INVALID_HANDLE_VALUE )
    return -1;

  // initialize the overlapped structures
  usbInt->overlappedRead.Offset  = usbInt->overlappedWrite.Offset = 0; 
  usbInt->overlappedRead.OffsetHigh = usbInt->overlappedWrite.OffsetHigh = 0; 
  usbInt->overlappedRead.hEvent = CreateEvent(0, TRUE, FALSE, 0);
  usbInt->overlappedWrite.hEvent  = CreateEvent(0, TRUE, FALSE, 0);

  if (!usbInt->overlappedRead.hEvent || !usbInt->overlappedWrite.hEvent )
  {
	  printf( "Could Not create overlapped events\n");
    return -1;
  }

  GetCommState( usbInt->deviceHandle, &dcb );
  dcb.BaudRate = CBR_115200;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;
  dcb.fOutxCtsFlow = TRUE;
  dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
  dcb.fAbortOnError = TRUE;
  if( !SetCommState( usbInt->deviceHandle, &dcb ) )
  {
	  printf( "SetCommState failed\n");
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
  if( ! SetCommTimeouts( usbInt->deviceHandle, &timeouts ) )
  {
	  printf( "SetCommTimeouts failed\n");
	  return -1;
  }

  EscapeCommFunction( usbInt->deviceHandle, SETDTR );

  usbInt->deviceOpen = true;

  return 0;
}

 // make sure to do this only once the device has been opened,
 // since it relies on the deviceHandle to register for the call.
bool DoRegisterForNotification( t_usbInterface* usbInt )
{
	DEV_BROADCAST_HANDLE NotificationFilter;
	HWND winId;
	bool test;
	
	ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
	NotificationFilter.dbch_size = sizeof( DEV_BROADCAST_HANDLE );
	NotificationFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
	NotificationFilter.dbch_handle = usbInt->deviceHandle;

	winId = main_get_frame( );
	if( winId )
		post( "Got winId!" );
	else
		post( "Didn't get winId" ); 

    usbInt->deviceNotificationHandle = RegisterDeviceNotification( winId, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE );

    if( !usbInt->deviceNotificationHandle ) 
    {
        post( "RegisterDeviceNotification failed: %ld", GetLastError() );
        return false;
    }
    post( "RegisterDeviceNotification success!" ); 
	
    return true;
}

#endif



