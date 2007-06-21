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


#ifndef USBSERIAL_H
#define USBSERIAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <QtGlobal>
#include "MessageInterface.h"
#include <QMutex>
#include <QMainWindow>

//Windows-only
#ifdef Q_WS_WIN

#define _UNICODE
#if (WINVER != 0x0501) // Hacky business for MinGW...this needs to be set BEFORE including windows.h
#define WINVER 0x0501
#endif

#include <windows.h>
#include <tchar.h>

#define OVERLAPPED_HANDLES 2

#endif  //Windows defines/includes 

//Mac only
#ifdef Q_WS_MAC

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <paths.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <AvailabilityMacros.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <CoreFoundation/CFNumber.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/IOMessage.h>

typedef const char cchar;
#endif

class UsbSerial
{									
	public:
	  enum UsbStatus { OK, ALREADY_OPEN, NOT_OPEN, NOTHING_AVAILABLE, IO_ERROR, UNKNOWN_ERROR, GOT_CHAR, ALLOC_ERROR, ERROR_CLOSE };
		UsbSerial( );
		
		UsbStatus usbOpen( );
		void usbClose( );
		int usbRead( char* buffer, int length );
		UsbStatus usbWrite( char* buffer, int length );
		UsbStatus usbWriteChar( char c );
		bool isOpen( );
		int numberOfAvailableBytes( );
		char portName[1024]; // should be protected?
		#ifdef Q_WS_WIN
		HANDLE deviceHandle;
		#endif
		#ifdef Q_WS_MAC
		UsbStatus setDeviceFilePath( char* filePath );
		#endif
		
	protected:
		bool deviceOpen;
		bool readInProgress;
		QMutex usbOpenMutex;
		
	  //Mac-only
		#ifdef Q_WS_MAC
		int sleepMs( long ms );
		#endif
		
	  MessageInterface* messageInterface;
	  QMainWindow* mainWindow;		
		
	private:
		//Windows only
		#ifdef Q_WS_WIN
		UsbStatus openDevice( TCHAR* deviceName );
		bool DoRegisterForNotification( );
		
		OVERLAPPED overlappedRead;
		char readBuffer[512];
		OVERLAPPED overlappedWrite;
		OVERLAPPED overlappedStatus;
		DWORD dwStoredFlags;
		HDEVNOTIFY notificationHandle;
		#endif
		
		//Mac only
		#ifdef Q_WS_MAC
		char deviceFilePath[MAXPATHLEN];
		int deviceHandle;
		mach_port_t masterPort;
		bool foundMakeController;
		io_object_t usbDeviceReference;
		
		
		void createMatchingDictionary( );
		#endif	
};

#endif // USBSERIAL_H


