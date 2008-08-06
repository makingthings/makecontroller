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

#include "config.h"
#ifdef MAKE_CTRL_NETWORK

#include "stdlib.h"
#include "webclient.h"
#include "lwip/api.h"

#define WEBCLIENT_INTERNAL_BUFFER_SIZE 200
char WebClient_InternalBuffer[ WEBCLIENT_INTERNAL_BUFFER_SIZE ];

/** \defgroup webclient Web Client
  A very simple web client for HTTP operations.

  The web client system allows the Make Controller to get/post data to a webserver.  This
  makes it straightforward to use the Make Controller as a source of data for your web apps.
  
  Note that these functions make liberal use of printf-style functions, which can require 
  lots of memory to be allocated to the task calling them.

  There's currently not a method provided for name resolution - you can always ping the 
  server you want to communicate with to see its IP address, and just use that.
  
  See Network_DnsGetHostByName() for a way to get the address of a particular web site.

	\ingroup Libraries
	@{
*/

/**	
	Performs an HTTP GET operation to the path at the address / port specified.  
  
  Reads through the HTTP header and copies the data into the buffer you pass in.  Because
  sites can often be slow in their responses, this will wait up to 1 second (in 100 ms. intervals)
  for data to become available.

  Some websites seem to reject connections occassionally - perhaps because we don't supply as
  much info to the server as a browser might, for example.  Simpler websites should be just fine.
  
  Note that this uses lots of printf style functions and may require a fair amount of memory to be allocated
  to the task calling it.  The result is returned in the specified buffer.

	@param address The IP address of the server to get from.  Usually created using the IP_ADDRESS( ) macro.
  @param port The port to connect on.  Usually 80 for HTTP.
  @param hostname A string specifying the name of the host to connect to.  When connecting to a server
  that does shared hosting, this will specify who to connect with.
  @param path The path on the server to connect to.
  @param buffer A pointer to the buffer read back into.  
	@param buffer_size An integer specifying the actual size of the buffer.
  @return the number of bytes read, or < 0 on error.

  \par Example
  \code
  int addr = IP_ADDRESS( 72, 249, 53, 185); // makingthings.com is 72.249.53.185
  int bufLength = 100;
  char myBuffer[bufLength];
  int getSize = WebClient_Get( addr, 80, "www.makingthings.com", "/test/path", myBuffer, bufLength );
  \endcode
  Now we should have the results of the HTTP GET from \b www.makingthings.com/test/path in \b myBuffer.
*/
int WebClient_Get( int address, int port, char* hostname, char* path, char* buffer, int buffer_size )
{
  char* b = WebClient_InternalBuffer;
  struct netconn *s = Socket( address, port );  
  if ( s != NULL )
  {
    // construct the GET request
    int send_len = snprintf( b, WEBCLIENT_INTERNAL_BUFFER_SIZE, "GET %s HTTP/1.1\r\n%s%s%s\r\n", 
                                path,
                                ( hostname != NULL ) ? "Host: " : "",
                                ( hostname != NULL ) ? hostname : "",
                                ( hostname != NULL ) ? "\r\n" : ""  );
    if ( send_len > WEBCLIENT_INTERNAL_BUFFER_SIZE )
    {
      SocketClose( s );
      return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;
    }
    
    // send the GET request
    if(!SocketWrite( s, b, send_len ))
    {
      SocketClose( s );
      return CONTROLLER_ERROR_WRITE_FAILED;
    }

    int content_length = 0;
    // read through the response header to get to the data, and pick up the content-length as we go
    int buffer_length;
    while ( ( buffer_length = SocketReadLine( s, b, WEBCLIENT_INTERNAL_BUFFER_SIZE ) ) )
    {
      if ( strncmp( b, "\r\n", 2 ) == 0 )
        break;
      if ( strncmp( b, "Content-Length", 14 ) == 0 )
        content_length = atoi( &b[ 15 ] );
    }
    
    // read the data into the given buffer until there's none left, or the passed in buffer is full
    int total_bytes_read = 0;
    int buf_remaining = buffer_size;
    if ( content_length > 0 && buffer_length > 0 )
    {
      char* bp = buffer;
      while( total_bytes_read < buffer_size && total_bytes_read < content_length )
      {
        int avail = SocketBytesAvailable(s);
        if(!avail) // sometimes the connection can be slooooow, sleep a bit and try again
        {
          int times = 10;
          while(times--)
          {
            Sleep(100);
            if((avail = SocketBytesAvailable(s)))
              break;
          }
        }
        if(!avail) // if we still didn't get anything, bail
          break;

        if(avail > buf_remaining) // make sure we don't read more than can fit
          avail = buf_remaining;
        buffer_length = SocketRead( s, bp, avail );
        if(!buffer_length) // this will be 0 when we get a read error - bail in that case
          break;

        // update counts
        buf_remaining -= buffer_length;
        total_bytes_read += buffer_length;
        bp += buffer_length;
      }
    }
          
    SocketClose( s );
    return total_bytes_read;
  }
  else
    return CONTROLLER_ERROR_BAD_ADDRESS;
}

