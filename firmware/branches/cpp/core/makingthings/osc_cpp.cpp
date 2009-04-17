

#include "core.h"
#ifdef OSC

#include "osc_cpp.h"
#include "osc_pattern.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

extern "C" {
  #include "rtos.h"
  #include "FreeRTOS.h"
  #include "task.h"
  #include "queue.h"
}

#include "usb_serial.h"

void oscAutoSendLoop( void* parameters );

Osc* Osc::_instance = 0;

Osc::Osc( )
{
  #ifdef MAKE_CTRL_NETWORK
  udpTask = NULL;
  #endif
  #ifdef MAKE_CTRL_USB
  usbTask = NULL;
  #endif
  autoSendTask = NULL;
  handler_count = 0;
  resetChannel( OscUDP, true, true );
  resetChannel( OscUSB, true, true );
}

void Osc::setAutoSender( bool enable )
{
  if( enable && !autoSendTask )
    autoSendTask = new Task( oscAutoSendLoop, "AutoSend", 1000, 3, this );
  else if( !enable && autoSendTask )
  {
    delete autoSendTask;
    autoSendTask = NULL;
  }
}

bool Osc::registerHandler( OscHandler* handler )
{
  if( handler_count >= OSC_MAX_HANDLERS )
    return false;
  else
  {
    handlers[handler_count++] = handler;
    return true;
  }
}

/*
  Loop that sits around waiting for UDP data to process as Osc messages.
*/
#ifdef MAKE_CTRL_NETWORK

void oscUdpLoop( void* parameters );

void oscUdpLoop( void* params )
{
  Osc* osc = (Osc*)params;
  
  // Chill until the Network is up
  // while ( !Network_GetActive() )
  //   Sleep( 100 );
  
  if( !osc->udpSock.valid() )
    return;
  
  osc->udpSock.bind( osc->udp_listen_port );  
  int address, port, length;
  
  while ( true )
  {
    length = osc->udpSock.read( osc->udpChannel.inBuf, OSC_MAX_MESSAGE_IN, &address, &port );
    if( length > 0 )
    {
      osc->udp_reply_address = address;
      osc->udp_reply_port = port;
      osc->receivePacket( OscUDP, osc->udpChannel.inBuf, length );
    }
    Task::yield( );
  }
}

void Osc::setUdpListener( bool enable, int port )
{
  if( enable && !udpTask )
  {
    udpTask = new Task( oscUdpLoop, "Osc UDP", 1000, 3, this );
    udp_listen_port = port;
  }
  else if( !enable && udpTask )
  {
    delete udpTask;
    udpTask = NULL;
  }
}

#endif

/*
  Loop that sits around waiting for USB data to process as Osc messages.
*/

#ifdef MAKE_CTRL_USB

void oscUsbLoop( void* parameters );

void oscUsbLoop( void* params )
{
  Osc* osc = (Osc*)params;
  UsbSerial* usb = UsbSerial::get();
  
  // Chill until the USB connection is up
  while ( !usb->isActive() )
    Task::sleep( 100 );
  
  int length;
  while ( true )
  {
    length = usb->readSlip( osc->usbChannel.inBuf, OSC_MAX_MESSAGE_IN );
    if ( length > 0 )
      osc->receivePacket( OscUSB, osc->usbChannel.inBuf, length );
    Task::sleep( 1 );
  }
}

void Osc::setUsbListener( bool enable )
{
  if( enable && !usbTask )
    usbTask = new Task( oscUsbLoop, "Osc USB", 1000, 3, this );
  else if( !enable && usbTask )
  {
    delete usbTask;
    usbTask = NULL;
  }
}

#endif // MAKE_CTRL_USB

