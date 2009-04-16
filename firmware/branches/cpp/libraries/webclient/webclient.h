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

/*
  WebClient.h
  MakingThings
*/

#ifndef WEBCLIENT_H
#define WEBCLIENT_H

#include "tcpsocket.h"

class WebClient
{
public:
  WebClient(int address = 0, int port = 80);
  void setAddress(int address, int port = 80);
  int get( char* hostname, char* path, char* buffer, int size );
  int post( char* hostname, char* path, char* buffer, int post_length, int response_length );

protected:
  int address;
  int port;
  TcpSocket socket;

  int readResponse( char* buf, int size );
};

#endif // WEBCLIENT_H
