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

#include "PacketUdp.h"

#include <QHostInfo>
#include <QHostAddress>
#include <QSettings>
#include <QString>

#define COMM_TIMEOUT 3000

PacketUdp::PacketUdp( )
{ 
	oscTranslator = new Osc();
	packetReadyInterface = oscTranslator;
	timer = new QTimer(this);
	lastMessage = NULL;
    connect( timer, SIGNAL(timeout()), this, SLOT( close( ) ) );
}

PacketUdp::Status PacketUdp::open( ) //part of PacketInterface
{	
  socket = new QUdpSocket( this );
    timer->start( COMM_TIMEOUT );
  
  return OK;	
}

PacketUdp::Status PacketUdp::close( )	//part of PacketInterface
{
  
  if ( socket != 0 )
	  socket->close( );
	
	delete remoteHostAddress;
	if( lastMessage != NULL )
  		delete lastMessage;
  		
  	monitor->deviceRemoved( socketKey );
  		
  return OK;
}

void PacketUdp::resetTimer( void )
{
	timer->start( COMM_TIMEOUT );
}

QString PacketUdp::location( )
{
	return socketKey;
}

int PacketUdp::sendPacket( char* packet, int length )	//part of PacketInterface
{
	qint64 result = socket->writeDatagram( (const char*)packet, (qint64)length, *remoteHostAddress, remotePort );
	if( result < 0 )
		messageInterface->message( 1, "udp> Could not send packet.\n" );

	return 0;
}

void PacketUdp::uiSendPacket( QString rawString )
{
  // pass this straight through to Osc
  oscTranslator->uiSendPacket(rawString);
}


bool PacketUdp::isPacketWaiting( )	//part of PacketInterface
{
  return lastMessage != NULL;
}

void PacketUdp::processPacket( )	//slot to be called back automatically when datagrams are ready to be read
{
  packetReadyInterface->packetWaiting( );
}

int PacketUdp::receivePacket( char* buffer, int size )
{
	int length = lastMessage->size( );
	if( length > size )
	{
		messageInterface->message( 1, "udp> error - packet too large.\n" );
		return 0;
	}
	memcpy( buffer, lastMessage->data( ), length );
	delete lastMessage;
	lastMessage = NULL;
	
	return length;
}

void PacketUdp::incomingMessage( QByteArray* message )
{
	lastMessage = message;
}

void PacketUdp::setRemoteHostInfo( QHostAddress* address, quint16 port )
{
	if( *address == QHostAddress::Null )
		return;
		
	remoteHostAddress = address;
	remoteHostName = remoteHostAddress->toString( ).toAscii();
	remotePort = port;
}

void PacketUdp::setKey( QString key )
{
	socketKey = key;
}

void PacketUdp::setInterfaces( MessageInterface* messageInterface, QApplication* application, MonitorInterface* monitor )
{
	this->messageInterface = messageInterface;
	this->packetReadyInterface = packetReadyInterface;
	this->monitor = monitor;
	// once we have these, we can set up our Osc object
	oscTranslator->setInterfaces( this, messageInterface, application );
	oscTranslator->setPreamble( remoteHostName.data() );
}

