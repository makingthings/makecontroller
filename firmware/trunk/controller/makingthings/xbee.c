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

/** \defgroup XBee
	Communicate with XBee (Zigbee) wireless modules via the Make Controller's serial port.
  
  XBee modules from \b MaxStream are small, cheap ($19 each), wireless \b RF (radio frequency) modules that
  can easily be used with the Make Controller Kit to create projects that require wireless communication.
  XBee modules are <b>ZigBee/IEEE 802.15.4</b> compliant and can operate in several modes:
  - Transparent serial port.  Messages in one side magically end up at the other endpoint.
  - AT command mode.  Send traditional AT commands to configure the module itself (as opposed to having
  the data go straight through via serial mode.
  - Packet (API) mode.  Lower level communication that doesn't have to wait for the module to be in
  AT command mode.  Check the \ref XBeePacketTypes for a description of how these packets are laid out.

  The general idea is that you have one XBee module connected directly to your Make Controller Kit, and then
  any number of other XBee modules that can communicate with it in order to get information to and from the
  Make Controller.

  XBee modules also have some digital and analog I/O right on them, which means you can directly connect
  sensors to the XBee modules which will both read the values and send them wirelessly.  Check the XBee doc
  for the appropriate commands to send in order to set this up.

  \ingroup Controller
  @{
*/

/**	
  Controls the active state of the \b XBee subsystem
  @param state Whether this subsystem is active or not
	@return Zero on success.
*/
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

/**	
  Read the active state of the \b XBee subsystem
	@return An integer specifying the active state - 1 (active) or 0 (inactive).
*/
int XBee_GetActive( )
{
  return ( XBee != NULL );
}

/**	
  Receive an incoming XBee packet.
  This function will not block, and will return as soon as there's no more incoming data or
  the packet is fully received.  If the packet was not fully received, call the function repeatedly
  until it is.

  Clear out a packet before reading into it with a call to XBee_InitPacket( )
  @param packet The XBeePacket to receive into.
	@return 1 if a complete packet has been received, 0 if not.
  @see XBee_SetPacketApiMode( )

  \par Example
  \code
  // we're inside a task here...
  XBeePacket myPacket;
  XBee_InitPacket( &myPacket );
  while( 1 )
  {
    if( XBee_GetPacket( &myPacket ) )
    {
      // process the new packet
      XBee_InitPacket( &myPacket ); // then clear it out before reading again
    }
    Sleep( 10 );
  }
  \endcode
*/
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
        packet->length = newChar << 8;
        packet->rxState = XBEE_PACKET_RX_LENGTH_2;
        break;
      case XBEE_PACKET_RX_LENGTH_2:
        packet->length += newChar;
        packet->rxState = XBEE_PACKET_RX_PAYLOAD;
        break;
      case XBEE_PACKET_RX_PAYLOAD:
        if( packet->index++ < packet->length )
          *packet->dataPtr++ = newChar;
        else
          packet->rxState = XBEE_PACKET_RX_CRC;
        packet->crc += newChar;
        break;
      case XBEE_PACKET_RX_CRC:
        // TODO: read the CRC here, not in the previous state
        packet->rxState = XBEE_PACKET_RX_START;
        return (packet->crc == 0xFF) ? 1 : 0;
    }
  }
  return 0;
}

/**	
  Send an XBee packet.

  Check the possible \ref XBeePacketTypes that can be sent, and populate the packet structures
  appropriately before sending them.
  @param packet The XBeePacket to send.
  @param datalength The length of the actual data being sent (not including headers, options, etc.)
	@return Zero on success.
  @see XBee_SetPacketApiMode( )

  \par Example
  \code
  XBeePacket myPacket;
  packet->apiId = XBEE_COMM_TX16; // we're going to send a packet with a 16-bit address
  packet->tx16.frameID = 0x00;
  packet->tx16.destination[0] = 0xFF; // 0xFFFF is the broadcast address
  packet->tx16.destination[1] = 0xFF;
  packet->tx16.options = 0x00; // no options
  packet->tx16.data[0] = 'A'; // now pack in the data
  packet->tx16.data[1] = 'B';
  packet->tx16.data[2] = 'C';
  // finally, send the packet indicating that we're sending 3 bytes of data - "ABC"
  XBee_SendPacket( &myPacket, 3 ); 
  \endcode
*/
int XBee_SendPacket( XBeePacket* packet, int datalength )
{
  if( CONTROLLER_OK != XBee_SetActive( 1 ) )
    return CONTROLLER_ERROR_SUBSYSTEM_INACTIVE;
  
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
  return CONTROLLER_OK;
}

