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

#include "webserver_.h"

#define HTTP_OK "HTTP/1.0 200 OK\r\nContent-type: "
#define HTTP_CONTENT_HTML "text/html\r\n\r\n"
#define HTTP_CONTENT_PLAIN "text/plain\r\n\r\n"
#define HTTP_PORT 80

void webServerLoop(void* parameters);
WebServer* WebServer::_instance = 0;

WebServer::WebServer()
{
  handler_count = hits = 0;
  listenPort = newListenPort = HTTP_PORT;
  webServerTask = new Task(webServerLoop, "WebServ", 800, this, 3);
}

WebServer* WebServer::get()
{
  if( !_instance )
    _instance = new WebServer();
  return _instance;
}

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
      ws->processRequest( request );
      delete request;
    }
    Task::sleep( 5 );
  }
}

void WebServer::processRequest( TcpSocket* request )
{
//  char* requestType = 0;
  char* address = 0;
  int i;
  
  request->readLine( requestBuf, REQUEST_SIZE_MAX ); 
  //address = getRequestAddress( requestBuf, REQUEST_SIZE_MAX, &requestType );
  
  bool responded = false;
  if ( handler_count )
  {
    for ( i = 0; i < handler_count; i++ )
    {
      WebHandler* hp = handlers[ i ];
      if ( strncmp( hp->address(), address, strlen( hp->address() ) ) == 0 )
      {
        //responded = hp->newRequest( requestType, address, WebServer->request, REQUEST_SIZE_MAX, requestSocket, WebServer->response, RESPONSE_SIZE_MAX );
        if ( responded )
          break;
      }
    }
  }
//  if(!responded)
//    TestHandler(requestType, address, WebServer->request, REQUEST_SIZE_MAX, requestSocket, WebServer->response, RESPONSE_SIZE_MAX);
  
  request->close();
}