/*
  Loop that checks if OscHandlers registered for auto sending have anything to auto send.
*/
void oscAutoSendLoop( void* params )
{
  Osc* osc = (Osc*)params;
  // int channel;
  // int i;
  // OscSubsystem* sub;
  // int newMsgs = 0;
  while( 1 )
  {
    // channel = System_GetAsyncDestination( );
    // if( channel >= 0 )
    // {
    //   for( i = 0; i < Osc->registeredSubsystems; i++ )
    //   {
    //     sub = Osc->subsystem[ i ];
    //     if( sub->async != NULL )
    //       newMsgs += (sub->async)( channel );   
    //   }
    //   if( newMsgs > 0 )
    //   {
    //     Osc_SendPacket( channel );
    //     newMsgs = 0;
    //   }
    //   Task::sleep( System_GetAutoSendInterval( ) );
    // }
    // else
    //   Task::sleep( 1000 );
  }
}

/*
  Main entry point for processing a new packet.
  Messages are sent to receiveMessage(), recursively in the case that we received a bundle.
*/
bool Osc::receivePacket( OscTransport t, char* packet, int length )
{
  // Got a packet.  Unpacket.
  int status = -1;
  switch ( *packet )
  {
    case '/':
      status = receiveMessage( t, packet, length );
      break;
    case '#':
      if ( strcmp( packet, "#bundle" ) == 0 )
      {
        // skip bundle text and timetag
        packet += 16;
        length -= 16;
        while ( length > 0 )
        {
          // read the length (pretend packet is a pointer to integer)
          int messageLength = endianSwap( *((int*)packet) );
          packet += 4;
          length -= 4;
          if ( messageLength <= length )
            receivePacket( t, packet, messageLength );
          length -= messageLength;
          packet += messageLength;
        }
      }
      break;
  }
  return send( t ); // send anything out that might have been created in response to the msg just received
}

/*
  Process a single Osc message.
  First check just the address string to see if it's a query.  If so, find the appropriate handler and dispatch it.
  Otherwise, process the message and populate an OscMessage structure with its data, and send that to
  any OscHandlers that have registered to hear about it.
*/
int Osc::receiveMessage( OscTransport t, char* message, int length )
{
  // Confirm it's a message
  if ( *message != '/')
    return CONTROLLER_ERROR_BAD_DATA;
  
  int address_len = strlen(message);
  int i;
  /*
    Check if it's a query - if the last character of the address string is a / then it is.
    The one special case is a root level query - then we just send back the names of all registered handlers.
  */
  if(*message == '/' && address_len == 1) // root level query
  {
    OscHandler* handler;
    for( i = 0; i < handler_count; i++ )
    {
      handler = handlers[i];
      createMessage(t, "/", ",s", handler->name());
    }
    send(t);
    return CONTROLLER_OK;
  }
  
  // otherwise check if it was a regular query
  if( *(message + address_len) == '/' ) // check the last character of the address
    return handleQuery( t, message );
  
  OscMessage* msg = extractData( t, (message + address_len), length - address_len );
  if( !msg )
    return CONTROLLER_ERROR_BAD_DATA;
  msg->address = message;
  for ( i = 0; i < handler_count; i++ )
  {
    OscHandler* handler = handlers[ i ];
    if ( OscPattern::match( message + 1, handler->name() ) )
    {
      #ifdef MAKE_CTRL_NETWORK
      if( t == OscUDP )
        handler->onNewMsg( t, msg, udp_reply_address, udp_reply_port );
      #endif

      #ifdef MAKE_CTRL_USB
      if( t == OscUSB )
        handler->onNewMsg( t, msg, 0, 0 );
      #endif
    }
  }
  resetChannel(t, false, true);
  return CONTROLLER_OK;
}

/*
  The message received is a query.  Find the appropriate handler(s) to dispatch it to.
  We do the OscHandler the favor of figuring out which element of the address string it was, so they
  can do whatever makes sense in their domain.
*/
int Osc::handleQuery( OscTransport t, char* message )
{
  int element_count = 0;
  int root_len = 0;
  char* next_slash = strchr(message+1, '/');
  if( next_slash )
  {
    root_len = next_slash - message + 1; // get the length of the root element
    while( next_slash )
    {
      if( (next_slash = strchr(message+1, '/')) )
        element_count++;
    }
  }
  int i, replies = 0;
  for( i = 0; i < handler_count; i++ )
  {
    OscHandler* handler = handlers[i];
    if( OscPattern::match(message + 1, handler->name()) )
      replies+= handler->onQuery(t, message, element_count);
  }
  if(replies)
    return send(t);
  return CONTROLLER_ERROR_UNKNOWN_PROPERTY; // if we got down here, we didn't find a matching handler
}

