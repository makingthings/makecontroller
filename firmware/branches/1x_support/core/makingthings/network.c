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

/** \file network.c	
	Communicate with the Make Controller Board via Ethernet.
*/

#include "config.h"
#ifdef MAKE_CTRL_NETWORK

#include "stdio.h"
#include "io.h"
#include "eeprom.h"
#include "timer.h"

/* lwIP includes. */
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/memp.h" 
#include "lwip/netbuf.h" 
#include "lwip/stats.h"
#include "netif/loopif.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"

/* Low level includes. */
#include "SAM7_EMAC.h"

int Network_AddressConvert( char* address, int* a0, int* a1, int* a2, int* a3 );

/* MAC address definition.  The MAC address must be unique on the network. */
char emacETHADDR0 = 0xAC;
char emacETHADDR1 = 0xDE;
char emacETHADDR2 = 0x48;
char emacETHADDR3 = 0x55;
char emacETHADDR4 = 0x0;
char emacETHADDR5 = 0x0;

#include "network.h"

typedef struct Network_
{
  int pending; // if somebody has started the process of getting an IP address, don't start another process
  int TempIpAddress; // hold onto the values that will ultimately get set for the network
  int TempGateway; // once it's all set as valid
  int TempMask;
  int OscUdpListenPort;
  int OscUdpSendPort;
  int TcpOutAddress;
  int TcpOutPort;
  bool TcpRequested;
  int DnsResolvedAddress;
  #ifdef OSC
  char scratch1[ OSC_SCRATCH_SIZE ];
  char scratch2[ OSC_SCRATCH_SIZE ];
  #endif // OSC
} NetworkStruct;

xSemaphoreHandle Network_DnsSemaphore;

// a few globals
NetworkStruct* Network;

enum { NET_UNCHECKED, NET_INVALID, NET_VALID } Network_Valid;

void Network_SetPending( int state );
int Network_GetPending( void );
void Network_DhcpStart( struct netif* netif );
void Network_DhcpStop( struct netif* netif );
void Network_SetDefaults( void );
static void Network_DnsCallback(const char *name, struct ip_addr *addr, void *arg);

/** \defgroup Sockets Sockets
  The Sockets system provides a simple interface for creating, reading and writing over both TCP and UDP.  
  This subsystem is a light wrapper around LwIP, the open source TCP/IP stack used by the Make Controller Kit.
	There are 3 groups of socket functions:
	- DatagramSocket - sockets for \b UDP communication
	- Socket - sockets for \b TCP communication
	- ServerSocket - sockets for accepting \b incoming TCP client connections
  \ingroup Core
  @{
*/

/**	
	Create a new TCP socket connected to the address and port specified.
	@param address An integer specifying the IP address to connect to.
	@param port An integer specifying the port to connect on.
	@return A pointer to the socket, if it was created successfully.  NULL if unsuccessful.
	@see SocketRead(), SocketWrite(), SocketClose()

  \par Example
  \code
  // use the IP_ADDRESS macro to format the address properly
  int addr = IP_ADDRESS( 192, 168, 0, 54 );
  // then create the socket, connecting on port 10101
  struct netconn* socket = Socket( addr, 10101 );
  \endcode
*/
void* Socket( int address, int port )
{
  Network_SetActive( 1 );

  struct netconn* conn;
  err_t retval;

  conn = netconn_new( NETCONN_TCP );
  // This is our addition to the conn structure to help with reading
  conn->readingbuf = NULL;

  struct ip_addr remote_addr;
  remote_addr.addr = htonl(address);

  retval = netconn_connect( conn, &remote_addr, port );
  
  if( ERR_OK != retval )
  {
    //netconn_delete( conn );
    while( netconn_delete( conn ) != ERR_OK )
      Sleep( 10 );

    conn = NULL;
  }
  
  return conn;
}

/**
  Get the number of bytes available in a TCP socket.
  Handy before calling SocketRead() so you know how many to read.
  @param socket The socket.
  @return The number of bytes available in that socket.

  \b Example
  \code
  struct netconn* mysocket = Socket(IP_ADDRESS( 192, 168, 0, 54 ), 10101);
  // ... reading and writing ...
  int avail = Socket_BytesAvailable(mysocket);
  \endcode
*/
int SocketBytesAvailable( void* socket )
{
  if(!socket)
    return 0;
  struct netconn *conn = socket;
  int len = conn->recv_avail;
  if(conn->readingbuf)
    len += (netbuf_len( conn->readingbuf ) - conn->readingoffset);
  return len;
}

/**	
	Read from a TCP socket.
  Make sure you have an open socket before trying to read from it.  This function
  will block until the requested number of bytes are read.  See Socket_BytesAvailable()
  to get the number of bytes waiting to be read.
	@param socket A pointer to the existing socket.
	@param data A pointer to the buffer to read to.
	@param length An integer specifying the maximum length in bytes that can be read.
	@return An integer: length of data read if successful, zero on failure.
	@see Socket(), SocketClose()

  \par Example
  \code
  // we should already have created a socket sock with Socket().
  struct netconn* sock = Socket( addr, 10101 );
  int length_read = SocketRead( sock, data, length )
  // if 0 bytes were read, there was some sort of error
  if( length_read == 0 )
    SocketClose( sock );
  \endcode
*/
int SocketRead( void* socket, char* data, int length )
{
  if(!socket)
    return 0;
  struct netconn *conn = socket;
  struct netbuf *buf;
  int totalBytesRead = 0;
  int extraBytes = 0;
  
  while(totalBytesRead < length)
  {
    if ( conn->readingbuf == NULL )
    {
      buf = netconn_recv( conn );
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
        conn->readingbuf = buf;
        conn->readingoffset = bytesRead;
      }
    }
    else // conn->readingbuf != NULL
    {
      buf = conn->readingbuf;
      int bytesRead = netbuf_len( buf ) - conn->readingoffset; // grab whatever was lying around from a previous read
      totalBytesRead += bytesRead;
      
      if( totalBytesRead <= length ) // there's less than or just enough left for what we need
      {
        netbuf_copy_partial( buf, data, bytesRead, conn->readingoffset ); // copy out the rest of what was in the netbuf
        netbuf_delete( buf ); // and get rid of it
        conn->readingbuf = NULL;
      }  
      else // there's more in there than we were asked for
      {
        extraBytes = totalBytesRead - length;
        bytesRead -= extraBytes; // how many do we need to write to get the originally requested len
        netbuf_copy_partial( buf, data, bytesRead, conn->readingoffset ); // only read out what we need
        //netbuf_copy( buf, data, bytesRead );
        conn->readingoffset += bytesRead;
      }
      data += bytesRead;
    }
  }
  return totalBytesRead - extraBytes;
}

/**	
  Read a line from a TCP socket terminated by CR LF (0x0D 0x0A).  
  Make sure you have an open socket before trying to read from it.
	@param socket A pointer to the existing socket.
	@param data A pointer to the buffer to read to.
	@param length An integer specifying the maximum length in bytes to read.
	@return An integer: length of data read if successful, zero on failure.
	@see Socket(), SocketRead(), SocketClose()
*/

