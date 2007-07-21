/*********************************************************************************

 Copyright 2006 MakingThings

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
#include "stdlib.h"
#include <stdio.h>
#include "string.h"
#include "xbee.h"

#define XBEE_PACKET_RX_START 0
#define XBEE_PACKET_RX_LENGTH_1 1
#define XBEE_PACKET_RX_LENGTH_2 2
#define XBEE_PACKET_RX_CMD_ID 3
#define XBEE_PACKET_RX_DATA 4
#define XBEE_PACKET_RX_CRC 5

uint8 XBee_CalculateChecksum( uchar* buffer, int length );
int XBee_VerifyChecksum( uchar* buffer, int length );
uint8* XBee_GetPacketDataPointer( XBeePacket_* packet );
int XBee_SetPacketData( XBeePacket_* packet );

XBee_* XBee;

int XBee_SetActive( int state )
{
  if ( state != 0 && XBee == NULL ) 
  {
    XBee = Malloc( sizeof( XBee_ ) );
    if( XBee == NULL )
      return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;
    
    XBee->mode = XB_IDLE;
    if( CONTROLLER_OK != Serial_SetActive( 1 ) )
    {
      if( XBee != NULL )
      {
        Free( XBee );
        XBee = 0;
      }
      return CONTROLLER_ERROR_SUBSYSTEM_INACTIVE;
    }
  }
  if ( state == 0 && XBee != NULL ) 
  {
    Free( XBee );
    XBee = 0;
  }
  return CONTROLLER_OK;
}

int XBee_GetActive( )
{
  return ( XBee != NULL );
}

int XBee_GetPacket( XBeePacket_* packet )
{
  if( CONTROLLER_OK != XBee_SetActive( 1 ) )
    return CONTROLLER_ERROR_SUBSYSTEM_INACTIVE;

  static uchar buffer[ sizeof( XBeePacket_ ) ], *bp = buffer;
  static uint8 *pp = 0;
  static int charsRxed = 0, state = XBEE_PACKET_RX_START, length = 0, bufferLength = 0;

  if( charsRxed == 0 ) // if we don't have any data from a previous read
  {
    static int available; // -O2 thing...
    available = Serial_GetReadable( );
    if( available < 1 )
      return 0;
    else
    {
      charsRxed = Serial_Read( buffer, available, 0 );
      bufferLength = charsRxed;
      bp = buffer;
    }
  }
  
  while( charsRxed-- )
  {
    switch( state )
    {
      case XBEE_PACKET_RX_START:
        if( *bp++ == XBEE_PACKET_STARTBYTE )
          state = XBEE_PACKET_RX_LENGTH_1;
        break;
      case XBEE_PACKET_RX_LENGTH_1:
        length = *bp++ << 1;
        state = XBEE_PACKET_RX_LENGTH_2;
        break;
      case XBEE_PACKET_RX_LENGTH_2:
        length += *bp++;
        state = XBEE_PACKET_RX_CMD_ID;
        break;
      case XBEE_PACKET_RX_CMD_ID:
        packet->apiIndentifier = *bp++;
        pp = XBee_GetPacketDataPointer( packet );
        state = XBEE_PACKET_RX_DATA;
        break;
      case XBEE_PACKET_RX_DATA:
        if( length != 0 && pp != NULL )
        {
          *pp++ = *bp++;
          length--;
        }
        else
        {
          // not working yet
          //XBee_SetPacketData( XBeePacket_* packet )
          state = XBEE_PACKET_RX_CRC;
        }
        break;
      case XBEE_PACKET_RX_CRC:
      {
        // not working yet
        //if( !XBee_VerifyChecksum( buffer, bufferLength ) )
          //return 0;
        pp = NULL;
        state = XBEE_PACKET_RX_START;
        return 1;
      }
    }
  }
  return 0;
}
/*
// zip the packet-specific parts of the data into their containers in the packet structure
int XBee_SetPacketData( XBeePacket_* packet )
{
  switch( packet->apiIndentifier )
  {
    case XBEE_COMM_TX64:
      return packet->tx64.data;
    case XBEE_COMM_TX16:
      return packet->tx16.data;
    case XBEE_COMM_RX64:
      return packet->rx64.data;
    case XBEE_COMM_RX16:
      return packet->rx16.data;
    case XBEE_COMM_ATCOMMAND:
      return packet->atCommand.parameter;
    case XBEE_COMM_ATCOMMANDQ:
      return packet->.data;
    case XBEE_COMM_ATCOMMANDRESPONSE:
      return packet->atResponse.value;
    default:
      return NULL;
  }
}
*/
uint8* XBee_GetPacketDataPointer( XBeePacket_* packet )
{
  switch( packet->apiIndentifier )
  {
    case XBEE_COMM_TX64:
      return packet->tx64.data;
    case XBEE_COMM_TX16:
      return packet->tx16.data;
    case XBEE_COMM_RX64:
      return packet->rx64.data;
    case XBEE_COMM_RX16:
      return packet->rx16.data;
    case XBEE_COMM_ATCOMMAND:
      return packet->atCommand.parameter;
    //case XBEE_COMM_ATCOMMANDQ:
      //return packet->.data;
    case XBEE_COMM_ATCOMMANDRESPONSE:
      return packet->atResponse.value;
    default:
      return NULL;
  }
}

void XBee_SetPacketApiMode( )
{
  char buf[50];
  snprintf( buf, 50, "+++" ); // enter command mode
  Serial_Write( (uchar*)buf, strlen(buf), 0 );
  Sleep( 1025 ); // have to wait one second after +++ to actually get set to receive in AT mode

  snprintf( buf, 50, "ATAP %x,CN\r", 1 ); // turn API mode on, and leave command mode
  Serial_Write( (uchar*)buf, strlen(buf), 0 );
}

/*
  from XBee doc: not including frame delimiters and length, add all non-escaped bytes keeping 
  only the lowest 8 bits of the result, and subtract from 0xFF
*/
uint8 XBee_CalculateChecksum( uchar* buffer, int length )
{
  uchar *bptr = buffer;
  bptr += 2; // skip the start byte and the 2 bytes of length
  uint8 crc = *bptr++;

  int i, count = length - 3; // we've already initialized to the first char
  for( i=0; i<count; i++ )
    crc += *bptr++;

  return (0xFF - crc);
  
}

/*
  from XBee doc: Add all bytes (include checksum, but not the delimiter and length).
  If the checksum is correct, the sum will equal 0xFF
*/
int XBee_VerifyChecksum( uchar* buffer, int length )
{
  uchar *bptr = buffer;
  bptr += 2; // skip the start byte and the 2 bytes of length
  uint8 crc = *bptr++;

  int i;
  static int count;
  count = length - 6;
  for( i=0; i<count; i++ )
    crc += *bptr++;

  return ( crc == 0xFF ) ? 1 : 0;
}

int XBee_GetMode( )
{
  return 0;
}

