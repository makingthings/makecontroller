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

//Windows-only
#ifdef Q_WS_WIN

	#define _UNICODE
	#include <windows.h>
	#include <tchar.h>

#endif  //Windows defines/includes

//Mac only
#ifdef Q_WS_MAC
	
	#include <fcntl.h>
	#include <termios.h>
	#include <errno.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <dirent.h>
	
	typedef const char cchar;

#endif

class UsbSerial
{									
	public:
	  enum UsbStatus { OK, ALREADY_OPEN, NOT_OPEN, NOTHING_AVAILABLE, IO_ERROR, UNKNOWN_ERROR, GOT_CHAR, ALLOC_ERROR, ERROR_CLOSE };
		UsbSerial( );
		
		UsbStatus usbOpen( );
		void usbClose( );
		UsbStatus usbRead( char* buffer, int length );
		UsbStatus usbWrite( char* buffer, int length );
		UsbStatus usbWriteChar( char c );
		bool usbIsOpen( );
		
	protected:
	  //Mac-only
		#ifdef Q_WS_MAC
  	  int sleepMs( long ms );
		#endif
		
	  MessageInterface* messageInterface;		
		
	private:
		bool deviceOpen;
		bool readInProgress;
		bool blocking;               // whether it's blocking or not
		QMutex usbOpenMutex;
				
		//Windows only
		#ifdef Q_WS_WIN
			int ScanEnumTree(LPCTSTR lpEnumPath, TCHAR** pname);
			LONG OpenSubKeyByIndex(HKEY hKey,DWORD dwIndex,REGSAM samDesired,PHKEY phkResult);
			LONG QueryStringValue(HKEY hKey,LPCTSTR lpValueName,LPTSTR* lppStringValue);
			int testOpen( TCHAR* deviceName );
			int openDevice( TCHAR* deviceName );
			
					     // the device handle
			OVERLAPPED overlappedRead;
			char readBuffer[512];
			OVERLAPPED overlappedWrite;
		#endif
		
		//Mac only
		#ifdef Q_WS_MAC
			cchar* deviceName;
			int deviceHandle;
			struct termios terminalSettings;
	    struct termios terminalSettingsOld;
			
			int FindUsbSerialDevice(cchar** dest, int index);
		#endif
		
	protected:
	#ifdef Q_WS_WIN
	HANDLE deviceHandle;
	#endif
};

#endif // USBSERIAL_H