int SocketReadLine( void* socket, char* data, int length )
{
  int readLength;
  int lineLength = -1;
  //int terminated = false;
  data--;

  // Upon entering, data points to char prior to buffer, length is -1
  do 
  {
    data++;
    // here data points to where byte will be written
    lineLength++;
    // linelength now reflects true number of bytes
    readLength = SocketRead( socket, data, 1 );
    // here, if readlength == 1, data has a new char in next position, linelength is one off,
    //       if readlength == 0, data had no new char and linelength is right
  } while ( ( readLength == 1 ) && ( lineLength < length - 1 ) && ( *data != '\n' ) );

  // here, length is corrected if there was a character  
  if ( readLength == 1 )
    lineLength++;
  
  return lineLength;
}


/**	
	Write to a TCP socket.
	Not surprisingly, we need an existing socket before we can write to it.

	@param socket A pointer to the existing socket.
	@param data A pointer to the buffer to write from.
	@param length An integer specifying the length in bytes of how much data should be written.
	@return An integer: 'length written' if successful, 0 on failure.
	@see Socket()

  \par Example
  \code
  // we should already have created a socket with Socket()
  struct netconn* sock = Socket( addr, 10101 );
  int length_written = SocketWrite( sock, data, length )
  // if 0 bytes were written, there was some sort of error
  if( length_written == 0 )
    SocketClose( sock );
  \endcode
*/
int SocketWrite( void* socket, char* data, int length )
{
  int err = netconn_write( (struct netconn *)socket, data, length, NETCONN_COPY);
  if ( err != ERR_OK )
    return 0;
  else
    return length;
}

/**	
	Close an existing TCP socket.
  Anytime you get an error when trying to read or write, it's best to close the socket and reopen
  it to make sure that the connection is corrently configured.
	@param socket A pointer to the existing socket.
	@return void
	@see Socket()

  \par Example
  \code
  // we should already have created a socket 'sock' with Socket().
  struct netconn* sock = Socket( addr, 10101 );
  // now close it
  SocketClose( sock )
  \endcode
*/
void SocketClose( void* socket )
{ 
  // Make sure there is no buffer lying around
  struct netconn *conn = socket;
  if ( conn->readingbuf != NULL )
  {   
    netbuf_delete( conn->readingbuf );
    conn->readingbuf = NULL;
  }

  netconn_close( (struct netconn *)socket );
  netconn_delete( (struct netconn *)socket );
  return;
}

/**	
	Create a new TCP server socket and start listening for connections.
	@param port An integer specifying the port to listen on.
	@return A pointer to the socket created.
	@see ServerSocketAccept( )
	
	\par Example
  \code
  // create a socket and start listening on port 10101
  struct netconn* server = ServerSocket( 10101 );
  // ServerSocketAccept( ) will block until an incoming connection is made
  struct netconn* newConnection = ServerSocketAccept( server );
  // now grab the data from the new connection
  \endcode
*/
void* ServerSocket( int port )
{
  Network_SetActive( 1 );

  struct netconn *conn;

  conn = netconn_new( NETCONN_TCP );
  if ( conn != NULL )
  { 
    netconn_bind( conn, 0, port );
    netconn_listen( conn );
  }

  return conn;
}

/**	
	Accept an incoming connection to a ServerSocket that you have created.  This
  function will block until a new connection is waiting to be serviced.  It returns
  a regular socket on which you can use SocketWrite(), SocketRead() and SocketClose().
	@param serverSocket a pointer to a ServerSocket that you created
	@return a pointer to the new socket created to handle the connection
	@see ServerSocket(), SocketWrite(), SocketRead(), SocketClose()
*/
void* ServerSocketAccept( void* serverSocket )
{
  struct netconn *conn;
  conn = netconn_accept( (struct netconn *)serverSocket );
  // This is our addition to the conn structure to help with reading
  if ( conn != NULL )
    conn->readingbuf = NULL;
  return conn;
}

/**	
	Close a ServerSocket that you have created.
	@param serverSocket A pointer to a ServerSocket.
	@return 0 if the process was successful.
	@see ServerSocket()
	
	\par Example
  \code
  // we created a server socket at some point
  struct netconn* server = ServerSocket( 10101 );
  // now close it
  ServerSocketClose( server );
  \endcode
*/
int ServerSocketClose( void* serverSocket )
{
  // netconn_close( serverSocket );
  netconn_delete( serverSocket );
  return 0;
}

/**	
	Create a socket to read and write UDP packets.
	@param port An integer specifying the port to open.
	@return a pointer to the socket created.
	@see DatagramSocketSend( ), DatagramSocketReceive( )
	
	\par Example
  \code
  // create a new UDP socket on port 10101
  struct netconn* udpSocket = DatagramSocket( 10101 );
  // now read and write to it using DatagramSocketSend( ) and DatagramSocketReceive( )
  \endcode
*/
void* DatagramSocket( int port )
{
  Network_SetActive( 1 );

  struct netconn *conn;

  conn = netconn_new( NETCONN_UDP );
  if( conn == NULL || conn->err != ERR_OK )
    return NULL;
  // This is our addition to the conn structure to help with reading
  conn->readingbuf = NULL;

  // hook it up
  netconn_bind( conn, NULL, port );

  return conn;
}

/**	
	Send a UDP packet to a specified address.
	@param datagramSocket A pointer to the DatagramSocket() you're using to write.
	@param address An integer specifying the IP address to write to.
	@param port An integer specifying the port to write to.
	@param data A pointer to the packet to send.
	@param length An integer specifying the number of bytes in the packet being sent.
	@return An integer corresponding to the number of bytes successfully written.
	@see DatagramSocket( )
	
	\par Example
  \code
  struct netconn* udpSocket = DatagramSocket( 10101 ); // our socket
  int address = IP_ADDRESS( 192, 168, 0, 200 );
  int sent = DatagramSocketSend( udpSocket, address, 10101, myBuffer, myLength );
  \endcode
*/
int DatagramSocketSend( void* datagramSocket, int address, int port, void* data, int length )
{ 
  struct netconn *conn = (struct netconn *)datagramSocket;
  struct netbuf *buf;
  struct ip_addr remote_addr;
  int lengthsent = 0;

  remote_addr.addr = htonl(address);
  if( ERR_OK != netconn_connect(conn, &remote_addr, port) )
		return lengthsent;

  // create a buffer
  buf = netbuf_new();
  if( buf != NULL )
  {
    netbuf_ref( buf, data, length); // make the buffer point to the data that should be sent
    if( ERR_OK != netconn_send( conn, buf) ) // send the data
		{
			netbuf_delete(buf);
			return 0;
		}
    lengthsent = length;
    netbuf_delete(buf); // deallocate the buffer
  }

  return lengthsent;
}

