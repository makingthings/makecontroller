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

#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include "config.h"
#ifdef MAKE_CTRL_NETWORK
#include "types.h"

/**
  Read and write Ethernet data via TCP.
  TCP is a reliable way to transfer information via Ethernet.
  
  \section Usage
  There are two main ways you might obtain a TCP socket - you might create
  one yourself, or you might receive one from a TCP server listening for incoming connections.
  If you're creating one yourself, you can use the connect() method to connect to other
  listening sockets, then read() and write() as you like.  You can always check the number
  of bytes available to be read with bytesAvailable().
  
  \code
  // Making a connection ourselves
  int sock = tcpNew();
  char buffer[512]; // buffer to read into
  if (tcpConnect(sock, IP_ADDRESS(192, 168, 0, 100), 10100) == true) {
    tcpWrite(sock, "hi", 2);
    int available = tcpBytesAvailable(sock);
    if (available > 0) {
      socket.read(buffer, available);
    }
    tcpClose(sock); // always remember to close it if connect() was successful
  }
  \endcode
  
  If you're looking to access the web check the \ref WebClient instead, which
  provides web-specific behavior on top of TCP sockets.
  
  \ingroup networking
*/

#ifdef __cplusplus
extern "C" {
#endif
int  tcpOpen(int address, int port);
bool tcpClose(int socket);
int  tcpAvailable(int socket);
int  tcpRead(int socket, char* data, int length);
int  tcpReadLine(int socket, char* data, int length);
int  tcpWrite(int socket, const char* data, int length);
int  tcpSetReadTimeout(int socket, int timeout);
#ifdef __cplusplus
}
#endif

#endif // MAKE_CTRL_NETWORK
#endif // TCP_SOCKET_H

