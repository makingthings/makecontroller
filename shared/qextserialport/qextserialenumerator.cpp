/**
 * @file qextserialenumerator.cpp
 * @author Michał Policht
 * @see QextSerialEnumerator
 */
 
#include "qextserialenumerator.h"
#include <QMetaType>

#define SAMBA_VID 0x03EB
#define SAMBA_PID 0x6124

QextSerialEnumerator::QextSerialEnumerator( )
{
  if( !QMetaType::isRegistered( QMetaType::type("QextPortInfo") ) )
    qRegisterMetaType<QextPortInfo>("QextPortInfo");
  #ifdef _TTY_WIN_
  notificationWidget = 0;
  #endif
}

QextSerialEnumerator::~QextSerialEnumerator( )
{
  #ifdef Q_OS_MAC
  IONotificationPortDestroy( notificationPortRef );
  #elif (defined _TTY_WIN_)
  UnregisterDeviceNotification( notificationHandle );
  if( notificationWidget )
    delete notificationWidget;
  #endif
}


#ifdef _TTY_WIN_
#include <QRegExp>
#include <objbase.h>
#include <initguid.h>
	//this is serial port GUID
	#ifndef GUID_CLASS_COMPORT
		//DEFINE_GUID(GUID_CLASS_COMPORT, 0x86e0d1e0L, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73);
        // use more Make Controller specific guid
		DEFINE_GUID(GUID_CLASS_COMPORT, 0x4D36E978, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 );
		DEFINE_GUID(SAMBA_GUID, 0xe6ef7dcd, 0x1795, 0x4a08, 0x9f, 0xbf, 0xaa, 0x78, 0x42, 0x3c, 0x26, 0xf0);

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
    BYTE buff[size];
    DWORD type;
    QString result;
    if( RegQueryValueEx(key, property, NULL, &type, buff, & size) == ERROR_SUCCESS ) {
      // might not be terminated...let QString terminate in this case
      if( type == REG_SZ || type == REG_MULTI_SZ || type == REG_EXPAND_SZ )
        result = TCHARToQString(buff);
      else
        result = TCHARToQStringN(buff, size);
    }
		RegCloseKey(key);
    return result;
	}
	
	//static
	QString QextSerialEnumerator::getDeviceProperty(HDEVINFO devInfo, PSP_DEVINFO_DATA devData, DWORD property)
	{
		DWORD buffSize = 0;
		SetupDiGetDeviceRegistryProperty(devInfo, devData, property, NULL, NULL, 0, & buffSize);
    BYTE buff[buffSize];
		if (!SetupDiGetDeviceRegistryProperty(devInfo, devData, property, NULL, buff, buffSize, NULL))
			qCritical("Can not obtain property: %ld from registry", property); 
    return TCHARToQString(buff);
	}

	//static
	void QextSerialEnumerator::setupAPIScan(QList<QextPortInfo> & infoList)
	{
		HDEVINFO devInfo = INVALID_HANDLE_VALUE;
		GUID * guidDev = (GUID *) & GUID_CLASS_COMPORT;
		GUID * sambaGuidDev = (GUID *) & SAMBA_GUID;

		devInfo = SetupDiGetClassDevs(guidDev, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if(devInfo == INVALID_HANDLE_VALUE)
			return qCritical("SetupDiGetClassDevs failed. Error code: %ld", GetLastError());
		enumerateDevicesWin( devInfo, guidDev, &infoList );
		
		devInfo = SetupDiGetClassDevs(sambaGuidDev, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if(devInfo == INVALID_HANDLE_VALUE)
      return qCritical("SetupDiGetClassDevs failed. Error code: %ld", GetLastError());
    enumerateDevicesWin( devInfo, sambaGuidDev, &infoList );
	}
	
	void QextSerialEnumerator::enumerateDevicesWin( HDEVINFO devInfo, GUID* guidDev, QList<QextPortInfo>* infoList )
	{
	  //enumerate the devices
    bool ok = true;
    SP_DEVICE_INTERFACE_DATA ifcData;
    ifcData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    PSP_DEVICE_INTERFACE_DETAIL_DATA detData = NULL;
    DWORD detDataPredictedLength;
	  for (DWORD i = 0; ok; i++) {
      ok = SetupDiEnumDeviceInterfaces(devInfo, NULL, guidDev, i, &ifcData);
      if (ok)
      {
        SP_DEVINFO_DATA devData = {sizeof(SP_DEVINFO_DATA)};
        //check for required detData size
        SetupDiGetDeviceInterfaceDetail(devInfo, & ifcData, NULL, 0, &detDataPredictedLength, & devData);
        detData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(detDataPredictedLength);
        detData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        //check the details
        if (SetupDiGetDeviceInterfaceDetail(devInfo, &ifcData, detData, detDataPredictedLength, NULL, & devData)) {
          // Got a device. Get the details.
          QextPortInfo info;
          getDeviceDetails( &info, devInfo, &devData );
          infoList->append(info);
        }
        else
          qCritical("SetupDiGetDeviceInterfaceDetail failed. Error code: %ld", GetLastError());
        delete detData;
      }
      else if (GetLastError() != ERROR_NO_MORE_ITEMS)
        return qCritical("SetupDiEnumDeviceInterfaces failed. Error code: %ld", GetLastError());
    }
    SetupDiDestroyDeviceInfoList(devInfo);
	}
  
  bool QextSerialRegistrationWidget::winEvent( MSG* message, long* result )
  {
    if ( message->message == WM_DEVICECHANGE ) {
      qese->onDeviceChangeWin( message->wParam, message->lParam );
      *result = 1;
      return true;
    }
    return false;
  }

  void QextSerialEnumerator::setUpNotificationWin( )
  {    
    if(notificationWidget)
      return;

    notificationWidget = new QextSerialRegistrationWidget(this);

    DEV_BROADCAST_DEVICEINTERFACE dbh;
    ZeroMemory(&dbh, sizeof(dbh));
    dbh.dbcc_size = sizeof(dbh);
    dbh.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    CopyMemory(&dbh.dbcc_classguid, &GUID_CLASS_COMPORT, sizeof(GUID));
    
    notificationHandle = RegisterDeviceNotification( notificationWidget->winId( ), &dbh, DEVICE_NOTIFY_WINDOW_HANDLE );
    if(!notificationHandle)
      qWarning( "RegisterDeviceNotification failed: %ld", GetLastError());
    
    ZeroMemory(&dbh, sizeof(dbh));
    dbh.dbcc_size = sizeof(dbh);
    dbh.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    CopyMemory(&dbh.dbcc_classguid, &SAMBA_GUID, sizeof(GUID));
    
    notificationHandle = RegisterDeviceNotification( notificationWidget->winId( ), &dbh, DEVICE_NOTIFY_WINDOW_HANDLE );
    if(!notificationHandle)
      qWarning( "RegisterDeviceNotification failed: %ld", GetLastError());
  }
  
  LRESULT QextSerialEnumerator::onDeviceChangeWin( WPARAM wParam, LPARAM lParam )
  {
    if ( DBT_DEVICEARRIVAL == wParam || DBT_DEVICEREMOVECOMPLETE == wParam )
    {
      PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
      if( pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE )
      {
        PDEV_BROADCAST_DEVICEINTERFACE pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
        QString devId = TCHARToQString(pDevInf->dbcc_name);
        // devId: \\?\USB#Vid_04e8&Pid_503b#0002F9A9828E0F06#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
        devId.remove("\\\\?\\"); // USB#Vid_04e8&Pid_503b#0002F9A9828E0F06#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
        devId.remove(QRegExp("#\\{(.+)\\}")); // USB#Vid_04e8&Pid_503b#0002F9A9828E0F06
        devId.replace("#", "\\"); // USB\Vid_04e8&Pid_503b\0002F9A9828E0F06
        devId = devId.toUpper();
        //qDebug("devname: %s", qPrintable(devId));
        
        DWORD dwFlag = DBT_DEVICEARRIVAL == wParam ? (DIGCF_ALLCLASSES | DIGCF_PRESENT) : DIGCF_ALLCLASSES;
        HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_CLASS_COMPORT,NULL,NULL,dwFlag);
        SP_DEVINFO_DATA spDevInfoData;
        spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        
        for(int i=0; SetupDiEnumDeviceInfo(hDevInfo, i, &spDevInfoData); i++)
        {
          DWORD nSize=0 ;
          TCHAR buf[MAX_PATH];
          if ( !SetupDiGetDeviceInstanceId(hDevInfo, &spDevInfoData, buf, sizeof(buf), &nSize) )
            qDebug("SetupDiGetDeviceInstanceId(): %ld", GetLastError());
          if( devId == TCHARToQString(buf) ) // we found a match
          {
            QextPortInfo info;
            getDeviceDetails( &info, hDevInfo, &spDevInfoData, wParam );
            if( wParam == DBT_DEVICEARRIVAL )
              emit deviceDiscovered(info);
            else if( wParam == DBT_DEVICEREMOVECOMPLETE )
              emit deviceTerminated(info);
            break;
          }
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
      }
    }
    return 0;
  }
  
  bool QextSerialEnumerator::getDeviceDetails( QextPortInfo* portInfo, HDEVINFO devInfo, PSP_DEVINFO_DATA devData, WPARAM wParam )
  {
    portInfo->friendName = getDeviceProperty(devInfo, devData, SPDRP_FRIENDLYNAME);
    if( wParam == DBT_DEVICEARRIVAL)
      portInfo->physName = getDeviceProperty(devInfo, devData, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME);
    portInfo->enumName = getDeviceProperty(devInfo, devData, SPDRP_ENUMERATOR_NAME);
    QString hardwareIDs = getDeviceProperty(devInfo, devData, SPDRP_HARDWAREID);
    HKEY devKey = SetupDiOpenDevRegKey(devInfo, devData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    portInfo->portName = getRegKeyValue(devKey, TEXT("PortName")).toAscii();
    QRegExp rx("COM(\\d+)");
    if(portInfo->portName.contains(rx))
    {
      int portnum = rx.cap(1).toInt();
      if(portnum > 9)
        portInfo->portName.prepend("\\\\.\\"); // COM ports greater than 9 need \\.\ prepended
    }
    QRegExp idRx("VID_(\\w+)&PID_(\\w+)&");
    if( hardwareIDs.toUpper().contains(idRx) )
    {
      bool dummy;
      portInfo->vendorID = idRx.cap(1).toInt(&dummy, 16);
      portInfo->productID = idRx.cap(2).toInt(&dummy, 16);
      //qDebug("got vid: %d, pid: %d", vid, pid);
    }
    return true;
  }


#endif /*_TTY_WIN_*/

#ifdef _TTY_POSIX_

#ifdef Q_OS_MAC
// OS X version
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <CoreFoundation/CFNumber.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <sys/param.h>
#include <IOKit/IOBSD.h>

// static
void QextSerialEnumerator::scanPortsOSX(QList<QextPortInfo> & infoList)
{
  io_iterator_t serialPortIterator = 0;
  io_object_t modemService;
  kern_return_t kernResult = KERN_FAILURE;
  CFMutableDictionaryRef matchingDictionary;
  
  matchingDictionary = IOServiceMatching(kIOSerialBSDServiceValue);
  if (matchingDictionary == NULL)
    return qWarning("IOServiceMatching returned a NULL dictionary.");
  CFDictionaryAddValue(matchingDictionary, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));
  
  // then create the iterator with all the matching devices
  if( IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDictionary, &serialPortIterator) != KERN_SUCCESS )
    return qCritical("IOServiceGetMatchingServices failed, returned %d", kernResult);
  
  // Iterate through all modems found.
  while((modemService = IOIteratorNext(serialPortIterator)))
  {
    QextPortInfo info;
    info.vendorID = 0;
    info.productID = 0;
    getServiceDetails( modemService, &info );
    infoList.append(info);
  }
  IOObjectRelease(serialPortIterator);
  
  getSamBaBoards( infoList );
}

void QextSerialEnumerator::getSamBaBoards( QList<QextPortInfo> & infoList )
{
  kern_return_t err;
  mach_port_t masterPort = 0;
  CFMutableDictionaryRef matchingDictionary = 0;
  io_iterator_t iterator = 0;
  io_service_t usbDeviceRef;
  
  if( (err = IOMasterPort( MACH_PORT_NULL, &masterPort )) )
    return qWarning( "could not create master port, err = %08x\n", err );
  
  if( !createSambaMatchingDict( &matchingDictionary ) )
    return qWarning("couldn't create samba matching dict");
  
  err = IOServiceGetMatchingServices( masterPort, matchingDictionary, &iterator );
  
  while( (usbDeviceRef = IOIteratorNext( iterator )) )
  {
    QextPortInfo info;
    info.vendorID = SAMBA_VID;
    info.productID = SAMBA_PID;
    infoList.append(info);
  }
  IOObjectRelease(iterator);
}

bool QextSerialEnumerator::createSambaMatchingDict( CFMutableDictionaryRef* matchingDictionary )
{
  SInt32 idVendor = SAMBA_VID;
  SInt32 idProduct = SAMBA_PID;
  CFNumberRef numberRef;
  
  if(QSysInfo::MacintoshVersion < QSysInfo::MV_LEOPARD) {
    if( !(*matchingDictionary = IOServiceMatching(kIOUSBDeviceClassName)) ) {
      qWarning( "could not create matching dictionary\n" );
      return false;
    }
  }
  else {
    if( !(*matchingDictionary = IOServiceNameMatching("AppleUSBCDC")) ) {
      qWarning( "could not create matching dictionary\n" );
      return false;
    }
  }
  
  if( !(numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idVendor)) )
  {
    qWarning( "could not create CFNumberRef for vendor\n" );
    return false;
  }

  CFDictionaryAddValue( *matchingDictionary, CFSTR(kUSBVendorID), numberRef);
  CFRelease( numberRef );
  numberRef = 0;

  if( !(numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idProduct)) )
  {
    qWarning( "could not create CFNumberRef for product\n" );
    return false;
  }

  CFDictionaryAddValue( *matchingDictionary, CFSTR(kUSBProductID), numberRef);
  CFRelease( numberRef );
  numberRef = 0;
  return true;
}