/**	
	Receive a UDP packet.  
  This function will block until a packet is received. The address and port of the 
  sender are returned in the locations pointed to by the address and port parameters.  
  If the incoming packet is larger than the specified size of the buffer, it will
  be truncated.
	@param datagramSocket A pointer to the DatagramSocket() you're using to read.
	@param incomingPort An integer specifying the port to listen on.
	@param address A pointer to the IP address that sent the packet.
	@param port A pointer to the port of the sender.
	@param data A pointer to the buffer to read into.
	@param length An integer specifying the number of bytes in the packet being read.
	@return An integer corresponding to the number of bytes successfully read.
	@see DatagramSocket( )
	
  \par Example
  \code
  struct netconn* udpSocket = DatagramSocket( 10101 ); // our socket
  int address, port;
  int sent = DatagramSocketReceive( udpSocket, &address, &port, myBuffer, myLength );
  \endcode
*/
int DatagramSocketReceive( void* datagramSocket, int incomingPort, int* address, int* port, void* data, int length )
{
  struct netconn *conn = (struct netconn*)datagramSocket;
  struct netbuf *buf;
  struct ip_addr *addr;
  int buflen = 0;

  if( ERR_OK != netconn_bind( conn, IP_ADDR_ANY, incomingPort ) )
		return buflen;
    
  buf = netconn_recv( conn );
  if( buf != NULL )
  {
    buflen = netbuf_len( buf );
    // copy the contents of the received buffer into
    //the supplied memory pointer 
    netbuf_copy(buf, data, length);
    addr = netbuf_fromaddr(buf);
    *port = netbuf_fromport(buf);
    *address = ntohl( addr->addr );
    netbuf_delete(buf);
  }

  /* if the length of the received data is larger than
  len, this data is discarded and we return len.
  otherwise we return the actual length of the received
  data */
  if(length > buflen)
    return buflen;
  else
    return length;
}

/**	
	Close a DatagramSocket().
	@param socket A pointer to the DatagramSocket() to close.
	@see DatagramSocket( )
	
  \par Example
  \code
  struct netconn* udpSocket = DatagramSocket( 10101 ); // create our socket
  // now close it
  DatagramSocketClose( udpSocket );
  \endcode
*/
void DatagramSocketClose( void* socket )
{
  netconn_close( socket );
  netconn_delete( socket );
}

/** @}
*/

/** \defgroup Network Network
  The Network subsystem manages the Ethernet controller.
	
  Like any other network enabled device, the Make Controller has an IP address, net mask and gateway.
	- The default IP address is 192.168.0.200
	- The default mask is 255.255.255.0
	- The default gateway 192.168.0.1
	
	You can set any of these values manually, or use DHCP to get them automatically.
	
	\section MAC
  The Make Controller's MAC address defaults to AC.DE.48.55.x.y where x & y are calculated from the 
  unit's serial number, handled by the \ref System subsystem.
	
	\section webserver Web Server
	The Make Controller Kit can also act as a web server.  The demo web server running on the Make Controller
	displays some stats about the board's current state through a web interface.  It is intended mainly as
	a starting point for more useful web applications.  See the source in webserver.c.

  \ingroup Core
  @{
*/

/**
	Sets whether the Network subsystem is active.  
	This fires up the networking system on the Make Controller, and will not return until a network is found,
	ie. a network cable is plugged in.
	@param state An integer specifying the active state - 1 (active) or 0 (inactive).
	@return 0 on success.
*/
int Network_SetActive( int state )
{
  if ( state ) 
  {
    if( Network == NULL )
    {
      Network = MallocWait( sizeof( NetworkStruct ), 100 );
      Network->pending = 1;
      Network->TcpRequested = 0;
      Network_DnsSemaphore = NULL;
      Network->DnsResolvedAddress = -1;


      if( !Network_GetValid() ) // if we don't have good values, set the defaults
        Network_SetDefaults( );
      else // load the values from EEPROM
      {
        Eeprom_Read( EEPROM_SYSTEM_NET_ADDRESS, (uchar*)&Network->TempIpAddress, 4 );
        Eeprom_Read( EEPROM_SYSTEM_NET_GATEWAY, (uchar*)&Network->TempGateway, 4 );
        Eeprom_Read( EEPROM_SYSTEM_NET_MASK, (uchar*)&Network->TempMask, 4 );
      }

      Network->OscUdpListenPort = NetworkOsc_GetUdpListenPort( );
      Network->OscUdpSendPort = NetworkOsc_GetUdpSendPort( );
      Eeprom_Read( EEPROM_TCP_OUT_ADDRESS, (uchar*)&Network->TcpOutAddress, 4 );
      Eeprom_Read( EEPROM_TCP_OUT_PORT, (uchar*)&Network->TcpOutPort, 4 );
      Network_Init();
    }
  }
  else 
	{
		if( Network )
		{
			Free( Network );
			Network = NULL;
		}
	}

  return CONTROLLER_OK;
}

/**
	Returns the active state of the Network subsystem.
	@return State - 1 (active) or 0 (inactive).
*/
int Network_GetActive( void )
{
  if( Network == NULL || Network_GetPending( ) )
  	return 0;
  else
  	return 1;
}

/**
	Set the IP address of the Make Controller.
	The IP address of the Make Controller, in dotted decimal form (xxx.xxx.xxx.xxx),
	can be set by passing in each of the numbers as a separate parameter.
	The default IP address of each Make Controller as it ships from the factory
	is 192.168.0.200.  

	This value is stored in EEPROM, so it persists even after the board
	is powered down.

	@param a0 An integer corresponding to the first of 4 numbers in the address.
	@param a1 An integer corresponding to the second of 4 numbers in the address.
	@param a2 An integer corresponding to the third of 4 numbers in the address.
	@param a3 An integer corresponding to the fourth of 4 numbers in the address.
	@return 0 on success.

  \par Example
  \code
  // set the address to 192.168.0.23
  Network_SetAddress( 192, 168, 0, 23 );
  \endcode

*/
int Network_SetAddress( int a0, int a1, int a2, int a3 )
{
	// just store this address, since we're only going to do something with it in response to
  if( !Network_GetValid() )
  {
    Network_SetDefaults( );
    Network_SetValid( 1 );
  }
  Network->TempIpAddress = NETIF_IP_ADDRESS( a0, a1, a2, a3 );
  Network_SetValid( 1 );

  return CONTROLLER_OK;
}

/**
	Set the network mask of the Make Controller on your local network.
	When on a subnet or local network, the network mask must be set in order
	for the gateway to route information to the board's IP address properly.
	The mask is commonly 255.255.255.0 for many home networks.
	Set the mask in dotted decimal form (xxx.xxx.xxx.xxx), passing in each 
	number as a separate parameter.

	This value is stored in EEPROM, so it persists even after the board
	is powered down.

	@param a0 An integer corresponding to the first of 4 numbers in the mask.
	@param a1 An integer corresponding to the second of 4 numbers in the mask.
	@param a2 An integer corresponding to the third of 4 numbers in the mask.
	@param a3 An integer corresponding to the fourth of 4 numbers in the mask.
	@return 0 on success.

  \par Example
  \code
  // set the mask to 255.255.255.254
  if( 0 != Network_SetMask( 255, 255, 255, 254 ) )
    // then there was a problem.
  \endcode
*/
int Network_SetMask( int a0, int a1, int a2, int a3 )
{
  // just store this address, since we're only going to do something with it in response to
  if( !Network_GetValid() )
  {
    Network_SetDefaults( );
    Network_SetValid( 1 );
  }
  Network->TempMask = NETIF_IP_ADDRESS( a0, a1, a2, a3 );
  Network_SetValid( 1 );

  return CONTROLLER_OK;
}

