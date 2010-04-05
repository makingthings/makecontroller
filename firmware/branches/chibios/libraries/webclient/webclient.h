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

#ifndef WEBCLIENT_H
#define WEBCLIENT_H

#include "tcpsocket.h"

/**
  Connect to sites and other services on the internet or local network via HTTP.

  The web client system allows the Make Controller to get/post data to a webserver.  This
  makes it straightforward to use the Make Controller as a source of data for your web apps.
  
  Note that these functions make liberal use of printf-style functions, which can require 
  lots of memory to be allocated to the task calling them.

  \ingroup networking
*/

int webclientGet(const char* hostname, const char* path, int port, char* response, int response_size, const char* headers[]);
int webclientPost(const char* hostname, const char* path, int port, char* data, int data_length, int response_size, const char* headers[]);



#endif // WEBCLIENT_H
