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

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "string.h"
#include "stdio.h"

#define MAX_FORM_ELEMENTS 10

/**
  A structure that represents a key-value pair in an HTML form.
  This structure only points at the data received by the web server - it does not copy it.  So be sure
  to note that this structure becomes invalid as soon as the data used to create is gone.
  \ingroup webserver
*/
typedef struct
{
  char *key;   /**< A pointer to the key of this element. */ 
  char *value; /**< A pointer to the value of this element. */
} HtmlFormElement;

/**
  A structure that represents a collection of HtmlFormElement structures.
  If you need a larger form, you can adjust \b MAX_FORM_ELEMENTS in webserver.h it accommodates 10 by default.
  \ingroup webserver
*/
typedef struct
{
  HtmlFormElement elements[MAX_FORM_ELEMENTS]; /**< An array of form elements. */
  int count;                                   /**< The number of form elements contained in this form. */
} HtmlForm;

// Web Server Task
int WebServer_SetActive( int active );
int WebServer_GetActive( void );
int WebServer_SetListenPort( int port );
int WebServer_GetListenPort( void );

int WebServer_Route( char* address, int (*handler)( char* requestType, char* request, char* requestBuffer, int request_maxsize, void* socket, char* responseBuffer, int len )  );

// HTTP Helpers
int WebServer_WriteResponseOkHTML( void* socket );
int WebServer_WriteResponseOkPlain( void* socket );

// HTML Helpers
int WebServer_WriteHeader( int includeCSS, void* socket, char* buffer, int len );
int WebServer_WriteBodyStart( char* reloadAddress, void* socket, char* buffer, int len );
int WebServer_WriteBodyEnd( void* socket );

bool WebServer_GetPostData( void *socket, char *requestBuffer, int maxSize );
int WebServer_ParseFormElements( char *request, HtmlForm *form );

// OSC interface
const char* WebServerOsc_GetName( void );
int WebServerOsc_ReceiveMessage( int channel, char* message, int length );
int WebServerOsc_Async( int channel );

#endif  // WEB_SERVER_H

