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

/* Standard includes. */
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "webserver.h"
#include "analogin.h"
#include "system.h"

// lwIP includes. 
#include "lwip/api.h" 
#include "lwip/tcpip.h"
#include "lwip/memp.h" 
#include "lwip/stats.h"
#include "netif/loopif.h"

struct Web_Server* Server = NULL;
static void ProcessConnection( struct netconn* NetConn );

/* 
 * Process an incoming connection on port 80.
 *
 * This simply checks to see if the incoming data contains a GET request, and
 * if so sends back a single dynamically created page.  The connection is then
 * closed.  A more complete implementation could create a task for each 
 * connection. 
 */

static void ProcessConnection( struct netconn *NetConn )
{
	/* We expect to immediately get data. */
	Server->RxBuffer = netconn_recv( NetConn );

	if( Server->RxBuffer != NULL )
	{
		/* Where is the data? */
		netbuf_data( Server->RxBuffer, (void*)&Server->RxString, &Server->Length );
	
		/* Is this a GET?  We don't handle anything else. */
		if( !strncmp( Server->RxString, "GET", 3 ) )
		{
			Server->RxString = Server->DynamicPage;

			/* Update the hit count. */
			Server->PageHits++;
			sprintf( Server->PageHitsBuf, "%lu", Server->PageHits );

			/* Write out the HTTP OK header. */
      netconn_write( NetConn, HTTP_OK, (u16_t)strlen( HTTP_OK ), NETCONN_COPY );

			/* Generate the dynamic page...

			... First the page header. */
			strcpy( Server->DynamicPage, HTML_START );
			/* ... Then the hit count... */
			strcat( Server->DynamicPage, Server->PageHitsBuf );
			strcat( Server->DynamicPage, "<p>Version.Build " );
	    sprintf( Server->PageHitsBuf, "%d.%d", System_GetVersionNumber(), System_GetBuildNumber() ); 
      strcat( Server->DynamicPage, Server->PageHitsBuf );
      strcat( Server->DynamicPage, "<p>Free Memory " );
	    sprintf( Server->PageHitsBuf, "%d", System_GetFreeMemory() ); 
      strcat( Server->DynamicPage, Server->PageHitsBuf );
      strcat( Server->DynamicPage, "</p>" );

			strcat( Server->DynamicPage, "<p>Tasks Currently Running" );
			strcat( Server->DynamicPage, "<p><pre>Task          State  Priority  Stck Rem	#<br>-------------------------------------------" );
			/* ... Then the list of tasks and their status... */
			vTaskList( (signed portCHAR*)Server->DynamicPage + strlen( Server->DynamicPage ) );	
      int i;

      strcat( Server->DynamicPage, "<p><pre>Inputs<br>--------------<br>" );

      for ( i = 0; i < 8; i++ )
      {
        char b[ 20 ];
        snprintf( b, 20, "%d: %4d<br>", i, AnalogIn_GetValue( i ) );
        strcat( Server->DynamicPage, b );
      }

      strcat( Server->DynamicPage, "</pre>" );

      /* ... Finally the page footer. */
			strcat( Server->DynamicPage, HTML_END );

			/* Write out the dynamically generated page. */
			netconn_write(NetConn, Server->DynamicPage, (u16_t)strlen( Server->DynamicPage ), NETCONN_COPY );
		}
 
		netbuf_delete( Server->RxBuffer );
	}

	netconn_close( NetConn );
}

void WebServer( void *p )
{
  (void)p;
  while( Server == NULL )
  {
    Server = Malloc( sizeof( struct Web_Server ) );
    Sleep( 100 );
  }
  // init
  Server->PageHits = 0;
  Server->NewConnection = NULL;
 	Server->HTTPListener = netconn_new( NETCONN_TCP );
	netconn_bind(Server->HTTPListener, NULL, HTTP_PORT );
	netconn_listen( Server->HTTPListener );

  while( 1 )
	{
		/* Wait for connection. */
		Server->NewConnection = netconn_accept( Server->HTTPListener );

		if(Server->NewConnection != NULL)
		{
			ProcessConnection( Server->NewConnection );
			while( netconn_delete( Server->NewConnection ) != ERR_OK )
			{
				Sleep( 10 );
			}
		}
    Sleep( 5 );
	}
}

void CloseWebServer( )
{
  if( Server != NULL )
  {
    netconn_delete( Server->HTTPListener );

    if( Server->NewConnection != NULL )
      netconn_delete( Server->NewConnection );

    Free( Server );
    Server = NULL;
  }
}




