
#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include "config.h"
#ifdef MAKE_CTRL_NETWORK

#include "lwip/api.h"
#include "tcpserver.h"

class TcpSocket
{
public:
  TcpSocket( );
  ~TcpSocket( );
  bool valid( );
  
  int bytesAvailable( ) const;
  bool connect( int address, int port );
  bool close( );
  bool isConnected( );
  
  int write( const char* data, int length );
  int read( char* data, int length );
  int readNonBlock( char* data, int length );
  int readLine( char* data, int length );
  
private:
  struct netconn* _socket;
  friend class TcpServer;
};

#endif // MAKE_CTRL_NETWORK
#endif // TCP_SOCKET_H

