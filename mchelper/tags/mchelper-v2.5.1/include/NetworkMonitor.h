/*********************************************************************************

 Copyright 2006-2009 MakingThings

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
  bool setListenPort( int port, bool announce = true );
  int listenPort( ) { return listen_port; }
  void setSendPort( int port ) { send_port = port; }
  int sendPort( ) { return send_port; }
  void setDiscoveryMode( bool enabled ) { sendDiscoveryPackets = enabled; }

private:
  MainWindow* mainWindow;
  QHash<QString, PacketUdp*> connectedDevices;
  int listen_port;
  int send_port;
  QTimer pingTimer;
  QByteArray broadcastPing;
  QHostAddress localBroadcastAddress;
  bool sendLocal;
  bool sendDiscoveryPackets;

private slots:
  void processPendingDatagrams( );
  void lookedUp( const QHostInfo &host );
  void sendPing( );

public slots:
  void onDeviceRemoved(const QString & key);

signals:
  void deviceArrived(PacketInterface* pi);
  void deviceRemoved(const QString & key);
  void msg(QString msg, MsgType::Type type, QString from);
};

#endif // NETWORK_MONITOR_H_

















