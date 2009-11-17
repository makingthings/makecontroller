


#include "qextserialenumerator.h"
#include <QDebug>
#include <QMetaType>

#define SAMBA_VID 0x03EB
#define SAMBA_PID 0x6124

QextSerialEnumerator::QextSerialEnumerator( )
{
    if( !QMetaType::isRegistered( QMetaType::type("QextPortInfo") ) )
        qRegisterMetaType<QextPortInfo>("QextPortInfo");
#ifdef Q_OS_MAC
    notificationPortRef = 0;
#endif
#if (defined Q_OS_WIN) && (defined QT_GUI_LIB)
    notificationWidget = 0;
#endif // Q_OS_WIN
}

QextSerialEnumerator::~QextSerialEnumerator( )
{
#ifdef Q_OS_MAC
    if(notificationPortRef)
        IONotificationPortDestroy( notificationPortRef );
#endif
#if (defined Q_OS_WIN) && (defined QT_GUI_LIB)
    if( notificationWidget )
        delete notificationWidget;
#endif
}

#ifdef Q_OS_WIN

    #include <objbase.h>
    #include <initguid.h>
    #include "qextserialport.h"
    #include <QRegExp>

    // see http://msdn.microsoft.com/en-us/library/ms791134.aspx for list of GUID classes
    #ifndef GUID_DEVCLASS_PORTS
        DEFINE_GUID(GUID_DEVCLASS_PORTS, 0x4D36E978, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 );
    #endif
    DEFINE_GUID(GUID_DEVCLASS_SAMBA, 0x36FC9E60, 0xC465, 0x11CF, 0x80, 0x56, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
    DEFINE_GUID(GUID_DEVINTERFACE_SAMBA, 0xe6ef7dcd, 0x1795, 0x4a08, 0x9f, 0xbf, 0xaa, 0x78, 0x42, 0x3c, 0x26, 0xf0);

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
        DWORD type;
        RegQueryValueEx(key, property, NULL, NULL, NULL, & size);
        BYTE* buff = new BYTE[size];
        QString result;
        if( RegQueryValueEx(key, property, NULL, &type, buff, & size) == ERROR_SUCCESS )
            result = TCHARToQString(buff);
        RegCloseKey(key);
        delete [] buff;
        return result;
    }

    //static
    QString QextSerialEnumerator::getDeviceProperty(HDEVINFO devInfo, PSP_DEVINFO_DATA devData, DWORD property)
    {
        DWORD buffSize = 0;
        SetupDiGetDeviceRegistryProperty(devInfo, devData, property, NULL, NULL, 0, & buffSize);
        BYTE* buff = new BYTE[buffSize];
        SetupDiGetDeviceRegistryProperty(devInfo, devData, property, NULL, buff, buffSize, NULL);
        QString result = TCHARToQString(buff);
        delete [] buff;
        return result;
    }

    //static
    void QextSerialEnumerator::setupAPIScan(QList<QextPortInfo> & infoList)
    {
        enumerateDevicesWin(GUID_DEVCLASS_PORTS, &infoList);
        enumerateDevicesWin(GUID_DEVCLASS_SAMBA, &infoList);
    }

    void QextSerialEnumerator::enumerateDevicesWin( const GUID & guid, QList<QextPortInfo>* infoList )
    {
        HDEVINFO devInfo;
        if( (devInfo = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT)) != INVALID_HANDLE_VALUE)
        {
            SP_DEVINFO_DATA devInfoData;
            devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            for(int i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devInfoData); i++)
            {
                QextPortInfo info;
                info.productID = info.vendorID = 0;
                getDeviceDetailsWin( &info, devInfo, &devInfoData );
                infoList->append(info);
            }
            SetupDiDestroyDeviceInfoList(devInfo);
        }
    }

#ifdef QT_GUI_LIB
    bool QextSerialRegistrationWidget::winEvent( MSG* message, long* result )
    {
        if ( message->message == WM_DEVICECHANGE ) {
            qese->onDeviceChangeWin( message->wParam, message->lParam );
            *result = 1;
            return true;
        }
        return false;
    }