/**	
	Performs an HTTP POST operation to the path at the address / port specified.  The actual post contents 
  are found read from a given buffer and the result is returned in the same buffer.
  @param address The IP address of the server to post to.
  @param port The port on the server you're connecting to. Usually 80 for HTTP.
  @param hostname A string specifying the name of the host to connect to.  When connecting to a server
  that does shared hosting, this will specify who to connect with.
  @param path The path on the server to post to.
	@param buffer A pointer to the buffer to write from and read back into.  
	@param buffer_length An integer specifying the number of bytes to write.
	@param buffer_size An integer specifying the actual size of the buffer.
  @return status.

  \par Example
  \code
  // we'll post a test message to www.makingthings.com/post/path
  int addr = IP_ADDRESS( 72, 249, 53, 185); // makingthings.com is 72.249.53.185
  int bufLength = 100;
  char myBuffer[bufLength];
  sprintf( myBuffer, "A test message to post" );
  int result = WebClient_Post( addr, 80, "www.makingthings.com", "/post/path", 
                                    myBuffer, strlen("A test message to post"), bufLength );
  \endcode
*/
int WebClient_Post( int address, int port, char* path, char* hostname, char* buffer, int buffer_length, int buffer_size )
{
  char* b = WebClient_InternalBuffer;
  int buffer_read = 0;
  int wrote = 0;
  void* s = Socket( address, port );  
  if ( s != NULL )
  { 
    int send_len = snprintf( b, WEBCLIENT_INTERNAL_BUFFER_SIZE, 
                                "POST %s HTTP/1.1\r\n%s%s%sContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n", 
                                path, 
                                ( hostname != NULL ) ? "Host: " : "",
                                ( hostname != NULL ) ? hostname : "",
                                ( hostname != NULL ) ? "\r\n" : "",
                                buffer_length );
    if ( send_len > WEBCLIENT_INTERNAL_BUFFER_SIZE )
    {
      SocketClose( s );
      return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;
    }

    wrote = SocketWrite( s, b, send_len );
    if ( wrote == 0 )
    {
      SocketClose( s );
      return CONTROLLER_ERROR_WRITE_FAILED;
    }

    SocketWrite( s, buffer, buffer_length );
    
    int content_length = 0;
    int b_len;
    while ( ( b_len = SocketReadLine( s, b, WEBCLIENT_INTERNAL_BUFFER_SIZE ) ) )
    {
      if ( strncmp( b, "\r\n", 2 ) == 0 )
        break;
      if ( strncmp( b, "Content-Length", 14 ) == 0 )
        content_length = atoi( &b[ 16 ] );
    }
          
    if ( content_length > 0 && b_len > 0 )
    {
      char* bp = buffer;
      while ( ( b_len = SocketRead( s, bp, buffer_size - buffer_read ) ) )
      {
        buffer_read += b_len;
        bp += b_len;
        if ( buffer_read >= content_length )
          break;
      }
    }          

    SocketClose( s );
    return buffer_read;
  }
  else
    return CONTROLLER_ERROR_BAD_ADDRESS;
}

/** @}
*/

#endif // MAKE_CTRL_NETWORK



