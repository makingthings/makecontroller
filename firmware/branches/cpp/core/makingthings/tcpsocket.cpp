/*********************************************************************************

 Copyright 2006-2008 MakingThings

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

bool TcpSocket::valid( )
{
  return _socket != NULL;
}

TcpSocket::~TcpSocket( )
{
  close();
}

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
  return ( ERR_OK == retval ) ? true : false;
}

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

bool TcpSocket::isConnected( )
{
  if( !_socket )
    return false;
  return ( _socket->state == NETCONN_CONNECT || _socket->state == NETCONN_WRITE );
}

int TcpSocket::bytesAvailable( ) const
{
  if(!_socket)
    return 0;
  int len = _socket->recv_avail;
  if(_socket->readingbuf)
    len += (netbuf_len( _socket->readingbuf ) - _socket->readingoffset);
  return len;
}

int TcpSocket::write( const char* data, int length )
{
  if(!_socket)
    return 0;
  err_t err = netconn_write( _socket, data, length, NETCONN_COPY);
  return ( err != ERR_OK ) ? 0 : length;
}

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

int TcpSocket::readNonBlock( char* data, int length )
{
  (void)data;
  (void)length;
  return 0;
}

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




