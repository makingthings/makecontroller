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

#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H

#include "config.h"
#ifdef MAKE_CTRL_NETWORK
#include "types.h"
#include "network.h"

/**
  Read and write Ethernet data via UDP.
  UDP is a lightweight network protocol that's great for sending lots of data
  at quick rates.  Unlike TcpSocket you're not always guaranteed that each and every message
  you send will ultimately reach its destination, but the ones that do will get there very quickly.
  
  \section Usage
  To get started with a UdpSocket, create a new UdpSocket object.  If you're only going to be
  writing, all you need is the write() method.  If you're reading, first call bind() to bind to
  a given port, and then read() from it as desired.
  
  \code
  UdpSocket sock; // create a new socket
  sock.write("hi there", strlen("hi there"), IP_ADDRESS(192,168,0,5), 10000); // can write immediately
  if(sock.bind(10000))
  {
    char buffer[128];
    int bytes_read = sock.read(buffer, 128); // this will wait for data to show up
    if(bytes_read) // did we read successfully?
    {
      // ...handle new data here...
    }
  }
  \endcode
  
  \ingroup networking
*/

int  udpNew(void);
bool udpClose(int socket);
bool udpBind(int socket, int port);
int  udpWrite(int socket, const char* data, int length, int address, int port);
int  udpRead(int socket, char* data, int length);
int  udpReadFrom(int socket, char* data, int length, int* src_address, int* src_port);
int  udpBytesAvailable(int socket);
int  udpSetBlocking(int socket, bool blocking);

#endif // MAKE_CTRL_NETWORK
#endif // UDP_SOCKET_H
