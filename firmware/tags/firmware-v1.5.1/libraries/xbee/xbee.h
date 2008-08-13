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

/** @file xbee.h
  XBee defines
*/

#ifndef XBEE_H
#define XBEE_H

#include "types.h"

// states for receiving packets
#define XBEE_PACKET_RX_START 0
#define XBEE_PACKET_RX_LENGTH_1 1
#define XBEE_PACKET_RX_LENGTH_2 2
#define XBEE_PACKET_RX_PAYLOAD 3
#define XBEE_PACKET_RX_CRC 4

// general xbee constants
#define XBEE_PACKET_STARTBYTE 0x7E
#define XBEE_PACKET_ESCAPE 0x7D
#define XBEE_INPUTS 15 // counts analog and digital ins separately
#define XBEE_MAX_PACKET_SIZE 100

// xbee io options
#define XBEE_IO_DISABLED 0
#define XBEE_IO_ANALOGIN 2
#define XBEE_IO_DIGITALIN 3
#define XBEE_IO_DIGOUT_HIGH 4
#define XBEE_IO_DIGOUT_LOW 5

/** \defgroup XBeePacketTypes XBee Packet Types
	The different types of packet that can be used with the \ref XBee subsystem.

  These structures are to be used when the module has already been set into packet (API) mode
  with a call to XBee_SetPacketApiMode( )

  \ingroup XBee
  @{
*/

/**
  The structure used for a standard AT Command packet.
*/
typedef struct
{
  uint8 frameID;          /**< the ID for this packet that subsequent response/status messages can refer to. */
  uint8 command[ 2 ];     /**< the 2-character AT command. */
  uint8 parameters[ 97 ]; /**< a buffer that holds the value of the AT command. */
} XBee_ATCommand;

/** 
  The structure used for a response to a standard AT Command packet.
*/
typedef struct
{
  uint8 frameID;      /**< the ID that this response is referring to. */
  uint8 command[ 2 ]; /**< the 2-character AT command. */
  uint8 status;       /**< response value - 0 (OK) or 1 (ERROR). */
  uint8 value[ 96 ];  /**< a buffer containing the value of the AT command response. */
} XBee_ATCommandResponse;

/** 
  The structure used to transmit a data packet with a 64-bit destination address.
*/
typedef struct
{
  uint8 frameID;          /**< the ID for this packet that subsequent response/status messages can refer to. */
  uint8 destination[ 8 ]; /**< the 64-bit (8-byte) address of the destination. */
  uint8 options;          /**< 0x01 (disable ACK) or 0x04 (Send with Broadcast PAN ID). */
  uint8 data[ 90 ];       /**< a buffer containing the value of the outgoing packet. */
}  XBee_TX64;

/** 
  The structure used to transmit a data packet with a 16-bit destination address.
 */
typedef struct
{
  uint8 frameID;          /**< the ID for this packet that subsequent response/status messages can refer to. */
  uint8 destination[ 2 ]; /**< the 16-bit (2-byte) address of the destination. */
  uint8 options;          /**< 0x01 (disable ACK) or 0x04 (Send with Broadcast PAN ID). */
  uint8 data[ 96 ];       /**< a buffer containing the value of the outgoing packet. */
}  XBee_TX16;

/** 
  When a transmit request is completed, the module sends a transmit status message.
 */
typedef struct
{
  uint8 frameID; /**< the ID that this response is referring to. */
  uint8 status;  /**< can have values of:
    - 0 - success
    - 1 - No ACK received
    - 2 - CCA failure
    - 3 - Purged. */
}  XBee_TXStatus;

/** 
  An incoming data packet with a 64-bit address.
 */
typedef struct
{
  uint8 source[ 8 ]; /**< the 64-bit (8-byte) address of the sender. */
  uint8 rssi;        /**< the signal strength of the received message. */
  uint8 options;     /**< bit 1 is Address Broadcast, bit 2 is PAN broadcast.  Other bits reserved. */
  uint8 data[ 89 ];  /**< a buffer containing the value of the incoming packet. */
}  XBee_RX64;

/** 
  An incoming data packet with a 16-bit address.
 */
