/****************************************************************************
**
** PACKETUDP
** MakingThings 2006.
**
****************************************************************************/

#ifndef PACKETUDP_H
#define PACKETUDP_H

#include <QUdpSocket>
#include <QHostAddress>

#include "PacketInterface.h"
#include "MessageInterface.h"

class PacketUdp : public PacketInterface
{		
	public:	  
	  PacketUdp( MessageInterface* messageInterface );
	  
	  Status connect( char* remoteAddress, int remotePort, int localPort );
	  Status disconnect( );
		
		// From PacketInterface
	  int sendPacket( char* packet, int length );
	  bool isPacketWaiting( );
	  int receivePacket( char* packet, int length );
		
	private:
	  MessageInterface* messageInterface;
	  
	  QUdpSocket* socket;
	  QHostAddress* remoteHostAddress;
	
    char* remoteAddress;
    int remotePort;
    int localPort;
};

#endif
