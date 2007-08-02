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
#include "BoardArrivalEvent.h"

#define BROADCAST_TX_PORT 10000
#define BROADCAST_RX_PORT 10000
#define PING_FREQUENCY 1000

NetworkMonitor::NetworkMonitor( )
{
	QHostInfo::lookupHost( QHostInfo::localHostName(), this, SLOT(lookedUp(QHostInfo)));
	socket = new QUdpSocket( );
	if ( !socket->bind( BROADCAST_RX_PORT ) )
	{
	  socket->close();
	  socket = 0;
	  mainWindow->messageThreadSafe( QString( "Udp> Can't listen on port %1 - make sure it's not already in use.").arg( BROADCAST_RX_PORT) );
	}
	connect( socket, SIGNAL(readyRead()), this, SLOT( processPendingDatagrams() ) );
	connect( &pingTimer, SIGNAL( timeout() ), this, SLOT( sendPing() ) );
	
	Osc* osc = new Osc();
	int length, i;
	char packet[1024], *ptr;
	osc->createOneRequest( packet, &length, "/network/find" ); // our constant OSC ping
	ptr = packet;
	broadcastPing.resize( length );
	for( i=0; i < length; i++ )
		broadcastPing.insert( i, *ptr++ );
	delete osc;
}

void NetworkMonitor::start( )
{
	pingTimer.start( PING_FREQUENCY );
}

void NetworkMonitor::sendPing( )
{
	socket->writeDatagram( broadcastPing.data(), broadcastPing.size(), QHostAddress::Broadcast, BROADCAST_TX_PORT );
}

NetworkMonitor::Status NetworkMonitor::scan( QList<PacketUdp*>* arrived )
{
	// not used
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
    if( !connectedDevices.contains( socketKey ) )
    {
    	PacketUdp* device = new PacketUdp( );
    	connectedDevices.insert( socketKey, device );  // stick it in our own list of boards we know about
    	
    	device->setRemoteHostInfo( sender, BROADCAST_TX_PORT );
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

    myAddress = host.addresses().first();
}

void NetworkMonitor::deviceRemoved( QString key )
{
	if( connectedDevices.contains( key ) )
	{
		mainWindow->removeDeviceThreadSafe( key );
		if( !connectedDevices.remove( key ) )
			return;  // TODO - return an error here
	}
}

void NetworkMonitor::setInterfaces( MessageInterface* messageInterface, McHelperWindow* mainWindow, QApplication* application )
{
	this->messageInterface = messageInterface;
	this->mainWindow = mainWindow;
	this->application = application;
}