typedef struct
{
  uint8 source[ 2 ]; /**< the 16-bit (2-byte) address of the sender. */
  uint8 rssi;        /**< the signal strength of the received message. */
  uint8 options;     /**< bit 1 is Address Broadcast, bit 2 is PAN broadcast.  Other bits reserved. */
  uint8 data[ 95 ];  /**< a buffer containing the value of the incoming packet. */
}  XBee_RX16;

/** 
  An incoming packet with IO data from a 64-bit address.
 */
typedef struct
{
  uint8 source[ 8 ];            /**< the 64-bit (8-byte) address of the sender. */
  uint8 rssi;                   /**< the signal strength of the received message. */
  uint8 options;                /**< bit 1 is Address Broadcast, bit 2 is PAN broadcast.  Other bits reserved. */
  uint8 samples;                /**< the number of samples in this packet. */
  uint8 channelIndicators[ 2 ]; /**< bit mask indicating which channels have been sampled. */
  uint8 data[ 86 ];             /**< a buffer containing the IO values as indicated by \b channelIndicators. */
}  XBee_IO64;

/**
  An incoming packet with IO data from a 16-bit address.
 */
typedef struct
{
  uint8 source[ 2 ];            /**< the 16-bit (2-byte) address of the sender. */
  uint8 rssi;                   /**< the signal strength of the received message. */
  uint8 options;                /**< bit 1 is Address Broadcast, bit 2 is PAN broadcast.  Other bits reserved. */
  uint8 samples;                /**< the number of samples in this packet. */
  uint8 channelIndicators[ 2 ]; /**< a bit mask indicating which channels have been sampled. */
  uint8 data[ 92 ];             /**< a buffer containing the IO values as indicated by \b channelIndicators. */
}  XBee_IO16;

/**
  Possible API IDs for different kinds of packets.
  In the main XBeePacket structure, its \b apiId member will be set to one of these values,
  indicating which kind of packet it is.

  \b Example
  \code
  XBeePacket* xbp;
  if( xbp->apiId == XBEE_IO16 )
  {
    // then we have an XBee_IO16 packet,
    // accessible at xbp->io16
  }
  \endcode
*/
enum XBeeApiId
{ 
  XBEE_TX64 = 0x00,               /**< An outgoing data packet with a 64-bit address. */
  XBEE_TX16 = 0x01,               /**< An outgoing data packet with a 16-bit address. */
  XBEE_TXSTATUS = 0x89,           /**< TX status packet. */
  XBEE_RX64 = 0x80,               /**< An incoming data packet with a 64-bit address. */
  XBEE_RX16 = 0x81,               /**< An incoming data packet with a 16-bit address. */
  XBEE_ATCOMMAND = 0x08,          /**< An AT command packet. */
  XBEE_ATCOMMANDQ = 0x09,         /**< An AT command queue packet. */
  XBEE_ATCOMMANDRESPONSE = 0x88,  /**< A response to an AT command query. */
  XBEE_IO64 = 0x82,               /**< An incoming IO packet with a 64-bit address. */
  XBEE_IO16 = 0x83                /**< An incoming IO packet with a 16-bit address. */
};

/** @}
*/

/**
  Representation of an XBee packet.
  The XBeePacket structure consists of an API ID, indicating the kind of packet it is, as well as 
  providing the structure for that particular packet.  Only one packet type will be valid at a time,
  as indicated by the \b apiId:
  
  \b Example
  \code
  XBeePacket* xbp;
  if( xbp->apiId == XBEE_IO16 )
  {
    int signalStrength = xbp->io16.rssi;
    // and so on...
  }
  \endcode
  See \ref XBeeApiId for a list of valid API IDs.

  \ingroup XBee
*/
typedef struct
{
  uint8 apiId; /**< the API ID for this packet that subsequent response/status messages can refer to. */
  union
  {
    uint8 payload[ 100 ];
    XBee_TX64 tx64;                    /**< TX 64 packet. */
    XBee_TX16 tx16;                    /**< TX 16 packet. */
    XBee_TXStatus txStatus;            /**< TX status packet. */
    XBee_RX64 rx64;                    /**< RX 64 packet. */
    XBee_RX16 rx16;                    /**< RX 16 packet. */
    XBee_ATCommand atCommand;          /**< AT Command packet. */
    XBee_ATCommandResponse atResponse; /**< AT Command Response packet. */
    XBee_IO64 io64;                    /**< IO 64 packet. */
    XBee_IO16 io16;                    /**< IO 16 packet. */
  }; 
  
  uint8 crc;       /**< The checksum for this packet - mostly used internally by XBee_SendPacket() and XBee_GetPacket().  */
  uint8 *dataPtr;  /**< A pointer into the packet structure itself.  */
  int rxState;     /**< Used internally by XBee_GetPacket() to keep track of the parse state. */
  int length;      /**< The length of a packet - only useful after a successful call to XBee_GetPacket().  */
  int index;       /**< Used internally by XBee_GetPacket() to keep track of the current length.  */
} __attribute__((packed)) XBeePacket;

