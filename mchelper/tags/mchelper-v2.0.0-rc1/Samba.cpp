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
// OS X elements from code by Erik Gilling
/*
 * Copyright (C) 2005 Erik Gilling, all rights reserved
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 */

#include "Samba.h"
#include "stdio.h"
#include "errno.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "sys/stat.h"
// #include "loader128_data.h"
#include "loader256_data.h"

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <QThread>

#ifdef Q_WS_WIN
#include "guid829.h"
#endif

// from sam7utils/samba.c
/*
static const char *eprocs[] = {
		"", // 000 
    "ARM946E-S", // 001
    "ARM7TDMI", // 010
    "", // 011
    "ARM920T", // 100
    "ARM926EJ-S", // 101
    "", // 110
    "" // 111
};
*/

#define K 1024

const int nvpsizs[] = {
  /* 0000 */ 0,
  /* 0001 */ 8 * K,
  /* 0010 */ 16 * K,
  /* 0011 */ 32 * K,
  /* 0100 */ -1,
  /* 0101 */ 64 * K,
  /* 0110 */ -1,
  /* 0111 */ 128 * K,
  /* 1000 */ -1,
  /* 1001 */ 256 * K,
  /* 1010 */ 512 * K,
  /* 1011 */ -1,
  /* 1100 */ 1024 * K,
  /* 1101 */ -1,
  /* 1110 */ 2048 * K,
  /* 1111 */ -1
};

const int sramsizs[] = {
  /* 0000 */ -1,
  /* 0001 */ 1 * K,
  /* 0010 */ 2 * K,
  /* 0011 */ -1,
  /* 0100 */ 112 * K,
  /* 0101 */ 4 * K,
  /* 0110 */ 80 * K,
  /* 0111 */ 160 * K,
  /* 1000 */ 8 * K,
  /* 1001 */ 16 * K,
  /* 1010 */ 32 * K,
  /* 1011 */ 64 * K,
  /* 1100 */ 128 * K,
  /* 1101 */ 256 * K,
  /* 1110 */ 96 * K,
  /* 1111 */ 512 * K
};

const struct { unsigned id; const char *name; } archs[] = {
  {AT91_ARCH_AT75Cxx,      "AT75Cxx"},
  {AT91_ARCH_AT91x40,      "AT91x40"},
  {AT91_ARCH_AT91x63,      "AT91x63"},
  {AT91_ARCH_AT91x55,      "AT91x55"},
  {AT91_ARCH_AT91x42,      "AT91x42"},
  {AT91_ARCH_AT91x92,      "AT91x92"},
  {AT91_ARCH_AT91x34,      "AT91x34"},
  {AT91_ARCH_AT91SAM7Axx,  "AT91SAM7Axx"},
  {AT91_ARCH_AT91SAM7Sxx,  "AT91SAM7Sxx"},
  {AT91_ARCH_AT91SAM7XC,   "AT91SAM7XC"},
  {AT91_ARCH_AT91SAM7SExx, "AT91SAM7SExx"},
  {AT91_ARCH_AT91SAM7Lxx,  "AT91SAM7Lxx"},
  {AT91_ARCH_AT91SAM7Xxx,  "AT91SAM7Xxx"},
  {AT91_ARCH_AT91SAM9xx,   "AT91SAM9xx"}
};

// Other stuff

Samba::Samba( SambaMonitor* monitor, MessageInterface* messageInterface )
{	
	#ifdef Q_WS_WIN
	BulkUSB = 0;
	#endif
	this->monitor = monitor;
	this->messageInterface = messageInterface;
}


Samba::Status Samba::connect( QString deviceKey )
{
	if ( usbOpen( deviceKey ) < 0 )
	  return ERROR_INITIALIZING;
	return OK;
}

Samba::Status Samba::disconnect( )
{
  usbClose( );
  return OK;
}

Samba::Status Samba::flashUpload( char* bin_file )
{
  struct stat stbuf;
  size_t loader_len;
  size_t file_len;
  size_t i;
  uint8_t *buff;
  void* file_fd;
  int read_len;
  int ps = samba_chip_info.page_size;
  uint8_t *loader_data;
  int block = 0;
	uploadProgress = 0;
	
	uploader->showStatus( QString( "Starting upload...don't disconnect board."), 3000 );
	
  if( ps == 256 ) 
  {
    loader_data = loader256_data;
    loader_len = sizeof( loader256_data );
  } else {
    printf( "no loader for %d byte pages\n", ps );
    return ERROR_INCORRECT_CHIP_INFO;
  }

  if( stat( bin_file, &stbuf ) < 0 ) {
    printf( "%s not found\n", bin_file );
    return ERROR_COULDNT_FIND_FILE;
  }

  file_len = stbuf.st_size;

  if( (buff = (uint8_t *) malloc( ps ) ) == NULL ) {
    printf( "can't alocate buffer of size 0x%x\n",  ps );
    goto error0;
  }

  if( this->sendFile( 0x00201600, loader_data, loader_len ) < 0 ) {
    printf( "could not upload samba.bin\n" );
    goto error1;
  }
  
  if( (file_fd = fileOpen( bin_file ) ) == 0 ) {
    printf( "could not open %s\n", bin_file );
    return ERROR_COULDNT_OPEN_FILE ;
  }

  // messageInterface->message( 3, "    " );

  for( i=0 ; i<file_len ; i+=ps ) {
    /* set page # */
    if( writeWord( 0x00201400+ps, i/ps ) < 0 ) {
      printf( "could not write page %d address\n", (int) i/ps );
      goto error2;
    }
    
    read_len = ((int)(file_len-i) < ps)?file_len-i:ps;
    /* XXX need to implement safe read */
    int r;
    if( ( r = fileRead( file_fd, (char*)buff, read_len ) ) < read_len ) {
      printf( "could not read 0x%x bytes from file, just got %d\n", read_len, r );
      goto error2;
    }

    if( sendFile( 0x00201400, buff, ps ) < 0 ) {
      printf( "could not send page %d\n", (int) i/ps );
      goto error2;
    }

    if( go( 0x00201600 ) < 0 ) {
      printf( "could not send go command for page %d\n", (int) i/ps );
      goto error2;
    }
    
    //(Every 20k)
    if ( (++block)%5==0 )
		{
      //messageInterface->message( 1, "." );
			uploadProgress = int(1000 * i / file_len);
			uploader->progress( uploadProgress );
		}
  }
  
  free( buff );
  fileClose( file_fd );
 
  
  //messageInterface->message( 3, "\n" );

  return OK;

 error2:
  fileClose( file_fd );

 error1:
  free( buff );

 error0:
  return ERROR_SENDING_FILE;
}

