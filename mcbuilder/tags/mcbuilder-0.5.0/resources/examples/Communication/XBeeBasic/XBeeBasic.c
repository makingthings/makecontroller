/*
  XBeeBasic, MakingThings 2008
  
  A very basic example of how to use the Make Controller with the XBee 
  wireless modules.  This assumes that you have an XBee module connected
  to your Make Controller, and just listens for incoming messages
  from other XBee modules.  
*/
#include "config.h"
#include "xbee.h"
#include <stdio.h>

void BlinkTask( void* p );
void XBeeTask( void* p );
void HandlePacket( XBeePacket* packet );

#define MSG_BUF_SIZE 250
char msgBuf[MSG_BUF_SIZE];

void Run( ) // this task gets called as soon as we boot up.
{
  Usb_SetActive(1);
  TaskCreate( BlinkTask, "Blink", 600, 0, 1 );
  TaskCreate( XBeeTask, "XBee", 1000, 0, 3 );
}

void XBeeTask( void* p )
{
  (void)p; // unused
  XBee_SetActive(1);
  XBeePacket xbp; // our XBee packet structure
  XBee_ResetPacket(&xbp); // reset it so it's ready to read into
  
  while(1) // forever
  {
    /* the entire packet might not come across the serial port
      all at once, so keep reading until we get a complete packet.
    */
    if( XBee_GetPacket( &xbp, 5 ) ) // if this returns non-zero, we received a full packet
    {
      HandlePacket( &xbp );     // decide what to do with the packet
      XBee_ResetPacket( &xbp ); // reset the packet structre so we can receive the next one
    }
  }
}

/*
  Handle a new XBee packet.
  Depending on what kind of packet it was, print out a message that tells us
  the packet type and the signal strength of the message.
*/
void HandlePacket( XBeePacket* packet )
{
  int packet_length = packet->length;
  int signal_strength;
  switch( packet->apiId ) // check what kind of packet this was...
  {
    case XBEE_RX64:
    {
      signal_strength = packet->rx64.rssi;
      int msg_len = printf(msgBuf, "got an RX 64 packet with len %d and signal strength %d.\n", packet_length, signal_strength);
      Usb_Write(msgBuf, msg_len);
      break;
    }
    case XBEE_RX16:
    {
      signal_strength = packet->rx16.rssi;
      int msg_len = printf(msgBuf, "got an RX 64 packet with len %d and signal strength %d.\n", packet_length, signal_strength);
      Usb_Write(msgBuf, msg_len);
      break;
    }
    case XBEE_IO64:
    {
      signal_strength = packet->io64.rssi;
      int msg_len = printf(msgBuf, "got an IO 64 packet with len %d and signal strength %d.\n", packet_length, signal_strength);
      Usb_Write(msgBuf, msg_len);
      break;
    }
    case XBEE_IO16:
    {
      signal_strength = packet->io16.rssi;
      int msg_len = printf(msgBuf, "got an IO 16 packet with len %d and signal strength %d.\n", packet_length, signal_strength);
      Usb_Write(msgBuf, msg_len);
      break;
    }
    default:
      break;
  }
}

void BlinkTask( void* p )
{
  (void)p;
  Led_SetState(1);
  Sleep(1000);

  while( true )
  {
    Led_SetState(0);
    Sleep(990);
    Led_SetState(1);
    Sleep(10);
  }
}