int XBee_SetActive( int state );
int XBee_GetActive( void );
int XBee_GetPacket( XBeePacket* packet, int timeout );
int XBee_SendPacket( XBeePacket* packet, int datalength );
void XBee_ResetPacket( XBeePacket* packet );
int XBee_Write( uchar *buffer, int count, int timeout );
int XBee_Read( uchar* buffer, int count, int timeout );
int XBee_IsBusyPacket( XBeePacket* packet );

// packet creators and unpackers
bool XBee_CreateTX16Packet( XBeePacket* xbp, uint8 frameID, uint16 destination, uint8 options, uint8* data, uint8 datalength );
bool XBee_CreateTX64Packet( XBeePacket* xbp, uint8 frameID, uint64 destination, uint8 options, uint8* data, uint8 datalength );
bool XBee_ReadRX16Packet( XBeePacket* xbp, uint16* srcAddress, uint8* sigstrength, uint8* options, uint8** data, uint8* datalength );
bool XBee_ReadRX64Packet( XBeePacket* xbp, uint64* srcAddress, uint8* sigstrength, uint8* options, uint8** data, uint8* datalength  );
bool XBee_ReadIO16Packet( XBeePacket* xbp, uint16* srcAddress, uint8* sigstrength, uint8* options, int* samples );
bool XBee_ReadIO64Packet( XBeePacket* xbp, uint64* srcAddress, uint8* sigstrength, uint8* options, int* samples );
void XBee_CreateATCommandPacket( XBeePacket* packet, uint8 frameID, char* cmd, uint8* params, uint8 datalength );
bool XBee_ReadAtResponsePacket( XBeePacket* xbp, uint8* frameID, char** command, uint8* status, int* datavalue );
bool XBee_ReadTXStatusPacket( XBeePacket* xbp, uint8* frameID, uint8* status );

// XBee Config stuff
void XBeeConfig_SetPacketApiMode( int value );
int XBeeConfig_RequestPacketApiMode( void );
void XBeeConfig_WriteStateToMemory( void );
void XBeeConfig_SetAddress( uint16 address );
int XBeeConfig_RequestAddress( void );
void XBeeConfig_SetPanID( uint16 id );
int XBeeConfig_RequestPanID( void );
void XBeeConfig_SetChannel( uint8 channel );
int XBeeConfig_RequestChannel( void );
void XBeeConfig_SetSampleRate( uint16 rate );
int XBeeConfig_RequestSampleRate( void );
void XBeeConfig_SetIO( int index, int value );
int XBeeConfig_RequestIO( int pin );
int XBeeConfig_RequestATResponse( char* cmd );

bool XBee_GetAutoSend( bool init );
void XBee_SetAutoSend( int onoff );

void XBee_IntToBigEndianArray( int value, uint8* array );


// XBee OSC stuff
const char* XBeeOsc_GetName( void );
int XBeeOsc_ReceiveMessage( int channel, char* message, int length );
int XBeeOsc_Async( int channel );
int XBeeOsc_HandleNewPacket( XBeePacket* xbp, int channel );

const char* XBeeConfigOsc_GetName( void );
int XBeeConfigOsc_ReceiveMessage( int channel, char* message, int length );

#endif // XBEE_H