/**
	Set the gateway address for the local network the Make Controller is on.
	The gateway address is commonly 192.168.0.1 for many home networks.
	Set the gateway address in dotted decimal form (xxx.xxx.xxx.xxx), passing in each 
	number as a separate parameter.

	This value is stored in EEPROM, so it persists even after the board
	is powered down.

	@param a0 An integer corresponding to the first of 4 numbers in the gateway address.
	@param a1 An integer corresponding to the second of 4 numbers in the gateway address.
	@param a2 An integer corresponding to the third of 4 numbers in the gateway address.
	@param a3 An integer corresponding to the fourth of 4 numbers in the gateway address.
	@return 0 on success.

  \par Example
  \code
  // set the gateway to 192.168.5.1
  if( 0 != Network_SetGateway( 192, 168, 5, 1 ) )
    // then there was a problem.
  \endcode
*/
int Network_SetGateway( int a0, int a1, int a2, int a3 )
{
  // just store this address, since we're only going to do something with it in response to
  if( !Network_GetValid() )
  {
    Network_SetDefaults( );
    Network_SetValid( 1 );
  }
  Network->TempGateway = NETIF_IP_ADDRESS( a0, a1, a2, a3 );
  Network_SetValid( 1 );
  
  return CONTROLLER_OK;
}

/**
	Read the board's current IP address.
	Pass in pointers to integers where the address should be stored.

	@param a0 A pointer to an integer where the first of 4 numbers of the address is to be stored.
	@param a1 A pointer to an integer where the second of 4 numbers of the address is to be stored.
	@param a2 A pointer to an integer where the third of 4 numbers of the address is to be stored.
	@param a3 A pointer to an integer where the fourth of 4 numbers of the address is to be stored.
	@return 0 on success.
	
  \par Example
  \code
  int a0, a1, a2, a3;
  Network_GetAddress( &a0, &a1, &a2, &a3 );
  // now our variables are filled with the current address values
  \endcode
*/
int Network_GetAddress( int* a0, int* a1, int* a2, int* a3 )
{
  if( Network_GetPending() )
    return CONTROLLER_ERROR_NO_NETWORK;

  struct netif* mc_netif;
  int address = 0;
  // we specify our network interface as en0 when we init
  mc_netif = netif_find( "en0" );
  if( mc_netif != NULL )
  {
    address = mc_netif->ip_addr.addr;

    *a0 = NETIF_IP_ADDRESS_A( address );
    *a1 = NETIF_IP_ADDRESS_B( address );
    *a2 = NETIF_IP_ADDRESS_C( address );
    *a3 = NETIF_IP_ADDRESS_D( address );
  }
  // if the Ethernet interface is not up, we'll just get garbage back
  if( *a0 < 0 || *a0 > 255 )
    *a0 = *a1 = *a2 = *a3 = -1;

  return CONTROLLER_OK;
}

/**
	Read the board's current network mask.
	Pass in pointers to integers where the mask should be stored.

	@param a0 A pointer to an integer where the first of 4 numbers of the mask is to be stored.
	@param a1 A pointer to an integer where the second of 4 numbers of the mask is to be stored.
	@param a2 A pointer to an integer where the third of 4 numbers of the mask is to be stored.
	@param a3 A pointer to an integer where the fourth of 4 numbers of the mask is to be stored.
	@return 0 on success.
	
  \par Example
  \code
  int a0, a1, a2, a3;
  Network_GetMask( &a0, &a1, &a2, &a3 );
  // now our variables are filled with the current mask values
  \endcode
*/
int Network_GetMask( int* a0, int* a1, int* a2, int* a3 )
{
  if( Network_GetPending() )
    return CONTROLLER_ERROR_NO_NETWORK;
  
  struct netif* mc_netif;
  int address = 0;
  // we specify our network interface as en0 when we init
  mc_netif = netif_find( "en0" );
  if( mc_netif != NULL )
  {
    address = mc_netif->netmask.addr;

    *a0 = NETIF_IP_ADDRESS_A( address );
    *a1 = NETIF_IP_ADDRESS_B( address );
    *a2 = NETIF_IP_ADDRESS_C( address );
    *a3 = NETIF_IP_ADDRESS_D( address );
  }
  if( *a0 < 0 || *a0 > 255 )
    *a0 = *a1 = *a2 = *a3 = -1;
    
  return CONTROLLER_OK;
}

/**
	Read the board's current gateway address.
	Pass in pointers to integers where the gateway address should be stored.

	@param a0 A pointer to an integer where the first of 4 numbers of the gateway address is to be stored.
	@param a1 A pointer to an integer where the second of 4 numbers of the gateway address is to be stored.
	@param a2 A pointer to an integer where the third of 4 numbers of the gateway address is to be stored.
	@param a3 A pointer to an integer where the fourth of 4 numbers of the gateway address is to be stored.
	@return 0 on success.
	
  \par Example
  \code
  int a0, a1, a2, a3;
  Network_GetGateway( &a0, &a1, &a2, &a3 );
  // now our variables are filled with the current gateway values
  \endcode
*/
int Network_GetGateway( int* a0, int* a1, int* a2, int* a3 )
{
  if( Network_GetPending() )
    return CONTROLLER_ERROR_NO_NETWORK;
  
  struct netif* mc_netif;
  int address = 0;
  // we specify our network interface as en0 when we init
  mc_netif = netif_find( "en0" );
  if( mc_netif != NULL )
  {
    address = mc_netif->gw.addr;

    *a0 = NETIF_IP_ADDRESS_A( address );
    *a1 = NETIF_IP_ADDRESS_B( address );
    *a2 = NETIF_IP_ADDRESS_C( address );
    *a3 = NETIF_IP_ADDRESS_D( address );
  }
  if( *a0 < 0 || *a0 > 255 )
    *a0 = *a1 = *a2 = *a3 = -1;
  
  return CONTROLLER_OK;
}

