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

#ifndef SAMBA_H
#define SAMBA_H

#include <QtGlobal>
#include <QString>
#include <QList>

// Linux-only includes
#ifdef Q_WS_LINUX
//#error testing...
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#endif


// Mac-only includes
#ifdef Q_WS_MAC
#include <mach/mach.h>
#include <CoreFoundation/CFNumber.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include "stdio.h"
#include <unistd.h>
#include <sys/stat.h>
#endif

// Windows-only
#ifdef Q_WS_WIN
#include "windows.h"
#include "SetupAPI.h"
#endif

// Windows-only
#ifdef Q_WS_WIN
////////////////////////////////////////////////////////////////////////
// from SAMBA source

#define	FC_OK	            	        0	//OK

//ERROR
#define FC_ERROR_MASK	            (short)(0x8000) 
#define FC_CODE_MASK	            (short)(0x7fff)

//Critical Errors
#define FC_ERROR	                (short)(0x8000)    //Internal error
#define FC_DLL_NOT_INITIALIZED	    (short)(0x8001)
#define FC_NOT_OPEN	                (short)(0x8002) //Device not opened
#define	FC_DRIVER_NOT_FOUND	        (short)(0x8004) //driver may not be installed 
#define FC_DEVICE_NOT_CONNECTED     (short)(0x8008) //Device may be  not connected
#define FC_DRIVER_ERROR	            (short)(0x8010) //Error during driver  acquisition
                                                //or device not plugged
#define FC_BUSY	                    (short)(0x8020) //device busy
#define FC_NOT_ENOUGH_MEMORY	    (short)(0x8040) //can't allocate memory
#define FC_NOT_INITIALIZED	        (short)(0x8080) //parameter not initialized
#define	FC_NO_SLICE_MEMORY	        (short)(0x8100) //no slice memory
#define	FC_NO_IMAGE_MEMORY	        (short)(0x8200) //no image memory

#define FC_SYNCHRO_LOST	            (short)(0x9001) //synchro bytes lost
#define FC_WRONG_SIZE_SLICE	        (short)(0x9002) //wrong slice size 
#ifdef WIN32
#define FC_WRONG_IMAGE_WIDTH	    (short)(0x9004)
#endif

#define FC_NO_BANDWIDTH	            (short)(0xA001) //No bandwidth allocated for usb driver
#define FC_OPERATION_ABORTED        (short)(0xC000) //Operation was aborted

//Not Critical Errors
#define FC_BAD_FINGER	            (short)(0xD001) //finger not swept fine
#define FC_IMAGE_TOO_BIG	        (short)(0xD002) //image buffer too big
#define	FC_IMAGE_TOO_SMALL	        (short)(0xD004) //image is too small
#define FC_HEIGHT_TOO_LITTLE	    (short)(0xD008) //Image height to little
#define FC_TOO_SLOW	                (short)(0xD010) //sweeping was too slow
#define FC_TOO_FAST	                (short)(0xD020) //sweeping was too fast
#define FC_NOT_ENOUGH_GOOD_SLICE    (short)(0xD040)//not enough good slices
#define FC_TIME_OUT	                (short)(0xD080) //timeOut
#define FC_NOT_SUPPORTED            (short)(0xD100) //Parameter or function not 
                                          //supported by the current device
//WARNING
#define FC_DIRECTORY_MISSING	    (short)(0x0001) //directory is missing
#define FC_UNABLE_OPEN_FILE	        (short)(0x0002) //can't open the file
#define FC_UNCONSISTENT_SLICE	    (short)(0x0004) //Slices are not consistent
#define FC_NO_THERMAL	            (short)(0x0008) //Chip warming does not work
#define FC_THRESHOLD_HIGH	        (short)(0x0010) //Finger detection threshold to high
#define FC_HALF_BANDWIDTH	        (short)(0x0020) //only half bandwidth allocated for usb driver
#define FC_TEMPERATURE_NOT_UPTODATE	(short)(0x0040) //Temperature is not uptodate
#define FC_TOO_WARM                 (short)(0x0080) //Temperature too high to warm 
#define FC_TOO_COLD                 (short)(0x0090) //Temperature too low to warm
#define FC_SLOW	                    (short)(0x0900) //Sweeping was a bit slow
#define	FC_IMAGE_TRUNCATED	        (short)(0x0300) //reconstructed image was truncated
#define FC_NO_FINGER	            (short)(0x2000) //No finger detected
#define FC_TRUNCATED_MESSAGE	    (short)(0x4000) //Error message was truncated
// end: from SAMBA source
#endif

// from sam7utils/samba.h

