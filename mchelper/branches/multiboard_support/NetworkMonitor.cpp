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
#define BROADCAST_PING_LENGTH 20

NetworkMonitor::NetworkMonitor( )
{
	QHostInfo::lookupHost( QHostInfo::localHostName(), this, SLOT(lookedUp(QHostInfo)));
	socket = new QUdpSocket( );
	if ( !socket->bind( BROADCAST_RX_PORT ) )
	{
	  socket->close();
	  socket = 0;
	  messageInterface->message( 1, "udp> Can't listen on port %d - make sure it's not already in use.\n", BROADCAST_RX_PORT );
	}
	Osc* osc = new Osc();
	broadcastPing.resize( BROADCAST_PING_LENGTH );
	osc->createOneRequest( broadcastPing.data(), "/system/info" ); // our constant OSC ping
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
        datagram->resize( socket->pendingDatagramSize() );
        socket->readDatagram( datagram->data(), datagram->size(), sender );
        if( datagram->size() <= 0 || *sender == myAddress )
        {
        	delete datagram;
        	delete sender;
        	break;
        }
        QString socketKey = sender->toString( );
        // printf( "Message from %s\n", socketKey.toAscii().data() );
        if( !connectedDevices.contains( socketKey ) )
        {
        	PacketUdp* device = new PacketUdp( );
	      	connectedDevices.insert( socketKey, device );  // stick it in our own list of boards we know about
	      	newDevices.insert( socketKey, device );
	      	
	      	device->setRemoteHostInfo( sender, BROADCAST_TX_PORT );
	      	device->setKey( socketKey );
	      	device->setInterfaces( messageInterface, application, this );
	      	device->open( );
        }
        if( connectedDevices.contains( socketKey ) ) // pass the packet through to the packet interface
        {
        	QString filter( "/system/info" );
        	if( QString( datagram->data( ) ) != filter )
        	{
        		connectedDevices.value( socketKey )->incomingMessage( datagram );
        		connectedDevices.value( socketKey )->processPacket( );
        	}
        	connectedDevices.value( socketKey )->resetTimer( );
        }
    }
}

void NetworkMonitor::lookedUp(const QHostInfo &host)
{
	if (host.error() != QHostInfo::NoError) // lookup failed
		return;

    myAddress = host.addresses().first();
}

void NetworkMonitor::deviceRemoved( QString key )
{
	if( connectedDevices.contains( key ) )
	{
		delete connectedDevices.value( key );
		boardListModel->removeBoard( key, Board::Udp );
		if( !connectedDevices.remove( key ) )
			return;  // TODO - return an error here
	}
}

void NetworkMonitor::setInterfaces( MessageInterface* messageInterface, QApplication* application, BoardListModel* boardListModel )
{
	this->messageInterface = messageInterface;
	this->application = application;
	this->boardListModel = boardListModel;
}










