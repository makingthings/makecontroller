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
#include "lwipopts.h"
#if defined(MAKE_CTRL_NETWORK) && LWIP_DNS == 1

#include "stdlib.h"
#include "string.h"
#include <stdio.h>
#include "webclient.h"
#include "network.h"
#include "error.h"

#ifndef WEBCLIENT_BUFFER_SIZE
#define WEBCLIENT_BUFFER_SIZE 128
#endif

#ifndef WEBCLIENT_DNS_TIMEOUT
#define WEBCLIENT_DNS_TIMEOUT 30
#endif

static int webclientReadResponse(int s, char* buf, int size);
static char buffer[WEBCLIENT_BUFFER_SIZE];

/** 
  Performs an HTTP GET operation to the path at the address / port specified.  
  
  Reads through the HTTP headers and copies the data into the buffer you pass in.
  
  Note that this uses lots of printf style functions and may require a fair amount of memory to be allocated
  to the task calling it.  The result is returned in the specified buffer.

  @param hostname The name of the host to connect to.
  @param port The port to connect on - standard http port is 80
  @param path The path on the server to connect to.
  @param response The buffer read the response back into.  
  @param response_size An integer specifying the size of the response buffer.
  @param headers (optional) An array of strings to be sent as headers - last element in the array must be 0.
  @return the number of bytes read, or < 0 on error.

  \par Example
  \code
  WebClient wc;
  int bufLength = 100;
  char myBuffer[bufLength];
  int justGot = wc.get("www.makingthings.com", "/test/path", myBuffer, bufLength);
  \endcode
  Now we should have the results of the HTTP GET from \b www.makingthings.com/test/path in \b myBuffer.
*/
int webclientGet(const char* hostname, const char* path, int port, char* response, int response_size, const char* headers[])
{
  int s = tcpOpen(networkGetHostByName(hostname, WEBCLIENT_DNS_TIMEOUT), port);
  if (s > -1) {
    // construct the GET request
    int len = sniprintf(buffer, WEBCLIENT_BUFFER_SIZE, "GET %s HTTP/1.1\r\n%s%s%s",
                                path,
                                (hostname != NULL) ? "Host: " : "",
                                (hostname != NULL) ? hostname : "",
                                (hostname != NULL) ? "\r\n" : ""  );
    tcpWrite(s, buffer, len);

    if (headers != NULL) {
      for ( ; *headers != 0; headers++) {
        tcpWrite(s, *headers, strlen(*headers));
        tcpWrite(s, "\r\n", 2);
      }
      if (tcpWrite(s, "\r\n", 2 ) < 0) { // all done with headers...just check our last write here...
        tcpClose(s);
        return -1;
      }
    }
    
    // read the data into the given buffer until there's none left, or the passed in buffer is full
    len = webclientReadResponse(s, response, response_size);
    tcpClose(s);
    return len;
  }
  else
    return -1;
}

/** 
  Performs an HTTP POST operation to the path at the address / port specified.  The actual post contents 
  are found read from a given buffer and the result is returned in the same buffer.
  @param hostname The name of the host to connect to.
  @param port The port to connect on - standard http port is 80
  @param path The path on the server to post to.
  @param data The buffer to write from, and then read the response back into
  @param data_length The number of bytes to write from \b data
  @param response_size How many bytes of the response to read back into \b data
  @param headers (optional) An array of strings to be sent as headers - last element in the array must be 0.
  @return status.

  \par Example
  \code
  // we'll post a test message to www.makingthings.com/post/path
  WebClient wc;
  int bufLength = 100;
  char myBuffer[bufLength];
  int datalength = sprintf( myBuffer, "A test message to post" ); // load the buffer with some data to send
  wc.post("www.makingthings.com", "/post/path", myBuffer, datalength, bufLength);
  \endcode
*/
int webclientPost(const char* hostname, const char* path, int port, char* data, int data_length, int response_size, const char* headers[])
{
  int s = tcpOpen(networkGetHostByName(hostname, WEBCLIENT_DNS_TIMEOUT), port);
  if (s > -1) {
    int len = sniprintf( buffer, WEBCLIENT_BUFFER_SIZE,
                                "POST %s HTTP/1.1\r\nContent-Length: %d\r\n%s%s%s", 
                                path, data_length,
                                (hostname != NULL) ? "Host: " : "",
                                (hostname != NULL) ? hostname : "",
                                (hostname != NULL) ? "\r\n" : "");
    tcpWrite(s, buffer, len);
    
    if (headers != NULL) {
      for ( ; *headers != 0; headers++) {
        tcpWrite(s, *headers, strlen(*headers));
        tcpWrite(s, "\r\n", 2);
      }
      tcpWrite(s, "\r\n", 2); // all done with headers
    }

    // send the body...just check our last write here...
    if (tcpWrite(s, data, data_length) <= 0) {
      tcpClose(s);
      return -1;
    }
    
    // read back the response
    len = webclientReadResponse(s, data, response_size);
    tcpClose(s);
    return len;
  }
  else
    return -1;
}

int webclientReadResponse(int s, char* buf, int size)
{
  // read back the response
  int content_length = 0;
  int b_len;
  bool chunked = false;
  
  // read through the headers - figure out the content length scheme
  while ((b_len = tcpReadLine(s, buffer, WEBCLIENT_BUFFER_SIZE))) {
    if (!strncasecmp(buffer, "Content-Length", 14)) // check for content length
      content_length = atoi(&buffer[16]);
    else if (!strncasecmp(buffer, "Transfer-Encoding: chunked", 26)) // check to see if we're chunked
      chunked = true;
    else if (strncmp(buffer, "\r\n", 2) == 0)
      break;
  }

  if (b_len <= 0)
    return 0;
  
  int content_read = 0;
  // read the actual response data into the caller's buffer, if there's any to grab
  if (chunked) { // first see if it's chunked
    int len = 1;
    while (len != 0 && content_read < size) {
      b_len = tcpReadLine(s, buffer, WEBCLIENT_BUFFER_SIZE);
      if (siscanf(buffer, "%x", &len) != 1) // the first part of the chunk should indicate the chunk's length (hex)
        break;
      if (len == 0) // an empty chunk indicates the end of the transfer
        break;
      if (len > (size - content_read)) // make sure we have enough room to read this chunk, based on what we've already read
        len = size - content_read;
      content_read += tcpRead(s, buf, len);
      tcpReadLine(s, buffer, WEBCLIENT_BUFFER_SIZE); // slurp out the remaining newlines
    }
  }
  // otherwise see if we got a content length
  else if (content_length > 0) {
    while ((b_len = tcpRead(s, buf, size - content_read))) {
      content_read += b_len;
      buf += b_len;
      if (content_read >= content_length)
        break;
    }
  }
  // lastly, just try to read until we get cut off
  else {
    while (content_read < size) {
      b_len = tcpRead(s, buf, size - content_read);
      if (b_len <= 0)
        break;
      content_read += b_len;
      buf += b_len;
    }
  }
  return content_read;
}

#endif // MAKE_CTRL_NETWORK



