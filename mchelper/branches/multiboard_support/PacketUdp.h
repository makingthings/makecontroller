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

#ifndef PACKETUDP_H
#define PACKETUDP_H

#include <QUdpSocket>
#include <QHostAddress>
#include <QHostInfo>
#include <QTimer>
#include <QList>

#include "PacketInterface.h"
#include "MessageInterface.h"
#include "PacketReadyInterface.h"
#include "MonitorInterface.h"


class PacketUdp : public QObject, public PacketInterface
{	
	Q_OBJECT
	
	public:
	  PacketUdp( );
	  ~PacketUdp( );
	  Status open( );
		void setInterfaces( MessageInterface* messageInterface , MonitorInterface* monitor );
		void setPacketReadyInterface( PacketReadyInterface* packetReadyInterface );
		void resetTimer( void );
		
		// From PacketInterface
	  int sendPacket( char* packet, int length );
		int receivePacket( char* packet, int length );
	  bool isPacketWaiting( );
	  QString getKey( void );
	  char* location( );
	  void incomingMessage( QByteArray* message );
	  void setRemoteHostInfo( QHostAddress* address, quint16 port );
	  void setKey( QString key );
		
	public slots:
		Status close( );
		void processPacket( );
		Status pingTimedOut( );
		
	private:
	  MessageInterface* messageInterface;
		PacketReadyInterface* packetReadyInterface;
	  QUdpSocket* socket;
	  QHostAddress* remoteHostAddress;
	  QByteArray remoteHostName;
	  QTimer* timer;
	  MonitorInterface* monitor;
	  QByteArray* lastMessage;
	  QString socketKey;
	
    char* remoteAddress;
    int localPort;
    int remotePort;
		
};

#endif
