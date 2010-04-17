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

#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
int  tcpOpen(int address, int port);
void tcpClose(int socket);
int  tcpAvailable(int socket);
int  tcpRead(int socket, char* data, int length);
int  tcpReadLine(int socket, char* data, int length);
int  tcpWrite(int socket, const char* data, int length);
int  tcpSetReadTimeout(int socket, int timeout);
#ifdef __cplusplus
}
#endif

#endif // TCP_SOCKET_H

