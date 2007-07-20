/*
  webclient.c
*/

#include "stdlib.h"
#include "config.h"

#include "string.h"
#include <stdio.h>

#include "webclient.h"

#define WEBCLIENT_INTERNAL_BUFFER_SIZE 100

/**	
	Performs an HTTP GET operation to the path at the address / port specified.  The result 
  is returned in the specified buffer.
	@param buffer A pointer to the buffer read back into.  
	@param buffer_size An integer specifying the actual size of the buffer.
  @return status.
*/
int WebClient_Get( int address, int port, char* path, char* buffer, int buffer_size )
{
  char b[ WEBCLIENT_INTERNAL_BUFFER_SIZE ];
  int buffer_read = 0;
  void* s = Socket( address, port );  
  if ( s != NULL )
  {
    int send_len = snprintf( b, WEBCLIENT_INTERNAL_BUFFER_SIZE, "GET %s HTTP/1.1\r\n\r\n", path );
    if ( send_len > WEBCLIENT_INTERNAL_BUFFER_SIZE )
    {
      SocketClose( s );
      return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;
    }

    AppLed_SetState( 1, 1 );
    SocketWrite( s, b, send_len );
    int content_length = 0;
    
    AppLed_SetState( 2, 1 );
    int buffer_length;
    while ( ( buffer_length = SocketReadLine( s, b, WEBCLIENT_INTERNAL_BUFFER_SIZE ) ) )
    {
      if ( strncmp( b, "\r\n", 2 ) == 0 )
        break;
      if ( strncmp( b, "Content-Length", 14 ) == 0 )
        content_length = atoi( &b[ 15 ] );
    }
    
    if ( content_length > 0 && buffer_length > 0 )
    {
      AppLed_SetState( 3, 1 );
      char* bp = buffer;
      while ( ( buffer_length = SocketRead( s, bp, buffer_size - buffer_read ) ) )
      {
        buffer_read += buffer_length;
        bp += buffer_read;
        if ( buffer_read >= content_length )
          break;
      }
    }
          
    SocketClose( s );

    AppLed_SetState( 1, 0 );
    AppLed_SetState( 2, 0 );
    AppLed_SetState( 3, 0 );
   
    return buffer_read;
  }
  else
    return CONTROLLER_ERROR_BAD_ADDRESS;
}

/**	
	Performs an HTTP POST operation to the path at the address / port specified.  The actual post contents 
  are found in buffer and the result is returned in buffer.
	@param buffer A pointer to the buffer to write from and read back into.  
	@param buffer_length An integer specifying the number of bytes to write.
	@param buffer_size An integer specifying the actual size of the buffer.
  @return status.
*/
int WebClient_Post( int address, int port, char* path, char* buffer, int buffer_length, int buffer_size )
{
  char b[ WEBCLIENT_INTERNAL_BUFFER_SIZE ];
  int buffer_read = 0;
  int wrote = 0;
  void* s = Socket( address, port );  
  if ( s != NULL )
  { 
    int send_len = snprintf( b, WEBCLIENT_INTERNAL_BUFFER_SIZE, 
                                "POST %s HTTP/1.1\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n", 
                                path, buffer_length );
    if ( send_len > WEBCLIENT_INTERNAL_BUFFER_SIZE )
    {
      SocketClose( s );
      return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;
    }

    AppLed_SetState( 1, 1 );

    wrote = SocketWrite( s, b, send_len );
    if ( wrote == 0 )
    {
      SocketClose( s );
      return CONTROLLER_ERROR_WRITE_FAILED;
    }

    SocketWrite( s, buffer, buffer_length );
    
    int content_length = 0;
    
    AppLed_SetState( 2, 1 );
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
      AppLed_SetState( 3, 1 );
      while ( ( b_len = SocketRead( s, bp, buffer_size - buffer_read ) ) )
      {
        buffer_read += b_len;
        bp += buffer_read;
        if ( buffer_read >= content_length )
          break;
      }
    }          

    SocketClose( s );

    
    AppLed_SetState( 1, 0 );
    AppLed_SetState( 2, 0 );
    AppLed_SetState( 3, 0 );
    
    return buffer_read;
  }
  else
    return CONTROLLER_ERROR_BAD_ADDRESS;
}

