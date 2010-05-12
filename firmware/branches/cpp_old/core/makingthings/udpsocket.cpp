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
UdpSocket::UdpSocket( int port )
{
  _socket = netconn_new( NETCONN_UDP );
  if( _socket == NULL || _socket->err != ERR_OK )
    return;
  _socket->readingbuf = NULL;
  if(port >= 0)
    bind(port);
}

UdpSocket::~UdpSocket( )
{
  if( _socket )
  {
    netconn_close( _socket );
    netconn_delete(_socket);
    if ( _socket->readingbuf != NULL )
      netbuf_delete( _socket->readingbuf );
  }
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
bool UdpSocket::bind( int port )
{
  if( !_socket )
    return false;
  else
    return (ERR_OK == netconn_bind( _socket, IP_ADDR_ANY, port ));
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
int UdpSocket::write( const char* data, int length, int address, int port )
{
  if( !_socket )
    return 0;
  struct netbuf *buf;
  struct ip_addr remote_addr;
  int lengthsent = 0;

  remote_addr.addr = address;
  if( ERR_OK != netconn_connect(_socket, &remote_addr, port) )
    return lengthsent;

  // create a buffer
  buf = netbuf_new();
  if( buf != NULL )
  {
    netbuf_ref( buf, data, length); // make the buffer point to the data that should be sent
    if( ERR_OK == netconn_send( _socket, buf) ) // send the data
      lengthsent = length;
    netbuf_delete(buf); // deallocate the buffer
  }
  netconn_disconnect( _socket );

  return lengthsent;
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
int UdpSocket::read( char* data, int length, int* src_address, int* src_port )
{
  if( !_socket )
    return 0;

  int buflen = 0;
  struct netbuf* buf = netconn_recv( _socket );
  if( buf != NULL )
  {
    buflen = netbuf_len( buf );
    if( buflen > length) // make sure we only write as much as was asked for...the rest gets dropped
      buflen = length;
    // copy the contents of the received buffer into the supplied memory pointer
    netbuf_copy(buf, data, buflen);
    
    // if we got passed in valid pointers for addr and port, fill them up
    if( src_port )
      *src_port = netbuf_fromport(buf);
    if( src_address )
    {
      struct ip_addr *addr = netbuf_fromaddr(buf);
      *src_address = addr->addr;
    }
    netbuf_delete(buf);
  }

  return buflen;
}

#endif // MAKE_CTRL_NETWORK




