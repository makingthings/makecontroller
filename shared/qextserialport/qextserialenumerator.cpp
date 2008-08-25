/**
 * @file qextserialenumerator.cpp
 * @author Michał Policht
 * @see QextSerialEnumerator
 */
 
#include "qextserialenumerator.h"


#ifdef _TTY_WIN_
#include <QRegExp>
#include <objbase.h>
#include <initguid.h>
	//this is serial port GUID
	#ifndef GUID_CLASS_COMPORT
		//DEFINE_GUID(GUID_CLASS_COMPORT, 0x86e0d1e0L, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73);
        // use more Make Controller specific guid
		DEFINE_GUID(GUID_CLASS_COMPORT, 0x4D36E978, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 );
	#endif

	/* Gordon Schumacher's macros for TCHAR -> QString conversions and vice versa */	
	#ifdef UNICODE
		#define QStringToTCHAR(x)     (wchar_t*) x.utf16()
		#define PQStringToTCHAR(x)    (wchar_t*) x->utf16()
		#define TCHARToQString(x)     QString::fromUtf16((ushort*)(x))
		#define TCHARToQStringN(x,y)  QString::fromUtf16((ushort*)(x),(y))
	#else
		#define QStringToTCHAR(x)     x.local8Bit().constData()
		#define PQStringToTCHAR(x)    x->local8Bit().constData()
		#define TCHARToQString(x)     QString::fromLocal8Bit((x))
		#define TCHARToQStringN(x,y)  QString::fromLocal8Bit((x),(y))
	#endif /*UNICODE*/


	//static
	QString QextSerialEnumerator::getRegKeyValue(HKEY key, LPCTSTR property)
	{
		DWORD size = 0;
		RegQueryValueEx(key, property, NULL, NULL, NULL, & size);
		BYTE * buff = new BYTE[size];
		if (RegQueryValueEx(key, property, NULL, NULL, buff, & size) == ERROR_SUCCESS) {
			return TCHARToQStringN(buff, size);
			delete [] buff;
		} else {
			qWarning("QextSerialEnumerator::getRegKeyValue: can not obtain value from registry");
			delete [] buff;
			return QString();
		}
	}
	
	//static
	QString QextSerialEnumerator::getDeviceProperty(HDEVINFO devInfo, PSP_DEVINFO_DATA devData, DWORD property)
	{
		DWORD buffSize = 0;
		SetupDiGetDeviceRegistryProperty(devInfo, devData, property, NULL, NULL, 0, & buffSize);
		BYTE * buff = new BYTE[buffSize];
		if (!SetupDiGetDeviceRegistryProperty(devInfo, devData, property, NULL, buff, buffSize, NULL))
			qCritical("Can not obtain property: %ld from registry", property); 
		QString result = TCHARToQString(buff);
		delete [] buff;
		return result;
	}

	//static
	void QextSerialEnumerator::setupAPIScan(QList<QextPortInfo> & infoList)
	{
		HDEVINFO devInfo = INVALID_HANDLE_VALUE;
		GUID * guidDev = (GUID *) & GUID_CLASS_COMPORT;

		devInfo = SetupDiGetClassDevs(guidDev, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if(devInfo == INVALID_HANDLE_VALUE) {
			qCritical("SetupDiGetClassDevs failed. Error code: %ld", GetLastError());
			return;
		}

		//enumerate the devices
		bool ok = true;
		SP_DEVICE_INTERFACE_DATA ifcData;
		ifcData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		SP_DEVICE_INTERFACE_DETAIL_DATA * detData = NULL;
		DWORD detDataSize = 0;
		DWORD oldDetDataSize = 0;
		
		for (DWORD i = 0; ok; i++) {
			ok = SetupDiEnumDeviceInterfaces(devInfo, NULL, guidDev, i, &ifcData);
			if (ok) {
				SP_DEVINFO_DATA devData = {sizeof(SP_DEVINFO_DATA)};
				//check for required detData size
				SetupDiGetDeviceInterfaceDetail(devInfo, & ifcData, NULL, 0, & detDataSize, & devData);
				//if larger than old detData size then reallocate the buffer
				if (detDataSize > oldDetDataSize) {
					delete [] detData;
					detData = (SP_DEVICE_INTERFACE_DETAIL_DATA *) new char[detDataSize];
					detData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
					oldDetDataSize = detDataSize;
				}
				//check the details
				if (SetupDiGetDeviceInterfaceDetail(devInfo, & ifcData, detData, detDataSize, 
													NULL, & devData)) {
					// Got a device. Get the details.
					QextPortInfo info;
					info.friendName = getDeviceProperty(devInfo, & devData, SPDRP_FRIENDLYNAME);
					info.physName = getDeviceProperty(devInfo, & devData, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME);
					info.enumName = getDeviceProperty(devInfo, & devData, SPDRP_ENUMERATOR_NAME);
					//anyway, to get the port name we must still open registry directly :( ??? 
					//Eh...			
					HKEY devKey = SetupDiOpenDevRegKey(devInfo, & devData, DICS_FLAG_GLOBAL, 0,
														DIREG_DEV, KEY_READ);
					info.portName = getRegKeyValue(devKey, TEXT("PortName"));
					
					// MakingThings
					QRegExp rx("COM(\\d+)");
					if(info.portName.contains(rx))
                    {
                      int portnum = rx.cap(1).toInt();
                      if(portnum > 9)
                        info.portName.prepend("\\\\.\\"); // COM ports greater than 9 need \\.\ prepended
                    }
                    // end MakingThings
					
					RegCloseKey(devKey);
					infoList.append(info);
				} else {
					qCritical("SetupDiGetDeviceInterfaceDetail failed. Error code: %ld", GetLastError());
					delete [] detData;
					return;
				}
			} else {
				if (GetLastError() != ERROR_NO_MORE_ITEMS) {
					delete [] detData;
					qCritical("SetupDiEnumDeviceInterfaces failed. Error code: %ld", GetLastError());
					return;
				}
			}
		}
		delete [] detData;
	}

#endif /*_TTY_WIN_*/

#ifdef _TTY_POSIX_

#ifdef Q_WS_MAC
// OS X version
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
// static
void QextSerialEnumerator::scanPortsOSX(QList<QextPortInfo> & infoList)
{
  io_iterator_t serialPortIterator = 0;
  io_object_t modemService;
  kern_return_t kernResult = KERN_FAILURE;
  CFMutableDictionaryRef bsdMatchingDictionary;
  
  bsdMatchingDictionary = IOServiceMatching(kIOSerialBSDServiceValue);
  if (bsdMatchingDictionary == NULL)
    qWarning("IOServiceMatching returned a NULL dictionary.");
  else
    CFDictionarySetValue(bsdMatchingDictionary, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));
  
  // then create the iterator with all the matching devices
  kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, bsdMatchingDictionary, &serialPortIterator);    
  if(KERN_SUCCESS != kernResult)
  {
    qCritical("IOServiceGetMatchingServices failed, returned %d", kernResult);
    return;
  }
  
  // Iterate through all modems found.
  while((modemService = IOIteratorNext(serialPortIterator)))
  {
    char path[PATH_MAX];
    char productName[PATH_MAX];
    CFTypeRef bsdPathAsCFString;
    CFTypeRef productNameAsCFString;
    // check the name of the modem's callout device
    bsdPathAsCFString = IORegistryEntrySearchCFProperty(modemService, kIOServicePlane, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0);
    // then, because the callout device could be any old thing, and because the reference to the modem returned by the
    // iterator doesn't include much device specific info, look at its parent, and check the product name
    io_registry_entry_t parent;  
    kernResult = IORegistryEntryGetParentEntry(modemService, kIOServicePlane, &parent);																										
    productNameAsCFString = IORegistryEntrySearchCFProperty(parent, kIOServicePlane, CFSTR("Product Name"), kCFAllocatorDefault, 0);
    IOObjectRelease(parent);
    
    bool path_result = false;
    bool name_result = false;
    if( bsdPathAsCFString )
    {   
      path_result = CFStringGetCString((CFStringRef)bsdPathAsCFString, path, PATH_MAX, kCFStringEncodingUTF8);
      CFRelease(bsdPathAsCFString);
    }      
    if(productNameAsCFString)
    {
      name_result = CFStringGetCString((CFStringRef)productNameAsCFString, productName, PATH_MAX, kCFStringEncodingUTF8);
      CFRelease(productNameAsCFString);
    }
    if(path_result && name_result)
    {
      QextPortInfo info;
      info.friendName = productName;
      info.portName = path;
      infoList.append(info);
    }
  }
  IOObjectRelease(modemService);
}

