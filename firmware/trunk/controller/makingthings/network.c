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

#include "network.h"
#include "io.h"
#include "eeprom.h"

/* lwIP includes. */
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/memp.h" 
#include "lwip/stats.h"
#include "netif/loopif.h"

/* Low level includes. */
#include "SAM7_EMAC.h"

// This has builtin endian compensation!
#define IP_ADDRESS( a, b, c, d ) ( ( (int)d << 24 ) + ( (int)c << 16 ) + ( (int)b << 8 ) + (int)a )
#define IP_ADDRESS_D( address )  ( ( (int)address >> 24 ) & 0xFF ) 
#define IP_ADDRESS_C( address )  ( ( (int)address >> 16 ) & 0xFF ) 
#define IP_ADDRESS_B( address )  ( ( (int)address >>  8 ) & 0xFF ) 
#define IP_ADDRESS_A( address )  ( ( (int)address       ) & 0xFF ) 

int Network_AddressConvert( char* address, int* a0, int* a1, int* a2, int* a3 );

/* MAC address definition.  The MAC address must be unique on the network. */
char emacETHADDR0 = 0xAC;
char emacETHADDR1 = 0xDE;
char emacETHADDR2 = 0x48;
char emacETHADDR3 = 0x55;
char emacETHADDR4 = 0x0;
char emacETHADDR5 = 0x0;

#include "config.h"

static int Network_Active;
enum { NET_UNCHECKED, NET_INVALID, NET_VALID } Network_Valid;

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
    if ( !Network_Active )
    {
      Network_Init();
      Network_Active = state;
    }
  }
  return CONTROLLER_OK;
}

