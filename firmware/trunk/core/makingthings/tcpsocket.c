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

#include "config.h"
#include "lwipopts.h"
#if defined(MAKE_CTRL_NETWORK) && LWIP_TCP
#include "tcpsocket.h"
#include "lwip/sockets.h"

/**
  \defgroup tcpsocket TCP Socket
  Read and write Ethernet data via TCP.
  TCP is a reliable way to transfer information via Ethernet.

  \section Usage
  There are two main ways you might obtain a TCP socket:
   - you create one yourself
   - you receive one from a \ref tcpserver listening for incoming connections.

  If you're creating one yourself, you can use the tcpOpen() method to connect to a
  server socket.  If you get one from a \ref tcpserver, it's already open.  Then,
  tcpRead() and tcpWrite() as you like.  You can always check the number
  of bytes available to be read with tcpAvailable().

  \code
  // Making a connection ourselves
  int sock = tcpOpen(IP_ADDRESS(192, 168, 0, 100), 10100);
  char buffer[512]; // buffer to read into
  if (sock > -1) {
    tcpWrite(sock, "hi", 2);
    int available = tcpAvailable(sock);
    if (available > 0) {
      tcpRead(sock, buffer, available);
    }
    tcpClose(sock); // always remember to close it if tcpOpen() was successful
  }
  \endcode

  If you're looking to access the web check the \ref webclient instead, which
  provides web-specific behavior on top of TCP sockets.

  \ingroup networking
  @{
*/

/**
  Open a new TCP socket.
  Be sure to close any sockets you open - otherwise, you won't be able to open any new ones.
  @param address The IP address to connect to - use the IP_ADDRESS macro if needed.
  @param port The port to connect on.
  @return A handle to the new socket, or -1 if it failed to connect.
  
  \b Example
  \code
  int sock = tcpOpen(IP_ADDRESS(192, 168, 0, 210), 11101);
  if (sock > -1) {
    // then we got a good connection
    // ...reading & writing...
    tcpClose(sock); // make sure to close it if we connected
  }
  \endcode
*/
int tcpOpen(int address, int port)
{
  int sock = lwip_socket(0, SOCK_STREAM, IPPROTO_TCP);
  if (sock >= 0) {
    struct sockaddr_in to = {
      .sin_family = AF_INET,
      .sin_addr.s_addr = address,
      .sin_port = htons(port)
    };

    if (lwip_connect(sock, (const struct sockaddr*)&to, sizeof(to)) != 0) {
      lwip_close(sock);
      sock = -1;
    }
  }
  return sock;
}

/**
  Close a TCP socket.
  @param socket A handle to the socket to close.

  \b Example
  \code
  int sock = tcpOpen(IP_ADDRESS(192, 168, 0, 210), 11101);
  tcpClose(sock);
  \endcode
*/
void tcpClose(int socket)
{
  lwip_close(socket);
}

int tcpSetReadTimeout(int socket, int timeout)
{
  return lwip_setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

/**
  The number of bytes available to be read.
  @return The number of bytes ready to be read.
  @see tcpRead() for an example
*/
int tcpAvailable(int socket)
{
  int bytes;
  return (lwip_ioctl(socket, FIONREAD, &bytes) == 0) ? bytes : -1;
}

/**
  Send data.
  Make sure you're already successfully connected before trying to write.
  @param socket The socket to send on, as returned by tcpOpen()
  @param data The data to send.
  @param length The number of bytes to send.
  @return The number of bytes written.
  
  \b Example
  \code
  int sock = tcpOpen(IP_ADDRESS(192, 168, 0, 210), 10000);
  char mydata = "some of my data";
  if (sock > -1) {
    int written = tcpWrite(sock, mydata, strlen(mydata));
    tcpClose(sock);
  }
  \endcode
*/
int tcpWrite(int socket, const char* data, int length)
{
  return lwip_send(socket, data, length, 0);
}

/**
  Read data.
  Note - this is free to return the number of bytes available,
  which is not necessarily as much as you asked for.
  Use tcpAvailable() to see how many are ready to be read, or tcpSetReadTimeout()
  to change the amount of time to wait for data to become available.
  @param socket The socket to read on, as returned by tcpOpen().
  @param data Where to store the incoming data.
  @param length How many bytes of data to read.
  @return The number of bytes successfully read.
  
  \b Example
  \code
  char mydata[512];
  int sock = tcpOpen(IP_ADDRESS(192, 168, 0, 210), 10101);
  if (sock > -1) {
    int available = tcpAvailable(sock);
    if(available > 512) // make sure we don't read more than we have room for
      available = 512;
    int read = tcpRead(mydata, sizeof(mydata));
    // handle our new data here
    tcpClose(sock); // and finally close down
  }
  \endcode
*/
int tcpRead(int socket, char* data, int length)
{
  return lwip_recvfrom(socket, data, length, 0, NULL, NULL);
}

/**
  Read a single line from a TCP socket, as terminated by CR LF (0x0D 0x0A).
  Make sure you have an open socket before trying to read from it.  The line
  endings are not included in the data returned.
  @param socket The socket to read from.
  @param data Where to store the incoming data.
  @param length The maximum number of bytes to read.
  @return The number of bytes of data successfully read.
  @see tcpRead() for a similar example
*/
int tcpReadLine(int socket, char* data, int length)
{
  int readLength;
  int lineLength = -1;
  data--;
  
  do // Upon entering, data points to char prior to buffer, length is -1
  {
    data++; // here data points to where byte will be written
    lineLength++; // linelength now reflects true number of bytes
    readLength = tcpRead(socket, data, 1);
    // here, if readlength == 1, data has a new char in next position, linelength is one off,
    //       if readlength == 0, data had no new char and linelength is right
  } while ((readLength == 1) && (lineLength < length - 1) && (*data != '\n'));
  
  if (readLength == 1) // here, length is corrected if there was a character
    lineLength++;
  
  return lineLength;
}

/** @}
*/

#endif // MAKE_CTRL_NETWORK