#else /* Q_WS_MAC */

#include <unistd.h>
#include <hal/libhal.h>
// thanks to EBo for unix support
void QextSerialEnumerator::scanPortsNix(QList<QextPortInfo> & infoList)
{
	int i;
	int num_udis;
	char **udis;
	DBusError error;
	LibHalContext *hal_ctx;

	dbus_error_init (&error);	
	if ((hal_ctx = libhal_ctx_new ()) == NULL) {
		qWarning("error: libhal_ctx_new");
		LIBHAL_FREE_DBUS_ERROR (&error);
		return;
	}
	if (!libhal_ctx_set_dbus_connection (hal_ctx, 
           dbus_bus_get (DBUS_BUS_SYSTEM, &error))) {
		qWarning("error: libhal_ctx_set_dbus_connection: %s: %s", error.name, error.message);
		LIBHAL_FREE_DBUS_ERROR (&error);
		return;
	}
	if (!libhal_ctx_init (hal_ctx, &error)) {
		if (dbus_error_is_set(&error)) {
			qWarning("error: libhal_ctx_init: %s: %s", error.name, error.message);
			LIBHAL_FREE_DBUS_ERROR (&error);
		}
		qWarning("Could not initialise connection to hald.\n  
                 Normally this means the HAL daemon (hald) is not running or not ready.");
		return;
	}


	udis = libhal_find_device_by_capability (hal_ctx, "serial",
             &num_udis, &error);

	if (dbus_error_is_set (&error)) {
		qWarning("libahl error: %s: %s", error.name, error.message);
		LIBHAL_FREE_DBUS_ERROR (&error);
		return;
	}

  // spin through and find the device names...
	for (i = 0; i < num_udis; i++)
  {
    //printf ("udis = %s\n", udis[i]);

    QextPortInfo info;
    char * iparent;

    // get the device file name and the product name...
    info.portName = libhal_device_get_property_string (hal_ctx,
                      udis[i],"linux.device_file",&error);

    info.friendName = libhal_device_get_property_string (hal_ctx, 
                       udis[i],"info.product",&error);

    // the vendor and product ID's cannot be obtained by the same
    // device entry as the serial device, but must be gotten for
    // the parent.
    iparent = libhal_device_get_property_string (hal_ctx,
    udis[i], "info.parent", &error);

    // get the vendor and product ID's from the parent
    info.vendorID  = libhal_device_get_property_int (hal_ctx,
                       iparent , "usb.vendor_id", &error);

    info.productID = libhal_device_get_property_int (hal_ctx, 
                       iparent,"usb.product_id",&error);

    // check for errors.
    if (dbus_error_is_set (&error))
    {
      qWarning("libhal error: %s: %s", error.name, error.message);
      LIBHAL_FREE_DBUS_ERROR (&error);
      return;
    }

    infoList.append(info);
	}

	libhal_free_string_array (udis);

	return;
}

#endif /* Q_WS_MAC */
#endif /* _TTY_POSIX_ */


//static
QList<QextPortInfo> QextSerialEnumerator::getPorts()
{
	QList<QextPortInfo> ports;

	#ifdef _TTY_WIN_
		OSVERSIONINFO vi;
		vi.dwOSVersionInfoSize = sizeof(vi);
		if (!::GetVersionEx(&vi)) {
			qCritical("Could not get OS version.");
			return ports;
		}
		// Handle windows 9x and NT4 specially
		if (vi.dwMajorVersion < 5) {
			qCritical("Enumeration for this version of Windows is not implemented yet");
/*			if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT)
				EnumPortsWNt4(ports);
			else
				EnumPortsW9x(ports);*/
		} else	//w2k or later
			setupAPIScan(ports);
	#endif /*_TTY_WIN_*/
	#ifdef _TTY_POSIX_
  #ifdef Q_WS_MAC
    scanPortsOSX(ports); 
  #else /* Q_WS_MAC */
    scanPortsNix(ports);
  #endif /* Q_WS_MAC */
	#endif /*_TTY_POSIX_*/
	
	return ports;
}