Samba::Status Samba::bootFromFlash( )
{
  /* 
   * word: 5A is key to send any message, 
   *       02 is GPNVM2 to boot from Flash, 
   *       0B is "set this bit"
   */
  uint32_t val;
  
  do {
    if( readWord( 0xffffff68, &val ) < 0 ) {
      return ERROR_SETTING_BOOT_BIT;
    }
  } while( !val & 0x1 );

  if( writeWord( 0xFFFFFF64, 0x5A00020B ) < 0 ) {
    printf( "Couldn't flip the bit to boot from Flash.\n" );
    return ERROR_SETTING_BOOT_BIT;
  }
  
  /* wait for EFC to finish command */
  do {
    if( readWord( 0xffffff68, &val ) < 0 ) {
      return ERROR_SETTING_BOOT_BIT;
    }
  } while( !val & 0x1 );
  
  return OK;
}

Samba::Status Samba::reset( )
{
  /* reset controller at 0xfffffd00
   *
   * RSTC_CR[31..24] = KEY = 0xa5
   * RSTC_CR[3]      = EXTRST
   * RSTC_CR[2]      = PERRST
   * RSTC_CR[0]      = PROCRST
   *
   * EXTRST, PERRST, and PROCRST are all aserted.  A possible
   * feature would be to add an argument to reset to specify 
   * the type of reset wanted.
   *  
   */

  if( writeWord( 0xFFFFFD00, 0xA500000D ) < 0 ) {
    printf( "Couldn't reset target.\n" );
    return ERROR_RESETTING;
  }

  return OK;
}

void Samba::setUploader( UploaderThread* uploader )
{
	this->uploader = uploader;
}

QString Samba::getDeviceKey( )
{
	return deviceKey;
}

int Samba::init( )
{
  
  uint32_t chipid;
  uint16_t response;

  sendCommand( "N#", &response, sizeof( response ) );

/*  if( sendCommand( "N#", &response, sizeof( response ) ) < 0 ) {
    printf( "can't init boot program: %s\n", strerror( errno ) );
    return -1;
  }
*/

  if( readWord( 0xfffff240, &chipid ) < 0 ) {
    return -1;
  }

  samba_chip_info.version = AT91_CHIPID_VERSION( chipid );
  samba_chip_info.eproc = AT91_CHIPID_EPROC( chipid );
  samba_chip_info.nvpsiz = nvpsizs[AT91_CHIPID_NVPSIZ( chipid )];
  samba_chip_info.nvpsiz2 = nvpsizs[AT91_CHIPID_NVPSIZ2( chipid )];
  samba_chip_info.sramsiz = sramsizs[AT91_CHIPID_SRAMSIZ( chipid )];
  samba_chip_info.arch = AT91_CHIPID_ARCH( chipid );
  
  if( samba_chip_info.arch == AT91_ARCH_AT91SAM7Sxx ) 
  {
    switch( samba_chip_info.nvpsiz) 
    {
    case 32*K:
      samba_chip_info.page_size = 128;
      samba_chip_info.lock_bits = 8;
      break;

    case 64*K:
      samba_chip_info.page_size = 128;
      samba_chip_info.lock_bits = 16;
      break;

    case 128*K:
      samba_chip_info.page_size = 256;
      samba_chip_info.lock_bits = 8;
      break;

    case 256*K:
      samba_chip_info.page_size = 256;
      samba_chip_info.lock_bits = 16;
      break;

    default:
      printf( "unknown sam7s flash size %d\n", samba_chip_info.nvpsiz );
      return -1;
    }

  } 
  else if( samba_chip_info.arch == AT91_ARCH_AT91SAM7Xxx || samba_chip_info.arch == AT91_ARCH_AT91SAM7XC )
	{
    switch( samba_chip_info.nvpsiz ) {
    case 128*K:
      samba_chip_info.page_size = 256;
      samba_chip_info.lock_bits = 8;
      break;

    case 256*K:
      samba_chip_info.page_size = 256;
      samba_chip_info.lock_bits = 16;
      break;

    default:
      printf( "unknown sam7x srflashsize %d\n", samba_chip_info.nvpsiz );
      return -1;
    }
  } 
  else 
  {
    printf( "Page size info of %s not known\n", 
	    at91ArchStr( samba_chip_info.arch ) );
    return -1;
  }
/*
  printf("Chip Version: %d\n", samba_chip_info.version );
  printf("Embedded Processor: %s\n", eprocs[samba_chip_info.eproc] );
  printf("NVRAM Region 1 Size: %d K\n", samba_chip_info.nvpsiz / K );
  printf("NVRAM Region 2 Size: %d K\n", samba_chip_info.nvpsiz2 / K );
  printf("SRAM Size: %d K\n", samba_chip_info.sramsiz / K );
  printf("Series: %s\n", at91ArchStr( samba_chip_info.arch ) );
  printf("Page Size: %d bytes\n", samba_chip_info.page_size );
  printf("Lock Regions: %d\n", samba_chip_info.lock_bits );
*/
  return 0;
}

