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

#include "http.h"
#include "types.h"

#ifndef MAX_FORM_ELEMENTS
#define MAX_FORM_ELEMENTS 5
#endif

/**
  A structure that represents a key-value pair in an HTML form.
  This structure only points at the data received by the web server - it does not copy it.  So be sure
  to note that this structure becomes invalid as soon as the data used to create is gone.
*/
typedef struct HtmlFormElement_t {
  char *key;   /**< A pointer to the key of this element. */ 
  char *value; /**< A pointer to the value of this element. */
} HtmlFormElement;

/**
  A structure that represents a collection of HtmlFormElement structures.
  If you need a larger form, you can adjust \b MAX_FORM_ELEMENTS in webserver.h it accommodates 10 by default.
*/
typedef struct HtmlForm_t {
  HtmlFormElement elements[MAX_FORM_ELEMENTS]; /**< An array of form elements. */
  int count;                                   /**< The number of form elements contained in this form. */
} HtmlForm;

typedef struct WebHandler_t {
  const char* address;
  bool (*onRequest)(int socket, HttpMethod method, char* path, char* body, int bodylen);
  struct WebHandler_t* next;
} WebHandler;

#ifdef __cplusplus
extern "C" {
#endif
bool webserverEnable(bool on, int port);
void webserverAddHandler(WebHandler* handler);
void webserverSetStatusOK(int socket);
void webserverSetStatusCode(int socket, int code);
#ifdef __cplusplus
}
#endif

#endif  // WEB_SERVER_H

