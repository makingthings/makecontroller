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

#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H

#include "config.h"
#ifdef MAKE_CTRL_NETWORK
#include "lwip/api.h"


/**
  Read and write Ethernet data via UDP.
  UDP is a lightweight network protocol that's great for streaming lots of data
  at quick rates.  Unlike \ref TcpSocket you're not always guaranteed that each and every message
  you send will ultimately reach its destination, but the ones that do will get there very quickly.
*/
class UdpSocket
{
public:
  
  UdpSocket( int port = -1 );
  ~UdpSocket( );
  bool valid( ) { return _socket != NULL; }
  
  bool bind( int port );
  int write( const char* data, int length, int address, int port );
  int read( char* data, int length, int* src_address = 0, int* src_port = 0 );
  
protected:
  struct netconn* _socket;
  bool close( );
};

#endif // MAKE_CTRL_NETWORK
#endif // UDP_SOCKET_H