/**	
  Initialize a packet before reading into it.
  @param packet The XBeePacket to initialize.
	@return An integer specifying the active state - 1 (active) or 0 (inactive).
  @see XBee_GetPacket( )
*/
void XBee_InitPacket( XBeePacket* packet )
{
  packet->dataPtr = (uint8*)packet;
  packet->crc = 0;
  packet->rxState = XBEE_PACKET_RX_START;
  packet->length = 0;
  packet->index = 0;
}

/**	
  Set a module into AT command mode.  
  Because XBee modules need to wait 1 second after sending the command sequence before they're 
  ready to receive any AT commands, this function will block for about a second.
	@return An integer specifying the active state - 1 (active) or 0 (inactive).
*/
void XBee_SetPacketApiMode( )
{
  if( CONTROLLER_OK != XBee_SetActive( 1 ) )
    return;
  
  char buf[50];
  snprintf( buf, 50, "+++" ); // enter command mode
  Serial_Write( (uchar*)buf, strlen(buf), 0 );
  Sleep( 1025 ); // have to wait one second after +++ to actually get set to receive in AT mode
  
  snprintf( buf, 50, "ATAP %x,CN\r", 1 ); // turn API mode on, and leave command mode
  Serial_Write( (uchar*)buf, strlen(buf), 0 );
}

/**	
  A convenience function for creating an AT command packet.
  Because XBee modules need to wait 1 second after sending the command sequence before they're 
  ready to receive any AT commands, this function will block for about a second.
  @param packet The XBeePacket to create.
  @param frameID The frame ID for this packet that subsequent response/status messages can refer to.
  @param cmd The 2-character AT command.
  @param params A pointer to the buffer containing the data to be sent.
  @param datalength The number of bytes to send from the params buffer.
	@return An integer specifying the active state - 1 (active) or 0 (inactive).
  
  \par Example
  \code
  XBeePacket txPacket;
  uint8 params[5];
  params[0] = 0x14; // only 1 byte of data in this case
  XBee_CreateATCommandPacket( &txPacket, 0, "IR", params, 1 );
  XBee_SendPacket( &txPacket, 1 );
  \endcode
*/
void XBee_CreateATCommandPacket( XBeePacket* packet, uint8 frameID, char* cmd, uint8* params, int datalength )
{
  packet->apiId = XBEE_COMM_ATCOMMAND;
  packet->atCommand.frameID = frameID;
  uint8* p = packet->atCommand.command;
  *p++ = *cmd++;
  *p++ = *cmd++;
  p = packet->atCommand.parameters;
  while( datalength-- )
    *p++ = *params++;
}

int XBee_GetIOValues( XBeePacket* packet, int *inputs )
{
  if( packet->apiId == XBEE_COMM_IO16 )
  {
    int i;
    static bool enabled;
    int digitalins = 0;
    uint8* p = packet->io16.data;
    int channelIndicators = (packet->io16.channelIndicators[0] << 0x08) | packet->io16.channelIndicators[1];
    for( i = 0; i < XBEE_INPUTS; i++ )
    {
      enabled = channelIndicators & 1;
      channelIndicators >>= 1;
      if( i < 9 ) // digital ins
      {
        if( enabled )
        {
          if( !digitalins )
          {
            int dig0 = *p++ << 0x08;
            digitalins = dig0 | *p++;
          }
          inputs[i] = ((digitalins >> i) & 1) * 1023;
        }
        else
          inputs[i] = 0;
      }
      else // analog ins
      {
        if( enabled )
        {
          int ain_msb = *p++ << 0x08;
          inputs[i-9] = ain_msb | *p++;
        }
        else
          inputs[i] = 0;
      }
    }
  }
  return 0;
}

/** @}
*/


