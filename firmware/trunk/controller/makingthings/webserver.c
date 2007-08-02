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

#define HANDLERS_MAX        5
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
} \
h1 { \
	font-family: Arial, Helvetica, sans-serif; \
	font-weight: bold; \
}\
-->\
</style>\
</head>\
<BODY onLoad=\"window.setTimeout(&quot;location.href='" what "'&quot;,1000)\"bgcolor=\"#eeeeee\">\
<br>Make Magazine - MakingThings<br><h1>MAKE Controller Kit</h1>Page Hits "

#define HTML_END \
"\r\n</pre>\
\r\n</BODY>\
</html>"

void WebServerTask( void *p );

typedef struct WebServerHandlerS
{
  char* address;
  int (*handler)( char* requestType, char* address, void* socket ); 
} WebServerHandler;

typedef struct WebServerHandlersS
{
  int count;
  WebServerHandler handlers[ HANDLERS_MAX ];
} WebServerHandlers_;

WebServerHandlers_* WebServerHandlers = NULL;

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
#include "task.h"

/* 
 * Process an incoming connection on port 80.
 *
 * This simply checks to see if the incoming data contains a GET request, and
 * if so sends back a single dynamically created page.  The connection is then
 * closed.  A more complete implementation could create a task for each 
 * connection. 
 */

void WebServer_OldSchool( void *requestSocket, char* address );
void WebServer_ProcessRequest( void* requestSocket );
char* WebServer_GetRequestAddress( char* request, int length, char** requestType );

int WebServer_Route( char* address, int (*handler)( char* requestType, char* address, void* socket )  )
{
  if ( WebServerHandlers == NULL )
  {
    WebServerHandlers = Malloc( sizeof( WebServerHandlers_ ) );    
    if ( WebServerHandlers != NULL )
      WebServerHandlers->count = 0;
  }
  if ( WebServerHandlers != NULL )
  {
    if ( WebServerHandlers->count < HANDLERS_MAX )
    {
      WebServerHandler* hp = &WebServerHandlers->handlers[ WebServerHandlers->count++ ];
      hp->address = address;
      hp->handler = handler;
      return CONTROLLER_OK;
    }
   return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;
  }

  return CONTROLLER_ERROR_NOT_OPEN;
}


void WebServer_ProcessRequest( void* requestSocket )
{
  char* requestType = 0;
  char* address = 0;
  int i;
  int responded;

  SocketRead( requestSocket, WebServer->request, REQUEST_SIZE_MAX ); 
  address = WebServer_GetRequestAddress( WebServer->request, REQUEST_SIZE_MAX, &requestType );

  SocketWrite( requestSocket, HTTP_OK, strlen( HTTP_OK ) );

  responded = false;
  if ( WebServerHandlers != NULL )
  {
    for ( i = 0; i < WebServerHandlers->count; i++ )
    {
      WebServerHandler* hp = &WebServerHandlers->handlers[ i ];
      if ( strncmp( hp->address, address, strlen( hp->address ) ) == 0 )
      {
        responded = (*hp->handler)( requestType, address, requestSocket );
        if ( responded )
          break;
      }
    }
  }

  if( !responded ) 
  {
    WebServer_OldSchool( requestSocket, address ); 
  }

  SocketClose( requestSocket );
}

void WebServer_OldSchool( void *requestSocket, char* address )
{
  char temp[ 100 ];
  #ifdef AUTOCHECK
  memset( temp, 0, 100 );
  #endif

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
  strcat( WebServer->response, "<p><pre>Task          State  Priority  StackRem	#<br>-------------------------------------------" );
  // ... Then the list of tasks and their status... 
  vTaskList( (signed portCHAR*)WebServer->response + strlen( WebServer->response ) );	
  int i;
  strcat( WebServer->response, "<p><pre>Analog Inputs<br>--------------<br>" );
  
  for ( i = 0; i < 8; i++ )
  {
    char b[ 20 ];
    #ifdef AUTOCHECK
    memset( b, 0, 20 );
    #endif
    snprintf( b, 20, "%d: %d<br>", i, AnalogIn_GetValue( i ) );
    strcat( WebServer->response, b );
  }
  strcat( WebServer->response, "</pre>" );
  
  // ... Finally the page footer.
  strcat( WebServer->response, HTML_END );
  // Write out the dynamically generated page.
  
  SocketWrite( requestSocket, WebServer->response, strlen( WebServer->response ) );
}

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

int  WebServer_Running( void )
{
  return WebServer != NULL;
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

char* WebServer_GetRequestAddress( char* request, int length, char** requestType  )
{
  char *last = request + length;
  *requestType = NULL;
  char* address = NULL;

  // Skip any initial spaces
  while( *request == ' ' )
    request++;

  if ( request > last )
    return address;

  *requestType = request;

  // Skip the request type
  while ( *request != ' ' )
    request++;

  if ( request > last )
    return address;

  // Skip any subsequent spaces
  while( *request == ' ' )
    request++;

  if ( request > last )
    return address;

  return request;
}

#endif

