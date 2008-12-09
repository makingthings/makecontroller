
#include "udpsocket.h"
#ifdef MAKE_CTRL_NETWORK

UdpSocket::UdpSocket( )
{
  _socket = netconn_new( NETCONN_UDP );
  if( _socket == NULL || _socket->err != ERR_OK )
    return;
  _socket->readingbuf = NULL;
}

UdpSocket::~UdpSocket( )
{
  if( _socket )
  {
    netconn_delete(_socket);
    if ( _socket->readingbuf != NULL )
      netbuf_delete( _socket->readingbuf );
  }
}

bool UdpSocket::bind( int port )
{
  if( !_socket )
    return false;
  if( ERR_OK == netconn_bind( _socket, IP_ADDR_ANY, port ) )
    return true;
  else
    return false;
}

bool UdpSocket::isBound( )
{
  if( !_socket )
    return false;
  return ( _socket->state == NETCONN_CONNECT );
}

bool UdpSocket::close( )
{
  if( !_socket )
    return false;
  return (ERR_OK == netconn_close( _socket )) ? true : false;
}

int UdpSocket::write( int address, int port, const char* data, int length )
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

int UdpSocket::read( char* data, int length, int* src_address, int* src_port )
{
  if( !_socket )
    return 0;
  struct netbuf *buf;
  struct ip_addr *addr;
  int buflen = 0;

  if( _socket->state != NETCONN_CONNECT ) // make sure we're bound
    return 0;

  buf = netconn_recv( _socket );
  if( buf != NULL )
  {
    buflen = netbuf_len( buf );
    if( buflen > length) // make sure we only write as much as was asked for...the rest gets dropped
      buflen = length;
    // copy the contents of the received buffer into the supplied memory pointer
    netbuf_copy(buf, data, buflen);
    
    addr = netbuf_fromaddr(buf);
    if( src_port )
      *src_port = netbuf_fromport(buf);
    if( src_address )
      *src_address = addr->addr;
    netbuf_delete(buf);
  }

  return buflen;
}

#endif // MAKE_CTRL_NETWORK




