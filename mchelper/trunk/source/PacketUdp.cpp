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

#include "PacketUdp.h"

#include <QHostInfo>
#include <QHostAddress>
#include <QSettings>
#include <QString>
#include <QMutexLocker>

#define COMM_TIMEOUT 3000

PacketUdp::PacketUdp( )
{ 
	timer = new QTimer(this);
	socket = NULL;
	lastMessage.clear( );
	packetReadyInterface = NULL;
	connect( timer, SIGNAL(timeout()), this, SLOT( pingTimedOut( ) ) );
}

PacketUdp::~PacketUdp( )
{
	delete timer;
}

PacketUdp::Status PacketUdp::open( ) //part of PacketInterface
{	
  socket = new QUdpSocket( this );
  return OK;	
}

PacketUdp::Status PacketUdp::pingTimedOut( )
{
	timer->stop( );
	monitor->deviceRemoved( socketKey );
	return OK;
}

PacketUdp::Status PacketUdp::close( )	//part of PacketInterface
{
	if ( socket != 0 )
	  socket->close( );
  return OK;
}

void PacketUdp::resetTimer( void )
{
	timer->start( COMM_TIMEOUT );
}

QString PacketUdp::getKey( )
{
	return socketKey;
}

char* PacketUdp::location( )
{
	return remoteHostName.data( );
}

PacketUdp::Status PacketUdp::sendPacket( char* packet, int length )	//part of PacketInterface
{
	qint64 result = socket->writeDatagram( (const char*)packet, (qint64)length, remoteHostAddress, monitor->getSendPort( ) );
	if( result < 0 )
  {
		QString msg = QString( "Error - Could not send packet.");
		messageInterface->messageThreadSafe( msg, MessageEvent::Error, QString("Ethernet") );
		return IO_ERROR;
  }
	return OK;
}

bool PacketUdp::isPacketWaiting( )	//part of PacketInterface
{
  QMutexLocker locker( &msgMutex );
  return lastMessage.size( ) > 0;
}

bool PacketUdp::isOpen( )
{
	return socket != NULL;
}

void PacketUdp::processPacket( )	//slot to be called back automatically when datagrams are ready to be read
{
  if( packetReadyInterface != NULL )
  	packetReadyInterface->packetWaiting( );
}

int PacketUdp::receivePacket( char* buffer, int size )
{
	QMutexLocker locker( &msgMutex );
	int length = lastMessage.size( );
	if( length > size )
	{
		QString msg = QString( "Error - packet too large.");
		messageInterface->messageThreadSafe( msg, MessageEvent::Error, QString("Ethernet") );
		return 0;
	}
	memcpy( buffer, lastMessage.data( ), length );
	lastMessage.clear( );
	return length;
}

void PacketUdp::incomingMessage( QByteArray message )
{
	QMutexLocker locker( &msgMutex );
	lastMessage = message;
}

void PacketUdp::setRemoteHostInfo( QHostAddress* address, quint16 port )
{
	if( *address == QHostAddress::Null )
		return;
		
	remoteHostAddress = *address;
	remoteHostName = remoteHostAddress.toString( ).toAscii();
	remotePort = port;
}

void PacketUdp::setKey( QString key )
{
	socketKey = key;
}

void PacketUdp::setPacketReadyInterface( PacketReadyInterface* packetReadyInterface )
{
	this->packetReadyInterface = packetReadyInterface;
}

void PacketUdp::setInterfaces( MessageInterface* messageInterface, NetworkMonitor* monitor )
{
	this->messageInterface = messageInterface;
	this->monitor = monitor;
}