/**
	Set whether DHCP is enabled.
  The Make Controller can use DHCP (Dynamic Host Configuration Protocol) to automatically
	retrieve an IP address from a router.  If you're using your Make Controller on a network
	with a router, it is generally preferred (and more convenient) to use DHCP.  Otherwise, turn
	DHCP off and set the IP address, mask, and gateway manually.
	
	Wikipedia has a good article about DHCP at http://en.wikipedia.org/wiki/Dynamic_Host_Configuration_Protocol
	
	This value is stored persistently, so it will remain constant across system reboots.
	@param enabled An integer specifying whether to enable DHCP - 1 (enable) or 0 (disable).
*/
void Network_SetDhcpEnabled( int enabled )
{
  if( enabled && !Network_GetDhcpEnabled() )
  {
    struct netif* mc_netif;
    // we specify our network interface as en0 when we init
    mc_netif = netif_find( "en0" );
    if( mc_netif != NULL )
      Network_DhcpStart( mc_netif );
      
    Eeprom_Write( EEPROM_DHCP_ENABLED, (uchar*)&enabled, 4 );
  }
  
  if( !enabled && Network_GetDhcpEnabled() )
  {
    struct netif* mc_netif;
    // we specify our network interface as en0 when we init
    mc_netif = netif_find( "en0" );
    if( mc_netif != NULL )
      Network_DhcpStop( mc_netif );
    Eeprom_Write( EEPROM_DHCP_ENABLED, (uchar*)&enabled, 4 );
    Network_SetValid( 1 );
  }
  return;
}

/**
	Read whether DHCP is currently enabled.
  This value is stored presistently, so it will be the same across system reboots.
	@return An integer specifying whether DHCP is enabled - 1 (enabled) or 0 (disabled).
*/
int Network_GetDhcpEnabled( )
{
  int state;
  Eeprom_Read( EEPROM_DHCP_ENABLED, (uchar*)&state, 4 );
  return (state == 1) ? 1 : 0;
}

/**
  Resolve the IP address for a given host name.
  Up to 4 DNS entries are cached, so if you make successive calls to this function, 
  you won't incur a whole lookup roundtrip - you'll just get the cached value.
  The cached values are maintained internally, so if one of them becomes invalid, a
  new lookup will be fired off the next time it's asked for.
  @param name A string specifying the name of the host to look up.
  @return An integer representation of the IP address of the host.  This can be 
  passed to the \ref Sockets functions to read and write.  Returns -1 on error.

  \b Example
  \code
  // try to open a socket connection to makingthings.com
  int addr = Network_DnsGetHostByName("makingthings.com");
  struct netconn* socket = Socket(addr, 80); // open up a new connection to that address
  \endcode
*/
int Network_DnsGetHostByName( const char *name )
{
  struct ip_addr addr;
  int retval = -1;
  if(!Network_DnsSemaphore)
  {
    Network_DnsSemaphore = SemaphoreCreate();
    if(!Network_DnsSemaphore) // the semaphore was not created successfully
      return retval;
    if(!SemaphoreTake(Network_DnsSemaphore, 0)) // do the initial take
      return retval;
  }
  err_t result = dns_gethostbyname( name, &addr, Network_DnsCallback, 0);
  if(result == ERR_OK) // the result was cached, just return it
    retval = addr.addr;
  else if(result == ERR_INPROGRESS) // a lookup is in progress - wait for the callback to signal that we've gotten a response
  {
    if(SemaphoreTake(Network_DnsSemaphore, 30000)) // timeout is 30 seconds by default
      retval = Network->DnsResolvedAddress;
  }
  return ntohl(retval);
}

// static
/*
  The callback for a DNS look up.  The original request is waiting (via semaphore) on
  this to pop the looked up address in the right spot.
*/
void Network_DnsCallback(const char *name, struct ip_addr *addr, void *arg)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(name);
  if(addr)
    Network->DnsResolvedAddress = addr->addr;
  else
    Network->DnsResolvedAddress = -1; // we didn't get an address, stuff an error value in there
  SemaphoreGive(Network_DnsSemaphore);
}

/** @}
*/

/**
	Create a checksum for the current address settings and store it in EEPROM.
	This should be called each time an address setting is changed so that if
	the board gets powered down, it will know when it comes back up whether or
	not the address settings is currently has are valid.

	@param v An integer specifying whether to validate the current settings (1)
  or to force them to be invalidated (0).
	Passing in 0 returns the address settings to their factory defaults.
	@return 0 on success.
*/
int Network_SetValid( int v )
{
  if ( v )
  {
    struct ip_addr ip, gw, mask;
    struct netif* mc_netif;

    ip.addr = Network->TempIpAddress; // these should each have been set previously
    mask.addr = Network->TempMask;  // by Network_SetAddress(), etc.
    gw.addr = Network->TempGateway;
		if( !Network_GetDhcpEnabled() ) // only actually change the address if we're not using DHCP
		{
			// we specify our network interface as en0 when we init
			mc_netif = netif_find( "en0" );
			if( mc_netif != NULL )
				netif_set_addr( mc_netif, &ip, &mask, &gw );
		}
    
		// but write the addresses to memory regardless, so we can use them next time we boot up without DHCP
    Eeprom_Write( EEPROM_SYSTEM_NET_ADDRESS, (uchar*)&ip.addr, 4 );
    Eeprom_Write( EEPROM_SYSTEM_NET_MASK, (uchar*)&mask.addr, 4 );
    Eeprom_Write( EEPROM_SYSTEM_NET_GATEWAY, (uchar*)&gw.addr, 4 );

    int total = Network->TempIpAddress + Network->TempMask + Network->TempGateway;
    Eeprom_Write( EEPROM_SYSTEM_NET_CHECK, (uchar*)&total, 4 );

    Network_Valid = NET_VALID;
  }
  else
  {
    int value = 0;
    Eeprom_Write( EEPROM_SYSTEM_NET_CHECK, (uchar*)&value, 4 );
    Network_Valid = NET_INVALID;
  }

  return CONTROLLER_OK;
}

/**
	Read the checksum for address settings in EEPROM, and determine if it matches 
  the current settings.

	@return An integer specifying the validity of the settings - 1 (valid) or 0 (invalid).
*/
int Network_GetValid( )
{
  int address, mask, gateway, total;
  Eeprom_Read( EEPROM_SYSTEM_NET_ADDRESS, (uchar*)&address, 4 );
  Eeprom_Read( EEPROM_SYSTEM_NET_MASK, (uchar*)&mask, 4 );
  Eeprom_Read( EEPROM_SYSTEM_NET_GATEWAY, (uchar*)&gateway, 4 );
  Eeprom_Read( EEPROM_SYSTEM_NET_CHECK, (uchar*)&total, 4 );

  if ( total == address + mask + gateway )
  {
    Network_Valid = NET_VALID;
    return 1;
  }
  else
  {
    Network_Valid = NET_INVALID;
    return 0;
  }
}

// if things aren't valid, just set the defaults
void Network_SetDefaults( )
{
  Network->TempIpAddress = NETIF_IP_ADDRESS( 192, 168, 0, 200 );
  Network->TempGateway = NETIF_IP_ADDRESS( 192, 168, 0, 1 );
  Network->TempMask = NETIF_IP_ADDRESS( 255, 255, 255, 0 );
  Network_SetValid( 1 );
}

/**
	Sets whether the Network subsystem is currently trying to negotiate its settings.
  This is mostly used by the Network init and deinit processes, so you shouldn't
  be calling this yourself unless you have a good reason to.
	@param state An integer specifying the pending state - 1 (pending) or 0 (not pending).
*/
void Network_SetPending( int state )
{
  if( state && !Network->pending )
      Network->pending = state;
  if( !state && Network->pending )
    Network->pending = state;
}

