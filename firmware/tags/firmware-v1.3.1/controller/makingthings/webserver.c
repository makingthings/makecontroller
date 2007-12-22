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


/** \file webserver.c	
	Functions for implementing a WebServer on the Make Controller Board.
*/

#include "config.h" // MakingThings.
#ifdef MAKE_CTRL_NETWORK

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "network.h"

#define HANDLERS_MAX        5
#define REQUEST_SIZE_MAX	256
#define RESPONSE_SIZE_MAX	1000
#define HTTP_OK	"HTTP/1.0 200 OK\r\nContent-type: "
#define HTTP_CONTENT_HTML "text/html\r\n\r\n"
#define HTTP_CONTENT_PLAIN "text/plain\r\n\r\n"
#define HTTP_PORT		( 80 )

#define HTML_OS_START \
"<BR>Make Magazine - MakingThings<BR><H1>MAKE Controller Kit</H1>Page Hits "

void WebServerTask( void *p );

typedef struct WebServerHandlerS
{
  char* address;
  int (*handler)( char* requestType, char* address,  char* requestBuffer, int requestMaxSize, void* socket, char* buffer, int len ); 
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
  void *serverSocket;
  void *requestSocket;

  char request[ REQUEST_SIZE_MAX ];
  char response[ RESPONSE_SIZE_MAX ];
} WebServer_;

WebServer_* WebServer = NULL;

#include "webserver.h"
#include "analogin.h"
#include "system.h"

#include "FreeRTOS.h"
#include "task.h"

int TestHandler( char* requestType, char* address, char* requestBuffer, int requestMaxSize, void* socket, char* buffer, int len );

static int WebServer_WriteResponseOk_( char* content, void* socket );


/** \defgroup webserver Web Server
  Very simple Web Server

  This Web Server implementation is based on the ServerSocket and Socket functions defined
  in \ref Sockets.  When started (usually by the Network subsystem), a ServerSocket is opened
  and set to listen on port 80.

  When the server receives a request, it checks the incoming address against a list of handlers.
  If the address matches, the handler is invoked and checking stops.  Users can add their own
  handlers to return custom information.  Handlers persist even when the server is deactivated.

  There are two default handlers.  One is mounted at "/test" and will reply with the message "Test"
  and a current hit count.  This handler will only be mounted if no other handlers are present when
  the server is started up.  The other handler is a default handler which will match any address not 
  already matched.  This is the standard information page showing free memory, running tasks, etc.  This
  page will reload every second from "/info".

	\ingroup Controller
	@{
*/

/**
	Set the active state of the WebServer subsystem.  This is automatically set to true
  by the Network Subsystem as it activates if the /network/webserver property is 
  set to true.  If there are no specifed handlers at the time of initialization,
  the default test handler is installed.
	@param active An integer specifying the active state - 1 (on) or 0 (off).
	@return CONTROLLER_OK (0) on success or the appropriate error if not
*/
int WebServer_SetActive( int active )
{
  if ( active )
  {
    if ( WebServer == NULL )
    {
      WebServer = MallocWait( sizeof( WebServer_ ), 100 );    
      WebServer->hits = 0;
      WebServer->serverSocket = NULL;
      WebServer->requestSocket = NULL;
      #ifdef CROSSWORKS_BUILD
      WebServer->serverTask = TaskCreate( WebServerTask, "WebServ", 800, NULL, 4 );
      #else
      WebServer->serverTask = TaskCreate( WebServerTask, "WebServ", 1800, NULL, 4 );
      #endif // CROSSWORKS_BUILD
      if ( WebServer->serverTask == NULL )
      {
        Free( WebServer );
        WebServer = NULL;
        return CONTROLLER_ERROR_CANT_START_TASK;
      }
  
      // Test the routing system
      if ( WebServerHandlers == NULL )
        WebServer_Route( "/test", TestHandler );
    }
  }
  else
  {
    if( WebServer != NULL )
    {
      if ( WebServer->serverTask != NULL )
        TaskDelete( WebServer->serverTask );
      if ( WebServer->serverSocket != NULL )
        ServerSocketClose( WebServer->serverSocket );
      if ( WebServer->requestSocket != NULL )
        SocketClose( WebServer->requestSocket );
  
      Free( WebServer );
      WebServer = NULL;
    }
  }
  return CONTROLLER_OK;
}

