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

#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include "config.h"
#ifdef MAKE_CTRL_NETWORK

#include "lwip/api.h"
#include "tcpserver.h"

/**
  Read and write Ethernet data via TCP.
  TCP is a reliable way to transfer information via Ethernet.  
  
  Note that you don't need to destroy a socket each time you connect and disconnect with it.
  You can call connect() and close() in succession as many times as you like.
*/
class TcpSocket
{
public:
  TcpSocket( );
  TcpSocket( void* sock );
  ~TcpSocket( );
  bool valid( );
  
  int bytesAvailable( ) const;
  bool connect( int address, int port );
  bool close( );
  bool isConnected( );
  
  int write( const char* data, int length );
  int read( char* data, int length );
  int readLine( char* data, int length );
  
private:
  struct netconn* _socket;
  bool getNewSocket( );
};

#endif // MAKE_CTRL_NETWORK
#endif // TCP_SOCKET_H

