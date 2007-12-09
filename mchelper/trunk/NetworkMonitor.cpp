/*********************************************************************************

 Copyright 2006-2007 MakingThings

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
#include "BoardArrivalEvent.h"

#define PING_FREQUENCY 1000

NetworkMonitor::NetworkMonitor( int listenPort )
{
	rxtxPort = listenPort;
	if ( !socket.bind( rxtxPort ) )
	{
	  socket.close();
	  mainWindow->messageThreadSafe( QString( "Error: Can't listen on port %1 - make sure it's not already in use.").arg( rxtxPort ), MessageEvent::Error, "Ethernet" );
	}
	connect( &socket, SIGNAL(readyRead()), this, SLOT( processPendingDatagrams() ) );
	connect( &pingTimer, SIGNAL( timeout() ), this, SLOT( sendPing() ) );
	createPing( );
}

void NetworkMonitor::createPing( )
{
	Osc osc;
	int length, i;
	char packet[1024], *ptr;
	osc.createOneRequest( packet, &length, "/network/find" ); // our constant OSC ping
	ptr = packet;
	for( i=0; i < length; i++ )
		broadcastPing.insert( i, *ptr++ );
	broadcastPing.resize( length );
}

void NetworkMonitor::start( )
{
	pingTimer.start( PING_FREQUENCY );
}

void NetworkMonitor::sendPing( )
{
	if( mainWindow->findNetBoardsEnabled( ) )
	{
		if( socket.state( ) != QAbstractSocket::BoundState )
			socket.bind( rxtxPort, QUdpSocket::ShareAddress );
			
		if( socket.state( ) == QAbstractSocket::BoundState ) // if we succeeded or we're already open
		{
			if( socket.writeDatagram( broadcastPing, QHostAddress::Broadcast, rxtxPort ) < 0 )
				printf( "UDP send error: %d\n", socket.error( ) );
		}
	}
}

bool NetworkMonitor::changeListenPort( int port )
{
	socket.close( );
	if( !socket.bind( port, QUdpSocket::ShareAddress ) )
	{
		mainWindow->messageThreadSafe( QString( "Error: Can't listen on port %1 - make sure it's not already in use.").arg( rxtxPort ), MessageEvent::Error, "Ethernet" );
		return false;
	}
	else
	{
		rxtxPort = port;
		return true;
	}
}

int NetworkMonitor::getListenPort( )
{
	return rxtxPort;
}

NetworkMonitor::Status NetworkMonitor::scan( QList<PacketUdp*>* arrived )
{
	// not used
	(void)arrived;
	return OK; 
}

void NetworkMonitor::processPendingDatagrams()
{
  while( socket.hasPendingDatagrams() )
  {
    QByteArray datagram;
    QHostAddress sender;
    datagram.resize( socket.pendingDatagramSize() );
    socket.readDatagram( datagram.data(), datagram.size(), &sender );
		
		if( QString( datagram.data() ) == QString( broadcastPing.data() ) && datagram.size() == broadcastPing.size() )
			break;
		
    QString socketKey = sender.toString( );
    if( !connectedDevices.contains( socketKey ) )
    {
    	PacketUdp* device = new PacketUdp( );
    	connectedDevices.insert( socketKey, device );  // stick it in our own list of boards we know about
    	
    	device->setRemoteHostInfo( &sender, rxtxPort );
    	device->setKey( socketKey );
    	device->setInterfaces( messageInterface, this );
    	device->open( );
    	
    	// post it to the UI
    	BoardArrivalEvent* event = new BoardArrivalEvent( Board::Udp );
			event->pUdp.append( device );
			application->postEvent( mainWindow, event );
    }
    if( connectedDevices.contains( socketKey ) ) // pass the packet through to the packet interface
    {
    	connectedDevices.value( socketKey )->incomingMessage( datagram );
    	connectedDevices.value( socketKey )->processPacket( );
    	connectedDevices.value( socketKey )->resetTimer( );
    }
  }
}

void NetworkMonitor::deviceRemoved( QString key )
{
	if( connectedDevices.contains( key ) )
	{
		PacketUdp* udp = connectedDevices.take( key );
		if( udp->isOpen() )
			udp->close( );
		mainWindow->removeDeviceThreadSafe( key );
	}
}

void NetworkMonitor::setInterfaces( MessageInterface* messageInterface, McHelperWindow* mainWindow, QApplication* application )
{
	this->messageInterface = messageInterface;
	this->mainWindow = mainWindow;
	this->application = application;
}










