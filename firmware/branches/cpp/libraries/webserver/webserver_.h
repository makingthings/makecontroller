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

#ifndef WEB_SERVER__H
#define WEB_SERVER__H

#include "string.h"
#include "stdio.h"
#include "rtos_.h"
#include "tcpserver.h"
#include "tcpsocket.h"

#define MAX_FORM_ELEMENTS 10
#define MAX_WEB_HANDLERS  5
#define REQUEST_SIZE_MAX  256
#define RESPONSE_SIZE_MAX 1000

enum HttpMethod {HTTP_GET, HTTP_PUT, HTTP_POST, HTTP_DELETE};

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

class WebHandler
{
  public:
    virtual const char* address() { return ""; }
    virtual bool get( char* path );
    virtual bool post( char* path, char* body, int len );
    virtual bool put( char* path, char* body, int len );
    virtual bool del( char* path );
    virtual ~WebHandler( ) { }
};

class WebServer
{
  public:
    static WebServer* get();
    int route();
    bool setListenPort(int port);
    int getListenPort();
    void sendResponse();
  
  protected:
    WebServer( );
    virtual ~WebServer( ) { }
    static WebServer* _instance; // the only instance of WebServer anywhere.
    friend void webServerLoop( void *parameters );
    Task* webServerTask;
    TcpServer tcpServer;
    int listenPort, newListenPort, hits, handler_count;
    char requestBuf[ REQUEST_SIZE_MAX ];
    char responseBuf[ RESPONSE_SIZE_MAX ];

    virtual void processRequest( TcpSocket* request, HttpMethod method, char* path );
    WebHandler* handlers[MAX_WEB_HANDLERS];
    char* getRequestAddress( char* request, int length, HttpMethod* method );
    int getBody( TcpSocket* socket, char* requestBuffer, int maxSize );
};

#endif  // WEB_SERVER_H

