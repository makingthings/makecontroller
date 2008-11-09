

#include "config.h"
#ifdef OSC

#include "osc_cpp.h"
#include <string.h>
#include <stdlib.h>

extern "C" {
  #include "rtos.h"
  #include "FreeRTOS.h"
  #include "task.h"
  #include "queue.h"
}

// void OSC::setUsbListener( bool enable )
// {
//   if( enable && !usbTaskPtr )
//     usbTaskPtr = TaskCreate( &OSC::usbTask, "OSC-USB", 1000, USB, 3 );
//   else if( !enable && usbTaskPtr )
//     TaskDelete( usbTaskPtr );
// }
// 
// void OSC::setUdpListener( bool enable, int port )
// {
//   if( enable && !udpTaskPtr )
//     usbTaskPtr = TaskCreate( OSC::udpTask, "OSC-UDP", (void*)port, UDP, 3 );
//   else if( !enable && udpTaskPtr )
//     TaskDelete( udpTaskPtr );
// }
// 
// void OSC::setAutoSender( bool enable )
// {
//   
// }

void OSCC::udpTask( void* parameters )
{
  int listen_port = (int)parameters;
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
  
  if( !send_sock.valid() )
    return;
  
  while( !udp_sock.isBound( ) )
  {
    udp_sock.bind( listen_port );
    vTaskDelay( 10 / portTICK_RATE_MS ); // Sleep( 10 );
  }
  
  int address, port, length;
  
  while ( true )
  {
    length = udp_sock.read( udpChannel.inBuf, OSC_MAX_MESSAGE_IN, &address, &port );
    if( length > 0 )
    {
      // Osc_SetReplyAddress( channel, address );
      // Osc_ReceivePacket( channel, ch->incoming, length );
    }
    taskYIELD( ); // TaskYield( );
  }
}

bool OSCC::receivePacket( int channel, char* packet, int length )
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
  if( index >= data_count || (data_items[index].type != Int) )
    return 0;
  if(ok)
    *ok = true;
  return data_items[index].i;
}

float OscMessage::dataItemAsFloat( int index, bool* ok )
{
  if(ok)
    *ok = false;
  if( index >= data_count || (data_items[index].type != Float) )
    return 0;
  if(ok)
    *ok = true;
  return data_items[index].f;
}

char* OscMessage::dataItemAsString( int index )
{
  if( index >= data_count || (data_items[index].type != String) )
    return 0;
  return data_items[index].s;
}

char* OscMessage::dataItemAsBlob( int index, int* blob_len )
{
  if( index >= data_count || (data_items[index].type != Blob) )
    return 0;
  char* p = data_items[index].s;
  *blob_len = *(int*)p;
  return p + 4;
}


#endif // OSC




