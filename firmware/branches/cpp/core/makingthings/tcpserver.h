

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "config.h"

#ifdef MAKE_CTRL_NETWORK

#include "lwip/api.h"
#include "tcpsocket.h"

class TcpSocket;

class TcpServer
{
public:
  TcpServer( );
  ~TcpServer( );
  bool valid( ) { return _socket != NULL; }
  
  bool listen( int port );
  bool isListening( );
  bool close( );
  bool accept( TcpSocket* next_connection );
  
private:
  struct netconn* _socket;
};

#endif //MAKE_CTRL_NETWORK
#endif // TCP_SERVER_H


