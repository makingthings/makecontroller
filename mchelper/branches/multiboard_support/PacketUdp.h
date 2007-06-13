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

#include "PacketInterface.h"
#include "MessageInterface.h"
#include "PacketReadyInterface.h"

class PacketUdp : public QObject, public PacketInterface
{	
	Q_OBJECT
	
	public:
	  PacketUdp( );
	  Status open( );
	  Status close( );
		void setInterfaces( PacketReadyInterface* packetReadyInterface, MessageInterface* messageInterface );
		
		// From PacketInterface
	  int sendPacket( char* packet, int length );
    void uiSendPacket( QString rawString );
		int receivePacket( char* packet, int length );
	  bool isPacketWaiting( );
	  char* location( void );
		
	public slots:
		void setLocalPort( int port, bool change );
		void setRemotePort( int port );
		void setHostAddress( QHostAddress address );
		
	private:
	  MessageInterface* messageInterface;
		PacketReadyInterface* packetReadyInterface;
	  QUdpSocket* socket;
	  QHostAddress* remoteHostAddress;
	
    char* remoteAddress;
    int remotePort;
    int localPort;
	
	private slots:
		void processPacket( );
};

#endif
