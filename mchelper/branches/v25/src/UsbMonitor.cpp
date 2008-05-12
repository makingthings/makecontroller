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
#include "qextserialenumerator.h"

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
}

/*
 This is the loop in our separate thread.
 Scan for boards.  If there are new boards, post them to the UI.
 If boards have been removed, alert the UI.
*/
void UsbMonitor::run( )
{
	QextSerialEnumerator enumerator;
  forever
  {
    QList<QextPortInfo> ports = enumerator.getPorts();
    QStringList newSerialPorts;
    QStringList newSambaPorts;
    QStringList portNames;
    
    // first check if there are any new boards
    foreach(QextPortInfo port, ports)
    {
      if(!usbSerialList.contains(port.portName) && port.friendName.startsWith("Make Controller Ki"))
      {
        usbSerialList.append(port.portName); // keep our internal list, the portName is the unique key
        newSerialPorts.append(port.portName); // on the list to be posted to the UI
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
    
    // then check the samba boards
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





