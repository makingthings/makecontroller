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

#include "udpsocket.h"
#ifdef MAKE_CTRL_NETWORK
#include "lwip/sockets.h"

/**
  Create a new UDP socket.
  When you create a new socket, you can optionally bind it directly to the port
  you want to listen on - the \b port argument is optional.  Otherwise, you don't
  need to pass in a port number, and the socket will not be bound to a port.
  @param port (optional) An integer specifying the port to open - anything less than 0 will
  prevent the socket from binding immediately.  Is -1 by default.
  
  \b Example
  \code
  // create a new socket without binding
  UdpSocket udp;
  
  // or create one that binds automatically
  UdpSocket udp(10000); // bind to port 10000
  \endcode
*/
int udpNew(void)
{
  return lwip_socket(0, SOCK_DGRAM, IPPROTO_UDP);
}

bool udpClose(int socket)
{
  return lwip_close(socket) == 0;
}

/**
  Bind to a port to listen for incoming data.
  You need to bind to a port before trying to read.  If you're only
  going to be writing, you don't need to bother binding.
  @param port An integer specifying the port to bind to.
  @return True on success, false on failure.
  
  \b Example
  \code
  UdpSocket udp; // create a new socket without binding
  udp.bind(10000); // then bind to port 10000
  // now we're ready to read
  \endcode
*/
bool udpBind(int socket, int port)
{
  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = INADDR_ANY;
  sa.sin_port = port;
  return lwip_bind(socket, (const struct sockaddr *)&sa, sizeof(sa)) == 0;
}

/**
  Send data.
  @param data The data to send.
  @param length The number of bytes to send.
  @param address The IP address to send to - use the IP_ADDRESS macro.
  @param port The port to send on.
  @return The number of bytes written.
  
  \b Example
  \code
  UdpSocket udp; // create a new socket without binding
  int address = IP_ADDRESS(192,168,0,210); // where to send
  int port = 10000; // which port to send on
  int written = udp.write("some data", strlen("some data"), address, port);
  \endcode
*/
int udpWrite( int socket, const char* data, int length, int address, int port )
{
  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = address;
  sa.sin_port = port;
  return lwip_sendto(socket, data, length, 0, (struct sockaddr*)&sa, sizeof(sa));
}

int udpSetBlocking(int socket, bool blocking)
{
  int b = blocking ? 1 : 0;
  return lwip_ioctl(socket, FIONBIO, &b);
}

/**
  Read data.
  Be sure to bind to a port before trying to read.  If you want to know which
  address the message came from, supply arguments for \b src_address and \b src_port,
  otherwise you can omit those parameters.
  
  @param data Where to store the incoming data.
  @param length How many bytes of data to read.
  @param src_address  (optional) An int that will be set to the address of the sender.
  @param src_port (optional) An int that will be set to the port of the sender.
  @return The number of bytes read.
  @see bind
  
  \b Example
  \code
  char mydata[128];
  UdpSocket udp(10000); // create a new socket, binding to port 10000
  int read = udp.read(mydata, 128);
  
  // or, if we wanted to check who sent the message
  int sender_address;
  int sender port;
  int read = udp.read(mydata, 128, &sender_address, &sender_port);
  \endcode
*/

int udpRead(int socket, char* data, int length)
{
  return lwip_recvfrom(socket, data, length, 0, NULL, NULL);
}

int udpReadFrom( int socket, char* data, int length, int* from_address, int* from_port )
{
  struct sockaddr_in from;
  socklen_t fromlen;
  int recvd = lwip_recvfrom(socket, data, length, 0, (struct sockaddr*)&from, &fromlen);
  if(from_address)
    *from_address = from.sin_addr.s_addr;
  if(from_port)
    *from_port = from.sin_port;
  return recvd;
}

int udpBytesAvailable(int socket)
{
  int bytes;
  return (lwip_ioctl(socket, FIONREAD, &bytes) == 0) ? bytes : -1;
}

#endif // MAKE_CTRL_NETWORK