/*
  Churn through the data received and populate an OscMessage in the appropriate channel
  with its data.  We expect to be passed in the message data after the address string.
  First find the type tag, then walk through the rest of the data according to it, stuffing the data
  items into the OscMessage.
*/
OscMessage* Osc::extractData( OscTransport t, char* message, int length )
{
  char* typetag = findTypeTag( message, length );
  if(!typetag)
    return NULL;
  
  OscMessage* msg = &(getChannel(t)->incomingMsg);
  
  // figure out where the data starts after the typetag
  int tagLen = strlen( typetag ) + 1;
  int pad = tagLen % 4;
  if ( pad != 0 )
    tagLen += ( 4 - pad );
  char* data = typetag + tagLen;
  length -= tagLen;
  typetag++; // step past the initial , that all typetags must start with
  
  while( *typetag && length && msg->data_count < OSC_MSG_MAX_DATA_ITEMS )
  {
    switch ( *typetag++ )
    {
      case 'i':
      {
        int i = endianSwap( *((int*)data) );
        msg->data_items[msg->data_count].i = i;
        msg->data_items[msg->data_count++].type = oscInt;
        data += 4;
        length -= 4;
        break;
      }
      case 'f':
      {
        float f = endianSwap( *((int*)data) );
        msg->data_items[msg->data_count].f = f;
        msg->data_items[msg->data_count++].type = oscFloat;
        data += 4;
        length -= 4;
        break;
      }
      case 's':
      {
        int len = strlen( data ) + 1;
        int pad = len % 4;
        if ( pad != 0 )
          len += ( 4 - pad );
        msg->data_items[msg->data_count].s = data;
        msg->data_items[msg->data_count++].type = oscString;
        data += len;
        length -= len;
        break;
      }
      case 'b':
      {
        int len = endianSwap( *((int*)data) );
        data += 4;
        length -= 4;
        int pad = len % 4;
        if ( pad != 0 )
          len += ( 4 - pad );
        msg->data_items[msg->data_count].b = data;
        msg->data_items[msg->data_count++].type = oscBlob;
        data += len;
        length -= len;
        break;
      }
    }
  }
  return msg;
}

char* Osc::findTypeTag( char* message, int length )
{
  while ( *message != ',' && length-- > 0 )
    message++;
  if ( length <= 0 )
    return NULL;
  else
    return message;
}

/*
  Create a new message in the outgoing buffer of messages.
  If your message fills up the buffer, it will be sent immediately to make space.  Otherwise,
  you'll need to send it yourself using send().
*/
int Osc::createMessage( OscTransport t, const char* address, const char* format, ... )
{
  if ( !address || !format || *format != ',' )
    return CONTROLLER_ERROR_BAD_DATA;
  
  OscChannel* ch = getChannel(t);
  if(!ch->outgoingSemaphore.take())
    return CONTROLLER_ERROR_CANT_LOCK;
  
  int count = 0;
  char *buf_p;
  do
  {  
    count++;

    char* buffer = ch->outBufPtr;
    int length = ch->outBufRemaining;
  
    buf_p = buffer;
  
    // First message in the buffer?
    if ( buf_p == ch->outBuf )
    {
      buf_p = createBundle( buf_p, &length, 0, 0 );
      if ( buf_p == NULL )
        return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;
    }
  
    // Make room for the new message
    int* msg_length_p = (int *)buf_p;
    buf_p += 4;
    length -= 4;

    // remember the start of the message
    char* msg_start_p = buf_p;    

    if ( length > 0 )
    {      
      // Set up to iterate through the arguments
      va_list args;
      va_start( args, format );
      buf_p = createMessageInternal( buf_p, &length, address, format, args );
    }
    else
      buf_p = 0;
      
    if ( buf_p != 0 )
    {
      *msg_length_p = endianSwap( buf_p - msg_start_p ); // Set the size
      ch->outBufPtr = buf_p;
      ch->outBufRemaining = length;
      ch->outgoingMsgCount++;
    }
    // else
    //   sendPacketInternal( ch );
  } while ( buf_p == 0 && count == 1 );
  
  ch->outgoingSemaphore.give();
  return CONTROLLER_OK;
}

