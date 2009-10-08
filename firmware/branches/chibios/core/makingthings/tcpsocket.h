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
  There are a couple typical ways you might get a TcpSocket - you might simply create
  one yourself, or you might receive one from a TcpServer listening for incoming connections.
  If you're creating one yourself, you can use the connect() method to connect to other
  listening sockets, then read() and write() as you like.  You can always check the number
  of bytes available to be read with bytesAvailable().
  
  \code
  // Making a connection ourselves
  TcpSocket socket;
  char buffer[512]; // buffer to read into
  if( socket.connect(IP_ADDRESS(192,168,0,100), 10100)) // connect to 
  {
    socket.write("hi", 2);
    int available = socket.bytesAvailable();
    if( available > 0) // is there anything to read?
    {
      if(available > 512) // only read as much as we have room for
        available = 512;
      socket.read(buffer, available);  // read everything available into our buffer
    }
    socket.close(); // always remember to close it if connect() was successful
  }
  \endcode
  
  Note that you don't need to destroy a socket each time you connect and disconnect with it.
  You can call connect() and close() in succession as many times as you like.
  
  If you're looking to make connections to web servers check the WebClient instead, which
  builds on top of TcpSocket.
  
  \ingroup networking
*/

typedef int TcpSocket;

TcpSocket tcpNew(void);
int tcpConnect(TcpSocket s, int address, int port);
bool tcpClose(TcpSocket s);
int tcpBytesAvailable(TcpSocket s);
int tcpRead(TcpSocket s, char* data, int length);
int tcpWrite(TcpSocket s, const char* data, int length);


#endif // MAKE_CTRL_NETWORK
#endif // TCP_SOCKET_H