/**
	Read the active state of the WebServer subsystem.
	@return State - 1/non-zero (on) or 0 (off).
*/
int  WebServer_GetActive( void )
{
  return ( WebServer != NULL ) ? 1 : 0;
}

/**
	Adds a route handler to the WebServer.

  This function binds an address fragment (e.g. "/", "/adcs", etc.) to a handler
  that will be called when the address matches.  Matching ignores characters in the 
  incoming address that are beyond the end of the address specified to this function.
  The first function to match the address will receive all the traffic and no more 
  handlers will be checked.  Thus if a handler is set up to match "/images" it will match
  "/images" and "/images/logo.png" and so on.  Also if there is a subseqent handler 
  set to match "/images/diagram" it will never be called since the prior handler
  will match the entire "/images" space.

  The handler will be called with the request type specified (usually "GET" or  "PUT"), 
  the incoming address ( "/device/0", "/images/logo.png", etc.) A buffer (and max length) is passed in that
  can be used to receive the rest of the message if necessary. Then there will be 
  the socket which will take the response and a helpful large buffer or specified length
  which can be used to build strings.

  At the time the handler is called, only the first line of the request has been read.  It is used
  to determing the request type and the address.  If you need to process the request further
  its contents may be read into a buffer (the request buffer passed in might be good once the request type 
  address have been used since they're in there initially).  It is suggested that you use the 
  SocketReadLine( ) function to read a line of the request at a time.

  The handler itself must first write the response using one of WebServer_WriteResponseOkHTML for
  sending HTML or WebServer_WriteResponseOkPlain for returning plain text.  
  These send the appropriate HTTP header (for example "HTTP/1.0 200 OK\r\nContent-type:text/html\r\n\r\n").
  All responses may be simply written to the Socket, but several helpers are provided to assist in the 
  construction of simple web pages (for example WebServer_WriteHeader, WebServer_WriteBodyStart, etc.)

  Here is an example handler which is installed when the server is started if there are no handlers already present.
  \code
int TestHandler( char* requestType, char* address, char* requestBuffer, int requestMaxSize, void* socket, char* buffer, int len )
{
  (void)requestType;
  (void)address;
  WebServer_WriteResponseOkHTML( socket );
  WebServer_WriteHeader( true, socket, buffer, len );
  WebServer_WriteBodyStart( address, socket, buffer, len );
  snprintf( buffer, len, "<H1>TEST</H1>%d hits", WebServer->hits );
  SocketWrite( socket, buffer, strlen( buffer ) );
  WebServer_WriteBodyEnd( socket );
  return true;
}
  \endcode
	@param address An string specify the addresses to match.
	@param handler pointer to a handler function that will be called when the address is matched.
  @return CONTROLLER_OK (=0) on success or the appropriate error if not
*/
int WebServer_Route( char* address, int (*handler)( char* requestType, char* address, char* requestBuffer, int requestMaxSize, void* socket, char* buffer, int len )  )
{
  if ( WebServerHandlers == NULL )
  {
    WebServerHandlers = MallocWait( sizeof( WebServerHandlers_ ), 100 );    
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


/**
	Writes the HTTP OK message and sets the content type to HTML.
	@param socket The socket to write to
*/
int WebServer_WriteResponseOkHTML( void* socket )
{
  //return WebServer_WriteResponseOk_( socket, HTTP_CONTENT_HTML );
  int written = 0;
  int ret = 0;
  ret = SocketWrite( socket, HTTP_OK, strlen( HTTP_OK ) );
  if( !ret )
    return 0;
  else
    written += ret;

  ret = SocketWrite( socket, HTTP_CONTENT_HTML, strlen( HTTP_CONTENT_HTML ) );
  if( !ret )
    return 0;
  else
    written += ret;

  return written;
}
  
/**
	Writes the HTTP OK message and sets the content type to HTML.
	@param socket The socket to write to
*/
int WebServer_WriteResponseOkPlain( void* socket )
{
  return WebServer_WriteResponseOk_( socket, HTTP_CONTENT_PLAIN );
}

/**
	Writes the HTML header.
  Should be preceded by a call to WebServer_WriteResponseOkHTML
  @param includeCSS flag signalling the inclusion of a very simple CSS header refining header one and body text slightly.
	@param socket The socket to write to
	@param buffer Helper buffer which the function can use.  Should be at least 300 bytes.
	@param len Helper buffer length
  \todo more parameterization
*/
int WebServer_WriteHeader( int includeCSS, void* socket, char* buffer, int len )
{
  (void)buffer;
  (void)len;
  char* headerStart = "<HTML>\r\n<HEAD>";
  char* headerEnd = "\r\n</HEAD>";
  char* style = "\r\n<STYLE type=\"text/css\"><!--\
body { font-family: Arial, Helvetica, sans-serif; } \
h1 { font-family: Arial, Helvetica, sans-serif; font-weight: bold; }\
--></STYLE>";

  strcpy( buffer, headerStart );
  if ( includeCSS )
    strcat( buffer, style );
  strcat( buffer, headerEnd );
  return SocketWrite( socket, buffer, strlen( buffer ) );
}

/**
	Writes the start of the BODY tag.
  Should be preceded by a call to WebServer_WriteHeader.  Also writes a light grey background.
  @param reloadAddress string signalling the address of a 1s reload request.  If it is NULL, no reload is requested.
	@param socket The socket to write to
	@param buffer Helper buffer which the function can use.  Should be at least 300 bytes.
	@param len Helper buffer length
  \todo more parameterization of the tag
*/
int WebServer_WriteBodyStart( char* reloadAddress, void* socket, char* buffer, int len )
{
  (void)buffer;
  (void)len;
  char* bodyStart = "\r\n<BODY";
  char* bodyEnd = " bgcolor=\"#eeeeee\">\r\n";

  char* reloadStart = " onLoad=\"window.setTimeout(&quot;location.href='";
  char* reloadEnd = "'&quot;,1000)\"";
  strcpy( buffer, bodyStart );
  if ( reloadAddress )
  {
    strcat( buffer, reloadStart  );
    strcat( buffer, reloadAddress  );
    strcat( buffer, reloadEnd  );
  }
  strcat( buffer, bodyEnd  );
  return SocketWrite( socket, buffer, strlen( buffer ) );
}

/**
	Writes the end of the Body tag - and the final end of HTML tag.
  Should be preceded writes to the socket with the content of the page.
	@param socket The socket to write to
*/
int WebServer_WriteBodyEnd( void* socket )
{
  char* bodyEnd = "\r\n</BODY>\r\n</HTML>";

  return SocketWrite( socket, bodyEnd, strlen( bodyEnd ) );
}

/** @}
*/

int TestHandler( char* requestType, char* address, char* requestBuffer, int requestMaxSize, void* socket, char* buffer, int len )
{
  (void)requestType;
  (void)address;
  (void)requestBuffer;
  (void)requestMaxSize;
  WebServer_WriteResponseOkHTML( socket );
  WebServer_WriteHeader( true, socket, buffer, len );
  WebServer_WriteBodyStart( address, socket, buffer, len );
  snprintf( buffer, len, "<H1>TEST</H1>%d hits", WebServer->hits );
  SocketWrite( socket, buffer, strlen( buffer ) );
  WebServer_WriteBodyEnd( socket );
  return true;
}

void WebServer_OldSchool( char* address, void *requestSocket, char* buffer, int len );
void WebServer_ProcessRequest( void* requestSocket );
char* WebServer_GetRequestAddress( char* request, int length, char** requestType );

void WebServer_ProcessRequest( void* requestSocket )
{
  char* requestType = 0;
  char* address = 0;
  int i;
  int responded;

  SocketReadLine( requestSocket, WebServer->request, REQUEST_SIZE_MAX ); 
  address = WebServer_GetRequestAddress( WebServer->request, REQUEST_SIZE_MAX, &requestType );

  responded = false;
  if ( WebServerHandlers != NULL )
  {
    for ( i = 0; i < WebServerHandlers->count; i++ )
    {
      WebServerHandler* hp = &WebServerHandlers->handlers[ i ];
      if ( strncmp( hp->address, address, strlen( hp->address ) ) == 0 )
      {
        responded = (*hp->handler)( requestType, address, WebServer->request, REQUEST_SIZE_MAX, requestSocket, WebServer->response, RESPONSE_SIZE_MAX );
        if ( responded )
          break;
      }
    }
  }

  if( !responded ) 
    WebServer_OldSchool( address, requestSocket, WebServer->response, RESPONSE_SIZE_MAX ); 

  SocketClose( requestSocket );
}

void WebServer_OldSchool( char* address, void *requestSocket, char* buffer, int len )
{
  (void)address;
  char temp[ 100 ];
  #ifdef AUTOCHECK
  memset( temp, 0, 50 );
  #endif

  if( !WebServer_WriteResponseOkHTML( requestSocket ) )
    return;

  if( !WebServer_WriteHeader( true, requestSocket, buffer, len ) )
    return;

  if( !WebServer_WriteBodyStart( "/info", requestSocket, buffer, len ) )
    return;

  // Generate the dynamic page...
  strcpy( buffer, HTML_OS_START );  // ... First the page header.
  // ... Then the hit count...
  snprintf( temp, 100, "%d", WebServer->hits );
  strcat( buffer, temp );
  strcat( buffer, "<p>Version: " );
  snprintf( temp, 100, "%s %d.%d.%d", FIRMWARE_NAME, FIRMWARE_MAJOR_VERSION, FIRMWARE_MINOR_VERSION, FIRMWARE_BUILD_NUMBER ); 
  strcat( buffer, temp );
  strcat( buffer, "<p>Free Memory " );
  sprintf( temp, "%d", System_GetFreeMemory() ); 
  strcat( buffer, temp );
  strcat( buffer, "</p>" );
  strcat( buffer, "<p>Tasks Currently Running" );
  strcat( buffer, "<p><pre>Task          State  Priority  StackRem	#<br>-------------------------------------------" );
  // ... Then the list of tasks and their status... 
  vTaskList( (signed portCHAR*)buffer + strlen( buffer ) );	
  
  int i;
  strcat( buffer, "<p><pre>Analog Inputs<br>--------------<br>" );
  
  for ( i = 0; i < 8; i++ )
  {
    char b[ 20 ];
    #ifdef AUTOCHECK
    memset( b, 0, 20 );
    #endif
    snprintf( b, 20, "%d: %d<br>", i, AnalogIn_GetValue( i ) );
    strcat( buffer, b );
  }
  strcat( buffer, "</pre>" );
  
  
  // Write out the dynamically generated page.
  if( !SocketWrite( requestSocket, buffer, strlen( buffer ) ) )
    return;
  
  WebServer_WriteBodyEnd( requestSocket );
}



void WebServerTask( void *p )
{
  (void)p;

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

static int WebServer_WriteResponseOk_( char* contentType, void* socket )
{
  int written = 0;
  written += SocketWrite( socket, HTTP_OK, strlen( HTTP_OK ) );
  written += SocketWrite( socket, contentType, strlen( contentType ) );
  return written;
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

  address = request;

  while( !isspace( *request ) )
    request++;
  
  if ( request < last )
    *request = 0;

  return address;
}

#endif

