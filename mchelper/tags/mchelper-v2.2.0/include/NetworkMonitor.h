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

#include <QList>
#include <QHash>
#include <QTimer>
#include <QtNetwork>
#include <QHostInfo>
#include <QMutex>
#include "McHelperWindow.h"
#include "PacketUdp.h"
#include "MonitorInterface.h"

class PacketUdp;
class McHelperWindow;

class NetworkMonitor : public QObject, public MonitorInterface
{
  Q_OBJECT
  public:
  	NetworkMonitor( int listenPort, int sendPort );
  	~NetworkMonitor( ) {}
  	void start( );
  	Status scan( QList<PacketUdp*>* arrived );
  	void setInterfaces( MessageInterface* messageInterface, McHelperWindow* mainWindow, QApplication* application );
  	void deviceRemoved( QString key );
		bool changeListenPort( int port );
		void changeSendPort( int port );
		int getSendPort( ) { return sendPort; }
		int getListenPort( ) { return listenPort; }
  	
  private:
		QHash<QString, PacketUdp*> connectedDevices; // our internal list
		MessageInterface* messageInterface;
		McHelperWindow* mainWindow;
		QApplication* application;
		QTimer pingTimer;
		QUdpSocket socket;
		QByteArray broadcastPing;
		QHostAddress localBroadcastAddress;
		int listenPort;
		int sendPort;
		bool sendLocal;
	
  private slots:
		void processPendingDatagrams( );
		void sendPing( );
		void lookedUp( const QHostInfo &host );
};

#endif // NETWORK_MONITOR_H_

