char* Osc::createMessageInternal( char* bp, int* length, const char* address, const char* format, va_list args )
{
  // do the address
  bp = writePaddedString( bp, length, address );
  if ( bp == NULL )
    return 0;

  // do the type
  bp = writePaddedString( bp, length, format );
  if ( bp == NULL )
    return 0;

  // Going to be walking the tag string, the format string and the data
  // skip the ',' comma
  const char* fp;
  bool cont = true;
  for ( fp = format + 1; *fp && cont; fp++ )
  {
    switch ( *fp )
    {
      case 'i':
          *length -= 4;
          if ( *length >= 0 )
          {
            int v = va_arg( args, int );
            v = endianSwap( v );
            *((int*)bp) = v;
            bp += 4;
          }
          else 
            cont = false;
        break;
      case 'f':
        *length -= 4;
        if ( *length >= 0 )
        {
          int v;
          *((float*)&v) = (float)( va_arg( args, double ) ); 
          v = endianSwap( v );
          *((int*)bp) = v;
          bp += 4;
        }
        else 
          cont = false;
        break;
      case 's':
      {
        char* s = va_arg( args, char* );
        bp = writePaddedString( bp, length, s );
        if ( bp == NULL )
          cont = false;
        break;
      }
      case 'b':
      {
        char* b = va_arg( args, char* );
        int blen = va_arg( args, int );
        bp = writePaddedBlob( bp, length, b, blen  );
        if ( bp == NULL )
          cont = false;
        break;
      }
      default:
        cont = false;
    }
  }
  return ( cont ) ? bp : NULL;
}

char* Osc::createBundle( char* buffer, int* length, int a, int b )
{
  char *bp = buffer;

  // do the bundle bit
  bp = writePaddedString( bp, length, "#bundle" );
  if ( bp == NULL )
    return 0;

  // do the timetag
  bp = writeTimetag( bp, length, a, b );
  if ( bp == NULL )
    return 0;

  return bp;
}

int Osc::send( OscTransport t )
{
  OscChannel* ch = getChannel(t);

  if(!ch->outgoingSemaphore.take())
    return CONTROLLER_ERROR_CANT_LOCK;

  int ret = sendInternal( t );
  ch->outgoingSemaphore.give();
  return ret;
}

int Osc::sendInternal( OscTransport t )
{
  OscChannel* ch = getChannel(t);
  if ( ch->outgoingMsgCount == 0 )
    return CONTROLLER_OK;

  // set the buffer and length up
  char* buffer = ch->outBuf;
  int length = OSC_MAX_MESSAGE_OUT - ch->outBufRemaining;

  // see if we can dispense with the bundle business
  if ( ch->outgoingMsgCount == 1 )
  {
    buffer += 20; // skip 8 bytes of "#bundle" and 8 bytes of timetag and 4 bytes of size
    length -= 20;
  }
  #ifdef MAKE_CTRL_NETWORK
  if( t == OscUDP )
  {
    int retval = udpSock.write(  buffer, length, udp_reply_address, udp_reply_port );
    return retval;
  }
  #endif

  #ifdef MAKE_CTRL_USB
  if( t == OscUSB )
    UsbSerial::get()->writeSlip( buffer, length );
  #endif

  resetChannel( t, true, false );

  return CONTROLLER_OK;
}

void Osc::resetChannel( OscTransport t, bool outgoing, bool incoming )
{
  OscChannel* ch = getChannel(t);
  if(outgoing)
  {
    ch->outBufPtr = ch->outBuf;
    ch->outBufRemaining = OSC_MAX_MESSAGE_OUT;
    ch->outgoingMsgCount = 0;
  }
  if(incoming)
  {
    ch->incomingMsg.data_count = 0;
    ch->incomingMsg.address = ch->inBuf;
  }
}

