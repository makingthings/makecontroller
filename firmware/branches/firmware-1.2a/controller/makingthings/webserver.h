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

#ifndef BASIC_WEB_SERVER_H
#define BASIC_WEB_SERVER_H

#define MAX_WEBPAGE_SIZE	1548
#define HTTP_OK	"HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n"
#define HTTP_PORT		( 80 )

#define HTML_START \
"<html>\
<head>\
</head>\
<BODY onLoad=\"window.setTimeout(&quot;location.href='index.html'&quot;,1000)\"bgcolor=\"#eeeeee\">\
<br>Make Magazine - MakingThings<br>MAKE Controller Kit<br><br>Page Hits "

#define HTML_END \
"\r\n</pre>\
\r\n</BODY>\
</html>"

struct Web_Server
{
  portCHAR DynamicPage[ MAX_WEBPAGE_SIZE ];
  portCHAR PageHitsBuf[ 11 ];
  struct netbuf* RxBuffer;
  portCHAR* RxString;
  unsigned portSHORT Length;
  unsigned portLONG PageHits;
  struct netconn* HTTPListener;
  struct netconn* NewConnection;
};

void WebServer( void *p );
void CloseWebServer( void );

#endif  // BASIC_WEB_SERVER_H

