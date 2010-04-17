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

#ifndef HTTP_H
#define HTTP_H

// HTTP definitions in a central location
#define HTTP_PORT 80
#define HTTP_CONTENT_HTML "text/html\r\n\r\n"
#define HTTP_CONTENT_PLAIN "text/plain\r\n\r\n"

typedef enum {
  HTTP_GET,
  HTTP_PUT,
  HTTP_POST,
  HTTP_DELETE
} HttpMethod;


#endif // HTTP_H
