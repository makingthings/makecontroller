/*********************************************************************************

 Copyright 2006-2008 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#include "tcpserver.h"

#ifdef MAKE_CTRL_NETWORK

TcpServer::TcpServer( )
{
  getNewSocket();
}

TcpServer::~TcpServer( )
{
  close();
}

bool TcpServer::getNewSocket( )
{
  _socket = netconn_new( NETCONN_TCP );
  if( !_socket )
    return false;
  _socket->readingbuf = NULL;
  return true;
}

bool TcpServer::listen( int port )
{
  if( !_socket )
    return false;
  if( close() )
  {
    if( getNewSocket() )
    {
      if( ERR_OK == netconn_bind( _socket, 0, port ) )
      {
        if( ERR_OK == netconn_listen( _socket ) )
          return true;
      }
    }
  }
  return false;
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
  if ( _socket->readingbuf != NULL )
  {   
    netbuf_delete( _socket->readingbuf );
    _socket->readingbuf = NULL;
  }
  netconn_close( _socket );
  netconn_delete( _socket );
  _socket = 0;
  return true;
}

TcpSocket* TcpServer::accept( )
{
  void* s = netconn_accept( _socket );
  return ( s == NULL ) ? NULL : new TcpSocket( s );
}

#endif // MAKE_CTRL_NETWORK