int Network_GetPending( )
{
  return Network->pending;
}

void NetworkOsc_SetTcpOutAddress( int a0, int a1, int a2, int a3 )
{
	int address = IP_ADDRESS( a0, a1, a2, a3 );
  if( address != Network->TcpOutAddress )
  {
    Network->TcpOutAddress = address;
    Eeprom_Write( EEPROM_TCP_OUT_ADDRESS, (uchar*)&address, 4 );
  }
}

int NetworkOsc_GetTcpOutAddress( )
{
  return Network->TcpOutAddress;
}

void NetworkOsc_SetTcpOutPort( int port )
{
  if( port != Network->TcpOutPort ) // only change if it's a new value
  {
    Network->TcpOutPort = port;
    Eeprom_Write( EEPROM_TCP_OUT_PORT, (uchar*)&port, 4 );
  }
}

int NetworkOsc_GetTcpOutPort( )
{
  return Network->TcpOutPort;
}

void NetworkOsc_SetUdpListenPort( int port )
{
  if( port != Network->OscUdpListenPort ) // only change things if it's a new value
  {
    Network->OscUdpListenPort = port;
    Eeprom_Write( EEPROM_OSC_UDP_LISTEN_PORT, (uchar*)&port, 4 );
  }
}

int NetworkOsc_GetUdpListenPort(  )
{
  int port;
  Eeprom_Read( EEPROM_OSC_UDP_LISTEN_PORT, (uchar*)&port, 4 );
  
  if( port > 0 && port < 65536 )
    return port;
  else
    return 10000;
}

void NetworkOsc_SetUdpSendPort( int port )
{
  if( port != Network->OscUdpSendPort ) // only change things if it's a new value
  {
    Network->OscUdpSendPort = port;
    Eeprom_Write( EEPROM_OSC_UDP_SEND_PORT, (uchar*)&port, 4 );
  }
}

int NetworkOsc_GetUdpSendPort(  )
{
  int port;
  Eeprom_Read( EEPROM_OSC_UDP_SEND_PORT, (uchar*)&port, 4 );
  
  if( port > 0 && port < 65536 )
    return port;
  else
    return 10000;
}

void Network_DhcpStart( struct netif* netif )
{
  Network_SetPending( 1 ); // set a flag so nobody else tries to set up this netif
  int count = 0;
  dhcp_start( netif );
  // now hang out for a second until we get an address
  // if DHCP is enabled but we don't find a DHCP server, just use the network config stored in EEPROM
  while( netif->ip_addr.addr == 0 && count < 100 ) // timeout after 10 (?) seconds of waiting for a DHCP address
  {
    count++;
    Sleep( 100 );
  }
  if( netif->ip_addr.addr == 0 ) // if we timed out getting an address via DHCP, just use whatever's in EEPROM
  {
    struct ip_addr ip, gw, mask; // network config stored in EEPROM
    ip.addr = Network->TempIpAddress;
    mask.addr = Network->TempMask;
    gw.addr = Network->TempGateway;
    netif_set_addr( netif, &ip, &mask, &gw );
  }
  Network_SetPending( 0 );
  return;
}

void Network_DhcpStop( struct netif* netif )
{
  dhcp_release( netif );
  netif_set_up(netif); // bring the interface back up, as dhcp_release() takes it down
  return;
}

int Network_AddressConvert( char* address, int* a0, int* a1, int* a2, int* a3 )
{
  return ( sscanf( address, "%d.%d.%d.%d", a0, a1, a2, a3 ) == 4 ) ? CONTROLLER_OK : CONTROLLER_ERROR_NO_ADDRESS;
}

#if ( CONTROLLER_VERSION == 50 || CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 || CONTROLLER_VERSION == 200 )
#define NETWORK_BITS IO_PB00_BIT | IO_PB01_BIT | IO_PB02_BIT | IO_PB03_BIT | IO_PB04_BIT | \
                     IO_PB05_BIT | IO_PB06_BIT | IO_PB07_BIT | IO_PB08_BIT | IO_PB09_BIT | \
                     IO_PB10_BIT | IO_PB11_BIT | IO_PB12_BIT | IO_PB13_BIT | IO_PB14_BIT | \
                     IO_PB15_BIT | IO_PB16_BIT | IO_PB17_BIT | IO_PB18_BIT | IO_PB26_BIT
#elif ( CONTROLLER_VERSION == 90 )
#define NETWORK_BITS IO_PB00_BIT | IO_PB01_BIT | IO_PB02_BIT | IO_PB03_BIT | IO_PB04_BIT | \
                     IO_PB05_BIT | IO_PB06_BIT | IO_PB07_BIT | IO_PB08_BIT | IO_PB09_BIT | \
                     IO_PB15_BIT | IO_PB26_BIT
#endif

int Network_Init( )
{
  // Start and Lock all the bits to do with the Ethernet Phy - can do this immediately, since there's no undoing this
  Network_SetPending( 1 );
  Io_StartBits( NETWORK_BITS, true );

  // Attempt to get a serial number and set it into the mac address low bytes to make a unique MAC address
  int serialNumber = System_GetSerialNumber();
  emacETHADDR5 = serialNumber & 0xFF;
  emacETHADDR4 = ( serialNumber >> 8 ) & 0xFF;
  // Low nibble of the third byte - gives us around 1M serial numbers
  emacETHADDR3 = 0x50 | ( ( serialNumber >> 12 ) & 0xF );

  /* Initialize lwIP and its interface layer. */
  tcpip_init( NULL, NULL ); // init all of lwip...see lwip_init() inside for the whole init story

  extern err_t ethernetif_init( struct netif *netif );
  static struct netif EMAC_if;
  int address, mask, gateway, dhcp;
  dhcp = Network_GetDhcpEnabled();

  if( dhcp )
  {
    address = 0;
    mask = 0;
    gateway = 0;
  }
  else // DHCP not enabled, just read whatever the manual IP address in EEPROM is.
  {
    address = Network->TempIpAddress;
    mask = Network->TempMask;
    gateway = Network->TempGateway;
  }
  // add our network interface to the system
  Network_SetPending( 1 ); //netif_add goes away for a long time if there's no Ethernet cable connected.
  netif_add(&EMAC_if, (struct ip_addr*)&address, (struct ip_addr*)&mask, 
                        (struct ip_addr*)&gateway, NULL, ethernetif_init, tcpip_input);
	// make it the default interface
  netif_set_default(&EMAC_if);
	// bring it up
  netif_set_up(&EMAC_if);
  // name it so we can find it later
  EMAC_if.name[0] = 'e';
  EMAC_if.name[1] = 'n';
  EMAC_if.num = 0;
  
  #ifdef OSC
  if( NetworkOsc_GetTcpAutoConnect( ) )
  {
    Network->TcpRequested = 1;
    Osc_StartTcpTask( );
  }
  #endif
  
  if( dhcp )
    Network_DhcpStart( &EMAC_if );

  Network_SetPending( 0 );
  return CONTROLLER_OK;
}

