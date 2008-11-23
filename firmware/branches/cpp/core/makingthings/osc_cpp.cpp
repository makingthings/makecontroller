

#include "config.h"
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

#include "usb_.h"

void oscUdpLoop( void* parameters );
void oscUsbLoop( void* parameters );
void oscAutoSendLoop( void* parameters );

OSCC::OSCC( )
{
  udpTask = NULL;
  usbTask = NULL;
  autoSendTask = NULL;
  handler_count = 0;
  resetChannel( oscUDP );
  resetChannel( oscUSB );
}

void OSCC::setUdpListener( bool enable, int port )
{
  if( enable && !udpTask )
  {
    udpTask = new Task( oscUdpLoop, "Osc UDP", 1000, this, 3 );
    udp_listen_port = port;
  }
  else if( !enable && udpTask )
    delete udpTask;
}

void OSCC::setUsbListener( bool enable )
{
  if( enable && !usbTask )
    usbTask = new Task( oscUsbLoop, "Osc USB", 1000, this, 3 );
  else if( !enable && usbTask )
    delete usbTask;
}

void OSCC::setAutoSender( bool enable )
{
  if( enable && !autoSendTask )
    autoSendTask = new Task( oscAutoSendLoop, "AutoSend", 1000, this, 3 );
  else if( !enable && autoSendTask )
    delete autoSendTask;
}

/*
  Loop that sits around waiting for UDP data to process as OSC messages.
*/
void oscUdpLoop( void* params )
{
  OSCC* osc = (OSCC*)params;
  osc->resetChannel( oscUDP );

  // Chill until the Network is up
  // while ( !Network_GetActive() )
  //   Sleep( 100 );

  UdpSocket udp_sock;
  if( !udp_sock.valid() )
    return;
  
  if( !osc->send_sock.valid() )
    return;
  
  while( !udp_sock.isBound( ) )
  {
    udp_sock.bind( osc->udp_listen_port );
    Task::sleep( 10 );
  }
  
  int address, port, length;
  
  while ( true )
  {
    length = udp_sock.read( osc->udpChannel.inBuf, OSC_MAX_MESSAGE_IN, &address, &port );
    if( length > 0 )
    {
      osc->udp_reply_address = address;
      osc->udp_reply_port = port;
      osc->receivePacket( oscUDP, osc->udpChannel.inBuf, length );
    }
    Task::yield( );
  }
}

/*
  Loop that sits around waiting for USB data to process as OSC messages.
*/
void oscUsbLoop( void* params )
{
  OSCC* osc = (OSCC*)params;
  osc->resetChannel( oscUSB );

  // Chill until the USB connection is up
  while ( !USB->isActive() )
    Task::sleep( 100 );
  
  int length;
  while ( true )
  {
    length = USB->readSlip( osc->usbChannel.inBuf, OSC_MAX_MESSAGE_IN );
    if ( length > 0 )
      osc->receivePacket( oscUSB, osc->usbChannel.inBuf, length );
    Task::sleep( 1 );
  }
}

/*
  Loop that checks if OscHandlers registered for auto sending have anything to auto send.
*/
void oscAutoSendLoop( void* params )
{
  OSCC* osc = (OSCC*)params;
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
bool OSCC::receivePacket( OscTransport t, char* packet, int length )
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
    default:
      // Something else?
      //Osc_CreateMessage( channel, "/error", ",s", "Packet Error" );
      break;
  }

  // return Osc_SendPacket( channel );
  return true;
}

/*
  Process a single OSC message.
  First check just the address string to see if it's a query.  If so, find the appropriate handler and dispatch it.
  Otherwise, process the message and populate an OscMessage structure with its data, and send that to
  any OscHandlers that have registered to hear about it.
*/
int OSCC::receiveMessage( OscTransport t, char* message, int length )
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
      //createMessage(t, "/", ",s", handler->name());
    }
    //sendMessages(t);
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
      if( t == oscUDP )
        handler->onNewMsg( t, msg, udp_reply_address, udp_reply_port );
      else if( t == oscUSB )
        handler->onNewMsg( t, msg, 0, 0 );
    }
  }
  return CONTROLLER_OK;
}

/*
  The message received is a query.  Find the appropriate handler(s) to dispatch it to.
  We do the OscHandler the favor of figuring out which element of the address string it was, so they
  can do whatever makes sense in their domain.
*/
int OSCC::handleQuery( OscTransport t, char* message )
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
  int i;
  for( i = 0; i < handler_count; i++ )
  {
    OscHandler* handler = handlers[i];
    if( !strncmp(message + 1, handler->name(), root_len) )
    {
      handler->onQuery(t, message, element_count);
      return CONTROLLER_OK;
    }
  }
  return CONTROLLER_ERROR_UNKNOWN_PROPERTY; // if we got down here, we didn't find a matching handler
}

/*
  Churn through the data received and populate an OscMessage in the appropriate channel
  with its data.  We expect to be passed in the message data after the address string.
  First find the type tag, then walk through the rest of the data according to it, stuffing the data
  items into the OscMessage.
*/
OscMessage* OSCC::extractData( OscTransport t, char* message, int length )
{
  char* typetag = findTypeTag( message, length );
  if(!typetag)
    return NULL;
  
  OscMessage* msg = (t == oscUDP) ? &(udpChannel.incomingMsg) : &(usbChannel.incomingMsg);
  
  // figure out where the data starts after the typetag
  int tagLen = strlen( typetag ) + 1;
  int pad = tagLen % 4;
  if ( pad != 0 )
    tagLen += ( 4 - pad );
  char* data = typetag + tagLen;
  length -= tagLen;
  
  while( typetag && length && msg->data_count < OSC_MSG_MAX_DATA_ITEMS )
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

char* OSCC::findTypeTag( char* message, int length )
{
  while ( *message != ',' && length-- > 0 )
    message++;
  if ( length <= 0 )
    return NULL;
  else
    return message;
}

void OSCC::resetChannel( OscTransport t )
{
  OscChannel* ch = (t == oscUDP) ? &udpChannel : &usbChannel;
  ch->outBufPtr = ch->outBuf;
  ch->outBufRemaining = OSC_MAX_MESSAGE_OUT;
  ch->outgoingMsgs = 0;
  ch->incomingMsg.data_count = 0;
  ch->incomingMsg.address = ch->inBuf;
}

int OSCC::endianSwap( int a ) // static
{
  return ( ( a & 0x000000FF ) << 24 ) |
         ( ( a & 0x0000FF00 ) << 8 )  |
         ( ( a & 0x00FF0000 ) >> 8 )  |
         ( ( a & 0xFF000000 ) >> 24 );

}

char* OSCC::writePaddedString( char* buffer, int* length, char* string )
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

char* OSCC::writeTimetag( char* buffer, int* length, int a, int b )
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
  const char* p = strchr(msg->address, '/'); // should give us the very first char of the OSC message
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
  // from OSC_NumberMatch()
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

bool OscRangeHelper::hasNextIndex( )
{
  return remaining > 0;
}

int OscRangeHelper::nextIndex( )
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




