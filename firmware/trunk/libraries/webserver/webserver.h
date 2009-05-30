/*********************************************************************************

 Copyright 2006-2009 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "config.h"
#ifdef MAKE_CTRL_NETWORK

#include "string.h"
#include "stdio.h"
#include "rtos.h"
#include "tcpserver.h"
#include "tcpsocket.h"
#include "http.h"

#define MAX_FORM_ELEMENTS 5
#define MAX_WEB_HANDLERS 5
#define REQUEST_SIZE_MAX 256

/**
  A structure that represents a key-value pair in an HTML form.
  This structure only points at the data received by the web server - it does not copy it.  So be sure
  to note that this structure becomes invalid as soon as the data used to create is gone.
  \ingroup Network
*/
typedef struct
{
  char *key;   /**< A pointer to the key of this element. */ 
  char *value; /**< A pointer to the value of this element. */
} HtmlFormElement;

/**
  A structure that represents a collection of HtmlFormElement structures.
  If you need a larger form, you can adjust \b MAX_FORM_ELEMENTS in webserver.h it accommodates 10 by default.
  \ingroup Network
*/
typedef struct
{
  HtmlFormElement elements[MAX_FORM_ELEMENTS]; /**< An array of form elements. */
  int count;                                   /**< The number of form elements contained in this form. */
} HtmlForm;

/**
  Helper class to respond to WebServer requests.
  To respond to requests from the WebSever, subclass WebResponder and re-implement the 
  methods need for your application.
  
  \section Usage
  The WebServer will call your responder when it receives a request whose first element 
  matches the name returned by address().  So if the request looked like <b>192.168.0.200/monkey/yak/tuba</b>
  the WebServer will call the WebResponder whose address() method returns "monkey".
  
  Depending on the type of request, the WebServer will call one of 4 methods, get(), put(), post(), del(),
  which correspond to the main HTTP verbs.  Web browsers send GET requests when they ask to view 
  a page, so that's the most common type, but other applications may require the other methods as well.
  
  To respond, your first call should be to setResponseCode() and then 1 or more calls to addHeader().  
  After that, write out whatever data you like in the body via the \ref response socket and you're all set.  
  Remember that you can write it out a chunk at a time - you don't need to send it all at once.  
  If you have responded to a request, return true from your handler and the WebServer will stop 
  processing the request.  Otherwise, it will continue to look for other responders.
  
  For more info about HTTP, check the Wikipedia article - 
  
  Here's a very simple example of a class that inherits from WebResponder and get respond to HTTP
  GET requests:
  \code
  class MyResponder : public WebResponder // inherit from WebResponder
  {
    public:
      const char* address()
      {
        return "myresponder"; // match all requests that start with "/myresponder"
      }
      // in this simple example, we only ever care about HTTP GETs,
      bool get( char* path ) // so that's the only one we implement
      {
        setResponseCode(200); // everything's OK
        addHeader("Content-type", "text/plain"); // just sending some text back
        if( strcmp(path, "/myresponder/test") == 0 ) // check the path to see what was requested
          response->write("here's a response", strlen("here's a response")); // the actual text
        else
          response->write("unknown path", strlen("unknown path"));
        return true; // indicate that we responded and 
      }
  };
  \endcode
*/
class WebHandler
{
  public:
    /**
      The top level element that your responder matches.
    */
    virtual const char* address() = 0;
    virtual bool get( TcpSocket* client, char* path );
    virtual bool post( TcpSocket* client, char* path, char* body, int len );
    virtual bool put( TcpSocket* client, char* path, char* body, int len );
    virtual bool del( TcpSocket* client, char* path );
    virtual ~WebHandler( ) { }

  protected:
    bool setResponseCode( TcpSocket* client, int code );
//    bool addHeader( const char* type, const char* value, bool lastone = true );
};

/**
  A simple webserver.
  Also see \ref WebResponder.
  \ingroup networking
*/
class WebServer
{
  public:
    static WebServer* get();
    bool route(WebHandler* handler);
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

    virtual void processRequest( TcpSocket* request, HttpMethod method, char* path );
    WebHandler* handlers[MAX_WEB_HANDLERS];
    char* getRequestAddress( char* request, int length, HttpMethod* method );
    int getBody( TcpSocket* socket, char* requestBuffer, int maxSize );
};

#endif // MAKE_CTRL_NETWORK

#endif  // WEB_SERVER_H

