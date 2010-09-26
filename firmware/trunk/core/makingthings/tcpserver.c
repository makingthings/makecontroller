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
#include "lwipopts.h"
#if defined(MAKE_CTRL_NETWORK) && LWIP_TCP
#include "lwip/sockets.h"

/**
  \defgroup tcpserver TCP Server
  Listen for incoming TCP connections.

  \section Usage
  Create a new TCP server, specifying which port to listen on, and then wait for
  incoming connections.

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

  If you're looking to serve HTTP requests check the \ref webserver instead, which is built on
  the TCP server.

  \ingroup networking
  @{
*/

/**
  Create a new TcpServer.
  @param port The port to listen on.
  @return A handle to the server.  -1 indicates that it was not created successfully.

  \b Example
  \code
  int server = tcpserverOpen(80);
  if (server > -1) {
    // then it was created successfully.
  }
  \endcode
*/
int tcpserverOpen(int port)
{
  struct sockaddr_in sa = {
    .sin_family = AF_INET,
    .sin_addr.s_addr = INADDR_ANY,
    .sin_port = htons(port)
  };

  int s = lwip_socket(0, SOCK_STREAM, IPPROTO_TCP);
  if (lwip_bind(s, (const struct sockaddr *)&sa, sizeof(sa)) != 0) {
    lwip_close(s);
    s = -1;
  }
  else if (lwip_listen(s, 0) != 0) {
    lwip_close(s);
    s = -1;
  }
  return s;
}

/**
  Close this socket.
  @param server The handle to the server, as returned by tcpserverOpen();
  
  \b Example
  \code
  int server = tcpserverOpen(80);
  // ... do some work for a while
  tcpserverClose(server);
  \endcode
*/
void tcpserverClose(int server)
{
  lwip_close(server);
}

/**
  Accept an incoming connection.
  This method will wait until a new connection is made, and return a handle to
  the connecting \ref tcpsocket.
  
  Note - be sure to close the TCP socket returned once you're done with it.
  @return The newly connected socket, or -1 if it failed.
  
  \b Example
  \code
  int server = tcpserverOpen(80);
  while (1) {
    int client = tcpserverAccept(server);
    // ...do something with the client connection here...
    tcpClose(client); // then clean it up
  }
  \endcode
*/
int tcpserverAccept(int server)
{
  return lwip_accept(server, 0, 0);
}

/** @}
*/

#endif // MAKE_CTRL_NETWORK
