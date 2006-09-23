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


UsbSerial::UsbSerial( )
{
	deviceOpen = false;
	readInProgress = false;
	#ifdef Q_WS_MAC
  	blocking = false;
	#endif
}

UsbSerial::UsbStatus UsbSerial::usbOpen( )
{
  // Linux Only
  #if (defined(Q_WS_LINUX))
	  printf( "OSC over USB not implemented in Linux\n" );
    return NOT_OPEN;
  #endif

  //Mac-only
	#ifdef Q_WS_MAC
		if( deviceOpen )  //if it's already open, do nothing.
		  return ALREADY_OPEN;
		int result = FindUsbSerialDevice(&deviceName, 0);
		//if( result );  //should return 0 on success
		  //return; 
		
		// now try to actually do something
		deviceHandle = ::open( deviceName, O_RDWR | O_NOCTTY | ( ( blocking ) ? 0 : O_NDELAY ) );
		if ( deviceHandle < 0 )
	  {
	    printf( "Could not open the port (Error %d)\n", deviceHandle );
	    return NOT_OPEN;
	  } else
		{/*
			tcgetattr( deviceHandle, &terminalSettingsOld ); 
			bzero( &terminalSettings, sizeof( terminalSettings ) );
	    
			terminalSettings.c_cflag = B115200 | CS8 | CREAD | CRTSCTS;
			terminalSettings.c_iflag = IGNPAR;
			terminalSettings.c_oflag = 0;
			terminalSettings.c_lflag = 0;
			
			if ( blocking )
			{
				terminalSettings.c_cc[VTIME]    = 0;   // inter-character timer unused
				terminalSettings.c_cc[VMIN]     = 1;   // blocking read until 5 chars received
			}
	    
			tcflush( deviceHandle, TCIFLUSH );
			tcsetattr( deviceHandle, TCSANOW, &terminalSettings );
			*/
	    deviceOpen = true;
			sleepMs( 10 ); //give it a moment after opening before trying to read/write
	    printf( "USB opened at %s\n", deviceName);
	  }
	  return OK;
		
	#endif //Mac-only UsbSerial::open( )
	
  //Windows-only
	#ifdef Q_WS_WIN
		TCHAR* openPorts[32];
		int foundOpen = ScanEnumTree( TEXT("SYSTEM\\CURRENTCONTROLSET\\ENUM\\USB"), openPorts);
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
			}
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
    return NOT_OPEN;
    
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
	    retval = UNKNOWN_ERROR;
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
		    retval = UNKNOWN_ERROR; //( 100 );
	      break;
		  case WAIT_TIMEOUT:
			  //if( debug > 1 ) TRACE_MESSAGE( "timeout occurred while waiting for event\n" );
		    retval = NOTHING_AVAILABLE; //( TELEO_E_NOTHING );
		    break;
		  case WAIT_OBJECT_0:
	  		// check to see if the pending operation completed
        if( !GetOverlappedResult( deviceHandle, &overlappedRead, &numTransferred, FALSE )  ) // don't wait
				{
	        ///FIXME is there anything else i need to clean up?
		      // reset events? terminate anything?
		      usbClose( );
		      retval = IO_ERROR;
				}
		    // messageInterface->message( 1, "USB read I: %c numx %d\n", *buffer, numTransferred );
  			retval = GOT_CHAR;
			  break; 
		  case WAIT_ABANDONED:
		  default:
		    usbClose( );
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
	  printf("usbWrite: write failed, errno %d", errno);
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
    
  if( !deviceOpen )  //then try to open it
  {
    UsbStatus portIsOpen = usbOpen( );
	  if( portIsOpen != OK )
	    return NOT_OPEN;
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
	    	retval = IO_ERROR;
	    }
	  }
	  else
	    retval = IO_ERROR;
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
			FILE_ATTRIBUTE_NORMAL, 0 );
  
  if ( deviceHandle == INVALID_HANDLE_VALUE )
    return -1;

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
    return -1;

  // initialize the overlapped structures
  overlappedRead.Offset  = overlappedWrite.Offset = 0; 
  overlappedRead.OffsetHigh = overlappedWrite.OffsetHigh = 0; 
  overlappedRead.hEvent  = CreateEvent(0, TRUE, FALSE, 0);
  overlappedWrite.hEvent  = CreateEvent(0, TRUE, FALSE, 0);

  if (!overlappedRead.hEvent || !overlappedWrite.hEvent )
  {
	  printf( "Could Not create overlapped events\n");
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
  if( ! SetCommTimeouts( deviceHandle, &timeouts ) )
  {
	  printf( "SetCommTimeouts failed\n");
	  return -1;
  }

  EscapeCommFunction( deviceHandle, SETDTR );

  deviceOpen = true;

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

static int matchUsbSerialDevice (struct dirent * tryThis);

int UsbSerial::FindUsbSerialDevice(cchar** dest, int index)
{
  struct dirent **namelist;
  int n;
  char* tempName;

  if (!dest)
    return -1; //(TELEO_E_ALLOC);
  *dest = NULL;

  n = scandir("/dev", &namelist, matchUsbSerialDevice, NULL);
  if (n < 0)
  {
    perror("scandir");
    return -1; //(TELEO_E_UNKNOWN);
  }
  if (n < index+1)
  {
    return -1; //(TELEO_E_UNKNOWN);
  }
  tempName = (char * ) malloc( sizeof ( namelist[index]->d_name ) + 
		 sizeof ( "/dev/" ) );
  if (!tempName) 
    return -1; //TELEO_E_ALLOC;
  strcpy(tempName, "/dev/");
  strcat(tempName, namelist[index]->d_name);

  for (;n;n--)
    free(namelist[n]);
  free(namelist);

  *dest = tempName;
  return 0; //(TELEO_OK);
}

//static void UsbSerial::message( int level, char* format, ... )
//{
//}

static int matchUsbSerialDevice (struct dirent * tryThis)
{
	if (strncmp("cu.usbmodem", tryThis->d_name, strlen("cu.usbmodem")))
		return 0;
	else
		return 1;
}

int UsbSerial::sleepMs( long ms )
{
	  usleep( 1000*ms );
	    return 0; //TELEO_OK;
}
#endif


