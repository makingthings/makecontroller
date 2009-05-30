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

#include "webserver.h"

#ifdef MAKE_CTRL_NETWORK

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void webServerLoop(void* parameters);
WebServer* WebServer::_instance = 0;

WebServer::WebServer()
{
  handler_count = hits = 0;
  listenPort = newListenPort = HTTP_PORT;
  webServerTask = new Task(webServerLoop, "WebServ", 800, 3, this);
}

WebServer* WebServer::get()
{
  if( !_instance )
    _instance = new WebServer();
  return _instance;
}

bool WebServer::route(WebHandler* handler)
{
  if( handler_count < MAX_WEB_HANDLERS )
  {
    handlers[handler_count++] = handler;
    return true;
  }
  else
    return false;
}

/*
  Wait for new connections (making sure we're listening on the right port)
  and then dispatch them to processRequest()
*/
void webServerLoop(void* parameters)
{
  WebServer* ws = (WebServer*)parameters;
  ws->tcpServer.listen(ws->listenPort);
  while( 1 )
  {
    if( ws->listenPort != ws->newListenPort )
    {
      ws->listenPort = ws->newListenPort;
      ws->tcpServer.listen(ws->listenPort);
    }

    TcpSocket* request = ws->tcpServer.accept( ); // Blocks waiting for connection
    if ( request != NULL )
    {
      ws->hits++;
      HttpMethod method;
      request->readLine( ws->requestBuf, REQUEST_SIZE_MAX );
      char* address = ws->getRequestAddress( ws->requestBuf, REQUEST_SIZE_MAX, &method );
      ws->processRequest( request, method, address );
      delete request;
    }
    Task::sleep( 5 );
  }
}

/**
  Handle a new web request.
  If you want to create a new webserver, you can inherit from WebServer and re-implement
  this function to provide new functionality.

  At this point, the request has had the first line read from it to determine the path and the method
  which are passed into you.  

  \b Example
  \code
  class MyWebServer : public WebServer
  {
    // ... other class definitions here ...
    void processRequest( TcpSocket* request, HttpMethod method, char* path );
  };
  \endcode
*/
void WebServer::processRequest( TcpSocket* request, HttpMethod method, char* path )
{
  bool responded = false;
  if ( handler_count )
  {
    int i;
    for ( i = 0; i < handler_count; i++ )
    {
      WebHandler* hp = handlers[ i ];
      const char* t = hp->address();
      if ( !strncmp( t, (path + 1), strlen( t ) ) )
      {
        switch(method)
        {
          case HTTP_GET:
            responded = hp->get(request, path);
            break;
          case HTTP_POST:
          {
            int content_len = getBody( request, requestBuf, REQUEST_SIZE_MAX );
            responded = hp->post(request, path, requestBuf, content_len);
            break;
          }
          case HTTP_PUT:
          {
            int content_len = getBody( request, requestBuf, REQUEST_SIZE_MAX );
            responded = hp->put(request, path, requestBuf, content_len);
            break;
          }
          case HTTP_DELETE:
            responded = hp->del(request, path);
            break;
          default:
            break;
        }
        if ( responded )
          break;
      }
    }
  }
//  if(!responded)
//    TestHandler(requestType, address, WebServer->request, REQUEST_SIZE_MAX, requestSocket, WebServer->response, RESPONSE_SIZE_MAX);
  
  request->close();
}

/*
  Extract the HTTP method for this request, and then return a pointer to the beginning
  of the URL path
*/
char* WebServer::getRequestAddress( char* request, int length, HttpMethod* method )
{
  char* last = request + length;
  char* address = NULL;

  // Skip any initial spaces
  while( *request == ' ' )
    request++;
  if ( request > last ) // make sure we didn't go too far
    return NULL;

  if( !strncmp(request, "GET", 3) )
    *method = HTTP_GET;
  else if( !strncmp(request, "POST", 4) )
    *method = HTTP_POST;
  else if( !strncmp(request, "PUT", 3) )
    *method = HTTP_PUT;
  else if( !strncmp(request, "DELETE", 6) )
    *method = HTTP_DELETE;

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
  
  // now terminate the end of the address so we can do stringy things on it
  while( !isspace( *request ) )
    request++;
  
  if ( request < last )
    *request = 0;

  return address;
}

