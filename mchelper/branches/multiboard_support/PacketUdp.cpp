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

PacketUdp::PacketUdp( )
{ }

PacketUdp::Status PacketUdp::open( ) //part of PacketInterface
{	
  socket = new QUdpSocket( );
  if ( !socket->bind( localPort ) )
  {
  	socket->close();
  	socket = 0;
		messageInterface->message( 1, "udp> Can't listen on port %d - make sure it's not already in use.\n", localPort );
    return ERROR_CANT_BIND;
  }
  QAbstractSocket::SocketState s = socket->state();
	connect( socket, SIGNAL( readyRead( ) ), this, SLOT( processPacket( ) ) );
  messageInterface->message( 2, "  PacketUdp Listening on %d - state %d\n", localPort, (int)s );
  
  // Make a QString out of the regular char*
  //QString as( remoteAddress );
  
  /*
  // Turn it into a real address
  QHostInfo info = QHostInfo::fromName( as );
  if ( !info.addresses().isEmpty() ) 
  {
    remoteHostAddress = new QHostAddress( info.addresses().first() );
  	messageInterface->message( 2, "PacketUdp %s : %s\n", remoteAddress, remoteHostAddress->toString().toAscii().data() );
  }
  else
  {
  	messageInterface->message( 2, "PacketUdp %s : Not found\n", remoteAddress );
  } 
  */
  //remoteHostAddress = new QHostAddress( );
  //if ( !remoteHostAddress->setAddress( as ) )
    //return ERROR_CANT_GET_ADDRESS;  
  
  return OK;	
}

PacketUdp::Status PacketUdp::close( )	//part of PacketInterface
{
  if ( socket != 0 )
	  socket->close();
  return OK;
}

char* PacketUdp::location( )
{
	return "not implemented";
}

int PacketUdp::sendPacket( char* packet, int length )	//part of PacketInterface
{
	//printf( "PacketUdp Sending %s:%d\n", *remoteHostAddress.toString().toAscii().toConstData(), remotePort );	

	qint64 result = socket->writeDatagram( (const char*)packet, (qint64)length, *remoteHostAddress, remotePort );
	if( result < 0 )
		messageInterface->message( 1, "udp> Could not send packet.\n" );

	return 0;
}

void PacketUdp::uiSendPacket( QString rawString )
{
  // :TODO: implement this
}


bool PacketUdp::isPacketWaiting( )	//part of PacketInterface
{
  return socket->hasPendingDatagrams();
}

void PacketUdp::processPacket( )	//slot to be called back automatically when datagrams are ready to be read
{
  packetReadyInterface->packetWaiting( );
}

int PacketUdp::receivePacket( char* buffer, int size )	//part of PacketInterface
{
	int length;
	if ( !( length = socket->readDatagram( buffer, size ) ) )
  {
		messageInterface->message( 1, "udp> Error receiving packet.\n" );	
		return 0;
	}
	return length;
}

void PacketUdp::setLocalPort( int port, bool change )
{
	if( port != 0 && port != localPort )	// this will be zero when nothing has been entered into the text field
	{	
		localPort = port;
		if( change )
		{
		  socket->close();
		  if( !socket->bind( localPort ) )
				messageInterface->message( 1, "udp> Can't listen on port %d - make sure it's not already in use.\n", localPort );
			else if( socket->state() == QAbstractSocket::BoundState )
				messageInterface->message( 1, "udp> New local port: %d\n", localPort );
		}
	}
}

void PacketUdp::setRemotePort( int port )
{
	if( port != 0 )		// this will be zero when nothing has been entered into the text field
		remotePort = port;
}

void PacketUdp::setHostAddress( QHostAddress address )
{
	if( address == QHostAddress::Null )
		return;
	QHostAddress* hostAddress = new QHostAddress( address );
	remoteHostAddress = hostAddress;
}

void PacketUdp::setInterfaces( PacketReadyInterface* packetReadyInterface, MessageInterface* messageInterface )
{
	this->messageInterface = messageInterface;
	this->packetReadyInterface = packetReadyInterface;
}

