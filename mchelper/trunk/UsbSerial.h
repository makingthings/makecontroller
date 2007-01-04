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
#include <IOKit/IOMessage.h>


typedef const char cchar;

////////////////////////////////////////////////////////////////////////////////////////
#define kUserClientdoRequest	0

#define kSuccess		0
#define kError			1

    // Command codes to pass between user-client and the kext
    // values are arbitrary, but must fit in a byte.
    
enum
{
    cmdACMData_Message	= 100,
    ACMData_Magic_Key	= 'ACM!'			// Magic cookie for connect
};

    // Messages

enum
{
    noWarning		= 0x2000,			// Arbitrary values for now
	warning
};

typedef struct 
{
    UInt8		command;
	UInt8		filler;
    UInt16		message;
    UInt16		vendor;
	UInt16		product;
} dataParms;

typedef struct 
{
    UInt16		status;
} statusData;

////////////////////////////////////////////////////////////////////////////////////////

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
		int ScanEnumTree( LPCTSTR lpEnumPath, TCHAR** pname );
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
		char deviceFilePath[MAXPATHLEN];
		int deviceHandle;
		struct termios terminalSettings;
		struct termios terminalSettingsOld;
		mach_port_t masterPort;
		bool foundMakeController;
		CFMutableDictionaryRef matchingDictionary;
		io_service_t usbDeviceReference;
		
		kern_return_t findMakeController( io_iterator_t *matchingServices );
		kern_return_t getDevicePath(io_iterator_t serialPortIterator, char *path, CFIndex maxPathSize);
		void createMatchingDictionary( );
		#endif
		
	protected:
	#ifdef Q_WS_WIN
	HANDLE deviceHandle;
	#endif
};

#endif // USBSERIAL_H


