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

/*
	NETWORK.h

  MakingThings
*/

#ifndef NETWORK_H
#define NETWORK_H


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

void* Socket( int address, int port );
int   SocketRead( void* socket, void* data, int length );
int   SocketWrite( void* socket, void* data, int length );
int   SocketClose( void* socket );

void* ServerSocket( int port );
void* ServerSocketAccept( void* serverSocket );
int   ServerSocketClose( void* serverSocket );

int SocketSendDecimal( void *socket, int d );

void* DatagramSocket( int port );
int   DatagramSocketSend( void* datagramSocket, int address, int port, void* data, int length );
int   DatagramSocketReceive( void* datagramSocket, int receivingPort, int* address, int* port, void* data, int length );
void  DatagramSocketClose( void* socket );


/* NetworkOsc Interface */

const char* NetworkOsc_GetName( void );
int NetworkOsc_ReceiveMessage( int channel, char* message, int length );
int NetworkOsc_Poll( void );


#endif
