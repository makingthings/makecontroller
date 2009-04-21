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

/**
  Create a new TCP socket.
  
  \b Example
  \code
  // create a new socket
  TcpSocket tcp;
  // that's all there is to it!
  \endcode
*/
TcpSocket::TcpSocket( )
{
  getNewSocket();
  return;
}

TcpSocket::TcpSocket( void* sock )
{
  struct netconn* s = (struct netconn*)sock;
  _socket = s;
  _socket->readingbuf = s->readingbuf;
  return;
}

bool TcpSocket::getNewSocket( )
{
  _socket = netconn_new( NETCONN_TCP );
  if( !_socket )
    return false;
  _socket->readingbuf = NULL;
  return true;
}

/**
  Check whether a newly created socket is valid.
  Sometimes, a new socket isn't made correctly - if there's not enough memory,
  for example.
  @return True if the socket is valid, false if it's not.
  
  \b Example
  \code
  TcpSocket tcp; // create a new socket
  if(tcp.valid())
  {
    // then we're all good
  }
  \endcode
*/
bool TcpSocket::valid( )
{
  return _socket != NULL;
}

TcpSocket::~TcpSocket( )
{
  close();
}

/**
  Make a connection to a host.
  You'll need to make a connection to a host before you can read or write.  
  @param address The IP address to connect to - use the IP_ADDRESS macro.
  @param port The port to connect on.
  @return True if the connection was successful, false if not.
  
  \b Example
  \code
  TcpSocket tcp; // create a new socket
  if(tcp.connect(IP_ADDRESS(192,168,0,210), 11101))
  {
    // then we got a good connection
    // ...reading & writing...
    tcp.close(); // make sure to close it if we connected
  }
  \endcode
*/
bool TcpSocket::connect( int address, int port )
{
  if( !_socket )
  {
    if(!getNewSocket())
      return false;
  }
  struct ip_addr remote_addr;
  remote_addr.addr = address;
  err_t retval = netconn_connect( _socket, &remote_addr, port );
  if(ERR_OK == retval)
    return true;
  else
  {
    close();
    return false;
  }
}

/**
  Close a connected socket.
  If your attempt to connect() to a host is ever successful, be sure to close()
  the connection once you're done with it.
  @return True if the close operation was successful, false if not.
  @see connect() for an example
*/
bool TcpSocket::close( )
{
  if( !_socket )
    return false;
  if ( _socket->readingbuf != NULL )
  {   
    netbuf_delete( _socket->readingbuf );
    _socket->readingbuf = NULL;
  }
  netconn_close( _socket );
  netconn_delete( _socket );
  _socket = 0;
  return true;
}

/**
  Check whether a socket is currently connected.
  @return True if the close operation was successful, false if not.
  
  \b Example
  \code
  TcpSocket tcp;
  // ...
  if( tcp.isConnected())
  {
    // then we're connected
  }
  \endcode
*/
bool TcpSocket::isConnected( )
{
  if( !_socket )
    return false;
  return ( _socket->state == NETCONN_CONNECT || _socket->state == NETCONN_WRITE );
}

/**
  The number of bytes available to be read.
  @return The number of bytes ready to be read.
  @see read() for an example
*/
int TcpSocket::bytesAvailable( ) const
{
  if(!_socket)
    return 0;
  int len = _socket->recv_avail;
  if(_socket->readingbuf)
    len += (netbuf_len( _socket->readingbuf ) - _socket->readingoffset);
  return len;
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
  TcpSocket tcp;
  char mydata = "some of my data";
  if( tcp.connect( IP_ADDRESS(192, 168, 0, 210), 10000 ) )
  {
    int written = tcp.write(mydata, strlen(mydata));
    tcp.close();
  }
  \endcode
*/
int TcpSocket::write( const char* data, int length )
{
  if(!_socket)
    return 0;
  err_t err = netconn_write( _socket, data, length, NETCONN_COPY);
  return ( err != ERR_OK ) ? 0 : length;
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
  TcpSocket tcp;
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
int TcpSocket::read( char* data, int length )
{
  if(!_socket)
    return 0;
  struct netbuf *buf;
  int totalBytesRead = 0;
  int extraBytes = 0;
  
  while(totalBytesRead < length)
  {
    if ( _socket->readingbuf == NULL )
    {
      buf = netconn_recv( _socket );
      if ( buf == NULL )
        return 0;
      
      /* 
      Now deal appropriately with what we got.  If we got more than we asked for,
      keep the extras in the conn->readingbuf and set the conn->readingoffset.
      Otherwise, copy everything we got back into the calling buffer.
      */
      int bytesRead = netbuf_len( buf );
      totalBytesRead += bytesRead;
      if( totalBytesRead <= length ) 
      {
        netbuf_copy( buf, data, bytesRead );
        netbuf_delete( buf );
        data += bytesRead;
        // conn->readingbuf remains NULL
      }
      else // if we got more than we asked for
      {
        extraBytes = totalBytesRead - length;
        bytesRead -= extraBytes; // how many do we need to write to get the originally requested len
        netbuf_copy( buf, data, bytesRead );
        _socket->readingbuf = buf;
        _socket->readingoffset = bytesRead;
      }
    }
    else // conn->readingbuf != NULL
    {
      buf = _socket->readingbuf;
      int bytesRead = netbuf_len( buf ) - _socket->readingoffset; // grab whatever was lying around from a previous read
      totalBytesRead += bytesRead;
      
      if( totalBytesRead <= length ) // there's less than or just enough left for what we need
      {
        netbuf_copy_partial( buf, data, bytesRead, _socket->readingoffset ); // copy out the rest of what was in the netbuf
        netbuf_delete( buf ); // and get rid of it
        _socket->readingbuf = NULL;
      }  
      else // there's more in there than we were asked for
      {
        extraBytes = totalBytesRead - length;
        bytesRead -= extraBytes; // how many do we need to write to get the originally requested len
        netbuf_copy_partial( buf, data, bytesRead, _socket->readingoffset ); // only read out what we need
        //netbuf_copy( buf, data, bytesRead );
        _socket->readingoffset += bytesRead;
      }
      data += bytesRead;
    }
  }
  return totalBytesRead - extraBytes;
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
int TcpSocket::readLine( char* data, int length )
{
  int readLength;
  int lineLength = -1;
  data--;
  
  do // Upon entering, data points to char prior to buffer, length is -1
  {
    data++; // here data points to where byte will be written
    lineLength++; // linelength now reflects true number of bytes
    readLength = read( data, 1 );
    // here, if readlength == 1, data has a new char in next position, linelength is one off,
    //       if readlength == 0, data had no new char and linelength is right
  } while ( ( readLength == 1 ) && ( lineLength < length - 1 ) && ( *data != '\n' ) );
  
  if ( readLength == 1 ) // here, length is corrected if there was a character  
    lineLength++;
  
  return lineLength;
}

#endif // MAKE_CTRL_NETWORK




