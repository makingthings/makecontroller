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

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "config.h"
#ifdef MAKE_CTRL_NETWORK

/**
  Listen for incoming TCP connections.
  
  \section Usage
  To get started using a TcpServer, create a new TcpServer object, put it in listen mode and
  start accepting requests.  The TcpServer will accept one client after another, blocking
  until a new client makes a connection.
  
  \code
  void myTask(void* p)
  {
    int server = tcpserverOpen(8080); // put it into listen mode on port 8080
    // now, wait for new connections, say hello to each of them, and close them
    while (1) {
      int client = tcpserverAccept(server); // this will wait for new connections to come in
      if (client > -1) { // make sure we got a good connection
        tcpWrite(client, "hello there", 11);
        tcpClose(client);
      }
    }
  }
  \endcode
  
  If you're looking to serve HTTP requests check the WebServer instead, which is built on
  the TcpServer.
  
  \ingroup networking
*/
#ifdef __cplusplus
extern "C" {
#endif
int  tcpserverOpen(int port);
int  tcpserverAccept(int server);
void tcpserverClose(int server);
#ifdef __cplusplus
}
#endif

#endif //MAKE_CTRL_NETWORK
#endif // TCP_SERVER_H