int Samba::readWord( uint32_t addr, uint32_t *value )
{
  char cmd[64];
  int err;

  snprintf( cmd, 64, "w%08X,4#", (unsigned int) addr );

  err = sendCommand( cmd, value, 4 );
  //*value = ntohl( *value );
	#ifdef __BIG_ENDIAN__
	*value = ( ( *value & 0x000000FF ) << 24 ) |
					( ( *value & 0x0000FF00 ) << 8 )  | 
					( ( *value & 0x00FF0000 ) >> 8 )  | 
					( ( *value & 0xFF000000 ) >> 24 );
	#endif

  return err; 
}

int Samba::sendFile( uint32_t addr, uint8_t *buff, int len )
{
  char cmd[64];
  int i=0;

  snprintf( cmd, 64, "S%X,%X#", (unsigned int) addr, (unsigned int) len );

  if( usbWrite( cmd, strlen( cmd ) ) < 0 ) {
    return -1;
  }

  uSleep( 2000 );
  
  for( i=0 ; i<len ; i+=64 ) 
  {
    if( usbWrite( (char*)buff+i, (len-i < 64)? len-i : 64 ) < 0 )
    {
      return -1;
    }
    uSleep( 2000 );
  }
  
  return 0;
}

int Samba::writeWord( uint32_t addr, uint32_t value )
{
  char cmd[64];

  snprintf( cmd, 64, "W%08X,%08X#", (unsigned int) addr,
	    (unsigned int) value );

  return sendCommand( cmd, NULL, 0 );
}

int Samba::go( uint32_t addr )
{
  char cmd[64];
  snprintf( cmd, 64, "G%08X#", (unsigned int) addr );

  return sendCommand( cmd, NULL, 0 );
}

int Samba::sendCommand( char *cmd, void *response, int response_len )
{
  
  if( usbWrite( cmd, strlen( cmd ) ) < 0 ) {
    return -1;
  }
  
  uSleep( 2000 );
    
  if( response_len == 0 ) {
    return 0;
  }

  if( usbRead( (char*)response, response_len ) < 0 ) {
    return -1;
  }

  uSleep( 2000 );
  
  return 0;
}

const char* Samba::at91ArchStr( int id )
{
  int i;
  for( i=0 ; i<(int)(sizeof(archs)/sizeof(*archs)) ; i++ ) {
    if( (int)archs[i].id == id ) {
      return archs[i].name;
    }
  }
  return "";
}

void Samba::uSleep( int usecs )
{
  #ifdef Q_WS_WIN
    Sleep( usecs / 1000 );
  #endif
  #ifdef Q_WS_MAC
    usleep( usecs );
  #endif
  #ifdef Q_WS_LINUX
    usleep( usecs );
  #endif
}

