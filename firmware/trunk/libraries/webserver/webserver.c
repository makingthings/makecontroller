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

#include "config.h"
#ifdef MAKE_CTRL_NETWORK

#include "webserver.h"
#include "core.h"
#include "tcpserver.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef WEBSERVER_STACK_SIZE
#define WEBSERVER_STACK_SIZE 512
#endif

#ifndef REQUEST_SIZE_MAX
#define REQUEST_SIZE_MAX 256
#endif

static WORKING_AREA(webserverWA, WEBSERVER_STACK_SIZE);
static msg_t webServerLoop(void *arg);

typedef struct WebServer_t {
  Thread* thd;
  uint32_t hits;
  int port;
  char buf[REQUEST_SIZE_MAX];
  WebHandler* handlers;
} WebServer;

static WebServer webserver;

static void webserverProcessRequest(int socket);
static char* webserverGetRequestAddress(int socket, char* request, int length, HttpMethod* method);
static int webserverGetBody(int socket, char* requestBuffer, int maxSize);

/**
  \defgroup webserver Web Server
  A simple web server.

  \section Usage
  Add handlers to the web server for addresses (URLs) that you would like to respond to.  A handler consists
  of an address, and the function that will handle web requests that match the address.

  Your handler will be passed some information about the incoming request - its HTTP method, the full
  address path and, in the case of a POST or PUT request, the body of the request.  You can use this
  information to determine what kind of action to take, and what kind of response to send.

  To respond, simply start writing to the socket passed into the handler.  Proper HTTP responses start
  with a status - you can use webserverSetStatusOK() to send a default OK status, or webserverSetStatusCode()
  for something more specific.  If you just want to print something out to a web browser though, you can
  usually omit this.

  After that, write out whatever data you like via the socket and you're all set.
  Remember that you can write it out a chunk at a time - you don't need to send it all at once.
  If you have responded to a request, return true from your handler and the web server will stop
  processing the request.  Otherwise, it will continue to look for other responders.

  For more info about HTTP, check the <A HREF="http://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol">Wikipedia article</A>.

  Here's a simple handler that responds to HTTP GET requests:
  \code
  bool handlerFunction(int socket, HttpMethod method, char* path, char* body, int bodylen)
  {
    if (method == HTTP_GET) {
      tcpWrite(socket, "hi there", strlen("hi there"));
      return true; // indicate that we responded
    }
    else {
      return false;
    }
  }

  WebHandler myhandler = {
    .address = "/simple",
    .onRequest = handlerFunction
  };

  webserverAddHandler(&myhandler);
  webserverEnable(YES, 80);
  // when a request comes in that starts with /simple, handlerFunction() will be called
  \endcode

  \section Configuration
  The web server runs as its own thread.  If you find that you need to change the stack size
  for the web server thread, you can define \b WEBSERVER_STACK_SIZE in your config.h.  The
  default value is 512.
  \ingroup networking
  @{
*/

/**
  Turn the web server on or off.
  @param on Whether to turn it on or off.
  @param port Which port to start listening on.  If disabling, this is ignored.

  \b Example
  \code
  webserverEnable(YES, 80); // 80 is the standard HTTP port
  \endcode
*/
bool webserverEnable(bool on, int port)
{
  if (on && webserver.thd == 0) {
    webserver.hits = 0;
    webserver.port = port;
    webserver.thd = chThdCreateStatic(webserverWA, sizeof(webserverWA), NORMALPRIO, webServerLoop, &webserver.port);
    return true;
  }
  else if (webserver.thd != 0) {
    chThdTerminate(webserver.thd);
    chThdWait(webserver.thd);
    webserver.thd = 0;
    return true;
  }
  return false;
}

/**
  Add a new handler to the web server.
  @param handler The handler to add.
*/
void webserverAddHandler(WebHandler* handler)
{
  handler->next = 0;
  if (webserver.handlers == 0) {
    webserver.handlers = handler;
  }
  else {
    WebHandler* h = webserver.handlers;
    while (h->next != 0)
      h = h->next;
    h->next = handler;
  }
}

