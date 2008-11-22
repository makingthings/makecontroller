

#include "config.h"
#ifdef OSC

#include "osc_cpp.h"
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
void oscAsyncLoop( void* parameters );

OSCC::OSCC( )
{
  udpTask = NULL;
  usbTask = NULL;
  asyncTask = NULL;
}

void OSCC::setUdpListener( bool enable, int port )
{
  if( enable && !udpTask )
  {
    udpTask = new Task( oscUdpLoop, "OSC-UDP", 1000, this, 3 );
    udp_listen_port = port;
  }
  else if( !enable && udpTask )
    delete udpTask;
}

void OSCC::setUsbListener( bool enable )
{
  if( enable && !usbTask )
    usbTask = new Task( oscUsbLoop, "OSC-USB", 1000, this, 3 );
  else if( !enable && usbTask )
    delete usbTask;
}


// 
// void OSC::setAutoSender( bool enable )
// {
//   
// }

void oscUdpLoop( void* params )
{
  // int listen_port = (int)parameters;
  OSCC* osc = (OSCC*)params;
  // Osc->channel[ channel ] = MallocWait( sizeof( OscChannel ), 100 );
  // OscChannel *ch = Osc->channel[ channel ];
  // ch->running = false;
  // Osc_SetReplyPort( channel, NetworkOsc_GetUdpSendPort() );
  // ch->sendMessage = Osc_UdpPacketSend;
  // Osc_ResetChannel( ch );

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
      // Osc_SetReplyAddress( channel, address );
      // Osc_ReceivePacket( channel, ch->incoming, length );
    }
    Task::yield( );
  }
}

void oscUsbLoop( void* params )
{
  OSCC* osc = (OSCC*)params;
  // Osc_ResetChannel( ch );

  // Chill until the USB connection is up
  while ( !USB->isActive() )
    Task::sleep( 100 );

  while ( true )
  {
    int length = USB->readSlip( osc->usbChannel.inBuf, OSC_MAX_MESSAGE_IN );
    if ( length > 0 )
      osc->receivePacket( oscUSB, osc->usbChannel.inBuf, length );
    Task::sleep( 1 );
  }
}

bool OSCC::receivePacket( OscTransport t, char* packet, int length )
{
  // Got a packet.  Unpacket.
  int status = -1;
  switch ( *packet )
  {
    case '/':
      //status = Osc_ReceiveMessage( channel, packet, length );
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
          // int messageLength = Osc_EndianSwap( *((int*)packet) );
          // packet += 4;
          // length -= 4;
          // if ( messageLength <= length )
          //   Osc_ReceivePacket( channel, packet, messageLength );
          // length -= messageLength;
          // packet += messageLength;
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
        // if ( Osc_PatternMatch( p, s ) )
        // {
        //   b |= 1;
        //   remaining++;
        // }
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

int OscMessage::addressElementAsInt( int element, bool* ok )
{
  if(ok)
    *ok = false;
  if( !address )
    return 0;
  int j;
  const char* p = strchr(address, '/'); // should give us the very first char of the OSC message
  if( !p++ ) // step to the beginning of the address element
    return false;
  for( j = 0; j < element; j++ )
  {
    p = strchr( p, '/');
    if(!p++)
      return false;
  }
  if(ok)
    *ok = true;
  return atoi(p);
}

float OscMessage::addressElementAsFloat( int element, bool* ok )
{
  if(ok)
    *ok = false;
  if( !address )
    return 0;
  int j;
  const char* p = strchr(address, '/'); // should give us the very first char of the OSC message
  if( !p++ ) // step to the beginning of the address element
    return 0;
  for( j = 0; j < element; j++ )
  {
    p = strchr( p, '/');
    if(!p++)
      return 0;
  }
  if(ok)
    *ok = true;
  return atof(p);
}

char* OscMessage::addressElementAsString( int element )
{
  if( !address )
    return 0;
  int j;
  const char* p = strchr(address, '/'); // should give us the very first char of the OSC message
  if( !p++ )
    return 0;
  for( j = 0; j < element; j++ )
  {
    p = strchr( p, '/');
    if(!p++)
      return 0;
  }
  return (char*)p;
}

int OscMessage::dataItemAsInt( int index, bool* ok )
{
  if(ok)
    *ok = false;
  if( index >= data_count || (data_items[index].type != oscInt) )
    return 0;
  if(ok)
    *ok = true;
  return data_items[index].i;
}

float OscMessage::dataItemAsFloat( int index, bool* ok )
{
  if(ok)
    *ok = false;
  if( index >= data_count || (data_items[index].type != oscFloat) )
    return 0;
  if(ok)
    *ok = true;
  return data_items[index].f;
}

char* OscMessage::dataItemAsString( int index )
{
  if( index >= data_count || (data_items[index].type != oscString) )
    return 0;
  return data_items[index].s;
}

char* OscMessage::dataItemAsBlob( int index, int* blob_len )
{
  if( index >= data_count || (data_items[index].type != oscBlob) )
    return 0;
  char* p = data_items[index].s;
  *blob_len = *(int*)p;
  return p + 4;
}


#endif // OSC