/*
  These need to be defined as C externs because IOServiceAddMatchingNotification
  doesn't accept callbacks within a class.
*/
#ifdef __cplusplus
extern "C"
{
#endif

void deviceDiscoveredCallbackOSX( void *ctxt, io_iterator_t serialPortIterator )
{
  QextSerialEnumerator* context = (QextSerialEnumerator*)ctxt;
  io_object_t serialService;
  while ((serialService = IOIteratorNext(serialPortIterator)))
    context->onDeviceDiscoveredOSX(serialService);
}

void deviceTerminatedCallbackOSX( void *ctxt, io_iterator_t serialPortIterator )
{
  QextSerialEnumerator* context = (QextSerialEnumerator*)ctxt;
  io_object_t serialService;
  while ((serialService = IOIteratorNext(serialPortIterator)))
    context->onDeviceTerminatedOSX(serialService);
}

#ifdef __cplusplus
}
#endif

extern void deviceDiscoveredCallbackOSX( void *ctxt, io_iterator_t serialPortIterator );
extern void deviceTerminatedCallbackOSX( void *ctxt, io_iterator_t serialPortIterator );

void QextSerialEnumerator::setUpNotificationOSX( )
{
  kern_return_t kernResult;
  mach_port_t masterPort;
  CFRunLoopSourceRef notificationRunLoopSource;
  CFMutableDictionaryRef classesToMatch;
  CFMutableDictionaryRef sambaClassesToMatch;
  io_iterator_t portIterator;
    
  kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);
  if (KERN_SUCCESS != kernResult)
    return qDebug("IOMasterPort returned %d", kernResult);
  
  classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
  if (classesToMatch == NULL)
    qDebug("IOServiceMatching returned a NULL dictionary.");
  else
    CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));
  
  if( !createSambaMatchingDict( &sambaClassesToMatch ) )
    return qWarning("couldn't create samba matching dict");
  
  // Retain an additional reference since each call to IOServiceAddMatchingNotification consumes one.
  classesToMatch = (CFMutableDictionaryRef) CFRetain(classesToMatch);
  sambaClassesToMatch = (CFMutableDictionaryRef) CFRetain(sambaClassesToMatch);
  
  notificationPortRef = IONotificationPortCreate(masterPort);
  if (notificationPortRef == NULL)
    return qDebug("IONotificationPortCreate return a NULL IONotificationPortRef.");
  
  notificationRunLoopSource = IONotificationPortGetRunLoopSource(notificationPortRef);
  if (notificationRunLoopSource == NULL)
    return qDebug("IONotificationPortGetRunLoopSource returned NULL CFRunLoopSourceRef.");
  
  CFRunLoopAddSource(CFRunLoopGetCurrent(), notificationRunLoopSource, kCFRunLoopDefaultMode);
  
  kernResult = IOServiceAddMatchingNotification(notificationPortRef, kIOMatchedNotification, classesToMatch,
                                                  deviceDiscoveredCallbackOSX, this, &portIterator);
  if (kernResult != KERN_SUCCESS)
    return qDebug("IOServiceAddMatchingNotification return %d", kernResult);
  
  // arm the callback, and grab any devices that are already connected
  deviceDiscoveredCallbackOSX( this, portIterator );
  
  kernResult = IOServiceAddMatchingNotification(notificationPortRef, kIOMatchedNotification, sambaClassesToMatch,
                                                  deviceDiscoveredCallbackOSX, this, &portIterator);
  if (kernResult != KERN_SUCCESS)
    return qDebug("IOServiceAddMatchingNotification return %d", kernResult);
  
  // arm the callback, and grab any devices that are already connected
  deviceDiscoveredCallbackOSX( this, portIterator );
  
  kernResult = IOServiceAddMatchingNotification(notificationPortRef, kIOTerminatedNotification, classesToMatch,
                                                  deviceTerminatedCallbackOSX, this, &portIterator);
  if (kernResult != KERN_SUCCESS)
    return qDebug("IOServiceAddMatchingNotification return %d.", kernResult);
  
  // arm the callback, and clear any devices that are terminated
  deviceTerminatedCallbackOSX( this, portIterator );
  
  kernResult = IOServiceAddMatchingNotification(notificationPortRef, kIOTerminatedNotification, sambaClassesToMatch,
                                                  deviceTerminatedCallbackOSX, this, &portIterator);
  if (kernResult != KERN_SUCCESS)
    return qDebug("IOServiceAddMatchingNotification return %d.", kernResult);
  
  // arm the callback, and clear any devices that are terminated
  deviceTerminatedCallbackOSX( this, portIterator );
}

