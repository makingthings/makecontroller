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

NetworkMonitor::NetworkMonitor( int listenPort, int sendPort )
{
	this->listenPort = listenPort;
	this->sendPort = sendPort;
	sendLocal = false;
	QHostInfo::lookupHost( QHostInfo::localHostName(), this, SLOT(lookedUp(QHostInfo))); 
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
	if ( !socket.bind( listenPort ) )
	{
	  socket.close();
	  mainWindow->messageThreadSafe( QString( "Error: Can't listen on port %1 - make sure it's not already in use.").arg( listenPort ), MessageEvent::Error, "Ethernet" );
	}
	pingTimer.start( PING_FREQUENCY );
}

void NetworkMonitor::sendPing( )
{
	if( mainWindow->findNetBoardsEnabled( ) )
	{
		if( socket.state( ) != QAbstractSocket::BoundState )
			socket.bind( listenPort, QUdpSocket::ShareAddress );
			
		// normally we'll be set to send on QHostAddress::Broadcast, but if that fails, just try
		// to send on the local broadcast address
		QHostAddress dest = ( sendLocal ) ? localBroadcastAddress : QHostAddress::Broadcast;
		if( socket.writeDatagram( broadcastPing.data(), broadcastPing.size(), dest, sendPort ) < 0 && !sendLocal )
		{
			socket.writeDatagram( broadcastPing.data(), broadcastPing.size(), localBroadcastAddress, sendPort );
			sendLocal = true;
		}
	}
}

bool NetworkMonitor::changeListenPort( int port )
{
	socket.close( );
	if( !socket.bind( port, QUdpSocket::ShareAddress ) )
	{
		mainWindow->messageThreadSafe( QString( "Error: Can't listen on port %1 - make sure it's not already in use.").arg( port ), MessageEvent::Error, "Ethernet" );
		return false;
	}
	else
	{
		listenPort = port;
		mainWindow->messageThreadSafe( QString( "Now listening on port %1 for messages.").arg( listenPort ), MessageEvent::Info, "Ethernet" );
		return true;
	}
}

void NetworkMonitor::changeSendPort( int port )
{
	sendPort = port;
	mainWindow->messageThreadSafe( QString( "Now sending messages on port %1.").arg( sendPort ), MessageEvent::Info, "Ethernet" );
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
    	
    	device->setRemoteHostInfo( &sender, listenPort );
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

void NetworkMonitor::lookedUp(const QHostInfo &host) 
{ 
	if (host.error() != QHostInfo::NoError) // lookup failed 
		return; 
		
		// find out our address, and then replace the last byte with 0xFF
		// this will be a local broadcast address
		QStringList addrlist = host.addresses().first().toString( ).split( "." );
		addrlist.replace( 3, "255" );
		localBroadcastAddress = addrlist.join( "." );
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