void* Samba::fileOpen( char* name )
{
  #ifdef Q_WS_WIN
    HANDLE f = ::CreateFileA( name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( f == INVALID_HANDLE_VALUE ) 
    {
      return 0;
    }
    return (void*)f;
  #endif
  #ifdef Q_WS_MAC
    int file_fd;
    if( (file_fd = open( name, O_RDONLY )) < 0 ) 
    {
      printf( "can't open %s\n", name );
      return 0;
    }
    return (void*)file_fd;
  #endif
  #ifdef Q_WS_LINUX
    int file_fd;
    if( (file_fd = open( name, O_RDONLY )) < 0 ) 
    {
      printf( "can't open %s\n", name );
      return 0;
    }
    return (void*)file_fd;
  #endif
}

int Samba::fileRead( void* file_fd, char* buff, int length )
{
  int r = 1;
  #ifdef Q_WS_WIN
    DWORD read;
    if ( !::ReadFile( (HANDLE)file_fd, buff, length, &read, NULL ) )
      return -1;
    return (int)read;
  #endif
  #ifdef Q_WS_MAC
    if( ( r = read( (int)file_fd, buff, length ) ) < length )
      return -1;
  #endif
  #ifdef Q_WS_LINUX
    if( ( r = read( (int)file_fd, buff, length ) ) < length )
      return -1;
  #endif
  return r;
}


void Samba::fileClose( void* file_fd )
{
  #ifdef Q_WS_WIN
    CloseHandle( (HANDLE)file_fd );
  #endif
  #ifdef Q_WS_MAC
    close( (int)file_fd );
  #endif
  #ifdef Q_WS_LINUX
    close( (int)file_fd );
  #endif
}

/*
 * USB FUNCTIONS BELOW
 */

#define SAM7_TTY "/dev/at91_0"

int Samba::usbOpen( QString key )
{
  // Linux-only
  #if (defined(Q_WS_LINUX))
  char *dev = NULL;
  if( dev == NULL ) {
    dev = SAM7_TTY;
  }
  if( (io_fd = open( dev, O_RDWR )) < 0 ) {
    printf( "can't open \"%s\": %s\n", dev, strerror( errno ) );
    return -1;
  }
  return init();
  #endif

  // Mac-only
  #ifdef Q_WS_MAC
  masterPort = 0;
  usbDev = NULL;
  intf = NULL;
  inPipeRef = 0;
  outPipeRef = 0;

  // from io_iokit.c
  kern_return_t err;
  CFMutableDictionaryRef matchingDictionary = 0;
  CFNumberRef numberRef;
  SInt32 idVendor = 0x03eb;
  SInt32 idProduct = 0x6124;
  io_iterator_t iterator = 0;
  io_service_t usbDeviceRef;
	
	if( ( err = IOMasterPort( MACH_PORT_NULL, &masterPort ) ) ) 
	{
	  //messageInterface->message( 2, "could not create master port, err = %08x\n", err );
    printf( "could not create master port, err = %08x\n", err );
		return -1;
  }
	
	if( !(matchingDictionary = IOServiceMatching(kIOUSBDeviceClassName)) )
	{
		//messageInterface->message( 1, "usb> could not create matching dictionary.\n" );
    printf( "could not create matching dictionary\n" );
		return -1;
  }
	
	if( !(numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idVendor)) )
	{
		//messageInterface->message( 1, "usb> could not create CFNumberRef for vendor.\n" );
    printf( "could not create CFNumberRef for vendor\n" );
		return -1;
  }
	CFDictionaryAddValue( matchingDictionary, CFSTR(kUSBVendorID), numberRef);
  CFRelease( numberRef );
  numberRef = 0;
	
	if( !(numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idProduct)) )
	{
		//messageInterface->message( 1, "usb> could not create CFNumberRef for product.\n" );
    printf( "could not create CFNumberRef for product\n" );
		return -1;
  }
  CFDictionaryAddValue( matchingDictionary, CFSTR(kUSBProductID), numberRef);
  CFRelease( numberRef );
  numberRef = 0;
	
	err = IOServiceGetMatchingServices( masterPort, matchingDictionary, &iterator );
  matchingDictionary = 0;  // consumed by the above call

  while( (usbDeviceRef = IOIteratorNext( iterator ) ) )
	{
		//printf( "usb> Found boot agent\n" );
		char path[1024];
		IORegistryEntryGetPath( usbDeviceRef, kIOServicePlane, path );
		if( key == QString(path) )
		{
			do_dev( usbDeviceRef );
			IOObjectRelease(usbDeviceRef);
			IOObjectRelease(iterator);
			return init();
		}
  }
	//printf( "cannot find boot agent\n" );
	usbClose( );
	IOObjectRelease(usbDeviceRef);
	IOObjectRelease(iterator);
	return -1;
	#endif /* Mac-only UsbConnection::init( ) */ 
    
  // Windows-only
  #ifdef Q_WS_WIN
  
  char /*message[100], */buffer[2], temp[2];

  int result = testOpen( deviceKey );
  
  if (result == FC_DRIVER_NOT_FOUND ) {
    disconnect();
    return -1;
  } else if (result != FC_OK) {
  	messageInterface->messageThreadSafe( QString( "Usb> Error - cannot open USB.") );
    disconnect();
    return -1;
  }
  
  // Flush buffer
  usbFlushOut();

  // Put Normal PutData mode for the target
  buffer[0] = 'N';
  buffer[1] = '#';
  if (usbWrite( (char*)buffer, 2 ) != FC_OK ) {
  	messageInterface->messageThreadSafe( QString( "Usb> Error initializing - could not write to USB.") );
    return -1;
  }

  // No errors test because 2 case possible : 0 byte or 2 bytes to flush... (depends if the board reset or not)
  usbRead((char*)temp,2);

  BulkUSB = 1; // BulkUSB Mode

  return init();
  
  #endif /* Windows-only usbInit( ) */
}


int Samba::usbWrite( char* buffer, int length )
{
  // Linux-only...
  #ifdef Q_WS_LINUX
  int write_len = 0;
  int ret;

  while( write_len < length ) {
    if( (ret = write( io_fd, buffer + write_len, length - write_len )) < 0 ) {
      return -1;
    }
    write_len += ret;
  }

  return write_len;
  #endif /* linux-only usbWrite( ) */
  
  // Mac-only...
  #ifdef Q_WS_MAC
  if( (*intf)->WritePipe( intf, outPipeRef, buffer, (UInt32) length ) != kIOReturnSuccess )
    messageInterface->messageThreadSafe( QString( "Usb> Write error." ) );

  return length;
  #endif /* Mac-only usbWrite( ) */
  
  // Windows-only...
  #ifdef Q_WS_WIN
  if(m_hPipeOut == INVALID_HANDLE_VALUE)
    return FC_ERROR;

  if( !buffer || !length )
    return FC_NOT_INITIALIZED;

  DWORD dwBytesWritten;

  if(! ::WriteFile(m_hPipeOut, buffer, length, &dwBytesWritten, NULL))
  {
    //DWORD dwErr = ::GetLastError();
    return FC_DRIVER_ERROR;
  }

  return FC_OK;
  #endif  /* Windows-only usbWrite( ) */
}

