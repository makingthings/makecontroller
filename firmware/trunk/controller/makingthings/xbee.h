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
#define XBEE_PACKET_RX_CMD_ID 3
#define XBEE_PACKET_RX_DATA 4
#define XBEE_PACKET_RX_CRC 5

// general xbee constants
#define XBEE_PACKET_STARTBYTE 0x7E
#define XBEE_PACKET_ESCAPE 0x7D

typedef struct
{
  uint8 frameID;
  uint8 command[ 2 ];
  uint8 parameter[ 97 ];
} XBee_ATCommand;

typedef struct
{
  uint8 frameID;
  uint8 command[ 2 ];
  uint8 status;
  uint8 value[ 96 ];
}  XBee_ATCommandResponse;

typedef struct
{
  uint8 frameID;
  uint8 destination[ 8 ];
  uint8 options;
  uint8 data[ 90 ];
}  XBee_TX64;

typedef struct
{
  uint8 frameID;
  uint8 destination[ 2 ];
  uint8 options;
  uint8 data[ 96 ];
}  XBee_TX16;

typedef struct
{
  uint8 frameID;
  uint8 destination[ 2 ];
  uint8 status;
}  XBee_TXStatus;

typedef struct
{
  uint8 frameID;
  uint8 source[ 8 ];
  uint8 rssi;
  uint8 options;
  uint8 data[ 89 ];
}  XBee_RX64;

typedef struct
{
  uint8 frameID;
  uint8 source[ 2 ];
  uint8 rssi;
  uint8 options;
  uint8 data[ 95 ];
}  XBee_RX16;

typedef struct
{
  enum { XB_IDLE, XB_SLEEP, XB_RECEIVE, XB_TRANSMIT, XB_COMMAND } mode;
} XBee_;

typedef enum 
{ 
  XBEE_COMM_TX64 = 0x00,
  XBEE_COMM_TX16 = 0x01,
  XBEE_COMM_TXSTATUS = 0x89,
  XBEE_COMM_RX64 = 0x80,
  XBEE_COMM_RX16 = 0x81,
  XBEE_COMM_ATCOMMAND = 0x08,
  XBEE_COMM_ATCOMMANDQ = 0x09,
  XBEE_COMM_ATCOMMANDRESPONSE = 0x88
} XBeeApiId;

typedef struct
{
  uint8 apiId;
  union
  {
    uint8 data[ 100 ];
    XBee_TX64 tx64;
    XBee_TX16 tx16;
    XBee_TXStatus txStatus;
    XBee_RX64 rx64;
    XBee_RX16 rx16;
    XBee_ATCommand atCommand;
    XBee_ATCommandResponse atResponse;
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

#endif // XBEE_H
