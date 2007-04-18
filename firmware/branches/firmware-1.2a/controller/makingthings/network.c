/*********************************************************************************

 Copyright 2006 MakingThings

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
	Network - Ethernet Control.
	Methods for communicating via the Ethernet port with the Make Controller Board.
*/

#include "stdio.h"

#include "io.h"
#include "eeprom.h"
#include "timer.h"


/* lwIP includes. */
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/memp.h" 
#include "lwip/stats.h"
#include "netif/loopif.h"
#include "lwip/dhcp.h"

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

#include "config.h"
#include "network.h"
#include "webserver.h"

// a few globals
struct Network_* Network;

enum { NET_UNCHECKED, NET_INVALID, NET_VALID } Network_Valid;

void Network_DhcpFineCallback( int id );
void Network_DhcpCoarseCallback( int id );
void Network_SetPending( int state );
int Network_GetPending( void );
void Network_DhcpStart( struct netif* netif );
void Network_DhcpStop( struct netif* netif );

/** \defgroup Network
  The Network subsystem manages the Ethernet controller.  
  This subsystem is based on LwIP, an open source TCP/IP stack.  It provides TCP
  and UPD sockets and handles storage and retrieval of the IP address, address 
  mask, gateway address.  Also handled is the device's MAC address.

  IP Address, mask and gatway default to 192.168.0.200, 255.255.255.0 and 192.168.0.1.
  The MAC address defaults to AC.DE.48.55.x.y where x & y are calculated from the 
  unit's serial number which is handled by the \ref System subsystem.

  From OSC this subsystem can be addressed as "network".  It has the following 
  properties:
    \li active - can be used to activate / deactivate the Network subsystem and to read its status
    \li address - reading or writing the ip address in string a1.a2.a3.a4 form
    \li mask - reading or writing the ip address mask in string m1.m2.m3.m4 form
    \li gateway - reading or writing the ip address mask in string g1.g2.g3.g4 form
    \li valid - reading or asserting the validity of the currently stored address set
    \li mac - reading the MAC address in string form.  To change it see the \ref System subsystem

  \todo provide some address packing and unpacking functions
  \todo re-org the function names slightly so they conform a bit better to the
    rest of the system
  \todo remove the port param from the datagram socket create function - it's not
    used
  \ingroup Controller
  @{
*/

