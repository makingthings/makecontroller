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
  WebClient.h
  MakingThings
*/

#ifndef XBEE_H
#define XBEE_H

#include "types.h"

typedef struct
{
  uint8 frameID;
  uint8 command[ 2 ];
  uint8 parameter[ 100 ];
}  XBee_ATCommand;

typedef struct
{
  uint8 frameID;
  uint8 command[ 2 ];
  uint8 status;
  uint8 value[ 100 ];
}  XBee_ATCommandResponse;

typedef struct
{
  uint8 frameID;
  uint8 destination[ 8 ];
  uint8 options;
  uint8 data[ 100 ];
}  XBee_TX64;

typedef struct
{
  uint8 frameID;
  uint8 destination[ 2 ];
  uint8 options;
  uint8 data[ 100 ];
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
  uint8 data[ 100 ];

}  XBee_RX64;

typedef struct
{
  uint8 frameID;
  uint8 source[ 2 ];
  uint8 rssi;
  uint8 options;
  uint8 data[ 100 ];

}  XBee_RX16;

typedef struct
{
  enum { XB_IDLE, XB_REC_ } state;
} XBee_;

typedef struct
{
  enum 
  { 
    XBEE_COMM_TX64 = 0x00,
    XBEE_COMM_TX16 = 0x01,
    XBEE_COMM_TXSTATUS = 0x89,
    XBEE_COMM_RX64 = 0x80,
    XBEE_COMM_RX16 = 0x81,
    XBEE_COMM_ATCOMMAND = 0x08,
    XBEE_COMM_ATCOMMANDQ = 0x09,
    XBEE_COMM_ATCOMMANDRESPONSE = 0x88
  } apiIndentifier;

  union
  {
    XBee_TX64 tx64;
    XBee_TX16 tx16;
    XBee_TXStatus txStatus;
    XBee_RX64 rx64;
    XBee_RX16 rx16;
    XBee_ATCommand;
    XBee_ATCommandResponse;
  };
} XBeePacket_;


int XBee_SetActive( int state );
int XBee_GetActive( );

int XBee_GetPacket();

#endif // XBEE_H
