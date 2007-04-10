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

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Demo includes. */
#include "BasicWEB.h"
#include "SAM7_EMAC.h"

/* lwIP includes. */
#include "lwip/api.h" 
#include "lwip/tcpip.h"
#include "lwip/memp.h" 
#include "lwip/stats.h"
#include "netif/loopif.h"

#include "analogin.h"
#include "system.h"

/* The size of the buffer in which the dynamic WEB page is created. */
#define webMAX_PAGE_SIZE	1548

/* Standard GET response. */
#define webHTTP_OK	"HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n"

/* The port on which we listen. */
#define webHTTP_PORT		( 80 )

/* Delay on close error. */
#define webSHORT_DELAY		( 10 )

/* Format of the dynamic page that is returned on each connection. */
#define webHTML_START \
"<html>\
<head>\
</head>\
<BODY onLoad=\"window.setTimeout(&quot;location.href='index.html'&quot;,1000)\"bgcolor=\"#eeeeee\">\
<br>Make Magazine - MakingThings<br>MAKE Controller Kit<br><br>Page Hits "

#define webHTML_END \
"\r\n</pre>\
\r\n</BODY>\
</html>"

struct Web_Server
{
  portCHAR cDynamicPage[ webMAX_PAGE_SIZE ];
  portCHAR cPageHits[ 11 ];
  struct netbuf* pxRxBuffer;
  portCHAR* pcRxString;
  unsigned portSHORT usLength;
  unsigned portLONG ulPageHits;
};

struct Web_Server* Server;

/*------------------------------------------------------------*/

/* 
 * Process an incoming connection on port 80.
 *
 * This simply checks to see if the incoming data contains a GET request, and
 * if so sends back a single dynamically created page.  The connection is then
 * closed.  A more complete implementation could create a task for each 
 * connection. 
 */
static void vProcessConnection( struct netconn *pxNetCon );

/*------------------------------------------------------------*/

static void vProcessConnection( struct netconn *pxNetCon )
{
	/* We expect to immediately get data. */
	Server->pxRxBuffer = netconn_recv( pxNetCon );

	if( Server->pxRxBuffer != NULL )
	{
		/* Where is the data? */
		netbuf_data( Server->pxRxBuffer, (void*)&Server->pcRxString, &Server->usLength );	   
	
		/* Is this a GET?  We don't handle anything else. */
		if( !strncmp( Server->pcRxString, "GET", 3 ) )
		{
			Server->pcRxString = Server->cDynamicPage;

			/* Update the hit count. */
			Server->ulPageHits++;
			sprintf( Server->cPageHits, "%lu", Server->ulPageHits );

			/* Write out the HTTP OK header. */
      netconn_write( pxNetCon, webHTTP_OK, (u16_t)strlen( webHTTP_OK ), NETCONN_COPY );

			/* Generate the dynamic page...

			... First the page header. */
			strcpy( Server->cDynamicPage, webHTML_START );
			/* ... Then the hit count... */
			strcat( Server->cDynamicPage, Server->cPageHits );
			strcat( Server->cDynamicPage, "<p>Version.Build " );
	    sprintf( Server->cPageHits, "%d.%d", System_GetVersionNumber(), System_GetBuildNumber() ); 
      strcat( Server->cDynamicPage, Server->cPageHits );
      strcat( Server->cDynamicPage, "<p>Free Memory " );
	    sprintf( Server->cPageHits, "%d", System_GetFreeMemory() ); 
      strcat( Server->cDynamicPage, Server->cPageHits );
      strcat( Server->cDynamicPage, "</p>" );

			strcat( Server->cDynamicPage, "<p>Tasks Currently Running" );
			strcat( Server->cDynamicPage, "<p><pre>Task          State  Priority  Stck Rem	#<br>-------------------------------------------" );
			/* ... Then the list of tasks and their status... */
			vTaskList( ( signed portCHAR * ) Server->cDynamicPage + strlen( Server->cDynamicPage ) );	
      int i;

      strcat( Server->cDynamicPage, "<p><pre>Inputs<br>--------------<br>" );

      for ( i = 0; i < 8; i++ )
      {
        char b[ 20 ];
        snprintf( b, 20, "%d: %4d<br>", i, AnalogIn_GetValue( i ) );
        strcat( Server->cDynamicPage, b );
      }

      strcat( Server->cDynamicPage, "</pre>" );

      /* ... Finally the page footer. */
			strcat( Server->cDynamicPage, webHTML_END );

			/* Write out the dynamically generated page. */
			netconn_write(pxNetCon, Server->cDynamicPage, (u16_t)strlen( Server->cDynamicPage ), NETCONN_COPY );
		}
 
		netbuf_delete( Server->pxRxBuffer );
	}

	netconn_close( pxNetCon );
}
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/

void vBasicWEBServer( void *pvParameters )
{
  Server = Malloc( sizeof( struct Web_Server ) );
  while( Server == NULL )
  {
    Malloc( sizeof( struct Web_Server ) );
    Sleep( 100 );
  }
  // init
  Server->ulPageHits = 0;

  struct netconn *pxHTTPListener, *pxNewConnection;

  //moreInit();

    /* Parameters are not used - suppress compiler error. */
  ( void ) pvParameters;
	/* Create a new tcp connection handle */

 	pxHTTPListener = netconn_new( NETCONN_TCP );
	netconn_bind(pxHTTPListener, NULL, webHTTP_PORT );
	netconn_listen( pxHTTPListener );

	/* Loop forever */
	for( ;; )
	{
		/* Wait for connection. */
		pxNewConnection = netconn_accept(pxHTTPListener);

		if(pxNewConnection != NULL)
		{
			/* Service connection. */
			vProcessConnection( pxNewConnection );
			while( netconn_delete( pxNewConnection ) != ERR_OK )
			{
				vTaskDelay( webSHORT_DELAY );
			}
		}
	}
}


