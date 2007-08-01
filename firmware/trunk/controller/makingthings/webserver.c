/*
	FreeRTOS V4.0.1 - copyright (C) 2003-2006 Richard Barry.

	This file is part of the FreeRTOS distribution.

	FreeRTOS is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	FreeRTOS is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FreeRTOS; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	A special exception to the GPL can be applied should you wish to distribute
	a combined work that includes FreeRTOS, without being obliged to provide
	the source code for any proprietary components.  See the licensing section
	of http://www.FreeRTOS.org for full details of how and when the exception
	can be applied.

	***************************************************************************
	See http://www.FreeRTOS.org for documentation, latest information, license
	and contact details.  Please ensure to read the configuration and relevant
	port sections of the online documentation.
	***************************************************************************
*/

/*
	Implements a simplistic WEB server.  Every time a connection is made and
	data is received a dynamic page that shows the current TCP/IP statistics
	is generated and returned.  The connection is then closed.

	This file was adapted from a FreeRTOS lwIP slip demo supplied by a third
	party.
*/

/*
	Changes from V3.2.2

	+ Changed the page returned by the lwIP WEB server demo to display the 
	  task status table rather than the TCP/IP statistics.
*/

/* MakingThings:  This is a sample file using LWIP and FreeRTOS directly
  to implement a Web Server. 
  
  We have extended it slightly to make it a bit more pretty and to add
  some additional functions to it */

#include "config.h" // MakingThings.
#ifdef MAKE_CTRL_NETWORK

/* Standard includes. */
#include <stdio.h>
#include <string.h>

#include "network.h"

#define REQUEST_SIZE_MAX	100
#define RESPONSE_SIZE_MAX	1548
#define HTTP_OK	"HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n"
#define HTTP_PORT		( 80 )

#define HTML_START_RELOAD( what ) \
"<html>\
<head>\
<style type=\"text/css\">\
<!--\
body {\
	font-family: Arial, Helvetica, sans-serif;\
}\
-->\
</style>\
</head>\
<BODY onLoad=\"window.setTimeout(&quot;location.href='" what "'&quot;,1000)\"bgcolor=\"#eeeeee\">\
<br>Make Magazine - MakingThings<br>MAKE Controller Kit<br><br>Page Hits "

#define HTML_END \
"\r\n</pre>\
\r\n</BODY>\
</html>"

void WebServerTask( void *p );

typedef struct WebServerS
{
  int hits;
  void* serverTask;
  void* serverSocket;
  void* requestSocket;

  char request[ REQUEST_SIZE_MAX ];
  char response[ RESPONSE_SIZE_MAX ];
} WebServer_;

WebServer_* WebServer = NULL;

#include "webserver.h"
#include "analogin.h"
#include "system.h"
#include "FreeRTOS.h"

/* 
 * Process an incoming connection on port 80.
 *
 * This simply checks to see if the incoming data contains a GET request, and
 * if so sends back a single dynamically created page.  The connection is then
 * closed.  A more complete implementation could create a task for each 
 * connection. 
 */

void WebServer_OldSchool( void *requestSocket );

void WebServer_ProcessRequest( void* requestSocket )
{
  SocketRead( requestSocket, WebServer->request, REQUEST_SIZE_MAX ); 
  if( !strncmp( WebServer->request, "GET", 3 ) ) 
  {
    SocketWrite( requestSocket, HTTP_OK, strlen( HTTP_OK ) );
    WebServer_OldSchool( requestSocket ); 
  }
  if( !strncmp( WebServer->request, "PUT", 3 ) ) 
  {
    SocketWrite( requestSocket, HTTP_OK, strlen( HTTP_OK ) );
    SocketWrite( requestSocket, "GET: HELLO", 10 );
  }
  SocketClose( requestSocket );
}

