/*********************************************************************************

 Copyright 2006-2009 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#ifndef XBEE_H
#define XBEE_H

#include "types.h"
#include "serial.h"

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
  The different types of XBeePacket.

  These structures are to be used when the module has already been set into packet (API) mode
  with a call to XBee_SetPacketApiMode( )
  
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
  Possible XBeePacket types.
  You can easily check a XBeePacket's type by calling its type() method.
*/
enum XBeePacketType
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
  Internal representation of packet data.
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
} __attribute__((packed)) PacketData;

/**
  Communicate with XBee (Zigbee) wireless modules via the Make Controller's serial port.

  XBee modules from \b MaxStream are small, cheap ($19 each), wireless \b RF (radio frequency) modules that
  can easily be used with the Make Controller Kit to create projects that require wireless communication.  Check
  http://www.maxstream.net/products/xbee/xbee-oem-rf-module-zigbee.php for more info.

  \section Overview
  XBee modules are <b>ZigBee/IEEE 802.15.4</b> compliant and can operate in several modes:
  - Transparent serial port.  Messages in one side magically end up at the other endpoint.  Great for enabling wireless
  communication between 2 Make Controllers.
  - AT command mode.  Send traditional AT commands to configure the module itself (as opposed to having
  the data go straight through via serial mode.
  - Packet (API) mode.  Lower level communication that doesn't have to wait for the module to be in
  AT command mode.  Check the \ref XBeePacketTypes for a description of how these packets are laid out.  
  Be sure to call setPacketApiMode() before trying to do anything in this mode.
  
  \section Usage
  The general idea is that you have one XBee module connected directly to your Make Controller Kit, and then
  any number of other XBee modules that can communicate with it in order to get information to and from the
  Make Controller.  Or, several Make Controllers can each have an XBee module connected in order to
  communicate wirelessly among themselves.

  XBee modules also have some digital and analog I/O right on them, which means you can directly connect
  sensors to the XBee modules which will both read the values and send them wirelessly.  Check the XBee doc
  for the appropriate commands to send in order to set this up.
  
  To get started, create an XBeePacket object and start reading & writing with it.  \b Note - this system is
  designed to reuse an existing XBeePacket object, as opposed to making a new one each read/write.  Calling reset()
  between operations will clear everything out.
  
  Once you've received a packet, you can extract the details with unpackRX16(), unpackRX64(), 
  unpackIO16() and unpackIO64(), depending on the type of packet.  To create a new packet to send out, use
  tx16(), tx64(), and atCmd() depending on the kind of message you want to send.
  
  \b Example
  \code
  XBeePacket xb; // create our object
  xb.setPacketApiMode(true); // turn on packet mode - always do this first
  
  while(true) // forever...assuming we're in a task loop here
  {
    if(xb.get())
    {
      // ... process the newly received packet here
      if(xb.type() == XBEE_RX16) // check what kind of packet it is
      {
        uint16 source;  // variable to read the source address into
        uint8 strength; // variable to read the signal strength into
        xb.unpackRX16( &source, &strength );
        // now source and strength are set to the values received in our packet
        // ... do something here if you like depending on what we got ...
      }
      xb.reset(); // and reset it before our next round through the loop
    }
    Task::sleep(5); // always sleep a little bit in our loop
  }
  \endcode

  \section serialmode Serial Mode
  The Make Controller API for working with the XBee modules makes use of the XBee Packet API.  If you simply want to make use
  of the transparent serial port functionality, you can just use the Serial port directly.

  Bytes sent in this way are broadcast to all XBee modules on the same chanel and with the same PAN ID.  All similarly
  configured modules will receive all bytes sent.  However, because these bytes are broadcast, there is no message
  reliability, so there's no guarantee that the messages will actually get there.
  
  \section packetapimode Packet API Mode
  The XBee Packet API allows for much more flexible and powerful communication with the modules.  With the Packet API
  you can address messages to a specific module, detect where messages came from, check signal strength, and
  packets are sent with a send / acknowledges / retry scheme which greatly increases message reliability.

  The packet API uses commands to configure the XBee module itself, and then a handful of Zigbee specified packet types can be sent and received.  
  See \ref XBeePacketTypes for details on these packet types.
*/
class XBeePacket
{
public:
  XBeePacket( int serialChannel = 0 );
  ~XBeePacket( );
  void tx16( uint8 frameID, uint16 destination, uint8 options, uint8* data, uint8 datalength );
  void tx64( uint8 frameID, uint64 destination, uint8 options, uint8* data, uint8 datalength );
  void atCmd( uint8 frameID, const char* cmd, int value = 0 );
  bool get( int timeout = 0 );
  bool send( int datalength = 0 );
  void reset( );
  XBeePacketType type();
  bool setPacketApiMode( bool enabled );
  bool unpackRX16( uint16* srcAddress, uint8* sigstrength = 0, uint8* options = 0, uint8** data = 0, uint8* datalength = 0 );
  bool unpackRX64( uint64* srcAddress, uint8* sigstrength = 0, uint8* options = 0, uint8** data = 0, uint8* datalength = 0  );
  bool unpackIO16( uint16* srcAddress, uint8* sigstrength = 0, uint8* options = 0, int* samples = 0 );
  bool unpackIO64( uint64* srcAddress, uint8* sigstrength = 0, uint8* options = 0, int* samples = 0 );
  bool unpackAtResponse( uint8* frameID, char** command = 0, uint8* status = 0, int* datavalue = 0 );
  bool unpackTXStatus( uint8* frameID, uint8* status = 0 );
  PacketData packetData; /**< Internal representation of the packet data. */

protected:
  Serial* ser;
  bool getIOValues( int *inputs );
};

// XBee OSC stuff
//const char* XBeeOsc_GetName( void );
//int XBeeOsc_ReceiveMessage( int channel, char* message, int length );
//int XBeeOsc_Async( int channel );
//int XBeeOsc_HandleNewPacket( XBeePacket* xbp, int channel );
//bool XBee_GetAutoSend( bool init );
//void XBee_SetAutoSend( int onoff );
//const char* XBeeConfigOsc_GetName( void );
//int XBeeConfigOsc_ReceiveMessage( int channel, char* message, int length );

#endif // XBEE_H
