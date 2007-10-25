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

/*
  xbee.h
  MakingThings
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
  - \b frameID is the ID for this packet that subsequent response/status messages can refer to.
  - \b command is the 2-character AT command.
  - \b parameters is a buffer that holds the value of the AT command.
 */
typedef struct
{
  uint8 frameID;
  uint8 command[ 2 ];
  uint8 parameters[ 97 ];
} XBee_ATCommand;

/** 
  The structure used for a response to a standard AT Command packet.
  - \b frameID is the ID that this response is referring to.
  - \b command is the 2-character AT command.
  - \b status is response value - 0 (OK) or 1 (ERROR).
  - \b value is a buffer that holds the value of the AT command response.
 */
typedef struct
{
  uint8 frameID;
  uint8 command[ 2 ];
  uint8 status;
  uint8 value[ 96 ];
}  XBee_ATCommandResponse;

/** 
  The structure used to transmit an XBee packet with a 64-bit destination address.
  - \b frameID is the ID for this packet that subsequent response/status messages can refer to.
  - \b destination is the 64-bit (8-byte) address of the destination.
  - \b options - 0x01 (disable ACK) or 0x04 (Send with Broadcast PAN ID).
  - \b data is a buffer that holds the value of the outgoing packet.
 */
typedef struct
{
  uint8 frameID;
  uint8 destination[ 8 ];
  uint8 options;
  uint8 data[ 90 ];
}  XBee_TX64;

/** 
  The structure used to transmit an XBee packet with a 16-bit destination address.
  - \b frameID is the ID for this packet that subsequent response/status messages can refer to.
  - \b destination is the 16-bit (2-byte) address of the destination.
  - \b options - 0x01 (disable ACK) or 0x04 (Send with Broadcast PAN ID).
  - \b data is a buffer that holds the value of the outgoing packet.
 */
typedef struct
{
  uint8 frameID;
  uint8 destination[ 2 ];
  uint8 options;
  uint8 data[ 96 ];
}  XBee_TX16;

/** 
  When a transmit request is completed, the module sends a transmit status message.
  - \b frameID is the ID that this response is referring to.
  - \b status can have values of:
    - 0 - success
    - 1 - No ACK received
    - 2 - CCA failure
    - 3 - Purged
 */
typedef struct
{
  uint8 frameID;
  uint8 status;
}  XBee_TXStatus;

/** 
  An incoming packet with a 64-bit address.
  - \b source is the 64-bit (8-byte) address of the sender.
  - \b rssi is the signal strength of the received message.
  - \b options - bit 1 is Address Broadcast, bit 2 is PAN broadcast.  Other bits reserved.
  - \b data is a buffer that holds the value of the incoming packet.
 */
typedef struct
{
  uint8 source[ 8 ];
  uint8 rssi;
  uint8 options;
  uint8 data[ 89 ];
}  XBee_RX64;

/** 
  An incoming packet with a 16-bit address.
  - \b source is the 16-bit (2-byte) address of the sender.
  - \b rssi is the signal strength of the received message.
  - \b options - bit 1 is Address Broadcast, bit 2 is PAN broadcast.  Other bits reserved.
  - \b data is a buffer that holds the value of the incoming packet.
 */
typedef struct
{
  uint8 source[ 2 ];
  uint8 rssi;
  uint8 options;
  uint8 data[ 95 ];
}  XBee_RX16;

/** 
  An incoming packet with IO data from a 64-bit address.
  - \b source is the 64-bit (8-byte) address of the sender.
  - \b rssi is the signal strength of the received message.
  - \b options - bit 1 is Address Broadcast, bit 2 is PAN broadcast.  Other bits reserved.
  - \b samples - the number of samples in this packet.
  - \b channelIndicators - bit mask indicating which channels have been sampled.
  - \b data is a buffer that holds the IO values as indicated by \b channelIndicators.
 */
typedef struct
{
  uint8 source[ 8 ];
  uint8 rssi;
  uint8 options;
  uint8 samples;
  uint8 channelIndicators[ 2 ];
  uint8 data[ 86 ];
}  XBee_IO64;

/** 
  An incoming packet with IO data from a 16-bit address.
  - \b source is the 16-bit (2-byte) address of the sender.
  - \b rssi is the signal strength of the received message.
  - \b options - bit 1 is Address Broadcast, bit 2 is PAN broadcast.  Other bits reserved.
  - \b samples - the number of samples in this packet.
  - \b channelIndicators - bit mask indicating which channels have been sampled.
  - \b data is a buffer that holds the IO values as indicated by \b channelIndicators.
 */
typedef struct
{
  uint8 source[ 2 ];
  uint8 rssi;
  uint8 options;
  uint8 samples;
  uint8 channelIndicators[ 2 ];
  uint8 data[ 92 ];
}  XBee_IO16;

typedef enum 
{ 
  XBEE_COMM_TX64 = 0x00,
  XBEE_COMM_TX16 = 0x01,
  XBEE_COMM_TXSTATUS = 0x89,
  XBEE_COMM_RX64 = 0x80,
  XBEE_COMM_RX16 = 0x81,
  XBEE_COMM_ATCOMMAND = 0x08,
  XBEE_COMM_ATCOMMANDQ = 0x09,
  XBEE_COMM_ATCOMMANDRESPONSE = 0x88,
  XBEE_COMM_IO64 = 0x82,
  XBEE_COMM_IO16 = 0x83
} XBeeApiId;

/** @}
*/

typedef struct
{
  uint8 apiId;
  union
  {
    uint8 payload[ 100 ];
    XBee_TX64 tx64;
    XBee_TX16 tx16;
    XBee_TXStatus txStatus;
    XBee_RX64 rx64;
    XBee_RX16 rx16;
    XBee_ATCommand atCommand;
    XBee_ATCommandResponse atResponse;
    XBee_IO64 io64;
    XBee_IO16 io16;
  };
  
  uint8 crc;
  uint8 *dataPtr;
  int rxState;
  int length;
  int index;
} __attribute__((packed)) XBeePacket;


int XBee_SetActive( int state );
int XBee_GetActive( void );
int XBee_GetPacket( XBeePacket* packet );
int XBee_SendPacket( XBeePacket* packet, int datalength );
void XBee_SetPacketApiMode( void );
void XBee_InitPacket( XBeePacket* packet );
void XBee_CreateATCommandPacket( XBeePacket* packet, uint8 frameID, char* cmd, uint8* params, int datalength );
int XBee_GetIOValues( XBeePacket* packet, int *inputs );
void XBee_SetIOConfig( int ioconfig[], int samplerate );
bool  XBee_GetTX16( XBeePacket* xbp );
uint8  XBee_GetTX16Length( XBeePacket* xbp );
uint8* XBee_GetTX16Data( XBeePacket* xbp );
uint8  XBee_GetRX16Length( XBeePacket* xbp );
uint8* XBee_GetRX16Data( XBeePacket* xbp );
uint8  XBee_GetSignalStrength( XBeePacket* xbp );
int  XBee_GetSourceAddress( XBeePacket* xbp );

void XBeeConfig_WriteStateToMemory( void );
void XBeeConfig_SetAddress( int address );
//void XBeeConfig_SetDestinationAddress64( uint64 address );
void XBeeConfig_SetDestinationAddress16( uint16 address );
void XBeeConfig_SetPanID( uint16 id );
void XBeeConfig_SetChannel( uint8 channel );
void XBeeConfig_SetSampleRate( uint16 rate );


// XBee OSC stuff
const char* XBeeOsc_GetName( void );
int XBeeOsc_ReceiveMessage( int channel, char* message, int length );

#endif // XBEE_H
