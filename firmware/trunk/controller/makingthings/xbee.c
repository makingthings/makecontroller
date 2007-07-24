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

int XBee_GetPacket( XBeePacket* packet )
{
  if( CONTROLLER_OK != XBee_SetActive( 1 ) )
    return CONTROLLER_ERROR_SUBSYSTEM_INACTIVE;
  
  while( Serial_GetReadable( ) )
  {
    int newChar = Serial_GetChar( );
    if( newChar == -1 )
      break;

    switch( packet->rxState )
    {
      case XBEE_PACKET_RX_START:
        if( newChar == XBEE_PACKET_STARTBYTE )
          packet->rxState = XBEE_PACKET_RX_LENGTH_1;
        break;
      case XBEE_PACKET_RX_LENGTH_1:
        packet->length = newChar << 1;
        packet->rxState = XBEE_PACKET_RX_LENGTH_2;
        break;
      case XBEE_PACKET_RX_LENGTH_2:
        packet->length += newChar;
        packet->rxState = XBEE_PACKET_RX_CMD_ID;
        break;
      case XBEE_PACKET_RX_CMD_ID:
        packet->apiId = newChar;
        packet->index++;
        packet->crc += newChar;
        packet->rxState = XBEE_PACKET_RX_DATA;
        break;
      case XBEE_PACKET_RX_DATA:
        if( packet->index++ < packet->length )
          *packet->dataPtr++ = newChar;
        else
          packet->rxState = XBEE_PACKET_RX_CRC;
        packet->crc += newChar;
        break;
      case XBEE_PACKET_RX_CRC:
        packet->rxState = XBEE_PACKET_RX_START;
        return (packet->crc == 0xFF) ? 1 : 0;
    }
  }
  return 0;
}

int XBee_SendPacket( XBeePacket* packet, int datalength )
{
  Serial_SetChar( XBEE_PACKET_STARTBYTE );
  int size = datalength;
  switch( packet->apiId )
  {
    case XBEE_COMM_RX64: //account for apiId, 8 bytes source address, signal strength, and options
    case XBEE_COMM_TX64: //account for apiId, frameId, 8 bytes destination, and options
      size += 11;
      break;
    case XBEE_COMM_RX16: //account for apiId, 2 bytes source address, signal strength, and options
    case XBEE_COMM_TX16: //account for apiId, frameId, 2 bytes destination, and options
    case XBEE_COMM_ATCOMMANDRESPONSE: // account for apiId, frameID, 2 bytes AT cmd, 1 byte status
      size += 5; 
      break;
    case XBEE_COMM_TXSTATUS:
      size = 3; // explicitly set this, since there's no data afterwards
      break;
    case XBEE_COMM_ATCOMMAND: // account for apiId, frameID, 2 bytes AT command
    case XBEE_COMM_ATCOMMANDQ: // same
      size += 4;
      break;
    default:
      size = 0;
      break;
  }

  Serial_SetChar( (size >> 8) & 0xFF ); // send the most significant bit
  Serial_SetChar( size & 0xFF ); // then the LSB
  packet->crc = 0; // just in case it hasn't been initialized.
  uint8* p = (uint8*)packet;
  while( size-- )
  {
    Serial_SetChar( *p );
    packet->crc += *p++;
  }
  Serial_SetChar( 0xFF - packet->crc );
  return 0;
}

void XBee_InitPacket( XBeePacket* packet )
{
  packet->dataPtr = (uint8*)packet;
  packet->crc = 0;
  packet->rxState = XBEE_PACKET_RX_START;
  packet->length = 0;
  packet->index = 0;
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