/**
	Returns the active state of the Network subsystem.
	@return State - 1/non-zero (active) or 0 (inactive).
*/
int Network_GetActive( void )
{
  return Network_Active;
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
*/
int Network_SetAddress( int a0, int a1, int a2, int a3 )
{
  int address;
	address = IP_ADDRESS( a0, a1, a2, a3 );

  Eeprom_Write( EEPROM_SYSTEM_NET_ADDRESS, (uchar*)&address, 4 );
  
  Network_Valid = NET_INVALID;

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
*/
int Network_SetMask( int a0, int a1, int a2, int a3 )
{
  int address;
	address = IP_ADDRESS( a0, a1, a2, a3 );

  Eeprom_Write( EEPROM_SYSTEM_NET_MASK, (uchar*)&address, 4 );

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
*/
int Network_SetGateway( int a0, int a1, int a2, int a3 )
{
  int address;
	address = IP_ADDRESS( a0, a1, a2, a3 );

  Eeprom_Write( EEPROM_SYSTEM_NET_GATEWAY, (uchar*)&address, 4 );

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
    int address;
    Eeprom_Read( EEPROM_SYSTEM_NET_ADDRESS, (uchar*)&address, 4 );
  
    int mask;
    Eeprom_Read( EEPROM_SYSTEM_NET_MASK, (uchar*)&mask, 4 );
  
    int gateway;
    Eeprom_Read( EEPROM_SYSTEM_NET_GATEWAY, (uchar*)&gateway, 4 );
  
    int total = address + mask + gateway;
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
  if ( Network_Valid == NET_UNCHECKED )
    Network_GetValid();

  if ( Network_Valid == NET_INVALID )
  {
    *a0 = 192;
    *a1 = 168;
    *a2 = 0;
    *a3 = 200;

    return CONTROLLER_OK;
  }

  int address;
  Eeprom_Read( EEPROM_SYSTEM_NET_ADDRESS, (uchar*)&address, 4 );

  *a0 = IP_ADDRESS_A( address );
  *a1 = IP_ADDRESS_B( address );
  *a2 = IP_ADDRESS_C( address );
  *a3 = IP_ADDRESS_D( address );

  return CONTROLLER_OK;
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
  if ( Network_Valid == NET_UNCHECKED )
    Network_GetValid();

  if ( Network_Valid == NET_INVALID )
  {
    *a0 = 255;
    *a1 = 255;
    *a2 = 255;
    *a3 = 0;

    return CONTROLLER_OK;
  }

  int address;
  Eeprom_Read( EEPROM_SYSTEM_NET_MASK, (uchar*)&address, 4 );

  *a0 = IP_ADDRESS_A( address );
  *a1 = IP_ADDRESS_B( address );
  *a2 = IP_ADDRESS_C( address );
  *a3 = IP_ADDRESS_D( address );
    
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
  if ( Network_Valid == NET_UNCHECKED )
    Network_GetValid();

  if ( Network_Valid == NET_INVALID )
  {
    *a0 = 192;
    *a1 = 168;
    *a2 = 0;
    *a3 = 1;

    return CONTROLLER_OK;
  }

  int address;
  Eeprom_Read( EEPROM_SYSTEM_NET_GATEWAY, (uchar*)&address, 4 );

  *a0 = IP_ADDRESS_A( address );
  *a1 = IP_ADDRESS_B( address );
  *a2 = IP_ADDRESS_C( address );
  *a3 = IP_ADDRESS_D( address );
  
  return CONTROLLER_OK;
}

/**	
	Make a new TCP socket connected to the address specified.
	@param address an integer specifying the IP address
	@param port an integer specifying the port to open
	@return a pointer to the socket created
	@see lwIP...
*/
void* Socket( int address, int port )
{
  Network_SetActive( 1 );

  struct netconn *conn;

  conn = netconn_new( NETCONN_TCP );
  // This is our addition to the conn structure to help with reading
  conn->readingbuf = NULL;

  struct ip_addr remote_addr;
  remote_addr.addr = htonl(address);

  netconn_connect( conn, &remote_addr, port );
  return conn;
}

/**	
	Read from a TCP socket.
	@param socket A pointer to the existing socket.
	@param data A pointer to the buffer to read to.
	@param length An integer specifying the length in bytes of how much data should be read.
	@return An integer: length of data read if successful, zero on failure.
	@see lwIP
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
	In the current implementation, this will be a whole packet.  
	So pack data up into a block before calling this.

	@param socket A pointer to the existing socket.
	@param data A pointer to the buffer to write from.
	@param length An integer specifying the length in bytes of how much data should be written.
	@return An integer: 'length writen' if successful, 0 on failure.
	@see lwIP
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
	Close a TCP socket.
	@param socket A pointer to the existing socket.
	@return Zero if the process was successful.
	@see lwIP
*/
int SocketClose( void* socket )
{
  netconn_close( (struct netconn *)socket );
  netconn_delete( (struct netconn *)socket );
  return 0;
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
  //netconn_connect(conn, 0, port);
  netconn_bind( conn, IP_ADDR_ANY, port );

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

  Network_Valid = NET_UNCHECKED;

  int a0, a1, a2, a3;
  Network_GetAddress( &a0, &a1, &a2, &a3 );
  unsigned int address = IP_ADDRESS( a0, a1, a2, a3 );

  int m0, m1, m2, m3;
  Network_GetMask( &m0, &m1, &m2, &m3 );
  int mask = IP_ADDRESS( m0, m1, m2, m3 );

  int g0, g1, g2, g3;
  Network_GetGateway( &g0, &g1, &g2, &g3 );
  int gateway = IP_ADDRESS( g0, g1, g2, g3 );

	/* Create and configure the EMAC interface. */
//	IP4_ADDR(&xIpAddr,emacIPADDR0,emacIPADDR1,emacIPADDR2,emacIPADDR3);
//	IP4_ADDR(&xNetMast,emacNET_MASK0,emacNET_MASK1,emacNET_MASK2,emacNET_MASK3);
//	IP4_ADDR(&xGateway,emacGATEWAY_ADDR0,emacGATEWAY_ADDR1,emacGATEWAY_ADDR2,emacGATEWAY_ADDR3);
	netif_add(&EMAC_if, (struct ip_addr *)&address, (struct ip_addr *)&mask, (struct ip_addr *)&gateway, NULL, ethernetif_init, tcpip_input);

	/* make it the default interface */
  netif_set_default(&EMAC_if);

	/* bring it up */
  netif_set_up(&EMAC_if);

  return CONTROLLER_OK;
}



/* NetworkOsc Interface */

#include "osc.h"

static char* NetworkOsc_Name = "network";
static char* NetworkOsc_PropertyNames[] = { "active", "address", "mask", "gateway", "valid", "mac", 0 }; // must have a trailing 0

int NetworkOsc_PropertySet( int property, char* typedata, int channel );
int NetworkOsc_PropertyGet( int property, int channel );

const char* NetworkOsc_GetName( void )
{
  return NetworkOsc_Name;
}

int NetworkOsc_ReceiveMessage( int channel, char* message, int length )
{
  return Osc_GeneralReceiverHelper( channel, message, length, 
                                NetworkOsc_Name,
                                NetworkOsc_PropertySet, NetworkOsc_PropertyGet, 
                                NetworkOsc_PropertyNames );
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
    case 0:
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need an int" );

      Network_SetActive( value );
      break;
    }
    case 1:
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
    case 2:
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
    case 3:
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
    case 4:
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, NetworkOsc_Name, "Incorrect data - need an int" );

      Network_SetValid( value ); 
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
      Network_GetAddress( &a0, &a1, &a2, &a3 );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      snprintf( output, OSC_SCRATCH_SIZE, "%d.%d.%d.%d", a0, a1, a2, a3 );
      Osc_CreateMessage( channel, address, ",s", output );      
      break;
    case 2:
      Network_GetMask( &a0, &a1, &a2, &a3 );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", NetworkOsc_Name, NetworkOsc_PropertyNames[ property ] ); 
      snprintf( output, OSC_SCRATCH_SIZE, "%d.%d.%d.%d", a0, a1, a2, a3 );
      Osc_CreateMessage( channel, address, ",s", output );      
      break;
    case 3:
      Network_GetGateway( &a0, &a1, &a2, &a3 );
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
  }
  
  return result;
}
