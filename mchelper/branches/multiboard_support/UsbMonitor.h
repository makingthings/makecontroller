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

#ifndef USB_MONITOR_H_
#define USB_MONITOR_H_

#include <QTimer>
#include <QList>
#include <QHash>
#include <QThread>
#include "McHelperWindow.h"
#include "PacketUsbCdc.h"
#include "MonitorInterface.h"
#include "PacketInterface.h"

#ifdef Q_WS_WIN
#include "Setupapi.h"
#endif

#ifdef Q_WS_MAC
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#endif

class PacketUsbCdc;
class PacketInterface;
class McHelperWindow;

class UsbMonitor : public QThread, public MonitorInterface
{
  public:
  	UsbMonitor( );
  	Status scan( QList<PacketInterface*>* arrived );
  	~UsbMonitor( ) {}
  	void run( );
  	void closeAll( );
  	void setInterfaces( MessageInterface* messageInterface, QApplication* application, McHelperWindow* mainWindow );
  	void deviceRemoved( QString key );
  	
  	
	#ifdef Q_WS_WIN
	void removalNotification( HANDLE handle );
	#endif
  	
  private:
  	QHash<QString, PacketUsbCdc*> connectedDevices;
  	QTimer deviceScanTimer;
  	void FindUsbDevices( QList<PacketInterface*>* arrived );
		
	#ifdef Q_WS_WIN
	HANDLE GetDeviceInfo( HDEVINFO HardwareDeviceInfo, PSP_INTERFACE_DEVICE_DATA DeviceInfoData, char* portName );
	bool checkFriendlyName( HDEVINFO HardwareDeviceInfo, PSP_DEVINFO_DATA deviceSpecificInfo, char* portName );
	#endif
	
	#ifdef Q_WS_MAC
	char portName[MAXPATHLEN];
	CFMutableDictionaryRef matchingDictionary;
	#endif
	
	MessageInterface* messageInterface;
	McHelperWindow* mainWindow;
	QApplication* application;
};



#endif // USB_MONITOR_H_





