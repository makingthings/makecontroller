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

#include "tcpsocket.h"

#ifdef MAKE_CTRL_NETWORK
#include "lwip/sockets.h"

/**
  Create a new TCP socket.
  
  \b Example
  \code
  // create a new socket
  int tcp;
  // that's all there is to it!
  \endcode
*/
int tcpNew(void)
{
  return lwip_socket(0, SOCK_STREAM, IPPROTO_TCP);
}

bool tcpClose(int socket)
{
  return lwip_close(socket) == 0;
}

int tcpSetReadTimeout(int socket, int timeout)
{
  return lwip_setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

/**
  Make a connection to a host.
  You'll need to make a connection to a host before you can read or write.  
  @param address The IP address to connect to - use the IP_ADDRESS macro.
  @param port The port to connect on.
  @return True if the connection was successful, false if not.
  
  \b Example
  \code
  int tcp; // create a new socket
  if(tcp.connect(IP_ADDRESS(192,168,0,210), 11101))
  {
    // then we got a good connection
    // ...reading & writing...
    tcp.close(); // make sure to close it if we connected
  }
  \endcode
*/
int tcpConnect(int socket, int address, int port)
{
  struct sockaddr_in to;
  to.sin_family = AF_INET;
  to.sin_addr.s_addr = address;
  to.sin_port = port;
  return lwip_connect(socket, (const struct sockaddr*)&to, sizeof(to)) == 0;
}

/**
  The number of bytes available to be read.
  @return The number of bytes ready to be read.
  @see read() for an example
*/
int tcpBytesAvailable(int socket)
{
  int bytes;
  return (lwip_ioctl(socket, FIONREAD, &bytes) == 0) ? bytes : -1;
}

/**
  Send data.
  Make sure you're already successfully connected before trying to write.
  @param data The data to send.
  @param length The number of bytes to send.
  @return The number of bytes written.
  @see connect()
  
  \b Example
  \code
  int tcp;
  char mydata = "some of my data";
  if( tcp.connect( IP_ADDRESS(192, 168, 0, 210), 10000 ) )
  {
    int written = tcp.write(mydata, strlen(mydata));
    tcp.close();
  }
  \endcode
*/
int tcpWrite(int socket, const char* data, int length)
{
  int flags = 0;
  return lwip_send(socket, data, length, flags);
}

/**
  Read data.
  Note this won't return until it has received as many bytes as you asked for.
  Use bytesAvailable() to see how many are ready to be read.
  @param data Where to store the incoming data.
  @param length How many bytes of data to read.
  @return The number of bytes successfully read.
  
  \b Example
  \code
  int tcp;
  char mydata[512];
  if( tcp.connect( IP_ADDRESS(192,168,0,210), 10101 ) )
  {
    int available = tcp.bytesAvailable();
    if(available > 512) // make sure we don't read more than we have room for
      available = 512;
    int read = tcp.read(mydata, available);
    // handle our new data here
    tcp.close(); // and finally close down
  }
  \endcode
*/
int tcpRead(int socket, char* data, int length)
{
  int flags = 0;
  return lwip_recvfrom(socket, data, length, flags, NULL, NULL);
}

/**
  Read a single line from a TCP socket, as terminated by CR LF (0x0D 0x0A).
  Make sure you have an open socket before trying to read from it.  The line
  endings are not included in the data returned.
  @param data Where to store the incoming data.
  @param length How many bytes to read.
  @return The number of bytes of data successfully read.
  @see read() for a similar example
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
    readLength = tcpRead( socket, data, 1 );
    // here, if readlength == 1, data has a new char in next position, linelength is one off,
    //       if readlength == 0, data had no new char and linelength is right
  } while ( ( readLength == 1 ) && ( lineLength < length - 1 ) && ( *data != '\n' ) );
  
  if ( readLength == 1 ) // here, length is corrected if there was a character  
    lineLength++;
  
  return lineLength;
}

#endif // MAKE_CTRL_NETWORK