int Samba::usbRead( char* buffer, int length )
{
  // Linux-only...
  #ifdef Q_WS_LINUX
  uint32_t size;

  size =  read( io_fd, buffer, length );

  return (int)size;
  #endif  /* Linux-only usbWrite( ) */

  // Mac-only...
  #ifdef Q_WS_MAC
  UInt32 size = length;

  if( (*intf)->ReadPipe( intf, inPipeRef, buffer, &size ) != kIOReturnSuccess )
    messageInterface->messageThreadSafe( QString( "Usb> Read error.") );
  
  return (int)size;
  #endif  /* Mac-only usbWrite( ) */

  // Windows-only...
  #ifdef Q_WS_WIN
  int timeout = 0;
  DWORD oldValue = 0;
  if(m_hPipeIn == INVALID_HANDLE_VALUE)
    return FC_ERROR;

  if(!buffer || !length)
    return FC_NOT_INITIALIZED;

  DWORD dwBytesRead;

  DWORD dwBytesToRead = length;
  DWORD dwOffset = 0;
  oldValue = dwBytesToRead;

  do
  {
    if( ! ::ReadFile(m_hPipeIn, ((BYTE *) buffer) + dwOffset, dwBytesToRead, &dwBytesRead, NULL))
    {
      //DWORD dwErr = ::GetLastError();
      return FC_DRIVER_ERROR;
    }

    dwBytesToRead -= dwBytesRead;
    dwOffset += dwBytesRead;
	if (dwBytesRead == 0)
		return FC_DRIVER_ERROR;
  } while((dwBytesToRead != 0) && (timeout != 5));

  
  if (dwBytesToRead != 0)
    return FC_DRIVER_ERROR;

  return FC_OK;
  #endif  /* Windows-only usbRead( ) */
}

int Samba::usbClose( )
{
  // Linux-only...
  #ifdef Q_WS_LINUX
  int ret;
  ret = close(io_fd);
  return ret;
  #endif /* Linux-only UsbConnection::cleanup( ) */

   // Mac-only...
  #ifdef Q_WS_MAC
  if( intf ) {
    (*intf)->USBInterfaceClose(intf);
    (*intf)->Release(intf);
    intf = NULL;
  }

  if( usbDev ) {
    (*usbDev)->USBDeviceClose(usbDev);
    (*usbDev)->Release(usbDev);
    usbDev = NULL;
  }
  
  return 0;
  #endif /* Mac-only UsbConnection::cleanup( ) */
	
  // Windows-only
  #ifdef Q_WS_WIN
  if(m_hPipeIn != INVALID_HANDLE_VALUE)
    ::CloseHandle(m_hPipeIn);
  if(m_hPipeOut != INVALID_HANDLE_VALUE)
    ::CloseHandle(m_hPipeOut);

  m_hPipeIn = INVALID_HANDLE_VALUE;
  m_hPipeOut = INVALID_HANDLE_VALUE;

  return FC_OK;
  #endif  // end: Windows-only disconnect()	
}

// Windows-only...
#ifdef Q_WS_WIN

BOOL Samba::GetUsbDeviceFileName(LPGUID  pGuid, WCHAR **outNameBuf, QString deviceKey)
{
  HANDLE hDev = OpenUsbDevice(pGuid, outNameBuf, deviceKey );

  if(hDev != INVALID_HANDLE_VALUE)
  {
    CloseHandle(hDev);
    return TRUE;
  }
  return FALSE;
}


int Samba::testOpen( QString deviceKey )
{
  WCHAR *sDeviceName;

  WCHAR *sPipeNameIn;
  WCHAR *sPipeNameOut;
  m_hPipeIn = INVALID_HANDLE_VALUE;
  m_hPipeOut = INVALID_HANDLE_VALUE;

  // messageInterface->message( 3, "  Getting usb device name\n" );
  // messageInterface->sleepMs( 100 );
  
  if(! GetUsbDeviceFileName( (LPGUID) &GUID_CLASS_I82930_BULK, &sDeviceName, deviceKey ))
    return FC_DRIVER_NOT_FOUND;

  // messageInterface->message( 3, "  Got usb device name\n" );
  // messageInterface->sleepMs( 100 );

  sPipeNameIn  = (WCHAR *) malloc(::wcslen(sDeviceName) * 2 + 20);
  sPipeNameOut = (WCHAR *) malloc(::wcslen(sDeviceName) * 2 + 20);
  ::wcscpy(sPipeNameIn, sDeviceName);                    
  ::wcscat(sPipeNameIn, L"\\PIPE01");

  ::wcscpy(sPipeNameOut, sDeviceName);
  ::wcscat(sPipeNameOut, L"\\PIPE00");
  free(sDeviceName);

  // messageInterface->message( 3, "  Opening Pipes\n" );
  // messageInterface->sleepMs( 100 );

  m_hPipeIn = ::CreateFile(sPipeNameIn, 
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ,
						   NULL,
						   OPEN_EXISTING, 
                           0, 
						   NULL);

  m_hPipeOut = ::CreateFile(sPipeNameOut, 
                            GENERIC_WRITE,
                            FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING, 
                            0,
							NULL);


  if( (m_hPipeIn == INVALID_HANDLE_VALUE) || (m_hPipeOut == INVALID_HANDLE_VALUE) )
  {
    usbClose();
	int i =GetLastError();
    return i;
  }

  return FC_OK;
}

