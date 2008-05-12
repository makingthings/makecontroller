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


#ifndef NETWORK_MONITOR_H_
#define NETWORK_MONITOR_H_

#include <QUdpSocket>
#include "MainWindow.h"
#include "BonjourServiceBrowser.h"
#include "BonjourServiceResolver.h"
#include "BonjourRecord.h"
#include "Board.h"
#include "PacketUdp.h"
#include "PacketInterface.h"
#include "MsgType.h"

class MainWindow;
class PacketUdp;

class NetworkMonitor : public QUdpSocket
{
  Q_OBJECT
public:
  NetworkMonitor( MainWindow* mainWindow );
  ~NetworkMonitor( ) {}
  bool setListenPort( int port );
  int listenPort( ) { return listen_port; }
  
private:
  MainWindow* mainWindow;
  BonjourServiceBrowser *bonjourBrowser;
  BonjourServiceResolver *bonjourResolver;
  QHash<QString, BonjourRecord> bonjourRecordHash;
  QHash<QString, PacketUdp*> connectedDevices;
  int listen_port;
	
private slots:
  void processPendingDatagrams( );
  void updateRecords(const QList<BonjourRecord> &list);
  void recordResolved(const QHostInfo &hostInfo, int port, BonjourRecord record);
  
signals:
  void deviceArrived(QList<PacketInterface*> pi);
  void deviceRemoved(QString key);
  void msg(QString msg, MsgType::Type type, QString from);
};

#endif // NETWORK_MONITOR_H_

















