

#include "tcpserver.h"

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

bool TcpServer::accept( TcpSocket* next_connection )
{
  next_connection->_socket = netconn_accept( _socket );
  if ( next_connection->_socket != NULL )
    next_connection->_socket->readingbuf = NULL;
  return true;
}


