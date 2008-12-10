

#include "tcpserver.h"

#ifdef MAKE_CTRL_NETWORK

TcpServer::TcpServer( )
{
  _socket = netconn_new( NETCONN_TCP );
}

TcpServer::~TcpServer( )
{
  if( _socket )
  {
    netconn_close( _socket );
    netconn_delete( _socket );
  }
}

bool TcpServer::listen( int port )
{
  if( !_socket )
    return false;
  
  if( ERR_OK != netconn_bind( _socket, 0, port ) )
    return false;
  if( netconn_listen( _socket ) )
    return false;
  return true;
}

bool TcpServer::isListening( )
{
  if( !_socket )
    return false;
  return ( _socket->state == NETCONN_LISTEN );
}

bool TcpServer::close( )
{
  if( !_socket )
    return false;
  return (ERR_OK == netconn_close( _socket )) ? true : false;
}

TcpSocket* TcpServer::accept( )
{
  void* s = netconn_accept( _socket );
  if( s == NULL )
    return NULL;
  else
    return new TcpSocket( s );
}

#endif // MAKE_CTRL_NETWORK