OscChannel* Osc::getChannel(OscTransport t)
{
  #ifdef MAKE_CTRL_NETWORK
  if(t == OscUDP)
    return &udpChannel;
  #endif
  #ifdef MAKE_CTRL_USB
  if(t == OscUSB)
    return &usbChannel;
  #endif
  return NULL;
}

int Osc::endianSwap( int a ) // static
{
  return ( ( a & 0x000000FF ) << 24 ) |
         ( ( a & 0x0000FF00 ) << 8 )  |
         ( ( a & 0x00FF0000 ) >> 8 )  |
         ( ( a & 0xFF000000 ) >> 24 );

}

char* Osc::writePaddedString( char* buffer, int* length, const char* string )
{
  int tagLen = strlen( string ) + 1;
  int tagPadLen = tagLen;
  int pad = ( tagPadLen ) % 4;
  if ( pad != 0 )
    tagPadLen += ( 4 - pad );
 
  *length -= tagPadLen;

  if ( *length >= 0 )
  {
    strcpy( buffer, string );
    int i;
    buffer += tagLen;
    for ( i = tagLen; i < tagPadLen; i++ )
      *buffer++ = 0;
  }
  else
    return NULL;

  return buffer;
}

char* Osc::writePaddedBlob( char* buffer, int* length, char* blob, int blen )
{
  int i;
  int padLength = blen;
  int pad = ( padLength ) % 4;
  if ( pad != 0 )
    padLength += ( 4 - pad );
 
  if ( *length < ( padLength + 4 ) )
    return 0;

  // add the length of the blob
  int l = endianSwap( blen );
  *((int*)buffer) = l;
  buffer += 4;
  *length -= 4;

  memcpy( buffer, blob, blen );
  buffer += blen;
  // reduce the remaining buffer size
  *length -= padLength;

  for ( i = blen; i < padLength; i++ ) 
      *buffer++ = 0;

  return buffer;
}

char* Osc::writeTimetag( char* buffer, int* length, int a, int b )
{
  if ( *length < 8 )
    return NULL;

  *((int*)buffer) = endianSwap( a );
  buffer += 4;
  *((int*)buffer) = endianSwap( b );
  buffer += 4;
  *length -= 8;
  return buffer;
}

OscRangeHelper::OscRangeHelper( OscMessage* msg, int element, int max, int min )
{
  remaining = 0;
  single = -1;
  if( !msg->address )
    return;
  const char* p = strchr(msg->address, '/'); // should give us the very first char of the Osc message
  if( !p++ ) // step to the beginning of the address element
    return;
  int j;
  for( j = 0; j < element; j++ )
  {
    p = strchr( p, '/');
    if(!p++)
      return;
  }
  
  int n = 0;
  int digits = 0;
  // from Osc_NumberMatch()
  while ( isdigit( *p ) )
  {
    digits++;
    n = n * 10 + ( *p++ - '0' );
  }

  bits = -1;
  if ( n >= max )
    return; // -1;

  switch ( *p )
  {
    case '*':
    case '?':
    case '[':
    case '{':
    {
      int i;
      int b = 0;
      char s[ 5 ];
      for ( i = max - 1; i >= min ; i-- )
      {
        b <<= 1;
        sprintf( s, "%d", i );
        if ( OscPattern::match( p, s ) )
        {
          b |= 1;
          remaining++;
        }
      }
      bits = b;
      current = max;
      return; // -1;
    }
    default:
      if ( digits == 0 )
        return; // -1;
      else
        single = n;
        return; // n;
  }
}

bool OscRangeHelper::hasNext( )
{
  return remaining > 0;
}

int OscRangeHelper::next( )
{
  int retval = 0;
  if( single != -1 )
  {
    remaining = 0;
    return single;
  }
  bool cont = true;
  while ( bits > 0 && remaining && cont )
  { 
    if ( bits & 1 )
    {
      retval = current;
      remaining--;
      cont = false;
    }
    current--;
    bits >>= 1;
  }
  return retval;
}

int OscHandler::propertyLookup( const char* propertyList[], char* property )
{
  const char** p = propertyList;
  int index = 0;
  while (*p != NULL )
  {
    if ( strcmp( property, *p++ ) == 0 )
      return index;
    index++;
  }
  return -1;
}

#endif // OSC