void QextSerialEnumerator::onDeviceDiscoveredOSX( io_object_t service )
{
  QextPortInfo info;
  info.vendorID = 0;
  info.productID = 0;
  if( getServiceDetails( service, &info ) )
    emit deviceDiscovered( info );
}

void QextSerialEnumerator::onDeviceTerminatedOSX( io_object_t service )
{
  QextPortInfo info;
  info.vendorID = 0;
  info.productID = 0;
  if( getServiceDetails( service, &info ) )
    emit deviceTerminated( info );
}

bool QextSerialEnumerator::getServiceDetails( io_object_t service, QextPortInfo* portInfo )
{
  bool retval = true;
  CFTypeRef bsdPathAsCFString = NULL;
  CFTypeRef productNameAsCFString = NULL;
  CFTypeRef vendorIdAsCFNumber = NULL;
  CFTypeRef productIdAsCFNumber = NULL;
  // check the name of the modem's callout device
  bsdPathAsCFString = IORegistryEntryCreateCFProperty(service, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0);

  // wander up the hierarchy until we find the level that can give us the vendor/product IDs and the product name, if available
  io_registry_entry_t parent;
  kern_return_t kernResult = IORegistryEntryGetParentEntry(service, kIOServicePlane, &parent);
  while( !kernResult && !vendorIdAsCFNumber && !productIdAsCFNumber )
  {
    if(!productNameAsCFString)
      productNameAsCFString = IORegistryEntrySearchCFProperty(parent, kIOServicePlane, CFSTR("Product Name"), kCFAllocatorDefault, 0);
    vendorIdAsCFNumber = IORegistryEntrySearchCFProperty(parent, kIOServicePlane, CFSTR(kUSBVendorID), kCFAllocatorDefault, 0);
    productIdAsCFNumber = IORegistryEntrySearchCFProperty(parent, kIOServicePlane, CFSTR(kUSBProductID), kCFAllocatorDefault, 0);
    io_registry_entry_t oldparent = parent;
    kernResult = IORegistryEntryGetParentEntry(parent, kIOServicePlane, &parent);
    IOObjectRelease(oldparent);
  }
    
  io_string_t ioPathName;
  IORegistryEntryGetPath( service, kIOServicePlane, ioPathName );
  portInfo->physName = ioPathName;
  
  if( bsdPathAsCFString )
  {   
    char path[MAXPATHLEN];
    if( CFStringGetCString((CFStringRef)bsdPathAsCFString, path, PATH_MAX, kCFStringEncodingUTF8) )
      portInfo->portName = path;
    CFRelease(bsdPathAsCFString);
  }
  
  if(productNameAsCFString)
  {
    char productName[MAXPATHLEN];
    if( CFStringGetCString((CFStringRef)productNameAsCFString, productName, PATH_MAX, kCFStringEncodingUTF8) )
      portInfo->friendName = productName;
    CFRelease(productNameAsCFString);
  }
  
  if(vendorIdAsCFNumber)
  {
    SInt32 vID;
    if(CFNumberGetValue((CFNumberRef)vendorIdAsCFNumber, kCFNumberSInt32Type, &vID))
      portInfo->vendorID = vID;
    CFRelease(vendorIdAsCFNumber);
  }
  
  if(productIdAsCFNumber)
  {
    SInt32 pID;
    if(CFNumberGetValue((CFNumberRef)productIdAsCFNumber, kCFNumberSInt32Type, &pID))
      portInfo->productID = pID;
    CFRelease(productIdAsCFNumber);
  }
  IOObjectRelease(service);
  return retval;
}

