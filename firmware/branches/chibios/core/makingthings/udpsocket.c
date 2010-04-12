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
#include "lwipopts.h"
#if defined(MAKE_CTRL_NETWORK) && LWIP_UDP
#include "lwip/sockets.h"

/**
  Create a new UDP socket.
  @return A handle to the new socket, or -1 if there was an error.
  @see udpWrite(), udpBind(), udpRead()
  
  \b Example
  \code
  // create a new socket
  int sock = udpOpen();
  if (sock > -1) {
    // then it was created successfully
  }
  \endcode
*/
int udpOpen(void)
{
  return lwip_socket(0, SOCK_DGRAM, IPPROTO_UDP);
}

void udpClose(int socket)
{
  lwip_close(socket);
}

/**
  Bind to a port to listen for incoming data.
  Before you can receive UDP data, you need to bind to a port.  If you're only
  going to be writing, you don't need to bother binding.
  @param socket The socket obtained from udpNew()
  @param port An integer specifying the port to bind to.
  @return True on success, false on failure.
  
  \b Example
  \code
  int sock = udpNew();  // create a new socket
  if (udpBind(sock, 10000) == true) { // then bind to port 10000
    // we're successfully bound, and ready to read
  }
  \endcode
*/
bool udpBind(int socket, int port)
{
  struct sockaddr_in sa = {
    .sin_family = AF_INET,
    .sin_addr.s_addr = INADDR_ANY,
    .sin_port = htons(port)
  };
  return lwip_bind(socket, (const struct sockaddr *)&sa, sizeof(sa)) == 0;
}

/**
  Send UDP data.
  @param socket The socket, obtained via udpNew()
  @param data The data to send.
  @param length The number of bytes to send.
  @param address The IP address to send to - use the IP_ADDRESS macro if necessary.
  @param port The port to send on.
  @return The number of bytes written.
  
  \b Example
  \code
  int sock = udpNew();  // create a new socket
  int address = IP_ADDRESS(192, 168, 0, 210); // where to send
  int port = 10000; // which port to send on
  int written = udpWrite(sock, "some data", strlen("some data"), address, port);
  \endcode
*/
int udpWrite(int socket, const char* data, int length, int address, int port)
{
  struct sockaddr_in sa = {
    .sin_family = AF_INET,
    .sin_addr.s_addr = address,
    .sin_port = htons(port)
  };
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
  address the message came from, see udpReadFrom().
  @param socket The UDP socket, as obtained from udpNew()
  @param data Where to store the incoming data.
  @param length How many bytes of data to read.
  @return The number of bytes read.
  @see udpBind()
  
  \b Example
  \code
  char mydata[128];
  int sock = udpNew();  // create a new socket
  if (udpBind(sock, 10000) == true) { // listen on port 10000
    int read = udpRead(sock, mydata, sizeof(mydata));
  }
  \endcode
*/

int udpRead(int socket, char* data, int length)
{
  return lwip_recvfrom(socket, data, length, 0, NULL, NULL);
}

int udpReadFrom(int socket, char* data, int length, int* from_address, int* from_port)
{
  struct sockaddr_in from;
  socklen_t fromlen;
  int recvd = lwip_recvfrom(socket, data, length, 0, (struct sockaddr*)&from, &fromlen);
  if (from_address)
    *from_address = from.sin_addr.s_addr;
  if (from_port)
    *from_port = from.sin_port;
  return recvd;
}

/*
  The number of bytes available for reading on this socket.
  @param socket The socket to check.
  @return The number of bytes available.

  \b Example
  \code
  int sock = udpNew();  // create a new socket
  if (udpBytesAvailable(sock) > 0) {
    // we have some reading to do...
  }
  \endcode
*/
int udpAvailable(int socket)
{
  int bytes;
  return (lwip_ioctl(socket, FIONREAD, &bytes) == 0) ? bytes : -1;
}

#endif // MAKE_CTRL_NETWORK