int WebServer::getBody( TcpSocket* socket, char* requestBuffer, int maxSize )
{
  // keep reading lines of the HTTP header until we get CRLF which signifies the end of the
  // header and the start of the body data.  If we see the contentlength along the way, keep that.
  int contentLength = 0;
  int bufferLength = 0;
  while ( ( bufferLength = socket->readLine( requestBuffer, maxSize ) ) )
  {
    if ( strncmp( requestBuffer, "\r\n", 2 ) == 0 )
      break;
    if ( strncmp( requestBuffer, "Content-Length", 14 ) == 0 )
      contentLength = atoi( &requestBuffer[ 15 ] );
  }
  
  // now we should be down to the HTTP POST data
  // if there's any data, get up into it
  int bufferRead = 0;
  if ( contentLength > 0 && bufferLength > 0 )
  {  
    int lengthToRead;
    int avail = socket->bytesAvailable();
    if(avail > maxSize)
      lengthToRead = maxSize - 1;
    else
      lengthToRead = avail;
    char *rbp = requestBuffer;
    // read all that the socket has to offer...may come in chunks, so keep going until there's none left
    while ( ( bufferLength = socket->read( rbp, lengthToRead ) ) )
    {
      bufferRead += bufferLength;
      rbp += bufferLength;
      lengthToRead -= bufferLength;
      if ( bufferRead >= contentLength )
        break;
    }
    requestBuffer[ bufferRead ] = 0; // null-terminate the request
  }
  return bufferRead;
}

/**
  Method called when an HTTP GET request has arrived.
  Re-implement this in your class that inherits from WebHandler to
  provide the desired functionality.
  
  @param path This is the request path.  If the entire URL looked like
  192.168.0.200/first/second/34, then path is "/first/second/34"
*/
bool WebHandler::get( TcpSocket* client, char* path )
{
 (void)client;
 (void)path;
 return false;
}

/**
  Method called when an HTTP POST request has arrived.
  Re-implement this in your class that inherits from WebHandler to
  provide the desired functionality.
  
  @param path This is the request path.  If the entire URL looked like
  192.168.0.200/first/second/34, then path is "/first/second/34"
  @param body The body data of the POST request.
  @param len How many bytes of data are in the body.
*/
bool WebHandler::post( TcpSocket* client, char* path, char* body, int len )
{
  (void)client;
  (void)path;
  (void)body;
  (void)len;
  return false;
}

/**
  Method called when an HTTP PUT request has arrived.
  Re-implement this in your class that inherits from WebHandler to
  provide the desired functionality.
  
  @param path This is the request path.  If the entire URL looked like
  192.168.0.200/first/second/34, then path is "/first/second/34"
  @param body The body data of the PUT request.
  @param len How many bytes of data are in the body.
*/
bool WebHandler::put( TcpSocket* client, char* path, char* body, int len )
{
  (void)client;
  (void)path;
  (void)body;
  (void)len;
  return false;
}

/**
  Method called when an HTTP DELETE request has arrived.
  Re-implement this in your class that inherits from WebHandler to
  provide the desired functionality.
  
  @param path This is the request path.  If the entire URL looked like
  192.168.0.200/first/second/34, then path is "/first/second/34"
  
  \b Example
  \code
  
  \endcode
*/
bool WebHandler::del( TcpSocket* client, char* path )
{
 (void)client;
 (void)path;
 return false;
}

/**
  Set the code for your response.
  This is usually the first thing you want to do any of your handlers.  200
  is the code that generally means OK.  There are a variety of HTTP response codes
  to choose from, though - .
  
  @param code The code for your response.
  @return True if it was successfully written, otherwise false.
  
  \b Example
  \code
  bool MyResponder::get( char* path ) // method that gets called on an HTTP GET
  {
   setResponseCode(200); // indicate everything is OK
   return true;
  }
  \endcode
*/
bool WebHandler::setResponseCode( TcpSocket* client, int code )
{
  char temp[26];
  sprintf(temp, "HTTP/1.0 %d OK\r\n", code);
  int written = client->write(temp, strlen(temp));
  return (written != 0);
}

/**
  Add a header to your response.
  A variety of HTTP headers can provide more info about your response.  A common one
  is to specify the content type...could be 'text/plain' or 'text/html' for example.
  
  @param type The type of header to add.
  @param value The value for the header.
  @param lastone Whether this header is the last header you're adding.  This is true 
  by default, so if you're adding more than one header, be sure to set it false for all 
  calls before the last one.  This is so the headers can be properly separated from 
  the body of the reponse.
  
  \b Example
  \code
  bool MyResponder::get( char* path ) // method that gets called on an HTTP GET
  {
   setResponseCode(200); // indicate everything is OK
   addHeader("Something", "blah", false);
   addHeader("Content-type", "text/plain"); // indicate we're sending back plain text
                                            // and that this is the last header
   return true;
  }
  \endcode
*/
//bool WebHandler::addHeader( const char* type, const char* value, bool lastone )
//{
//  if(response == NULL)
//    return false;
//  int len = strlen(type) + strlen(value);
//  char temp[len + 6];
//  if( lastone )
//    sprintf(temp, "%s: %s\r\n\r\n", type, value);
//  else
//    sprintf(temp, "%s: %s\r\n", type, value);
//  int written = response->write(temp, strlen(temp));
//  return (written != 0) ? true : false;
//}

#endif // MAKE_CTRL_NETWORK