#define AT91_CHIPID_VERSION( chipid ) (((chipid)>>0)&0x1f)
#define AT91_CHIPID_EPROC( chipid ) (((chipid)>>5)&0x7)
#define AT91_CHIPID_NVPSIZ( chipid ) (((chipid)>>8)&0xf)
#define AT91_CHIPID_NVPSIZ2( chipid ) (((chipid)>>12)&0xf)
#define AT91_CHIPID_SRAMSIZ( chipid ) (((chipid)>>16)&0xf)
#define AT91_CHIPID_ARCH( chipid ) (((chipid)>>20)&0xff)
#define AT91_CHIPID_NVPTYP( chipid ) (((chipid)>>28)&0x7)
#define AT91_CHIPID_EXT( chipid ) (((chipid)>>31)&0x1)

#define AT91_ARCH_AT75Cxx      0xF0
#define AT91_ARCH_AT91x40      0x40
#define AT91_ARCH_AT91x63      0x63
#define AT91_ARCH_AT91x55      0x55
#define AT91_ARCH_AT91x42      0x42
#define AT91_ARCH_AT91x92      0x92
#define AT91_ARCH_AT91x34      0x34
#define AT91_ARCH_AT91SAM7Axx  0x60
#define AT91_ARCH_AT91SAM7Sxx  0x70
#define AT91_ARCH_AT91SAM7XC   0x71
#define AT91_ARCH_AT91SAM7SExx 0x72
#define AT91_ARCH_AT91SAM7Lxx  0x73
#define AT91_ARCH_AT91SAM7Xxx  0x75
#define AT91_ARCH_AT91SAM9xx   0x19

struct sam7_chip_info{
  int version;
  int eproc;
  int nvpsiz;
  int nvpsiz2;
  int sramsiz;
  int arch;
  int page_size;
  int lock_bits;
};

// Other stuff

#include <stdint.h>

#include "UploaderThread.h"
#include "MessageInterface.h"
#include "SambaMonitor.h"

class SambaMonitor;
class UploaderThread;

class Samba
{
	public:
    enum Status { OK, ERROR_INITIALIZING, ERROR_INCORRECT_CHIP_INFO, 
    	            ERROR_COULDNT_FIND_FILE, ERROR_COULDNT_OPEN_FILE, 
    	            ERROR_SENDING_FILE, ERROR_SETTING_BOOT_BIT };

		Samba( SambaMonitor* monitor, MessageInterface* messageInterface );

		Status connect();
		Status disconnect();		

		Status flashUpload( char* bin_file );
		Status bootFromFlash( );
		void setUploader( UploaderThread* uploader );
		QString getDeviceKey( );
		int FindUsbDevices( QList<QString>* arrived );
    
  private:
		int init( );
		int readWord( uint32_t addr, uint32_t *value );
		int writeWord( uint32_t addr, uint32_t value );
		int sendFile( uint32_t addr, uint8_t *buff, int len );
		int go( uint32_t addr );
		int sendCommand( char *cmd, void *response, int response_len );
		const char* at91ArchStr( int id );

    int usbOpen( );
    int usbWrite( char* buffer, int length );
    int usbRead( char* buffer, int length );
    int usbClose( );
      
    void* fileOpen( char* name );
    int fileRead( void* file_fd, char* buff, int length );
    void fileClose( void* file_fd );

    void uSleep( int usecs );
		int uploadProgress;
    
    UploaderThread* uploader;
    MessageInterface* messageInterface;
    SambaMonitor* monitor;
    QString deviceKey;
		sam7_chip_info samba_chip_info;

#ifdef Q_WS_LINUX
    void *intf;
    int io_fd;
#endif
		
		// Mac-only
		#ifdef Q_WS_MAC
		mach_port_t masterPort;
		IOUSBDeviceInterface **usbDev;
		IOUSBInterfaceInterface **intf;
		UInt8 inPipeRef;
		UInt8 outPipeRef;
		
		int do_dev( io_service_t usbDeviceRef );
		int do_intf(io_service_t usbInterfaceRef);
		#endif
		
		// Windows-only
		#ifdef Q_WS_WIN
    // from SAMBA Source
    HANDLE m_hPipeIn;
    HANDLE m_hPipeOut;
    int BulkUSB;
    int usbFlushOut( );
    int testOpen( );
    BOOL GetUsbDeviceFileName(LPGUID  pGuid, WCHAR **outNameBuf);
    HANDLE OpenUsbDevice(LPGUID  pGuid, WCHAR **outNameBuf);
    HANDLE OpenOneDevice (HDEVINFO HardwareDeviceInfo,
                          PSP_INTERFACE_DEVICE_DATA DeviceInfoData,
	                        WCHAR **devName);
	bool checkDeviceService( HDEVINFO HardwareDeviceInfo, PSP_DEVINFO_DATA deviceSpecificInfo );
	bool getDeviceObjectName( HDEVINFO HardwareDeviceInfo, PSP_DEVINFO_DATA deviceSpecificInfo );
    #endif
};

#endif /* SAMBA_H */
