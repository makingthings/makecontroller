/*********************************************************************************

 Copyright 2006-2009 MakingThings

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

/**
  Create a new TcpServer.
  
  \b Example
  \code
  TcpServer server;
  // or allocate one...
  TcpServer* server = new TcpServer();
  \endcode
*/
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

/**
  Put this socket into listening mode.
  Once the socket is listening, you can wait for incoming connections
  using accept().
  
  If the socket is already listening, it will be closed and re-opened
  before listening on the specified port.
  @param port The port to listen for connections on
  @return True on success, false on failure.
  
  \b Example
  \code
  TcpServer s;
  s.listen(80); // listen on port 80, standard HTTP port
  \endcode
*/
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
  close();
  return false;
}

/**
  Read whether this socket is listening for connections.
  Start listening with a call to listen().
  @return True if it's listening, false if not.
  
  \b Example
  \code
  TcpServer s;
  s.isListening(); // false
  s.listen(80);
  s.isListening(); // true
  \endcode
*/
bool TcpServer::isListening( )
{
  if( !_socket )
    return false;
  return ( _socket->state == NETCONN_LISTEN );
}

/**
  Close this socket.
  
  @return True on success, false on failure.
  
  \b Example
  \code
  TcpServer server;
  server.listen(80);
  TcpSocket* newConnection = server.accept();
  server.close();
  \endcode
*/
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

/**
  Accept an incoming connection.
  This method will block until a new connection is made, and return the new TcpSocket
  that represents the remote connection.
  
  Note - you'll need to delete the TcpSocket returned once you're done with it.
  @return The newly connected socket, or NULL if it failed.
  
  \b Example
  \code
  TcpServer* s = new TcpServer();
  s->listen(8080);
  while(1)
  {
    TcpSocket* client = s->accept();
    // ...do something with the client connection here...
    delete client; // then clean it up
  }
  \endcode
*/
TcpSocket* TcpServer::accept( )
{
  void* s = netconn_accept( _socket );
  return ( s == NULL ) ? NULL : new TcpSocket( s );
}

#endif // MAKE_CTRL_NETWORK