int Samba::usbFlushOut( )
{
  if(m_hPipeOut == INVALID_HANDLE_VALUE)
    return FC_ERROR;

  ::FlushFileBuffers(m_hPipeOut);
  return FC_OK;	
}
#endif // Windows-only stuff

int Samba::FindUsbDevices( QList<QString>* arrived )
{
  #ifdef Q_WS_MAC
	masterPort = 0;
	int count = 0;

  // from io_iokit.c
  kern_return_t err;
  CFMutableDictionaryRef matchingDictionary = 0;
  CFNumberRef numberRef;
  SInt32 idVendor = 0x03eb;
  SInt32 idProduct = 0x6124;
  io_iterator_t iterator = 0;
  io_registry_entry_t usbDeviceRef;
	
	if( ( err = IOMasterPort( MACH_PORT_NULL, &masterPort ) ) ) 
	{
	  //messageInterface->message( 2, "could not create master port, err = %08x\n", err );
		printf( "could not create master port, err = %08x\n", err );
		return -1;
  }
	
	if( !(matchingDictionary = IOServiceMatching(kIOUSBDeviceClassName)) )
	{
		//messageInterface->message( 1, "usb> could not create matching dictionary.\n" );
		printf( "usb> could not create matching dictionary.\n" );
		return -1;
  }
	
	if( !(numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idVendor)) )
	{
		//messageInterface->message( 1, "usb> could not create CFNumberRef for vendor.\n" );
		printf( "usb> could not create CFNumberRef for vendor.\n" );
		return -1;
  }
	CFDictionaryAddValue( matchingDictionary, CFSTR(kUSBVendorID), numberRef);
  CFRelease( numberRef );
  numberRef = 0;
	
	if( !(numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idProduct)) )
	{
		//messageInterface->message( 1, "usb> could not create CFNumberRef for product.\n" );
		printf( "usb> could not create CFNumberRef for product.\n" );
		return -1;
  }
  CFDictionaryAddValue( matchingDictionary, CFSTR(kUSBProductID), numberRef);
  CFRelease( numberRef );
  numberRef = 0;
	
	err = IOServiceGetMatchingServices( masterPort, matchingDictionary, &iterator );
  matchingDictionary = 0;  // consumed by the above call

  while( (usbDeviceRef = IOIteratorNext( iterator ) ) )
	{
		char path[1024];
		IORegistryEntryGetPath(usbDeviceRef, kIOServicePlane, path );
		deviceKey = QString( path );
		arrived->append( deviceKey );
		IOObjectRelease(usbDeviceRef);
		count++;
  }
	IOObjectRelease(iterator);
	return count;
	#endif // Mac-only FindUsbDevices( )
	
	#ifdef Q_WS_WIN
	HANDLE hOut = INVALID_HANDLE_VALUE;
  HDEVINFO                 hardwareDeviceInfo;
  SP_INTERFACE_DEVICE_DATA deviceInfoData;
  ULONG                    i = 0;
  BOOLEAN					done = FALSE;
  int count = 0;

  //
  // Open a handle to the plug and play dev node.
  // SetupDiGetClassDevs() returns a device information set that contains info on all
  // installed devices of a specified class.
  //
  hardwareDeviceInfo = SetupDiGetClassDevs (
                         (LPGUID) &GUID_CLASS_I82930_BULK,
                         NULL,            // Define no enumerator (global)
                         NULL,            // Define no
                         (DIGCF_PRESENT | // Only Devices present
                         DIGCF_INTERFACEDEVICE)); // Function class devices.

  deviceInfoData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

  while( !done ) 
  {
      // SetupDiEnumDeviceInterfaces() returns information about device interfaces
      // exposed by one or more devices. Each call returns information about one interface;
      // the routine can be called repeatedly to get information about several interfaces
      // exposed by one or more devices.
      if(SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                      0, // We don't care about specific PDOs
                                      (LPGUID) &GUID_CLASS_I82930_BULK,
                                      i++,
                                      &deviceInfoData)) 
      {
        WCHAR *outNameBuf; // dummy
        hOut = OpenOneDevice (hardwareDeviceInfo, &deviceInfoData, &outNameBuf);
        if(hOut != INVALID_HANDLE_VALUE)
        {
			arrived->append( deviceKey );
			count++;
        }
      } 
      else 
      {
        if(ERROR_NO_MORE_ITEMS == GetLastError()) 
        {
           done = TRUE;
           break;
        }
      }
  }

  // SetupDiDestroyDeviceInfoList() destroys a device information set
  // and frees all associated memory.
  SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);

  return count;
	#endif // Windows-only FindUsbDevices( )
}

