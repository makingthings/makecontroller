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


#ifndef NETWORK_MONITOR_H_
#define NETWORK_MONITOR_H_

#include <QList>
#include <QHash>
#include <QTimer>
#include <QtNetwork>
#include "PacketUdp.h"
#include "MonitorInterface.h"
#include "BoardListModel.h"

class PacketUdp;
class BoardListModel;

class NetworkMonitor : public QObject, public MonitorInterface
{
  Q_OBJECT
  public:
  	NetworkMonitor( );
  	~NetworkMonitor( ) {}
  	Status scan( QList<PacketInterface*>* arrived );
  	void setInterfaces( MessageInterface* messageInterface, QApplication* application, BoardListModel* boardListModel );
  	void deviceRemoved( QString key );
  	
  private:
  	QHash<QString, PacketUdp*> connectedDevices; // our internal list
  	// and a list to keep track of whether or not we've reported a new device to the model/view
  	QHash<QString, PacketUdp*> newDevices;  
	
	MessageInterface* messageInterface;
	PacketReadyInterface* packetReadyInterface;
	QApplication* application;
	BoardListModel* boardListModel;
	QTimer* timer;
	QUdpSocket* socket;
	QByteArray broadcastPing;
	QHostAddress myAddress;
	
  private slots:
	void processPendingDatagrams( );
	void lookedUp(const QHostInfo &host);
};



#endif // NETWORK_MONITOR_H_

