#endif

    void QextSerialEnumerator::setUpNotificationWin( )
    {
        #ifdef QT_GUI_LIB
        if(notificationWidget)
            return;
        notificationWidget = new QextSerialRegistrationWidget(this);

        DEV_BROADCAST_DEVICEINTERFACE dbh;
        ZeroMemory(&dbh, sizeof(dbh));
        dbh.dbcc_size = sizeof(dbh);
        dbh.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        CopyMemory(&dbh.dbcc_classguid, &GUID_DEVCLASS_PORTS, sizeof(GUID));
        if( RegisterDeviceNotification( notificationWidget->winId( ), &dbh, DEVICE_NOTIFY_WINDOW_HANDLE ) == NULL)
            qWarning() << "RegisterDeviceNotification failed:" << GetLastError();

        CopyMemory(&dbh.dbcc_classguid, &GUID_DEVINTERFACE_SAMBA, sizeof(GUID));
        if( RegisterDeviceNotification( notificationWidget->winId( ), &dbh, DEVICE_NOTIFY_WINDOW_HANDLE ) == NULL)
            qWarning() << "RegisterDeviceNotification failed:" << GetLastError();
        #else
        qWarning("QextSerialEnumerator: GUI not enabled - can't register for device notifications.");
        #endif // QT_GUI_LIB
    }

    LRESULT QextSerialEnumerator::onDeviceChangeWin( WPARAM wParam, LPARAM lParam )
    {
        if ( DBT_DEVICEARRIVAL == wParam || DBT_DEVICEREMOVECOMPLETE == wParam )
        {
            PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
            if( pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE )
            {
                PDEV_BROADCAST_DEVICEINTERFACE pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
                 // delimiters are different across APIs...change to backslash.  ugh.
                QString deviceID = TCHARToQString(pDevInf->dbcc_name).toUpper().replace("#", "\\");

                matchAndDispatchChangedDevice(deviceID, GUID_DEVCLASS_PORTS, wParam);
                matchAndDispatchChangedDevice(deviceID, GUID_DEVCLASS_SAMBA, wParam);
            }
        }
        return 0;
    }

    bool QextSerialEnumerator::matchAndDispatchChangedDevice(const QString & deviceID, const GUID & guid, WPARAM wParam)
    {
        bool rv = false;
        DWORD dwFlag = (DBT_DEVICEARRIVAL == wParam) ? DIGCF_PRESENT : DIGCF_ALLCLASSES;
        HDEVINFO devInfo;
        if( (devInfo = SetupDiGetClassDevs(&guid,NULL,NULL,dwFlag)) != INVALID_HANDLE_VALUE )
        {
            SP_DEVINFO_DATA spDevInfoData;
            spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            for(int i=0; SetupDiEnumDeviceInfo(devInfo, i, &spDevInfoData); i++)
            {
                DWORD nSize=0 ;
                TCHAR buf[MAX_PATH];
                if ( SetupDiGetDeviceInstanceId(devInfo, &spDevInfoData, buf, MAX_PATH, &nSize) &&
                        deviceID.contains(TCHARToQString(buf))) // we found a match
                {
                    rv = true;
                    QextPortInfo info;
                    info.productID = info.vendorID = 0;
                    getDeviceDetailsWin( &info, devInfo, &spDevInfoData, wParam );
                    if( wParam == DBT_DEVICEARRIVAL )
                        emit deviceDiscovered(info);
                    else if( wParam == DBT_DEVICEREMOVECOMPLETE )
                        emit deviceRemoved(info);
                    break;
                }
            }
            SetupDiDestroyDeviceInfoList(devInfo);
        }
        return rv;
    }

    bool QextSerialEnumerator::getDeviceDetailsWin( QextPortInfo* portInfo, HDEVINFO devInfo, PSP_DEVINFO_DATA devData, WPARAM wParam )
    {
        portInfo->friendName = getDeviceProperty(devInfo, devData, SPDRP_FRIENDLYNAME);
        if( wParam == DBT_DEVICEARRIVAL)
            portInfo->physName = getDeviceProperty(devInfo, devData, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME);
        portInfo->enumName = getDeviceProperty(devInfo, devData, SPDRP_ENUMERATOR_NAME);
        QString hardwareIDs = getDeviceProperty(devInfo, devData, SPDRP_HARDWAREID);
        HKEY devKey = SetupDiOpenDevRegKey(devInfo, devData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        portInfo->portName = QextSerialPort::fullPortNameWin( getRegKeyValue(devKey, TEXT("PortName")) );
        QRegExp idRx("VID_(\\w+)&PID_(\\w+)");
        if( hardwareIDs.toUpper().contains(idRx) )
        {
            bool dummy;
            portInfo->vendorID = idRx.cap(1).toInt(&dummy, 16);
            portInfo->productID = idRx.cap(2).toInt(&dummy, 16);
            //qDebug() << "got vid:" << vid << "pid:" << pid;
        }
        return true;
    }

#endif /*Q_OS_WIN*/

#ifdef Q_OS_UNIX

#ifdef Q_OS_MAC
#include <IOKit/serial/IOSerialKeys.h>
#include <CoreFoundation/CFNumber.h>
#include <sys/param.h>

// static
void QextSerialEnumerator::scanPortsOSX(QList<QextPortInfo> & infoList)
{
    io_iterator_t serialPortIterator = 0;
    kern_return_t kernResult = KERN_FAILURE;
    CFMutableDictionaryRef matchingDictionary;
    
    // first try to get any serialbsd devices, then try any USBCDC devices
    if( !(matchingDictionary = IOServiceMatching(kIOSerialBSDServiceValue) ) ) {
        qWarning("IOServiceMatching returned a NULL dictionary.");
        return;
    }
    CFDictionaryAddValue(matchingDictionary, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));
    
    // then create the iterator with all the matching devices
    if( IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDictionary, &serialPortIterator) != KERN_SUCCESS ) {
        qCritical() << "IOServiceGetMatchingServices failed, returned" << kernResult;
        return;
    }
    iterateServicesOSX(serialPortIterator, infoList);
    IOObjectRelease(serialPortIterator);
    serialPortIterator = 0;
    
    if( !(matchingDictionary = IOServiceNameMatching("AppleUSBCDC")) ) {
        qWarning("IOServiceNameMatching returned a NULL dictionary.");
        return;
    }
    
    if( IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDictionary, &serialPortIterator) != KERN_SUCCESS ) {
        qCritical() << "IOServiceGetMatchingServices failed, returned" << kernResult;
        return;
    }
    iterateServicesOSX(serialPortIterator, infoList);
    IOObjectRelease(serialPortIterator);
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
    
    while((usbDeviceRef = IOIteratorNext(iterator))) {
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
    
    if( !(numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idVendor)) ) {
        qWarning( "could not create CFNumberRef for vendor\n" );
        return false;
    }
    
    CFDictionaryAddValue( *matchingDictionary, CFSTR(kUSBVendorID), numberRef);
    CFRelease( numberRef );
    numberRef = 0;
    
    if( !(numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idProduct)) ) {
        qWarning( "could not create CFNumberRef for product\n" );
        return false;
    }
    
    CFDictionaryAddValue( *matchingDictionary, CFSTR(kUSBProductID), numberRef);
    CFRelease( numberRef );
    numberRef = 0;
    return true;
}