#ifdef Q_WS_WIN
HANDLE Samba::OpenUsbDevice(LPGUID  pGuid, WCHAR **outNameBuf, QString deviceKey )
{
  HANDLE hOut = INVALID_HANDLE_VALUE;
  HDEVINFO                 hardwareDeviceInfo;
  SP_INTERFACE_DEVICE_DATA deviceInfoData;
  ULONG                    i = 0;
  BOOLEAN					done = FALSE;

  //
  // Open a handle to the plug and play dev node.
  // SetupDiGetClassDevs() returns a device information set that contains info on all
  // installed devices of a specified class.
  //
  hardwareDeviceInfo = SetupDiGetClassDevs (
                         pGuid,
                         NULL,            // Define no enumerator (global)
                         NULL,            // Define no
                         (DIGCF_PRESENT | // Only Devices present
                         DIGCF_INTERFACEDEVICE)); // Function class devices.

  deviceInfoData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

  while( !done ) 
  {
      // SetupDiEnumDeviceInterfaces() returns information about device interfaces
      // exposed by one or more devices. Each call returns information about one interface;
      // the routine can be called repeatedly to get information about several interfaces
      // exposed by one or more devices.
      if(SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                      0, // We don't care about specific PDOs
                                      pGuid,
                                      i++,
                                      &deviceInfoData)) 
      {
        hOut = OpenOneDevice (hardwareDeviceInfo, &deviceInfoData, outNameBuf);
        if(hOut != INVALID_HANDLE_VALUE && deviceKey == this->deviceKey ) 
        {
          	done = TRUE;
          	break;
        }
      } 
      else 
      {
        if(ERROR_NO_MORE_ITEMS == GetLastError()) 
        {
           done = TRUE;
           hOut = INVALID_HANDLE_VALUE;
           break;
        }
      }
  }

  // SetupDiDestroyDeviceInfoList() destroys a device information set
  // and frees all associated memory.
  SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);

  return hOut;
}