void WebServer_OldSchool( void *requestSocket )
{
  char temp[ 100 ];

  // Generate the dynamic page...
  strcpy( WebServer->response, HTML_START_RELOAD( "/" ) );  // ... First the page header.
  // ... Then the hit count...
  snprintf( temp, 100, "%d", WebServer->hits );
  strcat( WebServer->response, temp );
  strcat( WebServer->response, "<p>Version: " );
  snprintf( temp, 100, "%s %d.%d.%d", FIRMWARE_NAME, FIRMWARE_MAJOR_VERSION, FIRMWARE_MINOR_VERSION, FIRMWARE_BUILD_NUMBER ); 
  strcat( WebServer->response, temp );
  strcat( WebServer->response, "<p>Free Memory " );
  sprintf( temp, "%d", System_GetFreeMemory() ); 
  strcat( WebServer->response, temp );
  strcat( WebServer->response, "</p>" );
  strcat( WebServer->response, "<p>Tasks Currently Running" );
  strcat( WebServer->response, "<p><pre>Task          State  Priority  Stck Rem	#<br>-------------------------------------------" );
  // ... Then the list of tasks and their status... 
  vTaskList( (signed portCHAR*)WebServer->response + strlen( WebServer->response ) );	
  int i;
  strcat( WebServer->response, "<p><pre>Analog Inputs<br>--------------<br>" );
  
  for ( i = 0; i < 8; i++ )
  {
    char b[ 20 ];
    snprintf( b, 20, "%d: %d<br>", i, AnalogIn_GetValue( i ) );
    strcat( WebServer->response, b );
  }
  strcat( WebServer->response, "</pre>" );
  
  // ... Finally the page footer.
  strcat( WebServer->response, HTML_END );
  // Write out the dynamically generated page.
  
  SocketWrite( requestSocket, WebServer->response, strlen( WebServer->response ) );
}


/*
  WebServer->RxBuffer = netconn_recv( NetConn ); // We expect to immediately get data.

	if( Server->RxBuffer != NULL )
	{
		netbuf_data( Server->RxBuffer, (void*)&Server->RxString, &Server->Length );  // Where is the data?
	
		if( !strncmp( Server->RxString, "GET", 3 ) ) // Is this a GET?  We don't handle anything else. 
		{
			Server->RxString = Server->DynamicPage;
			Server->PageHits++; // Update the hit count.
			sprintf( Server->PageHitsBuf, "%lu", Server->PageHits );
      netconn_write( NetConn, HTTP_OK, (u16_t)strlen( HTTP_OK ), NETCONN_COPY ); // Write out the HTTP OK header.

		}
 
		netbuf_delete( Server->RxBuffer );
	}

	netconn_close( NetConn );
*/



void WebServer_Start( )
{
  if ( WebServer == NULL )
  {
    WebServer = Malloc( sizeof( WebServer_ ) );    
    if ( WebServer == NULL )
      return;
    WebServer->hits = 0;
    WebServer->serverSocket = NULL;
    WebServer->requestSocket = NULL;
    WebServer->serverTask = TaskCreate( WebServerTask, "WebServ", 600, NULL, 4 );
    if ( WebServer->serverTask == NULL )
      Free( WebServer );
  }
}

void WebServer_Stop( )
{
  if( WebServer != NULL )
  {
    TaskDelete( WebServer->serverTask );
    ServerSocketClose( WebServer->serverSocket );
    if ( WebServer->requestSocket != NULL )
      SocketClose( WebServer->requestSocket );

    Free( WebServer );
    WebServer = NULL;
  }
}

void WebServerTask( void *p )
{
  // Try to create a socket on the appropriate port
  while ( WebServer->serverSocket == NULL )
  { 
    WebServer->serverSocket = ServerSocket( HTTP_PORT );
    if ( WebServer->serverSocket == NULL )
      Sleep( 1000 );
  }

  while( 1 )
	{
		/* Wait for connection. */
		WebServer->requestSocket = ServerSocketAccept( WebServer->serverSocket );
    
		if ( WebServer->requestSocket != NULL )
		{
      WebServer->hits++;
      WebServer_ProcessRequest( WebServer->requestSocket );
      WebServer->requestSocket = NULL;
		}

    Sleep( 5 );
	}
}

#endif