void QextSerialEnumerator::iterateServicesOSX(io_object_t service, QList<QextPortInfo> & infoList)
{
    // Iterate through all modems found.
    io_object_t usbService;
    while( ( usbService = IOIteratorNext(service) ) )
    {
        QextPortInfo info;
        info.vendorID = 0;
        info.productID = 0;
        getServiceDetailsOSX( usbService, &info );
        infoList.append(info);
    }
}

bool QextSerialEnumerator::getServiceDetailsOSX( io_object_t service, QextPortInfo* portInfo )
{
    bool retval = true;
    CFTypeRef bsdPathAsCFString = NULL;
    CFTypeRef productNameAsCFString = NULL;
    CFTypeRef vendorIdAsCFNumber = NULL;
    CFTypeRef productIdAsCFNumber = NULL;
    // check the name of the modem's callout device
    bsdPathAsCFString = IORegistryEntryCreateCFProperty(service, CFSTR(kIOCalloutDeviceKey),
                                                        kCFAllocatorDefault, 0);

    // wander up the hierarchy until we find the level that can give us the
    // vendor/product IDs and the product name, if available
    io_registry_entry_t parent;
    kern_return_t kernResult = IORegistryEntryGetParentEntry(service, kIOServicePlane, &parent);
    while( kernResult == KERN_SUCCESS && !vendorIdAsCFNumber && !productIdAsCFNumber )
    {
        if(!productNameAsCFString)
            productNameAsCFString = IORegistryEntrySearchCFProperty(parent,
                                                                    kIOServicePlane,
                                                                    CFSTR("Product Name"),
                                                                    kCFAllocatorDefault, 0);
        vendorIdAsCFNumber = IORegistryEntrySearchCFProperty(parent,
                                                             kIOServicePlane,
                                                             CFSTR(kUSBVendorID),
                                                             kCFAllocatorDefault, 0);
        productIdAsCFNumber = IORegistryEntrySearchCFProperty(parent,
                                                              kIOServicePlane,
                                                              CFSTR(kUSBProductID),
                                                              kCFAllocatorDefault, 0);
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
        if( CFStringGetCString((CFStringRef)bsdPathAsCFString, path,
                               PATH_MAX, kCFStringEncodingUTF8) )
      portInfo->portName = path;
    CFRelease(bsdPathAsCFString);
  }
  
  if(productNameAsCFString)
  {
    char productName[MAXPATHLEN];
        if( CFStringGetCString((CFStringRef)productNameAsCFString, productName,
                               PATH_MAX, kCFStringEncodingUTF8) )
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

// IOKit callbacks registered via setupNotifications()
void deviceDiscoveredCallbackOSX( void *ctxt, io_iterator_t serialPortIterator );
void deviceTerminatedCallbackOSX( void *ctxt, io_iterator_t serialPortIterator );

void deviceDiscoveredCallbackOSX( void *ctxt, io_iterator_t serialPortIterator )
{
    QextSerialEnumerator* qese = (QextSerialEnumerator*)ctxt;
    io_object_t serialService;
    while ((serialService = IOIteratorNext(serialPortIterator)))
        qese->onDeviceDiscoveredOSX(serialService);
}

void deviceTerminatedCallbackOSX( void *ctxt, io_iterator_t serialPortIterator )
{
    QextSerialEnumerator* qese = (QextSerialEnumerator*)ctxt;
    io_object_t serialService;
    while ((serialService = IOIteratorNext(serialPortIterator)))
        qese->onDeviceTerminatedOSX(serialService);
}

/*
 A device has been discovered via IOKit.
 Create a QextPortInfo if possible, and emit the signal indicating that we've found it.
 */
void QextSerialEnumerator::onDeviceDiscoveredOSX( io_object_t service )
{
    QextPortInfo info;
    info.vendorID = 0;
    info.productID = 0;
    if( getServiceDetailsOSX( service, &info ) )
        emit deviceDiscovered( info );
}

/*
 Notification via IOKit that a device has been removed.
 Create a QextPortInfo if possible, and emit the signal indicating that it's gone.
 */
void QextSerialEnumerator::onDeviceTerminatedOSX( io_object_t service )
{
    QextPortInfo info;
    info.vendorID = 0;
    info.productID = 0;
    if( getServiceDetailsOSX( service, &info ) )
        emit deviceRemoved( info );
}

/*
 Create matching dictionaries for the devices we want to get notifications for,
 and add them to the current run loop.  Invoke the callbacks that will be responding
 to these notifications once to arm them, and discover any devices that
 are currently connected at the time notifications are setup.
 */
void QextSerialEnumerator::setUpNotificationOSX( )
{
    kern_return_t kernResult;
    mach_port_t masterPort;
    CFRunLoopSourceRef notificationRunLoopSource;
    CFMutableDictionaryRef classesToMatch;
    CFMutableDictionaryRef cdcClassesToMatch;
    CFMutableDictionaryRef sambaClassesToMatch;
    io_iterator_t portIterator;
    
    kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);
    if (KERN_SUCCESS != kernResult) {
        qDebug() << "IOMasterPort returned:" << kernResult;
        return;
    }
    
    classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    if (classesToMatch == NULL)
        qDebug("IOServiceMatching returned a NULL dictionary.");
    else
        CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));
    
    if( !(cdcClassesToMatch = IOServiceNameMatching("AppleUSBCDC") ) ) {
        qWarning("couldn't create cdc matching dict");
        return;
    }
    
    if( !createSambaMatchingDict( &sambaClassesToMatch ) ) {
        qWarning("couldn't create samba matching dict");
        return;
    }
    
    // Retain an additional reference since each call to IOServiceAddMatchingNotification consumes one.
    classesToMatch = (CFMutableDictionaryRef) CFRetain(classesToMatch);
    cdcClassesToMatch = (CFMutableDictionaryRef) CFRetain(cdcClassesToMatch);
    sambaClassesToMatch = (CFMutableDictionaryRef) CFRetain(sambaClassesToMatch);
    
    notificationPortRef = IONotificationPortCreate(masterPort);
    if(notificationPortRef == NULL) {
        qDebug("IONotificationPortCreate return a NULL IONotificationPortRef.");
        return;
    }
    
    notificationRunLoopSource = IONotificationPortGetRunLoopSource(notificationPortRef);
    if (notificationRunLoopSource == NULL) {
        qDebug("IONotificationPortGetRunLoopSource returned NULL CFRunLoopSourceRef.");
        return;
    }
    
    CFRunLoopAddSource(CFRunLoopGetCurrent(), notificationRunLoopSource, kCFRunLoopDefaultMode);
    
    kernResult = IOServiceAddMatchingNotification(notificationPortRef, kIOMatchedNotification, classesToMatch,
                                                  deviceDiscoveredCallbackOSX, this, &portIterator);
    if (kernResult != KERN_SUCCESS) {
        qDebug() << "IOServiceAddMatchingNotification return:" << kernResult;
        return;
    }

    // arm the callback, and grab any devices that are already connected
    deviceDiscoveredCallbackOSX( this, portIterator );
    
    kernResult = IOServiceAddMatchingNotification(notificationPortRef, kIOMatchedNotification, cdcClassesToMatch,
                                                  deviceDiscoveredCallbackOSX, this, &portIterator);
    if (kernResult != KERN_SUCCESS) {
        qDebug() << "IOServiceAddMatchingNotification return:" << kernResult;
        return;
    }
    deviceDiscoveredCallbackOSX( this, portIterator );
    
    kernResult = IOServiceAddMatchingNotification(notificationPortRef, kIOMatchedNotification, sambaClassesToMatch,
                                                  deviceDiscoveredCallbackOSX, this, &portIterator);
    if (kernResult != KERN_SUCCESS) {
        qDebug() << "IOServiceAddMatchingNotification return:" << kernResult;
        return;
    }
    deviceDiscoveredCallbackOSX( this, portIterator );
    
    kernResult = IOServiceAddMatchingNotification(notificationPortRef, kIOTerminatedNotification, classesToMatch,
                                                  deviceTerminatedCallbackOSX, this, &portIterator);
    if (kernResult != KERN_SUCCESS) {
        qDebug() << "IOServiceAddMatchingNotification return:" << kernResult;
        return;
    }
    deviceTerminatedCallbackOSX( this, portIterator );
    
    kernResult = IOServiceAddMatchingNotification(notificationPortRef, kIOTerminatedNotification, cdcClassesToMatch,
                                                  deviceTerminatedCallbackOSX, this, &portIterator);
    if (kernResult != KERN_SUCCESS) {
        qDebug() << "IOServiceAddMatchingNotification return:" << kernResult;
        return;
    }
    deviceTerminatedCallbackOSX( this, portIterator );
    
    kernResult = IOServiceAddMatchingNotification(notificationPortRef, kIOTerminatedNotification, sambaClassesToMatch,
                                                  deviceTerminatedCallbackOSX, this, &portIterator);
    if (kernResult != KERN_SUCCESS) {
        qDebug() << "IOServiceAddMatchingNotification return:" << kernResult;
        return;
    }
    deviceTerminatedCallbackOSX( this, portIterator );
}
#else // Q_OS_MAC

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