//-----------------------------------------------------------------
//                  Windows-only checkFriendlyName( )
//-----------------------------------------------------------------
bool Samba::checkDeviceService( HDEVINFO HardwareDeviceInfo, PSP_DEVINFO_DATA deviceSpecificInfo )
{
	DWORD DataT;
    LPTSTR buffer = NULL;
    DWORD buffersize = 0;
    
    while (!SetupDiGetDeviceRegistryProperty(
               HardwareDeviceInfo,
               deviceSpecificInfo,
               SPDRP_SERVICE, 
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
	{	// if it's using the atm6124 driver, then that's us.
		if(!_tcsncmp(TEXT("atm6124"), buffer, 7))
		{
			LocalFree(buffer);
			return true;
		}
			
		LocalFree(buffer);
	}
		
	return false;
}

//-----------------------------------------------------------------
//                  Windows-only getDeviceObjectName( )
//-----------------------------------------------------------------
bool Samba::getDeviceObjectName( HDEVINFO HardwareDeviceInfo, PSP_DEVINFO_DATA deviceSpecificInfo )
{
	DWORD DataT;
    LPTSTR buffer = NULL;
    DWORD buffersize = 0;
    
    while (!SetupDiGetDeviceRegistryProperty(
               HardwareDeviceInfo,
               deviceSpecificInfo,
               SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, 
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
	{	// this is a unique value for each instance of a SAMBA board in the system
		char name[buffersize], *ptr;
		TCHAR* tptr = buffer;
		ptr = name;
		while( buffersize-- )
			*ptr++ = *tptr++;

		deviceKey = QString( name );
		LocalFree(buffer);
		return true;
	}
		
	return false;
}

HANDLE Samba::OpenOneDevice (HDEVINFO HardwareDeviceInfo,
                                  PSP_INTERFACE_DEVICE_DATA DeviceInfoData,
                                  WCHAR **devName)
{
  PSP_INTERFACE_DEVICE_DETAIL_DATA functionClassDeviceData = NULL;
  ULONG                            predictedLength = 0;
  ULONG                            requiredLength = 0;

  HANDLE hOut = INVALID_HANDLE_VALUE;

  //
  // allocate a function class device data structure to receive the
  // goods about this particular device.
  //
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

  //
  // Retrieve the information from Plug and Play.
  //
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
  
  if( !checkDeviceService( HardwareDeviceInfo, &deviceSpecificInfo ) )
  	return INVALID_HANDLE_VALUE;
  	
  if( !getDeviceObjectName( HardwareDeviceInfo, &deviceSpecificInfo ) )
  	return INVALID_HANDLE_VALUE;

  *devName = wcsdup(functionClassDeviceData->DevicePath);
  //strcpy(devName, functionClassDeviceData->DevicePath) ;

  hOut = CreateFile( functionClassDeviceData->DevicePath,
                     GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL,          // no SECURITY_ATTRIBUTES structure
                     OPEN_EXISTING, // No special create flags
                     FILE_ATTRIBUTE_NORMAL,             // No special attributes
                     NULL);         // No template file

  free(functionClassDeviceData);
  return hOut;
}

#endif // Windows-only stuff

// Mac-only...
#ifdef Q_WS_MAC

int Samba::do_dev( io_service_t usbDeviceRef )
{  
  IOReturn err;
  IOCFPlugInInterface **iodev;		// requires <IOKit/IOCFPlugIn.h>
  SInt32 score;
  UInt8 numConf;
  IOUSBConfigurationDescriptorPtr confDesc;
  IOUSBFindInterfaceRequest interfaceRequest;
  io_iterator_t	iterator;
  io_service_t usbInterfaceRef;
  
  err = IOCreatePlugInInterfaceForService(usbDeviceRef, 
					  kIOUSBDeviceUserClientTypeID,
					  kIOCFPlugInInterfaceID, &iodev, &score);
  if (err || !iodev)
	{
    printf("unable to create plugin. ret = %08x, iodev = %p\n",
	   err, iodev);
		return -1;
    }
  
  err = (*iodev)->QueryInterface(iodev, 
				 CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
				 (LPVOID*)&usbDev);
  IODestroyPlugInInterface(iodev);				// done with this
  
  if (err || !usbDev) {
    printf("unable to create a device interface. ret = %08x, dev = %p\n",
	   err, usbDev);
    return -1;
  }
  
  err = (*usbDev)->USBDeviceOpen(usbDev);
  if (err) {
    printf("unable to open device. ret = %08x\n", err);
    return -1;
  }
  err = (*usbDev)->GetNumberOfConfigurations(usbDev, &numConf);
  if (err || !numConf) {
    printf("unable to obtain the number of configurations. ret = %08x\n", err);
    return -1;
  }

  //printf("found %d configurations\n", numConf);
  err = (*usbDev)->GetConfigurationDescriptorPtr(usbDev, 0, &confDesc);			// get the first config desc (index 0)
  if (err) {
      printf("unable to get config descriptor for index 0\n");
      return -1;
  }
  

  err = (*usbDev)->SetConfiguration(usbDev, confDesc->bConfigurationValue);
  if (err) {
    printf("unable to set the configuration\n");
    return -1;
  }

  // requested class
  interfaceRequest.bInterfaceClass = kIOUSBFindInterfaceDontCare;
  // requested subclass
  interfaceRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;		
  // requested protocol
  interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;		
  // requested alt setting
  interfaceRequest.bAlternateSetting = kIOUSBFindInterfaceDontCare;		
  
  err = (*usbDev)->CreateInterfaceIterator(usbDev, &interfaceRequest, &iterator);
  if (err) {
    printf("unable to create interface iterator\n");
    return -1;
  }
  
	
  //printf( "doing device thing\n" );
   
  while( (usbInterfaceRef = IOIteratorNext(iterator)) )
	{
		if( do_intf( usbInterfaceRef ) == 0 ) {
      IOObjectRelease(iterator);
      iterator = 0;
      return 0;
    }
  } 
    
  IOObjectRelease(iterator);
  iterator = 0;
  return -1;
}
#endif /* Mac-only do_dev( ) */


// Mac-only...
#ifdef Q_WS_MAC

int Samba::do_intf(io_service_t usbInterfaceRef)
{
    IOReturn err;
    IOCFPlugInInterface **iodev;
    SInt32 score;
    UInt8 numPipes;
    int i;
    UInt8 direction, number, transferType, interval;
    UInt16 maxPacketSize;
    
    err = IOCreatePlugInInterfaceForService(usbInterfaceRef, 
					    kIOUSBInterfaceUserClientTypeID, 
					    kIOCFPlugInInterfaceID, 
					    &iodev, &score);
    if( err || !iodev ) {
      printf("unable to create plugin. ret = %08x, iodev = %p\n", err, iodev);
      return -1;
    }
    
    err = (*iodev)->QueryInterface(iodev, 
				   CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID),
				   (LPVOID*)&intf);
    IODestroyPlugInInterface(iodev);
	
    if (err || !intf) {
      printf("unable to create a device interface. ret = %08x, intf = %p\n", 
	     err, intf);
      return -1;
    }

    err = (*intf)->USBInterfaceOpen(intf);
    if (err) {
      printf("unable to open interface. ret = %08x\n", err);
      return -1;
    }
    
    err = (*intf)->GetNumEndpoints(intf, &numPipes);
    if (err) {
      printf("unable to get number of endpoints. ret = %08x\n", err);
      return -1;
    }
    
    //printf("found %d pipes\n", numPipes);
#if 0
    if (numPipes == 0) {
      // try alternate setting 1
      err = (*intf)->SetAlternateInterface(intf, 1);
      if (err) {
	printf("unable to set alternate interface 1. ret = %08x\n", err);
	return -1;
      }
      
      err = (*intf)->GetNumEndpoints(intf, &numPipes);
      if (err) {
	printf("unable to get number of endpoints - alt setting 1. ret = %08x\n",
	       err);
	return -1;
      }
      numPipes = 13;  		// workaround. GetNumEndpoints does not work after SetAlternateInterface
    }

#endif

    if (numPipes) {
      for (i=1; i <= numPipes; i++) {
	
	err = (*intf)->GetPipeProperties(intf, i, &direction, 
					 &number, &transferType,
					 &maxPacketSize, &interval);
	if (err) {
	    printf("unable to get pipe properties for pipe %d, err = %08x\n", i, err);
	    continue;
	}

	if (transferType != kUSBBulk) {
	    printf("skipping pipe %d because it is not a bulk pipe\n", i);
	    continue;
	}
	
	if ((direction == kUSBIn) && !inPipeRef) {
	    printf("grabbing BULK IN pipe index %d, number %d\n",i, number);
	    inPipeRef = i;
	}
	if ((direction == kUSBOut) && !outPipeRef) {
	    printf("grabbing BULK OUT pipe index %d, number %d\n", i, number);
	    outPipeRef = i;
	}
      }
    }

    if( !inPipeRef || !outPipeRef ) {
      (*intf)->USBInterfaceClose(intf);
      (*intf)->Release(intf);
      intf = NULL;
      return -1;
    }

    return 0;
}
#endif /* Mac-only do_intf( ) */
