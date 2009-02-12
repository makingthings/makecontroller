/*********************************************************************************

 Copyright 2006-2008 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#include "UsbMonitor.h"
#include "PacketUsbSerial.h"

#define MAKE_CONTROLLER_VID 0xEB03
#define MAKE_CONTROLLER_PID 0x0920
#define SAM_BA_VID          0x03EB
#define SAM_BA_PID          0x6124

/*
 Scans the USB system for boards and reports whether boards have been attached/removed.
*/
UsbMonitor::UsbMonitor(MainWindow* mw) : QThread()
{
	mainWindow = mw;
  qRegisterMetaType<BoardType::Type>("BoardType::Type"); // silly Qt thing to communicate via signal across a thread
  connect(this, SIGNAL(newBoards(QStringList, BoardType::Type)), 
                       mainWindow, SLOT(onUsbDeviceArrived(QStringList, BoardType::Type)));
  connect(this, SIGNAL(boardsRemoved(QString)), mainWindow, SLOT(onDeviceRemoved(QString)));
  connect( &enumerator, SIGNAL(deviceDiscovered(QextPortInfo)), this, SLOT(onDeviceDiscovered(QextPortInfo)));
  connect( &enumerator, SIGNAL(deviceTerminated(QextPortInfo)), this, SLOT(onDeviceTerminated(QextPortInfo)));
  #ifdef Q_WS_MAC
  enumerator.setUpNotifications();
  #elif (defined Q_WS_WIN)
  QList<QextPortInfo> ports = enumerator.getPorts();
  if( ports.count())
  {
    QStringList boards;
    foreach( QextPortInfo port, ports )
      onDeviceDiscovered( port );
  }
  enumerator.setUpNotifications(mainWindow);
  #endif
}

/*
 This is the loop in our separate thread.
 Scan for boards.  If there are new boards, post them to the UI.
 If boards have been removed, alert the UI.
*/
void UsbMonitor::run( )
{
  forever
  {
    QList<QextPortInfo> ports = enumerator.getPorts();
    QStringList newSerialPorts;
    QStringList newSambaPorts;
    QStringList portNames;
    
    // first check if there are any new boards
    foreach(QextPortInfo port, ports)
    {
      // the portname needs to be tweeked 
      if( !usbSerialList.contains(port.portName) )
      {
        if( isMakeController( &port ) )
        {
          usbSerialList.append(port.portName);  // keep our internal list, the portName is the unique key
          newSerialPorts.append(port.portName); // on the list to be posted to the UI
        }
      }
      
      if( !usbSambaList.contains(port.portName) )
      {
        if( isSamBa( &port ) )
        {
          usbSambaList.append(port.portName);  // keep our internal list, the portName is the unique key
          newSambaPorts.append(port.portName); // on the list to be posted to the UI
        }
      }
      // check for samba boards...
      portNames << port.portName;
    }
    
    if(newSerialPorts.count())
      emit newBoards(newSerialPorts, BoardType::UsbSerial);
    if(newSambaPorts.count())
      emit newBoards(newSambaPorts, BoardType::UsbSamba);
      
    // if any boards we know about are no longer in the list, they've been removed
    foreach(QString key, usbSerialList)
    {
      if(!portNames.contains(key))
      {
        usbSerialList.removeAt(usbSerialList.indexOf(key));
        emit boardsRemoved(key);
      }
    }
    
    // same thing for the samba boards
    foreach(QString key, usbSambaList)
    {
      if(!portNames.contains(key))
      {
        usbSambaList.removeAt(usbSambaList.indexOf(key));
        emit boardsRemoved(key);
      }
    }
    
    sleep(1); // scan once per second
  }
}

#ifdef Q_WS_WIN
void UsbMonitor::onDeviceChangeEventWin( WPARAM wParam, LPARAM lParam )
{
  enumerator.onDeviceChangeWin( wParam, lParam );
}
#endif

void UsbMonitor::onDeviceDiscovered(QextPortInfo info)
{
  #ifdef Q_WS_MAC
  msleep(50); // not quite sure why this is necessary...only really need it when a port has been
  // opened, closed and then re-opened...
  #endif
  qDebug() << tr("device discovered at %s").arg(info.portName);
  if( isMakeController(&info) )
  {
    QStringList ports = QStringList() << info.portName.toAscii();
    emit newBoards(ports, BoardType::UsbSerial);
  }
  else if( isSamBa( &info ) )
  {
    QStringList ports = QStringList() << info.portName.toAscii();
    emit newBoards(ports, BoardType::UsbSamba);
  }
}

void UsbMonitor::onDeviceTerminated(QextPortInfo info)
{
  emit boardsRemoved(info.portName.toAscii());
}

bool UsbMonitor::isMakeController(QextPortInfo* info)
{
  if( info->portName.isEmpty() )
    return false;
  else
    return ( info->friendName.startsWith("Make Controller Ki") ||
           (info->vendorID == MAKE_CONTROLLER_VID && info->productID == MAKE_CONTROLLER_PID));
}

bool UsbMonitor::isSamBa(QextPortInfo* info)
{
  return (info->vendorID == SAM_BA_VID && info->productID == SAM_BA_PID);
}







