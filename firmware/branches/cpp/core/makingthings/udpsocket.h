

#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H

#include "lwip/api.h"

class UdpSocket
{
public:
  UdpSocket( );
  ~UdpSocket( );
  bool valid( ) { return _socket != NULL; }
  
  bool bind( int port );
  bool isBound( );
  bool close( );
  
  int write( int address, int port, const char* data, int length );
  int read( char* data, int length, int* src_address = 0, int* src_port = 0 );
  
private:
  struct netconn* _socket;
};

#endif // UDP_SOCKET_H
