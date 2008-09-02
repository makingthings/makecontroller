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

#include "usb_serial.h"
#include "usb_enum.h"
#include "ext.h" //for calling post() to the Max window.

#ifndef WIN32
#include <sys/ioctl.h>
#endif

t_usbInterface* usb_init( cchar* name, t_usbInterface** uip )
{
  t_usbInterface* usbInt = (t_usbInterface*)malloc( sizeof( t_usbInterface ) );
  usbInt->deviceOpen = false;
  usbInt->readInProgress = false;
  return usbInt;
}

int usb_open( t_usbInterface* usbInt, int devicetype )
{		
  //--------------------------------------- Mac-only -------------------------------
  #ifndef WIN32
  
  if( usbInt->deviceOpen )  //if it's already open, do nothing.
    return USB_E_ALREADY_OPEN;
	
	bool success = findUsbDevice( usbInt, devicetype );
	
	if (!success )
		return USB_E_NOT_OPEN;
		
  // now try to actually do something
  usbInt->deviceHandle = open( usbInt->deviceLocation, O_RDWR | O_NOCTTY | O_NDELAY );
  if ( usbInt->deviceHandle < 0 )
    return USB_E_NOT_OPEN;
  else
  {
    usbInt->deviceOpen = true;
		//post( "USB opened at %s, deviceHandle = %d", usbInt->deviceName, usbInt->deviceHandle);
    return USB_OK;
  }
  #endif
		
  //--------------------------------------- Windows-only -------------------------------
  
#ifdef WIN32
  bool result;

  if( usbInt->deviceOpen )  //if it's already open, do nothing.
    return USB_E_ALREADY_OPEN;
  
  if( findUsbDevice( usbInt ) )
	{
	  if( openDevice( usbInt ) == 0 )
	  {
		  // now set up to get called back when it's unplugged
		  result = DoRegisterForNotification( usbInt );

		  Sleep( 10 );  // wait after opening it before trying to read/write
		  usbInt->deviceOpen = true;
		  return USB_OK;
	  }
  }
  //post( "mc.usb did not open." );
  return USB_E_NOT_OPEN;
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
  }
}

int usb_read( t_usbInterface* usbInt, char* buffer, int length )
{
  //--------------------------------------- Mac-only -------------------------------
  #ifndef WIN32
  int count;
	
  if( !usbInt->deviceOpen )
    return USB_E_NOT_OPEN;
  
  count = read( usbInt->deviceHandle, buffer, length );
	if( count < 1 )
	{
		int retval = USB_E_IOERROR;
		if( count == 0 )
			retval = USB_E_CLOSE;
		else if( count == -1 )
		{
			if ( errno == EAGAIN )
	      retval = USB_E_NOTHING_AVAILABLE; // non-blocking but no data available
      else
			  //post( "Some other error...errno = %d", errno );
	      retval = USB_E_IOERROR;
		}
		return retval;
	}
	else
		return count;
	#endif
	
  //--------------------------------------- Windows-only -------------------------------
  #ifdef WIN32
  //Windows-only
    int retVal=0;
    COMSTAT Win_ComStat;
    DWORD Win_BytesRead=0;
    DWORD Win_ErrorMask=0;
    ClearCommError( usbInt->deviceHandle, &Win_ErrorMask, &Win_ComStat);
    if( (ReadFile( usbInt->deviceHandle, buffer, (DWORD)length, &Win_BytesRead, NULL)==0) || (Win_BytesRead==0) ) 
	{
		//lastErr=GetLastError();
        retVal=-1;
    }
    else {
        retVal=((int)Win_BytesRead);
    }

    return retVal;
  #endif //Windows-only usbRead( )	
}

int usb_write( t_usbInterface* usbInt, char* buffer, int length )
{ 
  //--------------------------------------- Mac-only -------------------------------
  #ifndef WIN32
	if( !usbInt->deviceOpen )  //then try to open it
    return USB_E_NOT_OPEN;
  
	int size = write( usbInt->deviceHandle, buffer, length );
	if ( length == size )
		return USB_OK;
	else if( errno == EAGAIN )
	{
	  //post( "Nothing available. ");
		return USB_E_NOTHING_AVAILABLE;
	}
    else
    {
	  //post("usbWrite: write failed, errno %d", errno);
      usb_close( usbInt );
      return USB_E_IOERROR;
    }
  #endif
	
  //--------------------------------------- Windows-only -------------------------------
  #ifdef WIN32
  int retVal=0;
  DWORD Win_BytesWritten;
  if (!WriteFile( usbInt->deviceHandle, (void*)buffer, (DWORD)length, &Win_BytesWritten, NULL))
    retVal=-1;
  else
    retVal=((int)Win_BytesWritten);

  return retVal;
  #endif //Windows-only usb_write( )
}

int usb_writeChar( t_usbInterface* usbInt, char c )
{
	return usb_write( usbInt, &c, 1 );
}

int usb_numBytesAvailable( t_usbInterface* usbInt )
{
	int n = 0;

	#ifndef WIN32
	if( ioctl( usbInt->deviceHandle, FIONREAD, &n ) < 0 )
	{
		// ioctl error
		return USB_E_CLOSE;
	}
	#endif // Mac-only usb_numBytesAvailable( )

	#ifdef WIN32
	COMSTAT status;
	unsigned long   state;

	if (usbInt->deviceHandle != INVALID_HANDLE_VALUE)
	{
			bool success = false;
			success = ClearCommError( usbInt->deviceHandle, &state, &status);
			n = status.cbInQue;
	}
	#endif // Windows-only usb_numBytesAvailable( )

	return n;
}

// Windows specific functions
#ifdef WIN32

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

int openDevice( t_usbInterface* usbInt )
{
  DCB dcb;
  COMMTIMEOUTS timeouts;

  // if it's already open, do nothing
  if( usbInt->deviceOpen )
    return 0;

  // Open the port
  usbInt->deviceHandle = CreateFile( (TCHAR*)usbInt->deviceHandle,
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

  //maxWind = wind_new( x, 0, 0, 0, 0, 0 );
  
  //winId = wind_gethwnd( maxWind );
  //wind_gethwnd( maxWind )

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



