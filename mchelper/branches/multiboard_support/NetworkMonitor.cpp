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

#include "NetworkMonitor.h"
#include "Osc.h"

#define BROADCAST_TX_PORT 10000
#define BROADCAST_RX_PORT 10000

NetworkMonitor::NetworkMonitor( )
{
	socket = new QUdpSocket( );
	if ( !socket->bind( BROADCAST_RX_PORT ) )
	{
	  socket->close();
	  socket = 0;
	  messageInterface->message( 1, "udp> Can't listen on port %d - make sure it's not already in use.\n", BROADCAST_RX_PORT );
	}
	Osc* osc = new Osc();
	broadcastPing.insert( 0, osc->createOneMessage( "/system/info" ) ); // our constant OSC ping
	delete osc;
	connect( socket, SIGNAL(readyRead()), this, SLOT( processPendingDatagrams() ) );
}

NetworkMonitor::Status NetworkMonitor::scan( QList<PacketInterface*>* arrived )
{
	// responses to our broadcast will be asynchronous...don't wait around, just check on the next scan
	socket->writeDatagram( broadcastPing.data(), broadcastPing.size(), QHostAddress::Broadcast, BROADCAST_TX_PORT ); 
	if( !newDevices.isEmpty() )
	{
		QHash<QString, PacketUdp*>::iterator i = newDevices.begin( );
		while( i != newDevices.end( ) )
		{
			arrived->append( i.value( ) );
			i = newDevices.erase( i ); // this increments the iterator
		}
	}
	return OK; 
}

void NetworkMonitor::processPendingDatagrams()
{
    while (socket->hasPendingDatagrams())
    {
        QByteArray* datagram = new QByteArray( );
        QHostAddress* sender = new QHostAddress( );
        quint16 senderPort;
        datagram->resize( socket->pendingDatagramSize() );
        socket->readDatagram( datagram->data(), datagram->size(), sender, &senderPort );
        QString socketKey = sender->toString( );
        if( !connectedDevices.contains( socketKey ) )
        {
        	PacketUdp* device = new PacketUdp( );
	      	connectedDevices.insert( socketKey, device );  // stick it in our own list of boards we know about
	      	newDevices.insert( socketKey, device );
	      	
	      	device->setInterfaces( messageInterface, application, this );
	      	device->setRemoteHostInfo( sender, senderPort );
	      	device->open( );
        }
        if( connectedDevices.contains( socketKey ) ) // pass the packet through to the packet interface
        {
        	connectedDevices.value( socketKey )->incomingMessage( datagram );
        	connectedDevices.value( socketKey )->processPacket( );
        	connectedDevices.value( socketKey )->resetTimer( );
        	
        }
    }
}

void NetworkMonitor::setInterfaces( MessageInterface* messageInterface, QApplication* application )
{
	this->messageInterface = messageInterface;
	this->application = application;
}