/** \defgroup NetworkOSC Network - OSC
  Configure the Controller Board's Network Settings via OSC.
  \ingroup OSC
   
    \section devices Devices
    There is only one Network system, so a device index is not used.
   
    \section properties Properties
    The Network system has the following properties
    - address
    - mask
    - gateway
    - valid
    - mac
    - osc_udp_listen_port
    - osc_udp_send_port
    - dhcp

    \par Address
    The \b address property corresponds to the IP address of the Controller Board.
    This value can be both read and written.  To set a new address, send a message like
    \verbatim /network/address 192.168.0.235 \endverbatim
    \par
    To read the current IP address, omit the argument value from the end of the message:
    \verbatim /network/address \endverbatim
   
    \par Mask
    The \b mask property corresponds to the network mask of the Controller Board.
    When on a subnet or local network, the network mask must be set in order
    for the gateway to route information to the board's IP address properly.
    The mask is commonly 255.255.255.0 for many home networks.
    \par
    To set the board's network mask, send a message like
    \verbatim /network/mask 255.255.255.0 \endverbatim
    To read the current mask, omit the argument value from the end of the message:
    \verbatim /network/mask \endverbatim
   
    \par Gateway
    The \b gateway property corresponds to the gateway address for the local network the Make Controller is on.
    The gateway address is the address
    The gateway address is commonly the address of the router on many home networks, and its
    value is commonly 192.168.0.1.\n
    This value is stored in EEPROM, so it persists even after the board
    is powered down.
    \par
    To set the board's gateway address, send a message like
    \verbatim /network/gateway 192.168.0.1 \endverbatim
    To read the current gateway, omit the argument value from the end of the message:
    \verbatim /network/gateway \endverbatim

    \par Valid
    The \b valid property is used to make sure the board's network settings are valid.
    If you're manually setting the \b address, \b gateway, or \b mask settings, you'll need
    to send the valid message for them to take effect.
    \par
    To set the board's current network settings as valid, send the message
    \verbatim /network/valid 1 \endverbatim
    To check if the current settings have been set as valid, send the message:
    \verbatim /network/valid \endverbatim
    with no argument value.

    \par OSC UDP Port
    The \b osc_udp_port property corresponds to the port that the Make Controller listens on for
    incoming OSC messages via UDP.  This value is stored persistently, so it's available
    even after the board has rebooted.  This is 10000 by default.
    \par
    To tell the board to listen on port 10001, instead of the default 10000, send the message
    \code /network/osc_udp_port 10001 \endcode
    To read back the port that the board is currently listening on, send the message
    \verbatim /network/osc_udp_port \endverbatim
    with no argument value.

    \par DHCP
    The \b dhcp property sets whether the board should try to dynamically retrieve a network address from 
    a network router.  If no DHCP server is available, the board will use the network settings stored in memory
    for the \b address, \b gateway, and \b mask properties. If you're connecting the board directly to your computer, 
		DHCP will not be available, so you should turn this off.
    \par
    To turn DHCP on, send the message
    \code /network/dhcp 1 \endcode
    and the board will immediately try to get an address.  To check what address the board got, send the message
    \code /network/address \endcode
    as you normally would.  To turn DHCP off, send the message
    \code /network/dhcp 0 \endcode
    To read whether DHCP is currently enabled on the board, send the message
    \code /network/dhcp \endcode
    with no argument value.
   
    \par MAC
    The \b mac property corresponds to the Ethernet MAC address of the Controller Board.
    This value is read-only.  Each board on a network must have a unique MAC address.  The MAC address is generated
		using the board's serial number, so ensure that your board's serial numbers are unique.
    \par
    To read the MAC address of the Controller, send the message
    \verbatim /network/mac \endverbatim
    The board will respond by sending back an OSC message with the MAC address.
*/
#ifdef OSC

void NetworkOsc_SetTcpAutoConnect( int yesorno )
{
  int val = yesorno;
  if( val != 1 )
    val = 0;
  if( yesorno != NetworkOsc_GetTcpAutoConnect( ) ) // only write it if it has changed
    Eeprom_Write( EEPROM_TCP_AUTOCONNECT, (uchar*)&yesorno, 4 );
  if( Network->TcpRequested == 0 && yesorno ) // if we're not already connected, connect now
  {
    Network->TcpRequested = 1;
    Osc_StartTcpTask( );
  }
}

int NetworkOsc_GetTcpAutoConnect( )
{
  int state;
  Eeprom_Read( EEPROM_TCP_AUTOCONNECT, (uchar*)&state, 4 );
  if( state != 1 )
    return 0;
  else
    return state;
}

int NetworkOsc_GetTcpRequested( )
{
  return Network->TcpRequested;
}


#include "osc.h"
static char* NetworkOsc_Name = "network";
static char* NetworkOsc_PropertyNames[] = { "active", "address", "mask", "gateway", "valid", "mac", 
                                              "osc_udp_listen_port", "osc_tcpout_address", "osc_tcpout_port", 
                                              "osc_tcpout_connect", "osc_tcpout_auto", "dhcp", 
                                              "find", "osc_udp_send_port", 0 }; // must have a trailing 0

int NetworkOsc_PropertySet( int property, char* typedata, int channel );
int NetworkOsc_PropertyGet( int property, int channel );

const char* NetworkOsc_GetName( void )
{
  return NetworkOsc_Name;
}

int NetworkOsc_ReceiveMessage( int channel, char* message, int length )
{
  if ( Network_GetPending() )
    return CONTROLLER_ERROR_NO_NETWORK;
  
  int status = Osc_GeneralReceiverHelper( channel, message, length, 
                                NetworkOsc_Name,
                                NetworkOsc_PropertySet, NetworkOsc_PropertyGet, 
                                NetworkOsc_PropertyNames );

  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, NetworkOsc_Name, status );

  return CONTROLLER_OK;
}

int NetworkOsc_Poll( )
{
  return CONTROLLER_OK;
}