/**
	Sets whether the Network subsystem is active.  This is automatically called by
  any of the Socket functions.  Make sure the address is set correctly before 
  calling this function.
	@param state An integer specifying the active state - 1 (active) or 0 (inactive).
	@return 0 on success.
*/
int Network_SetActive( int state )
{
  if ( state ) 
  {
    if( Network == NULL )
    {
      Network = Malloc( sizeof( struct Network_ ) );

      Network->pending = 0;
      Network->TcpRequested = 0;
      Network->WebServerTaskPtr = NULL;
      // set the temp addresses to use when setting a new IP address/mask/gateway
      Eeprom_Read( EEPROM_SYSTEM_NET_ADDRESS, (uchar*)&Network->TempIpAddress, 4 );
      Eeprom_Read( EEPROM_SYSTEM_NET_GATEWAY, (uchar*)&Network->TempGateway, 4 );
      Eeprom_Read( EEPROM_SYSTEM_NET_MASK, (uchar*)&Network->TempMask, 4 );
      Eeprom_Read( EEPROM_OSC_UDP_PORT, (uchar*)&Network->OscUdpPort, 4 );
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

/**
	Returns the active state of the Network subsystem.
	@return State - 1 (active) or 0 (inactive).
*/
int Network_GetActive( void )
{
  return Network != NULL;
}

/**
	Set the IP address of the Make Controller.
	The IP address of the Make Controller, in dotted decimal form (xxx.xxx.xxx.xxx),
	can be set by passing in each of the numbers as a separate parameter.
	The default IP address of each Make Controller as it ships from the factory
	is 192.168.0.200.  
  
  Because changing the network configuration of the board should
  really be an atomic operation (in terms of changing the address/mask/gateway), 
  we need to make a call to Network_SetValid() in order to actually apply
  the changed address.  

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
  // now implement the change
  Network_SetValid( 1 );
  \endcode

*/
int Network_SetAddress( int a0, int a1, int a2, int a3 )
{
	// just store this address, since we're only going to do something with it in response to
  // Network_SetValid().
  Network->TempIpAddress = NETIF_IP_ADDRESS( a0, a1, a2, a3 );
  Network_Valid = NET_INVALID;

  return CONTROLLER_OK;
}

/**
	Set the IP address of the Make Controller.
	The IP address of the Make Controller, in dotted decimal form (xxx.xxx.xxx.xxx),
	can be set by passing in each of the numbers as a separate parameter.
	The default IP address of each Make Controller as it ships from the factory
	is 192.168.0.200

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
  if( 0 != Network_SetAddress( 192, 168, 0, 23 ) )
    // then there was a problem.
  \endcode

*/
void NetworkOsc_SetTcpOutAddress( int a0, int a1, int a2, int a3 )
{
	int address = IP_ADDRESS( a0, a1, a2, a3 );
  if( address != Network->TcpOutAddress )
  {
    Network->TcpOutAddress = address;
    Eeprom_Write( EEPROM_TCP_OUT_ADDRESS, (uchar*)&address, 4 );
  }
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

void NetworkOsc_SetUdpPort( int port )
{
  if( port != Network->OscUdpPort ) // only change things if it's a new value
  {
    Network->OscUdpPort = port;
    Eeprom_Write( EEPROM_OSC_UDP_PORT, (uchar*)&port, 4 );
  }
}

int NetworkOsc_GetUdpPort(  )
{
  if( Network->OscUdpPort > 0 && Network->OscUdpPort < 65536 )
    return Network->OscUdpPort;
  else
    return 10000;
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
  // Network_SetValid().
  Network->TempMask = NETIF_IP_ADDRESS( a0, a1, a2, a3 );
  Network_Valid = NET_INVALID;

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
  // Network_SetValid().
  Network->TempGateway = NETIF_IP_ADDRESS( a0, a1, a2, a3 );
  Network_Valid = NET_INVALID;
  
  return CONTROLLER_OK;
}

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
  int address;
  Eeprom_Read( EEPROM_SYSTEM_NET_ADDRESS, (uchar*)&address, 4 );

  int mask;
  Eeprom_Read( EEPROM_SYSTEM_NET_MASK, (uchar*)&mask, 4 );

  int gateway;
  Eeprom_Read( EEPROM_SYSTEM_NET_GATEWAY, (uchar*)&gateway, 4 );

  int total;
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

/**
	Read the IP address stored in EEPROM.
	Pass in pointers to integers where the address should be stored.

	@param a0 A pointer to an integer where the first of 4 numbers of the address is to be stored.
	@param a1 A pointer to an integer where the second of 4 numbers of the address is to be stored.
	@param a2 A pointer to an integer where the third of 4 numbers of the address is to be stored.
	@param a3 A pointer to an integer where the fourth of 4 numbers of the address is to be stored.
	@return 0 on success.
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

  return CONTROLLER_OK;
}

/**
	Read the IP address stored in EEPROM that the board will use when told to
  make a connection to a remote TCP server.
	Pass in pointers to integers where the address should be stored.

	@param a0 A pointer to an integer where the first of 4 numbers of the address is to be stored.
	@param a1 A pointer to an integer where the second of 4 numbers of the address is to be stored.
	@param a2 A pointer to an integer where the third of 4 numbers of the address is to be stored.
	@param a3 A pointer to an integer where the fourth of 4 numbers of the address is to be stored.
	@return 0 on success.
*/
int NetworkOsc_GetTcpOutAddress( )
{
  return Network->TcpOutAddress;
}

/**
	Read the network mask stored in EEPROM.
	Pass in pointers to integers where the mask should be stored.

	@param a0 A pointer to an integer where the first of 4 numbers of the mask is to be stored.
	@param a1 A pointer to an integer where the second of 4 numbers of the mask is to be stored.
	@param a2 A pointer to an integer where the third of 4 numbers of the mask is to be stored.
	@param a3 A pointer to an integer where the fourth of 4 numbers of the mask is to be stored.
	@return 0 on success.
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
    
  return CONTROLLER_OK;
}

/**
	Read the gateway address stored in EEPROM.
	Pass in pointers to integers where the gateway address should be stored.

	@param a0 A pointer to an integer where the first of 4 numbers of the gateway address is to be stored.
	@param a1 A pointer to an integer where the second of 4 numbers of the gateway address is to be stored.
	@param a2 A pointer to an integer where the third of 4 numbers of the gateway address is to be stored.
	@param a3 A pointer to an integer where the fourth of 4 numbers of the gateway address is to be stored.
	@return 0 on success.
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
  
  return CONTROLLER_OK;
}

/**	
	Create a new TCP socket connected to the address and port specified.
	@param address An integer specifying the IP address to connect to.
	@param port An integer specifying the port to connect on.
	@return A pointer to the socket, if it was created successfully.  NULL if unsuccessful.
	@see lwIP, SocketRead(), SocketWrite(), SocketClose()

  \par Example
  \code
  // use the IP_ADDRESS macro to format the address properly
  int addr = IP_ADDRESS( 192, 168, 0, 54 );
  // then create the socket
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
			{
				vTaskDelay( 10 );
			}
    conn = NULL;
  }
  
  return conn;
}

/**	
	Read from a TCP socket.
  Make sure you have an open socket before trying to read from it.
	@param socket A pointer to the existing socket.
	@param data A pointer to the buffer to read to.
	@param length An integer specifying the length in bytes of how much data should be read.
	@return An integer: length of data read if successful, zero on failure.
	@see lwIP, Socket(), SocketClose()

  \par Example
  \code
  // we should already have created a socket \b sock with Socket().
  struct netconn* sock = Socket( addr, 10101 );
  // we should also have a buffer to read into - \b data.
  // and know how many bytes of it we want to read - \b length.
  int length_read = SocketRead( sock, data, length )
  // if 0 bytes were read, there was some sort of error
  if( length_read == 0 )
    SocketClose( sock );
  \endcode
*/
int SocketRead( void* socket, void* data, int length )
{
  struct netconn *conn = socket;
  struct netbuf *buf;
  int buflen;

  if ( conn->readingbuf == NULL )
  {
    buf = netconn_recv( conn );
    if ( buf == NULL )
      return 0;

    buflen = netbuf_len( buf );
    /* copy the contents of the received buffer into
    the supplied memory pointer mem */
    netbuf_copy( buf, data, length );

    // Test to see if the buffer is done, or needs to be saved
    if ( buflen <= length )
    {
      netbuf_delete( buf );
    }
    else
    {
      conn->readingbuf = buf;
      conn->readingoffset = length;
      buflen = length;
    }
  }
  else
  {
    buf = conn->readingbuf;
    buflen = netbuf_len( buf ) - conn->readingoffset;
    netbuf_copy_partial( buf, data, length, conn->readingoffset );

    if ( buflen <= length )
    {
      netbuf_delete( buf );
      conn->readingbuf = NULL;
    }  
    else
    {
      conn->readingoffset += length;
      buflen = length;
    }
  }

  return buflen;
}


/**	
	Write to a TCP socket.
	Not surprisingly, we need an existing socket before we can write to it.

	@param socket A pointer to the existing socket.
	@param data A pointer to the buffer to write from.
	@param length An integer specifying the length in bytes of how much data should be written.
	@return An integer: 'length written' if successful, 0 on failure.
	@see lwIP, Socket()

  \par Example
  \code
  // we should already have created a socket \b sock with Socket().
  struct netconn* sock = Socket( addr, 10101 );
  // we should also have a buffer to write from - \b data.
  char data[ MY_BUF_SIZE ];
  // and know how many bytes of it we want to write - \b length.
  int length = length_of_my_packet;
  int length_written = SocketWrite( sock, data, length )
  // if 0 bytes were written, there was some sort of error
  if( length_written == 0 )
    SocketClose( sock );
  \endcode
*/
int SocketWrite( void* socket, void* data, int length )
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
	@see lwIP, Socket()

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
  netconn_close( (struct netconn *)socket );
  netconn_delete( (struct netconn *)socket );
  return;
}

/**	
	Create a new TCP server socket and start listening for connections.
	@param port An integer specifying the port to listen on.
	@return A pointer to the socket created.
	@see lwIP
*/
void* ServerSocket( int port )
{
  Network_SetActive( 1 );

  struct netconn *conn;

  conn = netconn_new( NETCONN_TCP );
  netconn_listen( conn );
  netconn_bind( conn, 0, port );

  return conn;
}

/**	
	Accept an incoming connection to a ServerSocket that you have created.  This
  function will block until a new connection is waiting to be serviced.  It returns
  a regular socket on which you can use SocketWrite(), SocketRead() and SocketClose().
	@param serverSocket a pointer to a ServerSocket that you created
	@return a pointer to the new socket created to handle the connection
	@see lwIP
	@see ServerSocket()
*/
void* ServerSocketAccept( void* serverSocket )
{
  struct netconn *conn;
  conn = netconn_accept( (struct netconn *)serverSocket );
  // This is our addition to the conn structure to help with reading
  conn->readingbuf = NULL;
  return conn;
}

/**	
	Close a ServerSocket that you have created.
	@param serverSocket A pointer to a ServerSocket.
	@return 0 if the process was successful.
	@see lwIP
	@see ServerSocket()
*/
int ServerSocketClose( void* serverSocket )
{
  netconn_close( serverSocket );
  netconn_delete( serverSocket );
  return 0;
}

/**	
	Create a socket to read and write UDP packets.
	@param port An integer specifying the port to open.
	@return a pointer to the socket created.
	@see lwIP
*/
void* DatagramSocket( int port )
{
  Network_SetActive( 1 );

  struct netconn *conn;

  conn = netconn_new( NETCONN_UDP );
  // This is our addition to the conn structure to help with reading
  conn->readingbuf = NULL;

  // hook it up
  netconn_bind( conn, NULL, port ); // Liam
  //netconn_connect(conn, 0, port);
  //netconn_bind( conn, IP_ADDR_ANY, port );

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
	@see lwIP
*/
int DatagramSocketSend( void* datagramSocket, int address, int port, void* data, int length )
{ 
  struct netconn *conn = (struct netconn *)datagramSocket;
  struct netbuf *buf;

  struct ip_addr remote_addr;

  //struct ip_addr prior_addr;
  //int prior_port;
  //netconn_peer( conn, &prior_addr, &prior_port );

  remote_addr.addr = htonl(address);
  netconn_connect(conn, &remote_addr, port);

  // create a buffer
  buf = netbuf_new();
  // make the buffer point to the data that should be sent
  netbuf_ref( buf, data, length);
  // send the data
  netconn_send( conn, buf);
  // deallocated the buffer
  netbuf_delete(buf);

  // return the connection addr & port
  // netconn_bind( conn, &prior_addr, port );

  return length;
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
	@see lwIP
*/
int DatagramSocketReceive( void* datagramSocket, int incomingPort, int* address, int* port, void* data, int length )
{
  struct netconn *conn = (struct netconn*)datagramSocket;
  struct netbuf *buf;
  struct ip_addr *addr;
  int buflen;

  netconn_bind( conn, IP_ADDR_ANY, incomingPort );
    
  buf = netconn_recv( conn );
  buflen = netbuf_len( buf );

  // copy the contents of the received buffer into
  //the supplied memory pointer 
  netbuf_copy(buf, data, length);
  addr = netbuf_fromaddr(buf);
  *port = netbuf_fromport(buf);
  *address = ntohl( addr->addr );
  netbuf_delete(buf);

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
	@see lwIP
*/
void DatagramSocketClose( void* socket )
{
  netconn_close( socket );
  netconn_delete( socket );
}

/** @}
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
    Network_SetValid( 1 ); // bring the 
  }
  return;
}

int Network_GetDhcpEnabled( )
{
  int state;
  Eeprom_Read( EEPROM_DHCP_ENABLED, (uchar*)&state, 4 );
  return state; 
}

void Network_SetWebServerEnabled( int state )
{
  if( state )
  {
    if( Network->WebServerTaskPtr == NULL )
      Network_StartWebServer( );

    if( !Network_GetWebServerEnabled( ) )
      Eeprom_Write( EEPROM_WEBSERVER_ENABLED, (uchar*)&state, 4 );
  }
  else
  {
    Network_StopWebServer( );

    if( Network_GetWebServerEnabled( ) )
      Eeprom_Write( EEPROM_WEBSERVER_ENABLED, (uchar*)&state, 4 );
  }
}

void Network_StartWebServer( )
{
  if( Network->WebServerTaskPtr == NULL )
    Network->WebServerTaskPtr = TaskCreate( WebServer, "WebServ", 300, NULL, 4 );
}

void Network_StopWebServer( )
{
  if( Network->WebServerTaskPtr != NULL )
  {
    TaskDelete( Network->WebServerTaskPtr );
    Network->WebServerTaskPtr = NULL;
    CloseWebServer( );
  }
}

int Network_GetWebServerEnabled( )
{
  int state;
  Eeprom_Read( EEPROM_WEBSERVER_ENABLED, (uchar*)&state, 4 );
  if( state != 1 )
    state = 0;
  return state;
}

void Network_DhcpStart( struct netif* netif )
{
  Network_SetPending( 1 ); // set a flag so nobody else tries to set up this netif
  int count = 0;
  dhcp_start( netif );
  Network->DhcpFineTaskPtr = TaskCreate( DhcpFineTask, "DhcpFine", 150, 0, 1 );
  Network->DhcpCoarseTaskPtr = TaskCreate( DhcpCoarseTask, "DhcpCoarse", 100, 0, 1 );
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
  TaskDelete( Network->DhcpFineTaskPtr );
  TaskDelete( Network->DhcpCoarseTaskPtr );
  Network->DhcpFineTaskPtr = NULL;
  Network->DhcpCoarseTaskPtr = NULL;
  netif_set_up(netif); // bring the interface back up, as dhcp_release() takes it down
  return;
}

// TODO - eventually create these as timers instead of tasks
// right now, the dhcp_tmr functions cannot be called from within the Timer ISR
void DhcpFineTask( void* p )
{
  (void)p;
  while( true )
  {
    dhcp_fine_tmr();
    Sleep( DHCP_FINE_TIMER_MSECS );
  }
}

void DhcpCoarseTask( void* p )
{
  (void)p;
  while( true )
  {
    dhcp_fine_tmr();
    Sleep( DHCP_COARSE_TIMER_SECS * 1000 );
  }
}


int Network_AddressConvert( char* address, int* a0, int* a1, int* a2, int* a3 )
{
  return ( sscanf( address, "%d.%d.%d.%d", a0, a1, a2, a3 ) == 4 ) ? CONTROLLER_OK : CONTROLLER_ERROR_NO_ADDRESS;
}

#if ( CONTROLLER_VERSION == 50 || CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 )
#define NETWORK_BITS IO_PB00_BIT | IO_PB01_BIT | IO_PB02_BIT | IO_PB03_BIT | IO_PB04_BIT | \
                     IO_PB05_BIT | IO_PB06_BIT | IO_PB07_BIT | IO_PB08_BIT | IO_PB09_BIT | \
                     IO_PB10_BIT | IO_PB11_BIT | IO_PB12_BIT | IO_PB13_BIT | IO_PB14_BIT | \
                     IO_PB15_BIT | IO_PB16_BIT | IO_PB17_BIT | IO_PB18_BIT | IO_PB26_BIT
#endif
#if ( CONTROLLER_VERSION == 90 )
#define NETWORK_BITS IO_PB00_BIT | IO_PB01_BIT | IO_PB02_BIT | IO_PB03_BIT | IO_PB04_BIT | \
                     IO_PB05_BIT | IO_PB06_BIT | IO_PB07_BIT | IO_PB08_BIT | IO_PB09_BIT | \
                     IO_PB15_BIT | IO_PB26_BIT
#endif

int Network_Init( )
{
  // Start and Lock all the bits to do with the Ethernet Phy - can do this immediately, since there's no undoing this
  Io_StartBits( NETWORK_BITS, true );

  // Attempt to get a serial number and set it into the mac address low bytes to make a unique MAC address
  int serialNumber = System_GetSerialNumber();
  emacETHADDR5 = serialNumber & 0xFF;
  emacETHADDR4 = ( serialNumber >> 8 ) & 0xFF;
  // Low nibble of the third byte - gives us around 1M serial numbers
  emacETHADDR3 = 0x50 | ( ( serialNumber >> 12 ) & 0xF );

  /* Initialize lwIP and its interface layer. */
  stats_init();
	sys_init();
	mem_init();								
	memp_init();
	pbuf_init(); 
	netif_init();
	ip_init();
	tcpip_init( NULL, NULL );

  extern err_t ethernetif_init( struct netif *netif );
  static struct netif EMAC_if;
  int address, mask, gateway, dhcp;
  dhcp = 0; //Network_GetDhcpEnabled();

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
  Network_SetPending( 0 ); // all done for now

  if( dhcp )
    Network_DhcpStart( &EMAC_if );
  
  if( NetworkOsc_GetTcpAutoConnect( ) )
  {
    Network->TcpRequested = 1;
    Osc_StartTcpTask( );
  }

  if( Network_GetWebServerEnabled( ) )
    Network_StartWebServer( );
  
  
  /*
  hmm...some work to get this together.
  vSemaphoreCreateBinary( Network.semaphore );
  
  
  if( !Timer_GetActive() )
    Timer_SetActive( true );
  Timer_InitializeEntry( &Network.DhcpFineTimer, Network_DhcpFineCallback, 0, DHCP_FINE_TIMER_MSECS, true);
  // timer acting weird at the moment.......
  Timer_InitializeEntry( &Network.DhcpCoarseTimer, Network_DhcpCoarseCallback, 1, DHCP_COARSE_TIMER_SECS * 150, true);
  Timer_Set( &Network.DhcpFineTimer );
  Timer_Set( &Network.DhcpCoarseTimer );
  */
  return CONTROLLER_OK;
}

/** \defgroup NetworkOSC Network - OSC
  Configure the Controller Board's Network Settings via OSC.
  \ingroup OSC
   
    \section devices Devices
    There is only one Network system, so a device index is not used.
   
    \section properties Properties
    The Network system has twelve properties 
    - \b address
    - \b mask
    - \b gateway,
    - \b valid
    - \b mac
    - \b active 
    - \b osc_udp_port
    - \b osc_tcpout_address
    - \b osc_tcpout_port
    - \b osc_tcpout_connect
    - \b osc_tcpout_auto
    - \b dhcp

    \par Address
    The \b 'address' property corresponds to the IP address of the Controller Board.
    This value can be both read and written.  To set a new address, send a message like
    \verbatim /network/address 192.168.0.235 \endverbatim
    \par
    To read the current IP address, omit the argument value from the end of the message:
    \verbatim /network/address \endverbatim
   
    \par Mask
    The \b 'mask' property corresponds to the network mask of the Controller Board.
    When on a subnet or local network, the network mask must be set in order
    for the gateway to route information to the board's IP address properly.
    The mask is commonly 255.255.255.0 for many home networks.
    \par
    To set the board's network mask, send a message like
    \verbatim /network/mask 255.255.255.0 \endverbatim
    To read the current mask, omit the argument value from the end of the message:
    \verbatim /network/mask \endverbatim
   
    \par Gateway
    The \b 'gateway' property corresponds to the gateway address for the local network the Make Controller is on.
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
    The \b 'valid' property corresponds to a checksum used to make sure the board's network settings are valid.
    Ideally, this should be called each time an address setting is changed so that if
    the board gets powered down, it will know when it comes back up whether or
    not the address settings is currently has are valid.
    This creates a checksum for the current address settings and stores it in EEPROM.
    \par
    To set the board's current network settings as valid, send the message
    \verbatim /network/valid 1 \endverbatim
    To check if the current settings have been set as valid, send the message:
    \verbatim /network/valid \endverbatim
    with no argument value.

    \par OSC UDP Port
    The \b osc_udp_port corresponds to the port that the Make Controller listens on for
    incoming OSC messages via UDP.  This value is stored persistently, so it's available
    even after the board has rebooted.  This is 10000 by default.
    \par
    To tell the board to listen on port 10001, instead of the default 10000, send the message
    \code /network/osc_udp_port 10001 \endcode
    To read back the port that the board is currently listening on, send the message
    \verbatim /network/osc_udp_port \endverbatim
    with no argument value.

    \subsection tcpconnections TCP Connections
    The Make Controller can act as a TCP client, make a connection to a remote TCP server, and
    communicate with it via OSC.  This can be quite handy for providing a remote web interface for the board
    and for solving all the bi-directional communications issues present with UDP communication. 

    To do this, there are just a couple of steps.
    - You'll need to tell the board the address & the port of the server you want to connect to.
    - Tell the board to connect
    - optionally set the board to automatically reconnect when it starts back up (in case it crashes).
    The commands below allow you to control all of this via OSC.
    
    \par OSC TCP Address
    The \b osc_tcpout_address property corresponds to the IP address that the Make Controller
    will try to connect to when the \b osc_tcpout_connect \b 1 command is sent.
    \par
    To tell the board to prepare to connect to 192.168.0.118, send the message
    \code /network/osc_tcpout_address 192.168.0.118 \endcode
    To read back the address that the board is planning on connecting to, send the message
    \verbatim /network/osc_tcpout_address \endverbatim
    with no argument value.

    \par OSC TCP PORT
    The \b osc_tcpout_port property corresponds to the port that the board will try to connect
    on when making a TCP connection.  This value is stored persistently, so it's available
    even after the board has rebooted.  This is 10101 by default.
    \par
    To tell the board to prepare to connect on port 11111, send the message
    \code /network/osc_tcpout_port 11111 \endcode
    To read back the address that the port is planning on connecting on, send the message
    \verbatim /network/osc_tcpout_port \endverbatim
    with no argument value.
    
    \par TCP Connect
    The \b osc_tcpout_connect property allows you to make a connection to a remote TCP server.  It
    will try to connect to a server specified by the \b osc_tcpout_address and \b osc_tcpout_port properties.
    \par
    When you send the message
    \code /network/osc_tcpout_connect 1 \endcode
    the board will attempt to make a connection to a server specified by the \b osc_tcpout_address and \b osc_tcpout_port properties.
    You can tell the board to disconnect from the server by sending the message
    \code /network/osc_tcpout_connect 0 \endcode
    Lastly, you can read whether the board is currently connected (or trying to connect) by sending the message
    \code /network/osc_tcpout_connect \endcode
    with no argument value.

    \par  TCP Auto Connect
    The \b osc_tcpout_auto property specifies whether the board should attempt to make a connection
    to a remote TCP server as soon as it boots up.  It will try to connect to a server 
    specified by the \b osc_tcp_address and \b osc_tcp_port properties.  This value is stored 
    persistently, so it's available even after the board has rebooted.  If the board is not currently connected
    to a TCP server, it will attempt to make a connection when this command is given.
    \par 
    To set the board to make a TCP connection as soon as it boots up, send the message
    \code /network/osc_tcpout_auto 1 \endcode
    To read whether the board will automatically try to connect on reboot, send the message
    \code /network/osc_tcpout_auto \endcode
    with no argument value.

    \subsection Miscelleneous

    \par DHCP
    The \b dhcp property sets whether the board should try to dynamically retrieve a network address from 
    a network router.  If no DHCP server is available, the board will use the network settings stored in memory
    for the \b address, \b gateway, and \b mask properties.
    \par
    To turn DHCP on, send the message
    \code /network/dhcp 1 \endcode
    and the board will immediately try to get an address.  To check what address the board got, send the message
    \code /network/address \endcode
    as you normally would.  To turn DHCP off, send the message
    \code /network/dhcp 0 \endcode
    To read whether DHCP is currently set on the board, send the message
    \code /network/dhcp \endcode
    with no argument value.
   
    \par MAC
    The \b mac property corresponds to the Ethernet MAC address of the Controller Board.
    This value is read-only.
    \par
    To read the MAC address of the Controller, send the message
    \verbatim /network/mac \endverbatim
    The board will respond by sending back an OSC message with the MAC address.
   
    \par Active
    The \b 'active' property corresponds to the active state of the Network system.
    If the Network system is set to be inactive, it will not respond to any OSC messages. 
    If you're not seeing appropriate
    responses to your messages to the Network system, check the whether it's
    active by sending a message like
    \verbatim /network/active \endverbatim
    \par
    You can set the active flag by sending
    \verbatim /network/active 1 \endverbatim
*/
#ifdef OSC

void NetworkOsc_SetTcpAutoConnect( int yesorno )
{
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
  return state;
}

int NetworkOsc_GetTcpRequested( )
{
  return Network->TcpRequested;
}


#include "osc.h"
static char* NetworkOsc_Name = "network";
static char* NetworkOsc_PropertyNames[] = { "active", "address", "mask", "gateway", "valid", "mac", 
                                              "osc_udp_port", "osc_tcpout_address", "osc_tcpout_port", 
                                              "osc_tcpout_connect", "osc_tcpout_auto", "dhcp", "webserver", 0 }; // must have a trailing 0

int NetworkOsc_PropertySet( int property, char* typedata, int channel );
int NetworkOsc_PropertyGet( int property, int channel );

const char* NetworkOsc_GetName( void )
{
  return NetworkOsc_Name;
}

int NetworkOsc_ReceiveMessage( int channel, char* message, int length )
{
  if ( Network_GetPending() )
    return Osc_SubsystemError( channel, NetworkOsc_Name, "Network initializing...please wait." );
  
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
    case 6: // osc_udp_port
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need an int" );

      NetworkOsc_SetUdpPort( value );
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
    case 12: // webserver
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need an int" );
      
      Network_SetWebServerEnabled( value );
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
  char address[ OSC_SCRATCH_SIZE ];
  char output[ OSC_SCRATCH_SIZE ];
  int a0;
  int a1; 
  int a2;
  int a3;

  switch ( property )
  {
    case 0:
      value = Network_GetActive( );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, address, ",i", value );      
      break;
    case 1:
      if ( Network_GetAddress( &a0, &a1, &a2, &a3 ) == CONTROLLER_ERROR_NO_NETWORK )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "No network address available - try plugging in an Ethernet cable." );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      snprintf( output, OSC_SCRATCH_SIZE, "%d.%d.%d.%d", a0, a1, a2, a3 );
      Osc_CreateMessage( channel, address, ",s", output );      
      break;
    case 2:
      if ( Network_GetMask( &a0, &a1, &a2, &a3 ) == CONTROLLER_ERROR_NO_NETWORK )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "No mask available - try plugging in an Ethernet cable." );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      snprintf( output, OSC_SCRATCH_SIZE, "%d.%d.%d.%d", a0, a1, a2, a3 );
      Osc_CreateMessage( channel, address, ",s", output );      
      break;
    case 3:
      if ( Network_GetGateway( &a0, &a1, &a2, &a3 ) == CONTROLLER_ERROR_NO_NETWORK )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "No gateway available - try plugging in an Ethernet cable." );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      snprintf( output, OSC_SCRATCH_SIZE, "%d.%d.%d.%d", a0, a1, a2, a3 );
      Osc_CreateMessage( channel, address, ",s", output );      
      break;
    case 4:
      value = Network_GetValid( );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, address, ",i", value );      
      break;
    case 5:
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      snprintf( output, OSC_SCRATCH_SIZE, "%02X:%02X:%02X:%02X:%02X:%02X", 
                emacETHADDR0, emacETHADDR1, emacETHADDR2, emacETHADDR3, emacETHADDR4, emacETHADDR5 );
      Osc_CreateMessage( channel, address, ",s", output );      
      break;
    case 6: // osc_udp_port
      value = NetworkOsc_GetUdpPort( );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, address, ",i", value );      
      break;
    case 7: // osc_tcpout_address
      value = NetworkOsc_GetTcpOutAddress( );
      a0 = IP_ADDRESS_A( value );
      a1 = IP_ADDRESS_B( value );
      a2 = IP_ADDRESS_C( value );
      a3 = IP_ADDRESS_D( value );
      Network_AddressConvert( address, &a0, &a1, &a2, &a3 );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      snprintf( output, OSC_SCRATCH_SIZE, "%d.%d.%d.%d", a0, a1, a2, a3 );
      Osc_CreateMessage( channel, address, ",s", output );    
      break;
    case 8: // osc_tcpout_port
      value = NetworkOsc_GetTcpOutPort( );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, address, ",i", value );
      break;
    case 9: // osc_tcpout_connect
      value = NetworkOsc_GetTcpRequested( );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, address, ",i", value );
      break;
    case 10: // osc_tcpout_auto
      value = NetworkOsc_GetTcpAutoConnect( );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, address, ",i", value );
      break;
    case 11: // dhcp
      value = Network_GetDhcpEnabled( );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, address, ",i", value );      
      break;
    case 12: // webserver
      value = Network_GetWebServerEnabled( );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, address, ",i", value );      
      break;
  }
  
  return result;
}

#endif // OSC


