/****************************************************************************
**
** PACKETUDP
** MakingThings 2006.
**
****************************************************************************/

#include "PacketUdp.h"

#include <QHostInfo>
#include <QHostAddress>
#include <QList>

PacketUdp::PacketUdp( MessageInterface* messageInterface )
{
	this->messageInterface = messageInterface;
}

PacketUdp::Status PacketUdp::connect( char* remoteAddress, int remotePort, int localPort )
{	
  this->remoteAddress = remoteAddress;
  this->remotePort = remotePort; 
  this->localPort = localPort;	

  socket = new QUdpSocket( );
  if ( !socket->bind( localPort ) )
  {
  	socket->close();
  	socket = 0;
    return ERROR_CANT_BIND;
  }
  QAbstractSocket::SocketState s = socket->state();
     
  messageInterface->message( 2, "  PacketUdp Listening on %d - state %d\n", localPort, (int)s );
  
  // Make a QString out of the regular char*
  QString as( remoteAddress );
  
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
  remoteHostAddress = new QHostAddress( );
  if ( !remoteHostAddress->setAddress( as ) )
    return ERROR_CANT_GET_ADDRESS;  
  
  return OK;	
}

PacketUdp::Status PacketUdp::disconnect( )
{
  if ( socket != 0 )
	  socket->close();
  return OK;
}

int PacketUdp::sendPacket( char* packet, int length )
{
	// messageInterface->message( 2, "  PacketUdp Sending %s:%d\n", remoteAddress, remotePort );	

	qint64 result = socket->writeDatagram( (const char*)packet, (qint64)length, *remoteHostAddress, remotePort );
	if( result < 0 )
		messageInterface->message( 1, "  Could not send packet.\n" );

	return 0;
}

bool PacketUdp::isPacketWaiting( )
{
  return socket->hasPendingDatagrams();
}

int PacketUdp::receivePacket( char* buffer, int size )
{
	int length;
	if ( !( length = socket->readDatagram( buffer, size ) ) )
  {
		messageInterface->message( 1, "  PacketUdp Error Receiving\n" );	
		return 0;
	}
	return length;
}