// Sets the property with the value
int NetworkOsc_PropertySet( int property, char* typedata, int channel )
{
  int a0;
  int a1; 
  int a2;
  int a3;

  switch ( property )
  {
    case 0: // active
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need an int" );

      Network_SetActive( value );
      break;
    }
    case 1: // address
    {
      char* address;
      int count = Osc_ExtractData( typedata, "s", &address );
      if ( count != 1 ) 
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need a string" );
      int status = Network_AddressConvert( address, &a0, &a1, &a2, &a3 );
      if ( status != CONTROLLER_OK )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect address - need 'a.b.c.d'" );

      Network_SetAddress( a0, a1, a2, a3 );

      break;
    }
    case 2: // mask
    {
      char* address;
      int count = Osc_ExtractData( typedata, "s", &address );
      if ( count != 1 ) 
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need a string" );
      int status = Network_AddressConvert( address, &a0, &a1, &a2, &a3 );
      if ( status != CONTROLLER_OK )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect mask - need 'a.b.c.d'" );

      Network_SetMask( a0, a1, a2, a3 );

      break;
    }
    case 3: // gateway 
    {
      char* address;
      int count = Osc_ExtractData( typedata, "s", &address );
      if ( count != 1 ) 
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need a string" );
      int status = Network_AddressConvert( address, &a0, &a1, &a2, &a3 );
      if ( status != CONTROLLER_OK )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect gateway - need 'a.b.c.d'" );

      Network_SetGateway( a0, a1, a2, a3 );

      break;
    }
    case 4: // valid
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need an int" );

      Network_SetValid( value ); 
      break;
    }
    case 5: // mac
    {
      return Osc_SubsystemError( channel, NetworkOsc_Name, "MAC is read only." );
    }
    case 6: // osc_udp_listen_port
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need an int" );

      NetworkOsc_SetUdpListenPort( value );
      break;
    }
    case 7: // osc_tcpout_address
    {
      char* address;
      int count = Osc_ExtractData( typedata, "s", &address );
      if ( count != 1 ) 
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need a string" );
      // don't strictly need to do this, but it's god to make sure it's a proper address
      int status = Network_AddressConvert( address, &a0, &a1, &a2, &a3 );
      if ( status != CONTROLLER_OK )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect TCP address - need 'xxx.xxx.xxx.xxx'" );
      
      NetworkOsc_SetTcpOutAddress( a0, a1, a2, a3 );
      break;
    }
    case 8: // osc_tcpout_port
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need an int" );

      NetworkOsc_SetTcpOutPort( value );
      break;
    }
    case 9: // osc_tcpout_connect
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need an int" );

      if( value && Network->TcpRequested == 0 )
      {
        Network->TcpRequested = 1;
        Osc_StartTcpTask( );
      }
      if( !value && Network->TcpRequested == 1 )  
        Network->TcpRequested = 0;

      break;
    }
    case 10: // osc_tcpout_auto
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need an int" );

      NetworkOsc_SetTcpAutoConnect( value );
      break;
    }
    case 11: // dhcp
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need an int" );
      
      Network_SetDhcpEnabled( value );
      break;
    }
    case 13: // osc_udp_send_port
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need an int" );

      NetworkOsc_SetUdpSendPort( value );
      break;
    }
  }
  return CONTROLLER_OK;
}

// Get the property
int NetworkOsc_PropertyGet( int property, int channel )
{
  int value;
  int result = CONTROLLER_OK;
  int a0;
  int a1; 
  int a2;
  int a3;

  switch ( property )
  {
    case 0:
      value = Network_GetActive( );
      snprintf( Network->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, Network->scratch1, ",i", value );      
      break;
    case 1:
      if ( Network_GetAddress( &a0, &a1, &a2, &a3 ) == CONTROLLER_ERROR_NO_NETWORK )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "No network address available - try plugging in an Ethernet cable." );
      snprintf( Network->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      snprintf( Network->scratch2, OSC_SCRATCH_SIZE, "%d.%d.%d.%d", a0, a1, a2, a3 );
      Osc_CreateMessage( channel, Network->scratch1, ",s", Network->scratch2 );      
      break;
    case 2:
      if ( Network_GetMask( &a0, &a1, &a2, &a3 ) == CONTROLLER_ERROR_NO_NETWORK )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "No mask available - try plugging in an Ethernet cable." );
      snprintf( Network->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      snprintf( Network->scratch2, OSC_SCRATCH_SIZE, "%d.%d.%d.%d", a0, a1, a2, a3 );
      Osc_CreateMessage( channel, Network->scratch1, ",s", Network->scratch2 );      
      break;
    case 3:
      if ( Network_GetGateway( &a0, &a1, &a2, &a3 ) == CONTROLLER_ERROR_NO_NETWORK )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "No gateway available - try plugging in an Ethernet cable." );
      snprintf( Network->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      snprintf( Network->scratch2, OSC_SCRATCH_SIZE, "%d.%d.%d.%d", a0, a1, a2, a3 );
      Osc_CreateMessage( channel, Network->scratch1, ",s", Network->scratch2 );      
      break;
    case 4:
      value = Network_GetValid( );
      snprintf( Network->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, Network->scratch1, ",i", value );      
      break;
    case 5:
      snprintf( Network->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      snprintf( Network->scratch2, OSC_SCRATCH_SIZE, "%02X:%02X:%02X:%02X:%02X:%02X", 
                emacETHADDR0, emacETHADDR1, emacETHADDR2, emacETHADDR3, emacETHADDR4, emacETHADDR5 );
      Osc_CreateMessage( channel, Network->scratch1, ",s", Network->scratch2 );      
      break;
    case 6: // osc_udp_listen_port
      value = NetworkOsc_GetUdpListenPort( );
      snprintf( Network->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, Network->scratch1, ",i", value );      
      break;
    case 7: // osc_tcpout_address
      value = NetworkOsc_GetTcpOutAddress( );
      a0 = IP_ADDRESS_A( value );
      a1 = IP_ADDRESS_B( value );
      a2 = IP_ADDRESS_C( value );
      a3 = IP_ADDRESS_D( value );
      Network_AddressConvert( Network->scratch1, &a0, &a1, &a2, &a3 );
      snprintf( Network->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      snprintf( Network->scratch2, OSC_SCRATCH_SIZE, "%d.%d.%d.%d", a0, a1, a2, a3 );
      Osc_CreateMessage( channel, Network->scratch1, ",s", Network->scratch2 );    
      break;
    case 8: // osc_tcpout_port
      value = NetworkOsc_GetTcpOutPort( );
      snprintf( Network->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, Network->scratch1, ",i", value );
      break;
    case 9: // osc_tcpout_connect
      value = NetworkOsc_GetTcpRequested( );
      snprintf( Network->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, Network->scratch1, ",i", value );
      break;
    case 10: // osc_tcpout_auto
      value = NetworkOsc_GetTcpAutoConnect( );
      snprintf( Network->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, Network->scratch1, ",i", value );
      break;
    case 11: // dhcp
      value = Network_GetDhcpEnabled( );
      snprintf( Network->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, Network->scratch1, ",i", value );      
      break;
    case 12: // find
    {
      if ( Network_GetAddress( &a0, &a1, &a2, &a3 ) == CONTROLLER_ERROR_NO_NETWORK )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "No network address available - try plugging in an Ethernet cable." );
      snprintf( Network->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      snprintf( Network->scratch2, OSC_SCRATCH_SIZE, "%d.%d.%d.%d", a0, a1, a2, a3 );
      int listen = NetworkOsc_GetUdpListenPort( );
      int send = NetworkOsc_GetUdpSendPort( );
      char* sysName = System_GetName( );
      Osc_CreateMessage( channel, Network->scratch1, ",siis", Network->scratch2, listen, send, sysName );      
      break;
    }
    case 13: // osc_udp_send_port
      value = NetworkOsc_GetUdpSendPort( );
      snprintf( Network->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, Network->scratch1, ",i", value );         
      break;
  }
  
  return result;
}

#endif // OSC

#endif // MAKE_CTRL_NETWORK


