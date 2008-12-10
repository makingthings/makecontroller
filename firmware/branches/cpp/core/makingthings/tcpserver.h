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
  TcpSocket* accept( );
  
private:
  struct netconn* _socket;
};

#endif //MAKE_CTRL_NETWORK
#endif // TCP_SERVER_H


