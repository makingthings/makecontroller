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

/**
  @file
	Network system defines.
*/

#ifndef NETWORK_H
#define NETWORK_H

/**
  \def IP_ADDRESS( a, b, c, d )
  Generate an address appropriate for Socket functions from 4 integers.
  \b Example
  \code
  void* sock = Socket( IP_ADDRESS( 192, 168, 0, 200 ), 80 );
  \endcode
  \ingroup Sockets
*/
// lwIP socket addresses are formatted this way...
#define IP_ADDRESS( a, b, c, d ) ( ( (int)a << 24 ) + ( (int)b << 16 ) + ( (int)c << 8 ) + (int)d )
#define IP_ADDRESS_A( address )  ( ( (int)address >> 24 ) & 0xFF )
#define IP_ADDRESS_B( address )  ( ( (int)address >> 16 ) & 0xFF ) 
#define IP_ADDRESS_C( address )  ( ( (int)address >>  8 ) & 0xFF )
#define IP_ADDRESS_D( address )  ( ( (int)address       ) & 0xFF )
// the lwIP netif structure formats the address the other way round...weird.
#define NETIF_IP_ADDRESS( a, b, c, d ) ( ( (int)d << 24 ) + ( (int)c << 16 ) + ( (int)b << 8 ) + (int)a )
#define NETIF_IP_ADDRESS_D( address )  ( ( (int)address >> 24 ) & 0xFF )
#define NETIF_IP_ADDRESS_C( address )  ( ( (int)address >> 16 ) & 0xFF ) 
#define NETIF_IP_ADDRESS_B( address )  ( ( (int)address >>  8 ) & 0xFF )
#define NETIF_IP_ADDRESS_A( address )  ( ( (int)address       ) & 0xFF )


// Network API stuff
int Network_SetActive( int active );
int Network_GetActive( void );
int Network_Init( void );

int Network_SetAddress( int a0, int a1, int a2, int a3 );
int Network_SetMask( int a0, int a1, int a2, int a3 );
int Network_SetGateway( int a0, int a1, int a2, int a3 );
int Network_SetValid( int v );

int Network_GetAddress( int* a0, int* a1, int* a2, int* a3 );
int Network_GetMask( int* a0, int* a1, int* a2, int* a3 );
int Network_GetGateway( int* a0, int* a1, int* a2, int* a3 );
int Network_GetValid( void );

// TCP stuff
void* Socket( int address, int port );
int SocketBytesAvailable( void* socket );
int SocketRead( void* socket, char* data, int length );
int SocketReadLine( void* socket, char* data, int length );
int SocketWrite( void* socket, char* data, int length );
void SocketClose( void* socket );

void* ServerSocket( int port );
void* ServerSocketAccept( void* serverSocket );
int ServerSocketClose( void* serverSocket );

int SocketSendDecimal( void *socket, int d );

// UDP stuff
void* DatagramSocket( int port );
int DatagramSocketSend( void* datagramSocket, int address, int port, void* data, int length );
int DatagramSocketReceive( void* datagramSocket, int receivingPort, int* address, int* port, void* data, int length );
void DatagramSocketClose( void* socket );

// DHCP stuff
void Network_SetDhcpEnabled( int enabled );
int Network_GetDhcpEnabled( void );

// DNS stuff
int Network_DnsGetHostByName( const char *name );

// WebServer Stuff
void Network_SetWebServerEnabled( int enabled );
int Network_GetWebServerEnabled( void );
void Network_StartWebServer( void );
void Network_StopWebServer( void );

// NetworkOsc Interface
const char* NetworkOsc_GetName( void );
int NetworkOsc_ReceiveMessage( int channel, char* message, int length );
int NetworkOsc_Poll( void );
void NetworkOsc_SetUdpListenPort( int port );
int NetworkOsc_GetUdpListenPort( void );
void NetworkOsc_SetUdpSendPort( int port );
int NetworkOsc_GetUdpSendPort( void );
void NetworkOsc_SetTcpOutAddress( int a0, int a1, int a2, int a3 );
int NetworkOsc_GetTcpOutAddress( void );
int NetworkOsc_GetTcpOutPort( void );
void NetworkOsc_SetTcpOutPort( int port );
void NetworkOsc_SetTcpAutoConnect( int yesorno );
int NetworkOsc_GetTcpAutoConnect( void );
int NetworkOsc_GetTcpRequested( void );

#endif
