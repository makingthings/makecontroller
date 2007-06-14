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

#include <QList>
#include <QHash>
#include "PacketUsbCdc.h"
#include "MonitorInterface.h"
#include "BoardListModel.h"

class PacketUsbCdc;

class UsbMonitor : public MonitorInterface
{
  public:
  	UsbMonitor( );
  	Status scan( QList<PacketInterface*>* arrived );
  	~UsbMonitor( ) {}
  	void closeAll( );
  	void setInterfaces( MessageInterface* messageInterface, QApplication* application );
  	
  	
  	#ifdef Q_WS_WIN
	void setWidget( QMainWindow* mainWindow );
	void deviceRemoved( HANDLE handle );
	#endif
  	
  private:
  	QHash<QString, PacketUsbCdc*> connectedDevices;
  	void FindUsbDevices( QList<PacketInterface*>* arrived );
	HANDLE GetDeviceInfo( HDEVINFO HardwareDeviceInfo, PSP_INTERFACE_DEVICE_DATA DeviceInfoData, char* portName );
	bool checkFriendlyName( HDEVINFO HardwareDeviceInfo, PSP_DEVINFO_DATA deviceSpecificInfo, char* portName );
	
	MessageInterface* messageInterface;
	QMainWindow* mainWindow;
	QApplication* application;
	BoardListModel* boardListModel;
};



#endif // USB_MONITOR_H_