#else /* Q_OS_MAC */

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
    QextPortInfo info;

	dbus_error_init (&error);	
	if ((hal_ctx = libhal_ctx_new ()) == NULL) {
		qWarning("error: libhal_ctx_new");
		LIBHAL_FREE_DBUS_ERROR (&error);
		return;
	}
	if (!libhal_ctx_set_dbus_connection (hal_ctx, 
           dbus_bus_get (DBUS_BUS_SYSTEM, &error))) {
		qWarning("error: libhal_ctx_set_dbus_connection: %s: %s",
                 error.name, error.message);
		LIBHAL_FREE_DBUS_ERROR (&error);
		return;
	}
	if (!libhal_ctx_init (hal_ctx, &error)) {
		if (dbus_error_is_set(&error)) {
			qWarning("error: libhal_ctx_init: %s: %s",
                     error.name, error.message);
			LIBHAL_FREE_DBUS_ERROR (&error);
		}
		qWarning("Could not initialise connection to hald.\n Normally this means the HAL daemon (hald) is not running or not ready.");
		return;
	}

    // spin through all the serial devices.
	udis = libhal_manager_find_device_string_match (hal_ctx, 
             "usb.vendor", "Atmel Corp.", &num_udis, &error);

	if (dbus_error_is_set (&error)) {
		qWarning("libahl error: %s: %s", error.name, error.message);
		LIBHAL_FREE_DBUS_ERROR (&error);
		return;
	}

    // spin through and find the device names...
	for (i = 0; i < num_udis; i++)
    {
        dbus_error_init (&error);

        if (!libhal_device_property_exists(hal_ctx, 
                            udis[i],"info.product", &error)) continue;
        info.friendName = libhal_device_get_property_string (hal_ctx, 
                            udis[i],"info.product",&error);

        if (!libhal_device_property_exists(hal_ctx, 
                            udis[i],"usb.vendor_id", &error)) continue;
        info.vendorID  = libhal_device_get_property_int (hal_ctx,
                           udis[i], "usb.vendor_id", &error);
        
        if (-1 == info.vendorID) continue;
        
        if (!libhal_device_property_exists(hal_ctx, 
                            udis[i],"usb.product_id", &error)) continue;
        info.productID = libhal_device_get_property_int (hal_ctx, 
                           udis[i],"usb.product_id",&error);
        
        // check for errors.
        if (dbus_error_is_set (&error))
        {
            qWarning("libhal error: %s: %s", error.name, error.message);
            LIBHAL_FREE_DBUS_ERROR (&error);
            continue;
        }
        
        infoList.append(info);
	}
	libhal_free_string_array (udis);

    // spin through all the Amtal devices.
	dbus_error_init (&error);

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
        
        char * iparent;
        
        dbus_error_init (&error);

        // get the device file name and the product name...
        if (!libhal_device_property_exists(hal_ctx, 
                            udis[i],"linux.device_file", &error)) continue;
        info.portName = libhal_device_get_property_string (hal_ctx,
                          udis[i],"linux.device_file",&error);
        
        if (!libhal_device_property_exists(hal_ctx, 
                            udis[i],"info.product", &error)) continue;
        info.friendName = libhal_device_get_property_string (hal_ctx, 
                              udis[i],"info.product",&error);
        
        // the vendor and product ID's cannot be obtained by the same
        // device entry as the serial device, but must be gotten for
        // the parent.
        if (!libhal_device_property_exists(hal_ctx, 
                            udis[i],"info.parent", &error)) continue;
        iparent = libhal_device_get_property_string (hal_ctx,
                      udis[i], "info.parent", &error);
        
        // get the vendor and product ID's from the parent
        if (libhal_device_property_exists(hal_ctx, 
                            iparent,"usb.vendor_id", &error))
            info.vendorID  = libhal_device_get_property_int (hal_ctx,
                           iparent , "usb.vendor_id", &error);
        else if (libhal_device_property_exists(hal_ctx, 
                            iparent,"usb_device.vendor_id", &error))
            info.vendorID  = libhal_device_get_property_int (hal_ctx,
                           iparent , "usb_device.vendor_id", &error);

        if (libhal_device_property_exists(hal_ctx, 
                           iparent,"usb.product_id", &error))
            info.productID = libhal_device_get_property_int (hal_ctx, 
                           iparent,"usb.product_id",&error);
        else if (libhal_device_property_exists(hal_ctx, 
                           iparent,"usb_device.product_id", &error))
            info.productID = libhal_device_get_property_int (hal_ctx, 
                           iparent,"usb_device.product_id",&error);
        
        // check for errors.
        if (dbus_error_is_set (&error))
        {
            qWarning("libhal error: %s: %s", error.name, error.message);
            LIBHAL_FREE_DBUS_ERROR (&error);
            continue;
        }
        
        infoList.append(info);
	}
	libhal_free_string_array (udis);

    // free up the HAL context
    libhal_ctx_shutdown (hal_ctx, &error);
    libhal_ctx_free (hal_ctx);


	return;
}

#endif /* Q_OS_MAC */
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
  #ifdef Q_OS_MAC
    scanPortsOSX(ports); 
  #else /* Q_OS_MAC */
    scanPortsNix(ports);
  #endif /* Q_OS_MAC */
	#endif /*_TTY_POSIX_*/
	
	return ports;
}

void QextSerialEnumerator::setUpNotifications( )
{
  #ifdef _TTY_WIN_
    setUpNotificationWin( );
  #endif
  #ifdef _TTY_POSIX_
  #ifdef Q_OS_MAC
    setUpNotificationOSX( ); 
  #else /* Q_OS_MAC */
    qCritical("Notifications for *Nix/FreeBSD are not implemented yet");
  #endif /* Q_OS_MAC */
	#endif /*_TTY_POSIX_*/
}