/** @}
*/

/*
  Wait for new connections (making sure we're listening on the right port)
  and then dispatch them to processRequest()
*/
msg_t webServerLoop(void *arg)
{
  int client, serv = tcpserverOpen(*(int*)arg);

  while (!chThdShouldTerminate()) {
    // Block waiting for connection
    if ((client = tcpserverAccept(serv)) >= 0) {
      webserverProcessRequest(client);
      tcpClose(client);
      webserver.hits++;
    }
  }
  return 0;
}

/*
  A new request has come in - loop through our registered handlers until
  one of them indicates it has responded to it.
*/
void webserverProcessRequest(int socket)
{
  bool responded = false;
  HttpMethod method = 0;
  WebHandler* h = webserver.handlers;
  char* path = webserverGetRequestAddress(socket, webserver.buf, sizeof(webserver.buf), &method);

  while (h != NULL && responded == false) {
    if (strncmp(h->address, path, strlen(h->address)) == 0) {
      // if appropriate, get pointers to the request body
      if (method == HTTP_POST || method == HTTP_PUT) {
        int requestlen = webserverGetBody(socket, webserver.buf, sizeof(webserver.buf));
        responded = h->onRequest(socket, method, path, webserver.buf, requestlen);
      }
      else {
        responded = h->onRequest(socket, method, path, 0, 0);
      }
    }
    h = h->next;
  }
}

/*
  Extract the HTTP method for this request, and then return a pointer to the beginning
  of the URL path
*/
char* webserverGetRequestAddress(int socket, char* buf, int len, HttpMethod* method)
{
  int reqlen = tcpReadLine(socket, buf, len);
  char* request = buf;
  char* end = request + reqlen;
  char* address = NULL;

  // Skip any initial spaces
  while (isspace((int)*request))
    request++;
  if (request > end) // make sure we didn't go too far
    return NULL;

  if (strncmp("GET", request, 3) == 0)
    *method = HTTP_GET;
  else if (strncmp("POST", request, 4) == 0)
    *method = HTTP_POST;
  else if (strncmp("PUT", request, 3) == 0)
    *method = HTTP_PUT;
  else if (strncmp("DELETE", request, 6) == 0)
    *method = HTTP_DELETE;

  // Skip the request type
  if ((request = strchr(request, ' ')) == NULL)
    return address;

  // Skip any subsequent spaces
  while (isspace((int)*request))
    request++;

  if (request > end)
    return address;

  address = request;
  
  // now terminate the end of the address so we can do stringy things on it
  if ((request = strchr(request, ' ')) != 0)
    *request = 0;

  return address;
}

int webserverGetBody(int socket, char* requestBuffer, int maxSize)
{
  // keep reading lines of the HTTP header until we get CRLF which signifies the end of the
  // header.  If we see the contentlength along the way, keep that.
  int bufferLength, contentLength = 0;
  while ((bufferLength = tcpReadLine(socket, requestBuffer, maxSize))) {
    if (strncmp(requestBuffer, "\r\n", 2) == 0)
      break;
    if (strncasecmp(requestBuffer, "Content-Length", 14) == 0)
      contentLength = atoi(&requestBuffer[15]);
  }
  
  // now we should be down to the HTTP POST data
  // if there's any data, get up into it
  int bufferRead = 0;
  if (contentLength > 0 && bufferLength > 0) {
    int lengthToRead = tcpAvailable(socket);
    if (lengthToRead < 0)
      return -1;
    if (lengthToRead > maxSize)
      lengthToRead = maxSize - 1;
    char *rbp = requestBuffer;
    // read all that the socket has to offer...may come in chunks, so keep going until there's none left
    while ((bufferLength = tcpRead(socket, rbp, lengthToRead))) {
      bufferRead += bufferLength;
      rbp += bufferLength;
      lengthToRead -= bufferLength;
      if (bufferRead >= contentLength)
        break;
    }
    requestBuffer[bufferRead] = 0; // null-terminate the request
  }
  return bufferRead;
}

#endif // MAKE_CTRL_NETWORK





