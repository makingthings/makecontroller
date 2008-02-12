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

#include "UsbSerial.h"
#include <QMutexLocker>

#ifdef Q_WS_WIN
#include <dbt.h>
#endif 

UsbSerial::UsbSerial( )
{
	deviceOpen = false;

	#ifdef Q_WS_WIN
	deviceHandle = INVALID_HANDLE_VALUE;
	#endif
}

bool UsbSerial::isOpen( )
{
	return deviceOpen;
}

void UsbSerial::setPortName( QString name )
{
	portName = name;
}

UsbSerial::UsbStatus UsbSerial::open( )
{
  QMutexLocker locker( &usbMutex );
  
  	if( deviceOpen )  //if it's already open, do nothing.
	  return ALREADY_OPEN;
  
  // Linux Only
  #if (defined(Q_WS_LINUX))
	  printf( "OSC over USB not implemented in Linux\n" );
    return NOT_OPEN;
  #endif

  //Mac-only
	#ifdef Q_WS_MAC
		deviceHandle = ::open( portName.toAscii().data(), O_RDWR | O_NOCTTY | O_NDELAY ); 
		if ( deviceHandle < 0 )
	    return NOT_OPEN;
	  else
		{
			usleep( 10000 );
			deviceOpen = true;
			return OK;
	  }
	#endif //Mac-only UsbSerial::open( )
	
  //-----------------------------------------------------------------
  //                  Windows-only usbOpen( )
  //-----------------------------------------------------------------
	#ifdef Q_WS_WIN
	if( deviceHandle == INVALID_HANDLE_VALUE )
		return NOT_OPEN;
	UsbStatus result = openDevice( (TCHAR*)deviceHandle );
	if( result == OK )
	{
		// messageInterface->message( 1, "Usb> Make Controller connected at %s\n", portName );
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

void UsbSerial::close( )
{
  if( deviceOpen )
	{
    QMutexLocker locker( &usbMutex );
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
	}
}

int UsbSerial::read( char* buffer, int length )
{
  // make sure we're open
  if( !deviceOpen )
  {
    UsbStatus portIsOpen = open( );
    if( portIsOpen != OK )
		{
			close( );
			return NOT_OPEN;
		}
  }
	
	QMutexLocker locker( &usbMutex );
  
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
	if( count > 1 )
		return count;
	else
	{
		switch ( count )
		{
			case 0:
			return ERROR_CLOSE; // EOF; possibly file was closed
				break;

			case -1:
				if ( errno == EAGAIN )
					return NOTHING_AVAILABLE;
				else
					return IO_ERROR;
				break;
		}
	}
	return 0; // should never get here
	#endif //Mac-only UsbSerial::read( )
	
  //Windows-only///////////////////////////////////////////////////////////////////////
  #ifdef Q_WS_WIN
  int retVal=0;
    COMSTAT Win_ComStat;
    DWORD Win_BytesRead=0;
    DWORD Win_ErrorMask=0;
    ClearCommError(deviceHandle, &Win_ErrorMask, &Win_ComStat);
    if( (ReadFile(deviceHandle, buffer, (DWORD)length, &Win_BytesRead, NULL)==0) || (Win_BytesRead==0) ) 
	{
		//lastErr=GetLastError();
        retVal=-1;
    }
    else {
        retVal=((int)Win_BytesRead);
    }

    return retVal;
  	
  #endif //Windows-only UsbSerial::read( )//////////////////////////////////////////////////////////////
}

UsbSerial::UsbStatus UsbSerial::write( char* buffer, int length )
{
  if( !deviceOpen )
  {
    UsbStatus portIsOpen = open( );
    if( portIsOpen != OK )
		{
				close( );
			return NOT_OPEN;
		}
  }
  
	QMutexLocker locker( &usbMutex );
	
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
    return IO_ERROR;
	
	#endif //Mac-only UsbSerial::write( )

  //Windows-only
  #ifdef Q_WS_WIN
  int retVal=0;
    DWORD Win_BytesWritten;
    if (!WriteFile(deviceHandle, (void*)buffer, (DWORD)length, &Win_BytesWritten, NULL)) {
        //lastErr=E_WRITE_FAILED;
        retVal=-1;
    }
    else {
        retVal=((int)Win_BytesWritten);
    }

    //flush();
    return OK;
  
  #endif //Windows-only UsbSerial::write( )
}

int UsbSerial::bytesAvailable( )
{
    int n = 0;
		QMutexLocker locker( &usbMutex );
		#ifdef Q_WS_MAC
		if( ::ioctl( deviceHandle, FIONREAD, &n ) < 0 )
			return IO_ERROR;
		#endif // Mac-only numberOfAvailableBytes( )
		
		#ifdef Q_WS_WIN
		COMSTAT status;
    unsigned long state;

    if (deviceHandle != INVALID_HANDLE_VALUE)
    {
        bool success = false;
        success = ClearCommError( deviceHandle, &state, &status);
        n = status.cbInQue;
    }
		#endif // Windows-only numberOfAvailableBytes( )
    return(n);
}

QString UsbSerial::name( )
{
	return portName;
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
  deviceHandle = CreateFile( deviceName, GENERIC_READ|GENERIC_WRITE,
                              FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
                              OPEN_EXISTING, 0, NULL);
  
  if ( deviceHandle == INVALID_HANDLE_VALUE )
    return ERROR_CLOSE; 

  // initialize the overlapped structures
//  overlappedRead.Offset  = overlappedWrite.Offset = 0; 
//  overlappedRead.OffsetHigh = overlappedWrite.OffsetHigh = 0; 
//  overlappedRead.hEvent  = CreateEvent(0, TRUE, FALSE, 0);
//  overlappedWrite.hEvent  = CreateEvent(0, TRUE, FALSE, 0);
//
//  if (!overlappedRead.hEvent || !overlappedWrite.hEvent )
//  {
//  	deviceHandle = INVALID_HANDLE_VALUE;
//    return ERROR_CLOSE;
//  }

  GetCommState( deviceHandle, &dcb );
  dcb.BaudRate = CBR_115200;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;
  dcb.fOutxCtsFlow = TRUE;
  dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
  dcb.fAbortOnError = TRUE;
  dcb.fDtrControl = DTR_CONTROL_ENABLE; // magic testing...
  if( !SetCommState( deviceHandle, &dcb ) )
  {
  	deviceHandle = INVALID_HANDLE_VALUE;
		return ERROR_CLOSE;
  }

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
  {
  	deviceHandle = INVALID_HANDLE_VALUE;
  	return ERROR_CLOSE;
  }

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