#endif // Q_OS_MAC

#endif // Q_OS_UNIX

//static
QList<QextPortInfo> QextSerialEnumerator::getPorts()
{
    QList<QextPortInfo> ports;

    #ifdef Q_OS_WIN
        OSVERSIONINFO vi;
        vi.dwOSVersionInfoSize = sizeof(vi);
        if (!::GetVersionEx(&vi)) {
            qCritical("Could not get OS version.");
            return ports;
        }
        // Handle windows 9x and NT4 specially
        if (vi.dwMajorVersion < 5) {
            qCritical("Enumeration for this version of Windows is not implemented yet");
        /*if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT)
                EnumPortsWNt4(ports);
            else
                EnumPortsW9x(ports);*/
        } else  //w2k or later
            setupAPIScan(ports);
    #endif /*Q_OS_WIN*/
    #ifdef Q_OS_UNIX
      #ifdef Q_OS_MAC
        scanPortsOSX(ports);
      #else /* Q_OS_MAC */
        scanPortsNix(ports);
      #endif /* Q_OS_MAC */
    #endif /*Q_OS_UNIX*/

    return ports;
}

void QextSerialEnumerator::setUpNotifications( )
{
#ifdef Q_OS_WIN
    setUpNotificationWin( );
#endif

#ifdef Q_OS_UNIX
#ifdef Q_OS_MAC
    setUpNotificationOSX( );
#else
    qCritical("Notifications for *Nix/FreeBSD are not implemented yet");
#endif // Q_OS_MAC
#endif // Q_OS_UNIX
}
