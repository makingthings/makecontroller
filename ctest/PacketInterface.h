/****************************************************************************
**
** PACKETINTERFACE
** MakingThings 2006.
**
****************************************************************************/

#ifndef PACKETINTERFACE_H
#define PACKETINTERFACE_H

class PacketInterface
{		
	public:
		enum Status { OK, ERROR_CANT_BIND, ERROR_CANT_SEND, ERROR_CANT_GET_ADDRESS };
	  	  
	  virtual Status connect( char* outgoingAddress, int outgoingPort, int incomingPort) = 0;
	  virtual Status disconnect( ) = 0;
	
	  virtual int sendPacket( char* packet, int length ) = 0;
	  virtual bool isPacketWaiting( ) = 0;
	  virtual int receivePacket( char* buffer, int length ) = 0;
};

#endif
