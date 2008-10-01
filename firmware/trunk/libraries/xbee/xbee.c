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
#include "config.h"

#include "stdlib.h"
#include <stdio.h>
#include "string.h"
#include "xbee.h"

static bool XBee_GetIOValues( XBeePacket* packet, int *inputs );
void XBeeTask( void* p );
#define XBEEPACKET_Q_SIZE 5
#define XBEE_OSC_RX_TIMEOUT 500

typedef struct
{
  XBeePacket* currentPkt;
  int packetIndex;
  #ifdef OSC
  bool autosend;
  #endif
  bool waitingForConfirm;
} XBeeSubsystem;

XBeeSubsystem* XBee;

/** \defgroup XBee XBee
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
  Be sure to call XBeeConfig_SetPacketApiMode before trying to do anything in this mode.

  The general idea is that you have one XBee module connected directly to your Make Controller Kit, and then
  any number of other XBee modules that can communicate with it in order to get information to and from the
  Make Controller.  Or, several Make Controllers can each have an XBee module connected in order to
  communicate wirelessly among themselves.

  XBee modules also have some digital and analog I/O right on them, which means you can directly connect
  sensors to the XBee modules which will both read the values and send them wirelessly.  Check the XBee doc
  for the appropriate commands to send in order to set this up.

  \section API
  The Make Controller API for working with the XBee modules makes use of the XBee Packet API.  If you simply want to make use
  of the transparent serial port functionality, you can use the following functions:
    XBee_SetActive( )
    XBee_Write(  );
    XBee_Read( );
    XBee_GetReadable( );

  Or if you want to handle setup etc. yourself, you don't need to deal with these - just hook up the module to your 
  Make Controller and start reading and writing over the serial port.

  Bytes sent in this way are broadcast to all XBee modules on the same chanel and with the same PAN ID.  All similarly
  configured modules will receive all bytes sent.  However, because these bytes are broadcast, there is no message
  reliability, so there's no guarantee that the messages will actually get there.

  The XBee Packet API allows for much more flexible and powerful communication with the modules.  With the Packet API
  you can send messages to a specific module, detect where messages came from, check signal strength, and most importantly
  packets are sent with a send / acknowledges / retry scheme which greatly increases message reliability.
  
  Packet API uses commands to configure the XBee module itself, and then a handful of Zigbee specified packet types can be sent and received.  
  See \ref XBeePacketTypes for details on these packet types.  
	
	The \b XBeeConfig_ functions are convenient wrappers around some of the most common AT commands you might want to send.  For
	any of the other AT commands, check the XBee documentation and create them using XBee_CreateATCommandPacket( ).  These will always 
	be sent to the XBee module attached to the Make Controller.  The \b XBee_ functions deal with sending and receiving 
	messages to other XBee modules not connected to the Make Controller.

  \ingroup Libraries
  @{
*/

/**	
  Controls the active state of the \b XBee subsystem
  @param state Whether this subsystem is active or not
	@return Zero on success.
*/
int XBee_SetActive( int state )
{
  if( state ) 
  { 
    if( XBee == NULL )
    {
      if( CONTROLLER_OK != Serial_SetActive( SERIAL_0, 1 ) )
        return CONTROLLER_ERROR_SUBSYSTEM_INACTIVE;
      
      // Configure the serial port
      Serial_SetBaud( SERIAL_0, 9600 );
      Serial_SetBits( SERIAL_0, 8 );
      Serial_SetParity( SERIAL_0, 0 );
      Serial_SetStopBits( SERIAL_0, 1 );

      XBee = MallocWait( sizeof( XBeeSubsystem ), 100 );
      XBee->packetIndex = 0;
      #ifdef OSC
        XBee->autosend = XBee_GetAutoSend( true );
      #endif
      XBee->currentPkt = MallocWait( sizeof( XBeePacket ), 100 );
      XBee_ResetPacket( XBee->currentPkt );
      XBee->waitingForConfirm = false;
    }
  }
  else
  {
    Serial_SetActive( SERIAL_0, 0 );

    if( XBee )
    {
      Free( XBee );
      Free( XBee->currentPkt );
      XBee = NULL;
    }
  }
  return CONTROLLER_OK;
}

/**	
  Read the active state of the \b XBee subsystem
	@return An integer specifying the active state - 1 (active) or 0 (inactive).
*/
int XBee_GetActive( )
{
  return Serial_GetActive( SERIAL_0 );
}


/**	
  Write the specified number of bytes into the XBee unit.  It is
  assumed that the unit is in TRANSPARENT, not in PACKET API mode.  To write to the 
  unit using packets first set packet mode (using XBeeConfig_SetPacketApiMode) then use
  XBee_CreateXXXPacket() followed by XBee_SendPacket().
	@param buffer The block of bytes to write
  @param count The number of bytes to write
  @param timeout The time in ms to linger waiting to succeed (0 for no wait)
  @return status
*/
int XBee_Write( uchar *buffer, int count, int timeout )
{
  return Serial_Write( SERIAL_0, buffer, count, timeout );
}

/**	
	Read data from the Xbee unit waiting for the specified time. Use XBee_GetReadable() to 
  determine how many bytes are waiting to avoid waiting.    It is
  assumed that the unit is in TRANSPARENT, not in PACKET API mode.  To write to the 
  unit using packets first set packet mode (using XBeeConfig_SetPacketApiMode) then use
  XBee_CreateXXXPacket() followed by XBee_SendPacket().
	@param buffer A pointer to the buffer to read into.
	@param count An integer specifying the maximum number of bytes to read.
  @param timeout Time in milliseconds to block waiting for the specified number of bytes. 0 means don't wait.
  @return number of bytes read (>=0) or error <0 .
*/
int XBee_Read( uchar* buffer, int count, int timeout )
{
  return Serial_Read( SERIAL_0, buffer, count, timeout );
}

/**	
	Returns the number of bytes in the queue waiting to be read.
  @return bytes in the receive queue.
*/
int XBee_GetReadable( void );

/**	
  Receive an incoming XBee packet.
  A single call to this will continue to read from the serial port as long as there are characters 
  to read or until it times out.  If a packet has not been completely received, call it repeatedly
  with the same packet.

  Clear out a packet before reading into it with a call to XBee_ResetPacket( )
  @param packet The XBeePacket to receive into.
  @param timeout The number of milliseconds to wait for a packet to arrive.  Set this to 0 to return as
  soon as there are no characters left to process.
	@return 1 if a complete packet has been received, 0 if not.
  @see XBeeConfig_SetPacketApiMode( )
  @todo Probably need some way to reset the parser if there are no bytes for a while

  \par Example
  \code
  // we're inside a task here...
  XBeePacket myPacket;
  XBee_ResetPacket( &myPacket );
  while( 1 )
  {
    if( XBee_GetPacket( &myPacket, 100 ) )
    {
      // process the new packet
      XBee_ResetPacket( &myPacket ); // then clear it out before reading again
    }
    Sleep( 10 );
  }
  \endcode
*/
int XBee_GetPacket( XBeePacket* packet, int timeout )
{
  if( CONTROLLER_OK != XBee_SetActive( 1 ) )
    return CONTROLLER_ERROR_SUBSYSTEM_INACTIVE;

  int time = TaskGetTickCount( );

  do
  {
    Serial_ClearErrors( SERIAL_0 );
    while( Serial_GetReadable( SERIAL_0 ) )
    {
      int newChar = Serial_GetChar( SERIAL_0 );
      if( newChar == -1 )
        break;
  
      switch( packet->rxState )
      {
        case XBEE_PACKET_RX_START:
          if( newChar == XBEE_PACKET_STARTBYTE )
            packet->rxState = XBEE_PACKET_RX_LENGTH_1;
          break;
        case XBEE_PACKET_RX_LENGTH_1:
          packet->length = newChar;
          packet->length <<= 8;
          packet->rxState = XBEE_PACKET_RX_LENGTH_2;
          break;
        case XBEE_PACKET_RX_LENGTH_2:
          packet->length += newChar;
          if( packet->length > XBEE_MAX_PACKET_SIZE ) // in case we somehow get some garbage
            packet->rxState = XBEE_PACKET_RX_START;
          else
            packet->rxState = XBEE_PACKET_RX_PAYLOAD;
          packet->crc = 0;
          break;
        case XBEE_PACKET_RX_PAYLOAD:
          *packet->dataPtr++ = newChar;
          if( ++packet->index >= packet->length )
            packet->rxState = XBEE_PACKET_RX_CRC;
          packet->crc += newChar;
          break;
        case XBEE_PACKET_RX_CRC:
          packet->crc += newChar;
          packet->rxState = XBEE_PACKET_RX_START;
          if( packet->crc == 0xFF )
            return 1;
          else
          {
            XBee_ResetPacket( packet );
            return 0;
          }
      }
    }
    if ( timeout > 0 )
      Sleep( 1 );
  } while( ( TaskGetTickCount( ) - time ) < timeout );
  return 0;
}

/**	
  Send an XBee packet.
	Use the following functions to create packets to be sent:
	- XBee_CreateTX16Packet( ) - create a data packet to be sent out wirelessly with a 16-bit address
	- XBee_CreateTX64Packet( ) - create a data packet to be sent out wirelessly with a 64-bit address
	- XBee_CreateATCommandPacket( ) - create an AT command to configure an attached XBee module
  @param packet The XBeePacket to send.
  @param datalength The length of the actual data being sent (not including headers, options, etc.)
	@return Zero on success.
  @see XBeeConfig_SetPacketApiMode( )

  \par Example
  \code
  XBeePacket txPacket;
  uint8 data[] = "ABC"; // 3 bytes of data
  XBee_CreateTX16Packet( &txPacket, 0, 0, 0, data, 3 );
  XBee_SendPacket( &txPacket, 3 );
  \endcode
*/
int XBee_SendPacket( XBeePacket* packet, int datalength )
{
  if( CONTROLLER_OK != XBee_SetActive( 1 ) )
    return CONTROLLER_ERROR_SUBSYSTEM_INACTIVE;
  
  Serial_SetChar( SERIAL_0, XBEE_PACKET_STARTBYTE );
  int size = datalength;
  switch( packet->apiId )
  {
    case XBEE_TX64: //account for apiId, frameId, 8 bytes destination, and options
      size += 11;
      break;
    case XBEE_TX16: //account for apiId, frameId, 2 bytes destination, and options
      size += 5; 
      break;
    case XBEE_ATCOMMAND: // length = API ID + Frame ID, + AT Command (+ Parameter Value)
      size = (datalength > 0) ? 8 : 4; // if we're writing, there are 4 bytes of data, otherwise, just the length above
      break;
    default:
      size = 0;
      break;
  }

  Serial_SetChar( SERIAL_0, (size >> 8) & 0xFF ); // send the most significant bit
  Serial_SetChar( SERIAL_0, size & 0xFF ); // then the LSB
  packet->crc = 0; // just in case it hasn't been initialized.
  uint8* p = (uint8*)packet;
  while( size-- )
  {
    Serial_SetChar( SERIAL_0, *p );
    packet->crc += *p++;
  }
  //uint8 test = 0xFF - packet->crc;
  Serial_SetChar( SERIAL_0, 0xFF - packet->crc );
  return CONTROLLER_OK;
}

/**	
  Initialize a packet before reading into it.
  @param packet The XBeePacket to initialize.
  @see XBee_GetPacket( )
*/
void XBee_ResetPacket( XBeePacket* packet )
{
  packet->dataPtr = (uint8*)packet;
  packet->crc = 0;
  packet->rxState = XBEE_PACKET_RX_START;
  packet->length = 0;
  packet->index = 0;
  packet->apiId = 0;
  memset( packet->payload, 0, 100 );
}

/** 
  Checks to see if a packet is receiving a message
  @param packet The XBeePacket to check
  @returns true if the packet is busy, false if it's free
*/
int XBee_IsBusyPacket( XBeePacket* packet )
{
  return ( packet->rxState != XBEE_PACKET_RX_START );
}

/**	
  Set a module into packet API mode.  
  XBee modules are in transparent serial port mode by default.  This allows you to work with them
  via the packet API.

  When setting this on, the XBee module needs to wait 1 second after sending the command sequence before it's 
  ready to receive any AT commands - this function will block for that amount of time.  Once you turn it off,
  you won't get any responses to packets you send the module until you turn packet mode on again.
  @param value 1 to turn packet mode on, 0 to turn it off.
	
  \par Example
  \code
	MyTask( void * p )
	{
		XBeeConfig_SetPacketApiMode( 1 ); // initialize the module to be in API mode
		while( 1 )
		{
			// your task here.
		}
	}
  \endcode
*/
void XBeeConfig_SetPacketApiMode( int value )
{
  if( CONTROLLER_OK != XBee_SetActive( 1 ) )
    return;
  
  if( value )
  {
    char buf[50];
    sprintf( buf, "+++" ); // enter command mode
    Serial_Write( SERIAL_0, (uchar*)buf, strlen(buf), 0 );
    Sleep( 1025 ); // have to wait one second after +++ to actually get set to receive in AT mode
    sprintf( buf, "ATAP1,CN\r" ); // turn API mode on, and leave command mode
    Serial_Write( SERIAL_0, (uchar*)buf, strlen(buf), 0 );
    Sleep(50);
    Serial_Flush( SERIAL_0 ); // rip the OKs out of there
  }
  else
  {
    XBeePacket xbp;
    uint8 params[4];
    memset( params, 0, 4 );
    XBee_CreateATCommandPacket( &xbp, 0, "AP", params, 4 );
    XBee_SendPacket( &xbp, 4 );
  }
}

/**	
  Query whether the module is in API mode.
  This will block for up to a 1/2 second waiting for a response from the XBee module.
  @return 0 if off, 1 if enabled, 2 if enabled with escape characters.
	
  \par Example
  \code
  int mode = XBeeConfig_RequestPacketApiMode( );
  if( mode >= 0 )
  {
    // then we have our mode
  }
  \endcode
*/
int XBeeConfig_RequestPacketApiMode( )
{
  XBeePacket xbp;
  XBee_CreateATCommandPacket( &xbp, 0x52, "AP", NULL, 0 );
  XBee_SendPacket( &xbp, 0 );
  return CONTROLLER_OK;
}

/**	
  A convenience function for creating an AT command packet.
  As per the XBee spec, AT Command packets that want to set/write a value must have 4 bytes of data.  
  Packets that query the value of an AT parameter send 0 bytes of data.  Note that multi-byte
  data must be sent \b big-endian (most significant byte first) - see XBee_IntToBigEndianArray( ).
  
  Make sure you're in API mode before creating & sending packets - see XBeeConfig_SetPacketApiMode( ).
	See the XBee documentation for the official list of AT commands that the XBee modules understand.
  @param packet The XBeePacket to create.
  @param frameID The frame ID for this packet that subsequent response/status messages can refer to.
  @param cmd The 2-character AT command.
  @param params A pointer to the buffer containing the data to be sent.
  @param datalength The number of bytes to send from the params buffer.
  
  \par Example - Writing
  \code
  XBeePacket txPacket;
  uint8 params[4];
  XBee_IntToBigEndianArray( 1000, params ); // set our sampling rate to 1000
  XBee_CreateATCommandPacket( &txPacket, 0, "IR", &params, 4 ); // set the sampling rate of the IO pins
  XBee_SendPacket( &txPacket, 4 );
  \endcode

  \par Example - Reading
  \code
  XBeePacket txPacket;
  XBee_CreateATCommandPacket( &txPacket, 0, "IR", NULL, 0 ); // query the sampling rate of the IO pins
  XBee_SendPacket( &txPacket, 0 );
  // then we'll receive a response packet
  \endcode
*/
void XBee_CreateATCommandPacket( XBeePacket* packet, uint8 frameID, char* cmd, uint8* params, uint8 datalength )
{
  packet->apiId = XBEE_ATCOMMAND;
  packet->atCommand.frameID = frameID;
  uint8* p = packet->atCommand.command;
  *p++ = *cmd++;
  *p++ = *cmd;
  p = packet->atCommand.parameters;
  while( datalength-- )
    *p++ = *params++;
}

/**	
  Query the address of the module.
  This will block for up to a 1/2 second waiting for a response from the XBee module.
  @return An integer corresponding to the address of the module, or negative number on failure.
	
  \par Example
  \code
  int address = XBeeConfig_RequestAddress( );
  if( address >= 0 )
  {
    // then we have our address
  }
  \endcode
*/
int XBeeConfig_RequestATResponse( char* cmd )
{
  XBeePacket xbp;
  XBee_CreateATCommandPacket( &xbp, 0x52, cmd, NULL, 0 );
  XBee_SendPacket( &xbp, 0 );
  return CONTROLLER_OK;
}

/**	
  Configure the IO settings on an XBee module.
  IO pins can have one of 5 values:
  - \b XBEE_IO_DISABLED
  - \b XBEE_IO_ANALOGIN - Analog input (10-bit)
  - \b XBEE_IO_DIGITALIN - Digital input
  - \b XBEE_IO_DIGOUT_HIGH - Digital out high
  - \b XBEE_IO_DIGOUT_LOW - Digital out low
  @param pin An integer specifying which pin to configure. There are 9 IO pins (numbered 0-8) on the XBee modules.  
	Only channels 0-7 can be analog inputs - channel 8 can only operate as a digital in or out.
	@param value An int specifying the behavior of this pin (options shown above).
  \par Example
  \code
  // set channel 0 to analog in
  XBeeConfig_SetIO( 0, XBEE_IO_ANALOGIN );
  \endcode
*/
void XBeeConfig_SetIO( int pin, int value )
{
  XBeePacket packet;
  XBee_ResetPacket( &packet );
  uint8 params[4];
  char cmd[2];
	sprintf( cmd, "D%d", pin );
	XBee_IntToBigEndianArray( value, params );
	XBee_CreateATCommandPacket( &packet, 0, cmd, params, 4 );
	XBee_SendPacket( &packet, 4 );
}

/**	
  Query the configuration of an IO pin.
  See XBeeConfig_SetIO( ) for the possible return values.
  This will block for up to a 1/2 second waiting for a response from the XBee module.
  @return An integer corresponding to the config of the requested pin, or negative number on failure.
	
  \par Example
  \code
  int pin = XBeeConfig_RequestIO( 0 ); // request the configuration of pin 0
  if( pin >= 0 )
  {
    // then we have our pin config
  }
  \endcode
*/
int XBeeConfig_RequestIO( int pin )
{
  XBeePacket xbp;
  char cmd[2];
	sprintf( cmd, "D%d", pin );
  XBee_CreateATCommandPacket( &xbp, 0x52, cmd, NULL, 0 );
  XBee_SendPacket( &xbp, 0 );
  return CONTROLLER_OK;
}

/**	
  Create a packet to be transmitted with a 16-bit address.
  If the \b frameID is 0, you won't receive a TX Status message in response.

  @param xbp The XBeePacket to create.
  @param frameID The frame ID for this packet that subsequent response/status messages can refer to.  Set to 0 for no response.
  @param destination The destination address for this packet.  Broadcast Address: 0xFFFF.
  @param options The XBee options for this packet (0 if none).
  @param data A pointer to the data to be sent in this packet.  Up to 100 bytes.
  @param datalength The number of bytes of data to be sent. Maximum 100 bytes.
	@return True on success, false on failure.
  
  \par Example
  \code
  XBeePacket txPacket;
  uint8 data[] = "ABC";
  XBee_CreateTX16Packet( &txPacket, 0x52, 0, 0, data, 3 );
  XBee_SendPacket( &txPacket, 3 );
  \endcode
*/
bool XBee_CreateTX16Packet( XBeePacket* xbp, uint8 frameID, uint16 destination, uint8 options, uint8* data, uint8 datalength )
{
  xbp->apiId = XBEE_TX16;
  xbp->tx16.frameID = frameID;
  xbp->tx16.destination[0] = destination >> 8;
  xbp->tx16.destination[1] = destination & 0xFF;
  xbp->tx16.options = options;
  xbp->length = datalength + 5;
  uint8* p = xbp->tx16.data;
  while( datalength-- )
    *p++ = *data++;
  return true;
}

/**	
  Create a packet to be transmitted with a 64-bit address.

  @param xbp The XBeePacket to create.
  @param frameID The frame ID for this packet that subsequent response/status messages can refer to.  Set to 0 for no response.
  @param destination The destination address for this packet.  Broadcast Address: 0xFFFF (same as 16b broadcast address)
  @param options The XBee options for this packet (0 if none).
  @param data A pointer to the data to be sent in this packet.  Up to 100 bytes.
  @param datalength The number of bytes of data to be sent. Maximum 100 bytes.
	@return True on success, false on failure.
  
  \par Example
  \code
  XBeePacket txPacket;
  uint8 data[] = "ABCDE";
  XBee_CreateTX16Packet( &txPacket, 0, 0, 0, data, 5 );
  XBee_SendPacket( &txPacket, 5 );
  \endcode
*/
bool XBee_CreateTX64Packet( XBeePacket* xbp, uint8 frameID, uint64 destination, uint8 options, uint8* data, uint8 datalength )
{
  uint8* p;
  int i;
  xbp->apiId = XBEE_TX64;
  xbp->tx64.frameID = frameID;
  for( i = 0; i < 8; i++ )
    xbp->tx64.destination[i] = (destination >> 8*i) & (0xFF * i); // ????????
  xbp->tx64.options = options;
  xbp->length = datalength + 5;
  p = xbp->tx64.data;
  while( datalength-- )
    *p++ = *data++;
  return true;
}

/**	
  Unpack the info from an incoming packet with a 16-bit address.
  Pass \b NULL into any of the parameters you don't care about.
  @param xbp The XBeePacket to read from.
  @param srcAddress The 16-bit address of this packet.
  @param sigstrength The signal strength of this packet.
  @param options The XBee options for this packet.
  @param data A pointer that will be set to the data of this packet.
  @param datalength The length of data in this packet.
	@return True on success, false on failure.
  
  \par Example
  \code
  XBeePacket rxPacket;
  if( XBee_GetPacket( &rxPacket, 0 ) )
  {
    uint16 src;
    uint8 sigstrength;
    uint8* data;
    uint8 datalength;
    if( XBee_ReadRX16Packet( &rxPacket, &src, &sigstrength, NULL, &data, &datalength ) )
    {
      // then process the new packet here
      XBee_ResetPacket( &rxPacket ); // and clear it out before reading again
    }
  }
  \endcode
*/
bool XBee_ReadRX16Packet( XBeePacket* xbp, uint16* srcAddress, uint8* sigstrength, uint8* options, uint8** data, uint8* datalength )
{
  if( xbp->apiId != XBEE_RX16 )
    return false;

  if( srcAddress )
  {
    *srcAddress = xbp->rx16.source[0];
    *srcAddress <<= 8;
    *srcAddress += xbp->rx16.source[1];
  }
  if( sigstrength )
    *sigstrength = xbp->rx16.rssi;
  if( options )
    *options = xbp->rx16.options;
  if( data )
    *data = xbp->rx16.data;
  if( datalength )
    *datalength = xbp->length - 5;
  return true;
}

/**	
  Unpack the info from an incoming packet with a 64-bit address.
  Pass \b NULL into any of the parameters you don't care about.
  @param xbp The XBeePacket to read from.
  @param srcAddress The 64-bit address of this packet.
  @param sigstrength The signal strength of this packet.
  @param options The XBee options for this packet.
  @param data A pointer that will be set to the data of this packet.
  @param datalength The length of data in this packet.
	@return True on success, false on failure.
  
  \par Example
  \code
  XBeePacket rxPacket;
  if( XBee_GetPacket( &rxPacket, 0 ) )
  {
    uint64 src;
    uint8 sigstrength;
    uint8* data;
    uint8 datalength;
    if( XBee_ReadRX64Packet( &rxPacket, &src, &sigstrength, NULL, &data, &datalength ) )
    {
      // then process the new packet here
      XBee_ResetPacket( &rxPacket ); // and clear it out before reading again
    }
  }
  \endcode
*/
bool XBee_ReadRX64Packet( XBeePacket* xbp, uint64* srcAddress, uint8* sigstrength, uint8* options, uint8** data, uint8* datalength )
{
  if( xbp->apiId != XBEE_RX64 )
    return false;

  int i;
  if( srcAddress )
  {
    for( i = 0; i < 8; i++ )
    {
      *srcAddress <<= i*8;
      *srcAddress += xbp->rx64.source[i];
    }
  }
  if( sigstrength )
    *sigstrength = xbp->rx64.rssi;
  if( options )
    *options = xbp->rx64.options;
  if( data )
    *data = xbp->rx64.data;
  if( datalength )
    *datalength = xbp->length - 11;
  return true;
}

/**	
  Unpack the info from an incoming IO packet with a 16-bit address.
  When an XBee module has been given a sample rate, it will sample its IO pins according to their current configuration
  and send an IO packet with the sample data.  This function will extract the sample info into an array of ints for you.
  There are 9 IO pins on the XBee modules, so be sure that the array you pass in has room for 9 ints.

  Pass \b NULL into any of the parameters you don't care about.
  @param xbp The XBeePacket to read from.
  @param srcAddress A pointer to a uint16 that will be filled up with the 16-bit address of this packet.
  @param sigstrength A pointer to a uint8 that will be filled up with the signal strength of this packet.
  @param options A pointer to a uint8 that will be filled up with the XBee options for this packet.
  @param samples A pointer to an array of ints that will be filled up with the sample values from this packet.
	@return True on success, false on failure.
  @see XBeeConfig_SetSampleRate( ), XBeeConfig_SetIOs( )
  
  \par Example
  \code
  XBeePacket rxPacket;
  if( XBee_GetPacket( &rxPacket, 0 ) )
  {
    uint16 src;
    uint8 sigstrength;
    int samples[9];
    if( XBee_ReadIO16Packet( &rxPacket, &src, &sigstrength, NULL, samples ) )
    {
      // then process the new packet here
      XBee_ResetPacket( &rxPacket ); // and clear it out before reading again
    }
  }
  \endcode
*/
bool XBee_ReadIO16Packet( XBeePacket* xbp, uint16* srcAddress, uint8* sigstrength, uint8* options, int* samples )
{
  if( xbp->apiId != XBEE_IO16 )
    return false;
  if( srcAddress )
  {
    *srcAddress = xbp->io16.source[0];
    *srcAddress <<= 8;
    *srcAddress += xbp->io16.source[1];
  }
  if( sigstrength )
    *sigstrength = xbp->io16.rssi;
  if( options )
    *options = xbp->io16.options;
  if( samples )
  {
    if( !XBee_GetIOValues( xbp, samples ) )
      return false;
  }
  return true;
}

/**	
  Unpack the info from an incoming IO packet with a 64-bit address.
  When an XBee module has been given a sample rate, it will sample its IO pins according to their current configuration
  and send an IO packet with the sample data.  This function will extract the sample info into an array of ints for you.
  There are 9 IO pins on the XBee modules, so be sure that the array you pass in has room for 9 ints.

  Pass \b NULL into any of the parameters you don't care about.
  @param xbp The XBeePacket to read from.
  @param srcAddress A pointer to a uint64 that will be filled up with the 16-bit address of this packet.
  @param sigstrength A pointer to a uint8 that will be filled up with the signal strength of this packet.
  @param options A pointer to a uint8 that will be filled up with the XBee options for this packet.
  @param samples A pointer to an array of ints that will be filled up with the sample values from this packet.
	@return True on success, false on failure.
  @see XBeeConfig_SetSampleRate( ), XBeeConfig_SetIOs( )
  
  \par Example
  \code
  XBeePacket rxPacket;
  if( XBee_GetPacket( &rxPacket, 0 ) )
  {
    uint64 src;
    uint8 sigstrength;
    int samples[9];
    if( XBee_ReadIO16Packet( &rxPacket, &src, &sigstrength, NULL, samples ) )
    {
      // then process the new packet here
      XBee_ResetPacket( &rxPacket ); // and clear it out before reading again
    }
  }
  \endcode
*/
bool XBee_ReadIO64Packet( XBeePacket* xbp, uint64* srcAddress, uint8* sigstrength, uint8* options, int* samples )
{
  if( xbp->apiId != XBEE_RX64 )
    return false;
  if( srcAddress )
  {
    int i;
    for( i = 0; i < 8; i++ )
    {
      *srcAddress <<= i*8;
      *srcAddress += xbp->io64.source[i];
    }
  }
  if( sigstrength )
    *sigstrength = xbp->io64.rssi;
  if( options )
    *options = xbp->io64.options;
  if( samples )
  {
    if( !XBee_GetIOValues( xbp, samples ) )
      return false;
  }
  return true;
}

/**	
  Unpack the info from an incoming AT Command Response packet.
  In response to a previous AT Command message, the module will send an AT Command Response message.

  Pass \b NULL into any of the parameters you don't care about.
  @param xbp The XBeePacket to read from.
  @param frameID A pointer to a uint64 that will be filled up with the 16-bit address of this packet.
  @param command A pointer to a uint8 that will be filled up with the signal strength of this packet.
  @param status A pointer to a uint8 that will be filled up with the XBee options for this packet.
  @param datavalue The value of the requested command.
	@return True on success, false on failure.
  
  \par Example
  \code
  XBeePacket rxPacket;
  if( XBee_GetPacket( &rxPacket, 0 ) )
  {
    uint8 frameID;
    char* command;
    uint8 status;
    int value = -1;
    if( XBee_ReadAtResponsePacket( &rxPacket, &frameID, command, &status, &value ) )
    {
      // then process the new packet here
      XBee_ResetPacket( &rxPacket ); // and clear it out before reading again
    }
  }
  \endcode
*/
bool XBee_ReadAtResponsePacket( XBeePacket* xbp, uint8* frameID, char** command, uint8* status, int* datavalue )
{
  if( xbp->apiId != XBEE_ATCOMMANDRESPONSE )
    return false;
  if( frameID )
    *frameID = xbp->atResponse.frameID;
  if( command )
    *command = (char*)xbp->atResponse.command;
  if( status )
    *status = xbp->atResponse.status;
  if( datavalue )
  {
    uint8 *dataPtr = xbp->atResponse.value;
    int i;
    int datalength = xbp->length - 5; // data comes after apiID, frameID, 2-bytes of cmd, and 1-byte status
    *datavalue = 0;
    for( i = 0; i < datalength; i++ )
    {
      *datavalue <<= 8;
      *datavalue += *dataPtr++;
    }
  }
  return true;
}

/**	
  Unpack the info from TX Status packet.
  When a TX is completed, the modules esnds a TX Status message.  This indicates whether the packet was transmitted
  successfully or not.  If the message you sent had a frameID of 0, a TX status message will not be generated.

  Pass \b NULL into any of the parameters you don't care about.
  @param xbp The XBeePacket to read from.
  @param frameID A pointer to a uint8 that will be filled up with the frame ID of this packet.
  @param status A pointer to a uint8 that will be filled up with the status of this packet.
	@return True on success, false on failure.
  
  \par Example
  \code
  XBeePacket rxPacket;
  if( XBee_GetPacket( &rxPacket, 0 ) )
  {
    uint8 frameID;
    uint8 status;
    if( XBee_ReadTXStatusPacket( &rxPacket, &frameID, &status ) )
    {
      // then process the new packet here
      XBee_ResetPacket( &rxPacket ); // and clear it out before reading again
    }
  }
  \endcode
*/
bool XBee_ReadTXStatusPacket( XBeePacket* xbp, uint8* frameID, uint8* status )
{
  if( xbp->apiId != XBEE_TXSTATUS )
    return false;
  if( frameID )
    *frameID = xbp->txStatus.frameID;
  if( status )
    *status = xbp->txStatus.status;
  return true;
}

/**	
  Save the configuration changes you've made on the module to memory.
  When you make configuration changes - setting the module to API mode, or configuring the sample rate, for example - 
  those changes will be lost when the module restarts.  Call this function to save the current state to non-volatile memory.

  As with the other \b XBeeConfig functions, make sure you're in API mode before trying to use this function.
	@return True on success, false on failure.
  @see XBeeConfig_SetPacketApiMode( )
  
  \par Example
  \code
  XBeeConfig_SetPacketApiMode( );
  XBeeConfig_SetSampleRate( 100 );
  XBeeConfig_WriteStateToMemory( );
  \endcode
*/
void XBeeConfig_WriteStateToMemory( void )
{
  XBeePacket xbp;
  uint8 params[4];
  memset( params, 0, 4 );
  XBee_CreateATCommandPacket( &xbp, 0, "WR", params, 4 );
  XBee_SendPacket( &xbp, 4 );
}

/**	
  Set this module's address.

  As with the other \b XBeeConfig functions, make sure you're in API mode before trying to use this function.
  @param address An integer specifying the module's address.
	@return True on success, false on failure.
  @see XBeeConfig_SetPacketApiMode( )
  
  \par Example
  \code
  XBeeConfig_SetPacketApiMode( );
  XBeeConfig_SetAddress( 100 );
  XBeeConfig_WriteStateToMemory( );
  \endcode
*/
void XBeeConfig_SetAddress( uint16 address )
{
  XBeePacket xbp;
  uint8 params[4]; // big endian - most significant bit first
  XBee_IntToBigEndianArray( address, params );
  XBee_CreateATCommandPacket( &xbp, 0, "MY", params, 4 );
  XBee_SendPacket( &xbp, 4 );
}

/**	
  Query the address of the module.
  This will block for up to a 1/2 second waiting for a response from the XBee module.
  @return An integer corresponding to the address of the module, or negative number on failure.
	
  \par Example
  \code
  int address = XBeeConfig_RequestAddress( );
  if( address >= 0 )
  {
    // then we have our address
  }
  \endcode
*/
int XBeeConfig_RequestAddress( )
{
  XBeePacket xbp;
  XBee_CreateATCommandPacket( &xbp, 0x52, "MY", NULL, 0 );
  XBee_SendPacket( &xbp, 0 );
  return CONTROLLER_OK;
}

/**	
  Set this PAN (Personal Area Network) ID.
  Only modules with matching PAN IDs can communicate with each other.  Unique PAN IDs enable control of which
  RF packets are received by a module.  Default is \b 0x3332.

  As with the other \b XBeeConfig functions, make sure you're in API mode before trying to use this function.
  @param id A uint16 specifying the PAN ID.
	@return True on success, false on failure.
  @see XBeeConfig_SetPacketApiMode( )
  
  \par Example
  \code
  XBeeConfig_SetPacketApiMode( );
  XBeeConfig_SetPanID( 0x1234 );
  \endcode
*/
void XBeeConfig_SetPanID( uint16 id )
{
  XBeePacket xbp;
  uint8 params[4]; // big endian - most significant bit first
  XBee_IntToBigEndianArray( id, params );
  XBee_CreateATCommandPacket( &xbp, 0, "ID", params, 4 );
  XBee_SendPacket( &xbp, 4 );
}

/**	
  Query the PAN ID of the module.
  This will block for up to a 1/2 second waiting for a response from the XBee module.
  @return An integer corresponding to the PAN ID of the module, or negative number on failure.
	
  \par Example
  \code
  int panid = XBeeConfig_RequestPanID( );
  if( panid >= 0 )
  {
    // then we have our pan id
  }
  \endcode
*/
int XBeeConfig_RequestPanID( )
{
  XBeePacket xbp;
  XBee_CreateATCommandPacket( &xbp, 0x52, "ID", NULL, 0 );
  XBee_SendPacket( &xbp, 0 );
  return CONTROLLER_OK;
}

/**	
  Set the module's channel.
  The channel is one of 3 addressing options available to the module - the others are the PAN ID and the
  destination address.  In order for modules to communicate with each other, the modules must share the same
  channel number.  Default is \b 0x0C.

  This value can have the range \b 0x0B - \b 0x1A for XBee modules, and \b 0x0C - \b 0x17 for XBee-Pro modules.

  As with the other \b XBeeConfig functions, make sure you're in API mode before trying to use this function.
  @param channel A uint8 specifying the channel.
	@return True on success, false on failure.
  @see XBeeConfig_SetPacketApiMode( )
  
  \par Example
  \code
  XBeeConfig_SetPacketApiMode( );
  XBeeConfig_SetChannel( 0x0D );
  \endcode
*/
void XBeeConfig_SetChannel( uint8 channel )
{
  XBeePacket xbp;
  uint8 params[4];
  XBee_IntToBigEndianArray( channel, params );
  XBee_CreateATCommandPacket( &xbp, 0, "CH", params, 4 );
  XBee_SendPacket( &xbp, 4 );
}

/**	
  Query the channel of the module.
  This will block for up to a 1/2 second waiting for a response from the XBee module.
  @return An integer corresponding to the channel of the module, or negative number on failure.
	
  \par Example
  \code
  int chan = XBeeConfig_RequestChannel( );
  if( chan >= 0 )
  {
    // then we have our channel
  }
  \endcode
*/
int XBeeConfig_RequestChannel( )
{
  XBeePacket xbp;
  XBee_CreateATCommandPacket( &xbp, 0x52, "CH", NULL, 0 );
  XBee_SendPacket( &xbp, 0 );
  return CONTROLLER_OK;
}

/**	
  Set the rate at which the module will sample its IO pins.
  When the sample rate is set, the module will sample all its IO pins according to its current IO configuration and send
  a packet with the sample data.  If this is set too low and the IO configuration is sampling too many channels, 
  the RF module won't be able to keep up.  You can also adjust how many samples are gathered before a packet is sent.

  As with the other \b XBeeConfig functions, make sure you're in API mode before trying to use this function.
  @param rate A uint16 specifying the sample rate in milliseconds.
	@return True on success, false on failure.
  @see XBeeConfig_SetIOs( ), XBeeConfig_SetPacketApiMode( )
  
  \par Example
  \code
  XBeeConfig_SetPacketApiMode( );
  XBeeConfig_SetSampleRate( 0x14 );
  \endcode
*/
void XBeeConfig_SetSampleRate( uint16 rate )
{
  XBeePacket xbp;
  uint8 params[4]; // big endian - most significant bit first
  XBee_IntToBigEndianArray( rate, params );
  XBee_CreateATCommandPacket( &xbp, 0, "IR", params, 4 );
  XBee_SendPacket( &xbp, 4 );
}

/**	
  Query the sample rate of the module.
  This will block for up to a 1/2 second waiting for a response from the XBee module.
  @return An integer corresponding to the sample rate of the module, or negative number on failure.
	
  \par Example
  \code
  int rate = XBeeConfig_RequestSampleRate( );
  if( rate >= 0 )
  {
    // then we have our rate
  }
  \endcode
*/
int XBeeConfig_RequestSampleRate( )
{
  XBeePacket xbp;
  XBee_CreateATCommandPacket( &xbp, 0x52, "IR", NULL, 0 );
  XBee_SendPacket( &xbp, 0 );
  return CONTROLLER_OK;
}

/**	
  Convert a 32-bit integer into a big endian array of 4 unsigned 8-bit integers.
  This is mostly useful for sending AT Command packets.
  @param value The value to convert.
  @param array An array of 4 unsigned 8-bit integers (uint8).  Be sure you have 4.
  \par Example
  see XBee_CreateATCommandPacket( ) for an example.
*/
void XBee_IntToBigEndianArray( int value, uint8* array )
{
  array[0] = (value >> 24) & 0xFF;
  array[1] = (value >> 16) & 0xFF;
  array[2] = (value >> 8) & 0xFF;
  array[3] = value & 0xFF;
}

/** @}
*/

/**	
  Unpack IO values from an incoming packet.
  @param packet The XBeePacket to read from.
  @param inputs An array of at least 9 integers, which will be populated with the values of the 9 input lines on the XBee module.
	@return 1 if IO values were successfully retrieved, otherwise zero.
  
  \par Example
  \code
  XBeePacket rxPacket;
  if( XBee_GetPacket( &rxPacket, 0 ) )
  {
    int inputs[9];
    if( XBee_GetIOValues( &rxPacket, inputs ) )
    {
      // process new input values here
    }
  }
  \endcode
*/
static bool XBee_GetIOValues( XBeePacket* packet, int *inputs )
{
  if( packet->apiId == XBEE_IO16 || packet->apiId == XBEE_IO64 )
  {
    int i;
    static bool enabled;
    int digitalins = 0;
    uint8* p;
    int channelIndicators;
    if( packet->apiId == XBEE_IO16 )
    {
      p = packet->io16.data;
      channelIndicators = (packet->io16.channelIndicators[0] << 0x08) | packet->io16.channelIndicators[1];
    }
    else // packet->apiId == XBEE_IO64
    {
      p = packet->io64.data;
      channelIndicators = (packet->io64.channelIndicators[0] << 0x08) | packet->io64.channelIndicators[1];
    }

    for( i = 0; i < XBEE_INPUTS; i++ )
    {
      enabled = channelIndicators & 1;
      channelIndicators >>= 1;
      if( i < 9 ) // digital ins
      {
        inputs[i] = 0; // zero out the 9 inputs beforehand
        if( enabled )
        {
          if( !digitalins )
          {
            int dig0 = *p++ << 0x08;
            digitalins = dig0 | *p++;
          }
          inputs[i] = ((digitalins >> i) & 1) * 1023;
        }
      }
      else // analog ins
      {
        if( enabled )
        {
          int ain_msb = *p++ << 0x08;
          inputs[i-9] = ain_msb | *p++;
        }
      }
    }
    return true;
  }
  else
    return false;
}

#ifdef OSC
#include "osc.h"

/** \defgroup XBeeOSC XBee - OSC
  Communicate with XBee modules with the Make Controller Kit via OSC.
  \ingroup OSC
	
	\section devices Devices
	There can only be one XBee board connected to the Make Controller Kit's serial port at a time,
  so there is not a device index in XBee OSC messages.
	
	\section properties Properties
	The XBee system has the following properties:
  - autosend
  - io16
  - io64
  - rx16
  - rx64
  - tx16
  - tx64
  - tx-status
  - active
  
  \par Autosend
	The \b autosend property corresponds to whether the Make Controller will automatically send out 
  messages it receives from a connected XBee module.  By default, this is turned off.
  To turn this on, send the message
	\verbatim /xbee/autosend 1 \endverbatim
  and to turn it off, send
	\verbatim /xbee/autosend 0 \endverbatim
  All autosend messages send at the same interval.  You can set this interval, in 
	milliseconds, by sending the message
	\verbatim /system/autosend-interval 10 \endverbatim
	so that messages will be sent every 10 milliseconds.  This can be anywhere from 1 to 5000 milliseconds.
  \par
  You also need to select whether the board should send to you over USB or Ethernet.  Send
  \verbatim /system/autosend-usb 1 \endverbatim
  to send via USB, and 
  \verbatim /system/autosend-udp 1 \endverbatim
  to send via Ethernet.  Via Ethernet, the board will send messages to the last address it received a message from.
	
  \par io16
	The \b io16 property corresponds to an incoming message from an XBee module with samples
  from its IO pins.  The best way to use this is to turn the XBee system's autosend property
  on - then the Make Controller can relay io16 messages as soon as they're received.
	\par
	Once you've turned on autosend, if there are boards on your network that are sending IO packets, 
  you'll receive messages like
	\verbatim /xbee/io16 1234 28 12 0 0 1023 1023 0 512 0 1023 \endverbatim
	The first two numbers are:
	-# the address of the module that sent the message (1234 in the example above)
  -# signal strength (28 above) 
	\par
	The next 9 numbers are the values from the 9 IO pins on the XBee module.
	
	\par io64
	The \b io64 property corresponds to an incoming message from an XBee module with samples from its IO
	pins.  This message is just like the \b io16 message, except it's coming from a board with a 64-bit
	address, rather than a 16-bit address.  The structure of the message is the same (see above).
	
	\par rx16
	The \b rx16 property corresponds to an incoming message from a 16-bit address XBee module with arbitrary data.  
	The best way to use this is to turn the XBee system's autosend property
  on - then the Make Controller can relay rx16 messages as soon as they're received.
	\par
	Once you've turned on autosend, if there are boards on your network that are sending IO packets, 
  you'll receive messages like
	\verbatim /xbee/rx16 1234 28 0 [43 44 45 32 46 47 48] \endverbatim
	The first three numbers are:
	-# the address of the module that sent the message (1234 in the example above)
  -# signal strength (28 above)
	-# the options byte (0 above), respectively. 
	\par
	Following those is an OSC blob with the data (enclosed in square brackets above).  These are the hex values for each byte of data.
	
	\par rx64
	The \b rx64 property corresponds to an incoming message from an XBee module with samples from its IO
	pins.  This message is just like the \b io16 message, except it's coming from a board with a 64-bit
	address, rather than a 16-bit address.  The structure of the message is the same (see above).

  \par Transmit Status
  The \b tx-status property gives you the status of a previously sent message.  It tells you the frameID of the message
  that was sent and its status, which can be one of:
  - \b Success.  Message was successfully transmitted and received.
  - <b> No acknowledgement received</b>.  The message was successfully sent, but not successfully received on the other end.
  - <b> CCA Failure</b>.
  - \b Purged.
  \par
  If you don't include a frameID for the message that you sent, a tx-status message will not be generated.  An example
  tx-status message will look like
	\verbatim /xbee/tx-status 52 Success \endverbatim
	where 52 is the frameID and "Success" is the status.
	
	\par Active
	The \b active property corresponds to the active state of the XBee system.
	If you're not seeing appropriate responses to your messages to the XBee system, 
  check whether it's active by sending the message
	\verbatim /xbee/active \endverbatim
	\par
	You can set the active flag by sending
	\verbatim /xbee/active 1 \endverbatim
*/


bool XBee_GetAutoSend( bool init )
{
  XBee_SetActive( 1 );
  if( init )
  {
    int autosend;
    Eeprom_Read( EEPROM_XBEE_AUTOSEND, (uchar*)&autosend, 4 );
    XBee->autosend = (autosend == 1 ) ? 1 : 0;
  }
  return XBee->autosend;
}

void XBee_SetAutoSend( int onoff )
{
  XBee_SetActive( 1 );  
  if( XBee->autosend != onoff )
  {
    XBee->autosend = onoff;
    Eeprom_Write( EEPROM_XBEE_AUTOSEND, (uchar*)&onoff, 4 );
  }
}

static char* XBeeOsc_Name = "xbee";
static char* XBeeOsc_PropertyNames[] = { "active", "io16", "rx16", "autosend", "get-message", 0 }; // must have a trailing 0

int XBeeOsc_PropertySet( int property, char* typedata, int channel );
int XBeeOsc_PropertyGet( int property, int channel );

const char* XBeeOsc_GetName( void )
{
  return XBeeOsc_Name;
}

int XBeeOsc_ReceiveMessage( int channel, char* message, int length )
{
  XBee_SetActive( 1 );
  int status = Osc_GeneralReceiverHelper( channel, message, length, 
                                XBeeOsc_Name,
                                XBeeOsc_PropertySet, XBeeOsc_PropertyGet, 
                                XBeeOsc_PropertyNames );

  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, XBeeOsc_Name, status );

  return CONTROLLER_OK;
}

int XBeeOsc_PropertySet( int property, char* typedata, int channel )
{
  switch ( property )
  {
    case 0: // active
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, XBeeOsc_Name, "Incorrect data - need an int" );

      XBee_SetActive( value );
      break;
    }
    case 1: // io16
    case 2: // rx16
      return Osc_SubsystemError( channel, XBeeOsc_Name, "Property is read-only" );
    case 3: // autosend
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, XBeeOsc_Name, "Incorrect data - need an int" );

      XBee_SetAutoSend( value );
      break;
    }
  }
  return CONTROLLER_OK;
}

int XBeeOsc_PropertyGet( int property, int channel )
{
  switch ( property )
  {
    case 0: // active
    {
      char address[ OSC_SCRATCH_SIZE ];
      int value = XBee_GetActive( );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", XBeeOsc_Name, XBeeOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, address, ",i", value );
      break;
    }
    case 1: // io16
    case 2: // rx16
      return Osc_SubsystemError( channel, XBeeOsc_Name, "Can't get specific messages - use /xbee/get-message instead." );
    case 3: // autosend
    {
      char address[ OSC_SCRATCH_SIZE ];
      int value = XBee_GetAutoSend( false );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", XBeeOsc_Name, XBeeOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, address, ",i", value );
      break;
    } 
    case 4: // get-message
    {
      XBeePacket xbp;
      if( XBee_GetPacket( &xbp, XBEE_OSC_RX_TIMEOUT ) )
        XBeeOsc_HandleNewPacket( XBee->currentPkt, channel );
      break;
    }
  }
  return CONTROLLER_OK;
}

/** \defgroup XBeeConfigOSC XBee Configuration - OSC
  Configure an XBee module connected to your Make Controller Kit via OSC.
  \ingroup OSC
	
	\section devices Devices
	There can only be one XBee board connected to the Make Controller Kit's serial port at a time,
  so there is not a device index in XBee Config OSC messages.
	
	\section properties Properties
	The XBee Config system has the following properties:
  - write
  - get-message
  - packet-mode
  - samplerate
  - address
  - panid
  - channel
  - at-command
  - io
  - active

  Reading values from the XBee Config system is a bit different than the other systems.  Because we need to see any messages
  that arrive at the XBee module and not just the one we're requesting at a given moment, to get any message back you need to 
  use the \b get-message property.  Imagine the case where we want to ask the module its address, but another module is busy 
  sending us sensor values.  If we waited for the address message, we'd miss all the sensor values.  

  So to read a value, first send the request then call \b get-message until you get the response you're looking for.
  If you have autosend turned on, it will get messages for you, so you just need to send the request - this is much
  easier.
  
  \par Write
	When you change any of the paramters of your XBee module, it will by default revert back to its previous settings
  when it gets powered down.  The \b write property sets the current values (all of them) into memory.
  \par
  For example, to set the sample rate to 100 milliseconds, and save it permanently, send the messages
	\verbatim 
  /xbeeconfig/samplerate 100
  /xbeeconfig/write 1 \endverbatim

  \par Get Message
	The \b get-message property fetches the most recent message received by the XBee module.  If you have autosend
  turned on, you don't need to use get-message and, in fact, it won't have any effect.
  To get a message from your XBee module, send the message
  \verbatim /xbeeconfig/get-message\endverbatim
  and the board will send one back if there were any available.
	
  \par Sample Rate
	The \b samplerate property corresponds to how often the XBee module will sample its IO pins and send a message
  with those values.  If it's set to 0, the module will not sample its IO pins.  The maximum sample rate is 65535.
  When the sample rate is set very low, the XBee module cannot guarantee that it will be able to keep up with all
  the samples requested of it.
  \par
  To set the sample rate to once a second (every 1000 milliseconds), send the message
	\verbatim /xbeeconfig/samplerate 1000\endverbatim
  To read the sample rate, send the message
	\verbatim /xbeeconfig/samplerate \endverbatim

  \par Address
	The \b address property corresponds to the address of the XBee module.  Valid ranges for the address are
  from 0 - 65535.
  \par
  To set the address to 1234, send the message
	\verbatim /xbeeconfig/address 1234\endverbatim
  To read the address, send the message
	\verbatim /xbeeconfig/address \endverbatim

  \par Channel
	The \b channel property corresponds to the channel of the XBee module.  Valid ranges for the address are
  from 11 - 26 for XBee modules and 12 - 23 for XBee Pro modules.
  \par
  To set the channel to 15, send the message
	\verbatim /xbeeconfig/channel 15\endverbatim
  To read the channel, send the message
	\verbatim /xbeeconfig/channel \endverbatim
  
  \par AT Command
	The \b at-command property allows you to read/write AT commands to your XBee module via OSC.  The most common
  commands are included - samplerate, channel, address, etc. but this is helpful if you need to send any of the
  other commands to your module.  To write a command, specify the 2-letter command and then the value to set.  
  To enable encryption, for example, send the message
  \verbatim /xbeeconfig/at-command EE 1\endverbatim
  \par
  To read a value back via an AT command, simply send the 2-letter command you'd like to get the value for.
  To read back the hardware version, send the message
	\verbatim /xbeeconfig/at-command HV\endverbatim
  Check the XBee documentation for a complete list of commands the boards will respond to.

  \par PAN ID
	The \b panid property corresponds to the Personal Area Network ID of the XBee module.  Valid ranges for the address are
  from 0 - 65535.  The default value is 13106.
  \par
  To set the PAN ID to 512, send the message
	\verbatim /xbeeconfig/panid 512\endverbatim
  To read the PAN ID, send the message
	\verbatim /xbeeconfig/panid \endverbatim
	
	\par Input/Output Pins
	There are several \b io properties that allow you to configure each of the 9 IO pins on the XBee module.  
	Pins can be set to one of 5 values:
  - \b 0 - disabled
  - \b 2 - analogin
  - \b 3 - digital in
  - \b 4 - digital out high
  - \b 5 - digital out low
  
  \par
	There are 9 IO pins on the XBee module, numbered 0 - 8. Send messages to them by specifying \b io + their number.  
  Pin 8 cannot be an analogin - it can only be a digital in or out.
	For example,	to set IO 0 to analogin, send the message
	\verbatim /xbeeconfig/io0 2\endverbatim
	To set IO 6 to a digital in, send the message
	\verbatim /xbeeconfig/io6 3\endverbatim
  To read the configuration of pin 5, send the message
	\verbatim /xbeeconfig/io5 \endverbatim
	
	\par Active
	The \b active property corresponds to the active state of the XBee system.
	If you're not seeing appropriate responses to your messages to the XBee system, 
  check whether it's active by sending the message
	\verbatim /xbee/active \endverbatim
	\par
	You can set the active flag by sending
	\verbatim /xbee/active 1 \endverbatim
*/

static char* XBeeConfigOsc_Name = "xbeeconfig";
static char* XBeeConfigOsc_PropertyNames[] = { "active", "address", "panid", "channel", "samplerate", 
                                                "write", "io0", "io1", "io2", "io3", "io4", 
                                                "io5", "io6", "io7", "io8", "packet-mode", "at-command", 
                                                "get-message", "write-command", "confirm", 0 }; // must have a trailing 0

int XBeeConfigOsc_PropertySet( int property, char* typedata, int channel );
int XBeeConfigOsc_PropertyGet( int property, int channel );

const char* XBeeConfigOsc_GetName( void )
{
  return XBeeConfigOsc_Name;
}

int XBeeConfigOsc_PropertySet( int property, char* typedata, int channel )
{
  int value = -1;
  int count;
  switch ( property )
  {
    case 0: // active
      count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, XBeeConfigOsc_Name, "Incorrect data - need an int" );
      XBee_SetActive( value );
      break;
    case 1: // address
      count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, XBeeConfigOsc_Name, "Incorrect data - need an int" );
      XBeeConfig_SetAddress( value );
      break;
    case 2: // panid
      count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, XBeeConfigOsc_Name, "Incorrect data - need an int" );
      XBeeConfig_SetPanID( value );
      break;
    case 3: // channel
      count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, XBeeConfigOsc_Name, "Incorrect data - need an int" );
      XBeeConfig_SetChannel( value );
      break;
    case 4: // samplerate
      count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, XBeeConfigOsc_Name, "Incorrect data - need an int" );
      XBeeConfig_SetSampleRate( value );
      break;
    case 5: // write
      count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, XBeeConfigOsc_Name, "Incorrect data - need an int" );
      XBeeConfig_WriteStateToMemory( );
      break;
    case 6: // io0
    case 7: // io1
    case 8: // io2
    case 9: // io3
    case 10: // io4
    case 11: // io5
    case 12: // io6
    case 13: // io7
    case 14: // io8
      count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, XBeeConfigOsc_Name, "Incorrect data - need an int" );
      XBeeConfig_SetIO( property - 6, value );
      break;
    case 15: // packet-mode
      count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, XBeeConfigOsc_Name, "Incorrect data - need an int" );
      XBeeConfig_SetPacketApiMode( value );
      break;
    case 16: // at-command
    {
      char* cmd;
      count = Osc_ExtractData( typedata, "si", &cmd, &value );
      if ( count != 2 && count != 1 )
        return Osc_SubsystemError( channel, XBeeConfigOsc_Name, "Incorrect data - need a string, optionally followed by an int" );
      
      if( count == 2 )
      {
        XBeePacket xbp;
        uint8 params[4]; // big endian - most significant bit first
        XBee_IntToBigEndianArray( value, params );
        XBee_CreateATCommandPacket( &xbp, 0, cmd, params, 4 );
        XBee_SendPacket( &xbp, 4 );
      }
      if( count == 1 ) // this is a little wonky, but this is actually a read.
        value = XBeeConfig_RequestATResponse( cmd );
      break;
    }
    case 18: // write-command
    {
      char* cmd;
      count = Osc_ExtractData( typedata, "si", &cmd, &value );
      if ( count != 2 )
        return Osc_SubsystemError( channel, XBeeConfigOsc_Name, "Incorrect data - need a string and an int" );
      XBeePacket xbp;
      uint8 params[4]; // big endian - most significant bit first
      XBee_IntToBigEndianArray( value, params );
      XBee_CreateATCommandPacket( &xbp, 0, cmd, params, 4 );
      XBee_SendPacket( &xbp, 4 );
      XBee_ResetPacket( &xbp );
      XBee_CreateATCommandPacket( &xbp, 0, "WR", NULL, 0 );
      XBee_SendPacket( &xbp, 0 ); 
      break;
    }
    case 19: // confirm
    {
      char* cmd;
      count = Osc_ExtractData( typedata, "si", &cmd, &value );
      if ( count != 2 )
        return Osc_SubsystemError( channel, XBeeConfigOsc_Name, "Incorrect data - need a string and an int" );
      XBeePacket xbp;
      uint8 params[4]; // big endian - most significant bit first
      XBee_IntToBigEndianArray( value, params );

      XBee->waitingForConfirm = true;
      Sleep( System_GetAutoSendInterval( ) + 1 ); // make sure autosend isn't grabbing packets if it's on
      XBee_ResetPacket( XBee->currentPkt );
      while( true )
      {
        // send the setter
        XBee_CreateATCommandPacket( &xbp, 0x53, cmd, params, 4 );
        XBee_SendPacket( &xbp, 4 );
        XBee_ResetPacket( &xbp );
        // then the getter
        XBee_CreateATCommandPacket( &xbp, 0x52, cmd, NULL, 0 );
        XBee_SendPacket( &xbp, 0 );
        XBee_ResetPacket( &xbp );
        // and try to get it
        if( XBee_GetPacket( XBee->currentPkt, 5 ) )
        {
          XBeeOsc_HandleNewPacket( XBee->currentPkt, channel );
          break;
        }
        Sleep( 1 );
      }
      XBee->waitingForConfirm = false;
      break;
    }
  }
  return CONTROLLER_OK;
}

int XBeeConfigOsc_PropertyGet( int property, int channel )
{
  char address[ OSC_SCRATCH_SIZE ];
  switch ( property )
  {
    case 0: // active
    {
      int value = XBee_GetActive( );
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", XBeeConfigOsc_Name, XBeeConfigOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, address, ",i", value );
      break;
    }
    case 1: // address
      XBeeConfig_RequestAddress( );
      break;
    case 2: // panid
      XBeeConfig_RequestPanID( );
      break;
    case 3: // channel
      XBeeConfig_RequestChannel( );
      break;
    case 4: // samplerate
      XBeeConfig_RequestSampleRate( );
      break;
    case 6: // ios
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
      XBeeConfig_RequestIO( property - 6 ); // send the query
      break;
    case 15: // packet-mode
      XBeeConfig_RequestPacketApiMode( ); // send the query
      break;
    case 17: // get-message
    {
      XBeePacket xbp;
      if( XBee_GetPacket( &xbp, XBEE_OSC_RX_TIMEOUT ) )
        XBeeOsc_HandleNewPacket( XBee->currentPkt, channel );
      break;
    }
  }
  return CONTROLLER_OK;
}

int XBeeConfigOsc_ReceiveMessage( int channel, char* message, int length )
{
  XBee_SetActive( 1 );
  int status = Osc_GeneralReceiverHelper( channel, message, length, 
                                XBeeConfigOsc_Name,
                                XBeeConfigOsc_PropertySet, XBeeConfigOsc_PropertyGet, 
                                XBeeConfigOsc_PropertyNames );

  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, XBeeConfigOsc_Name, status );

  return CONTROLLER_OK;
}

int XBeeOsc_HandleNewPacket( XBeePacket* xbp, int channel )
{
  char address[OSC_SCRATCH_SIZE];
  int retval = xbp->apiId;
  switch( xbp->apiId )
  {
    case XBEE_RX16:
    {
      uint16 srcAddr;
      uint8 sigStrength;
      uint8 opts;
      uint8* data;
      uint8 datalen;
      if( XBee_ReadRX16Packet( xbp, &srcAddr, &sigStrength, &opts, &data, &datalen ) )
      {
        snprintf( address, OSC_SCRATCH_SIZE, "/%s/rx16", XBeeOsc_Name );
        Osc_CreateMessage( channel, address, ",iiib", srcAddr, sigStrength, opts, data, datalen );
      }
      break;
    }
    case XBEE_RX64:
    {
      uint64 srcAddr;
      uint8 sigStrength;
      uint8 opts;
      uint8* data;
      uint8 datalen;
      if( XBee_ReadRX64Packet( xbp, &srcAddr, &sigStrength, &opts, &data, &datalen ) )
      {
        snprintf( address, OSC_SCRATCH_SIZE, "/%s/rx64", XBeeOsc_Name );
        Osc_CreateMessage( channel, address, ",iiib", srcAddr, sigStrength, opts, data, datalen );
      }
      break;
    }
    case XBEE_IO16:
    {
      int in[9];
      uint16 src;
      uint8 sigstrength;
      if( XBee_ReadIO16Packet( xbp, &src, &sigstrength, NULL, in ) )
      {
        snprintf( address, OSC_SCRATCH_SIZE, "/%s/io16", XBeeOsc_Name );
        Osc_CreateMessage( channel, address, ",iiiiiiiiiii", src, sigstrength, 
                            in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7], in[8] );
      }
      break;
    }
    case XBEE_IO64:
    {
      int in[9];
      uint64 src;
      uint8 sigstrength;
      if( XBee_ReadIO64Packet( xbp, &src, &sigstrength, NULL, in ) )
      {
        snprintf( address, OSC_SCRATCH_SIZE, "/%s/io64", XBeeOsc_Name );
        Osc_CreateMessage( channel, address, ",iiiiiiiiiii", src, sigstrength, 
                            in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7], in[8] );
      }
      break;
    }
    case XBEE_TXSTATUS:
    {
      uint8 frameID;
      uint8 status;
      if( XBee_ReadTXStatusPacket( xbp, &frameID, &status ) )
      {
        snprintf( address, OSC_SCRATCH_SIZE, "/%s/tx-status", XBeeOsc_Name );
        char* statusText = "No status received";
        switch( status )
        {
          case 0:
            statusText = "Success";
            break;
          case 1:
            statusText = "No acknowledgement received";
            break;
          case 2:
            statusText = "CCA Failure";
            break;
          case 3:
            statusText = "Purged";
            break;
        }
        Osc_CreateMessage( channel, address, ",is", frameID, statusText );
      }
      break;
    }
    case XBEE_ATCOMMANDRESPONSE:
    {
      uint8 frameID;
      char* command;
      uint8 status;
      int value;
      if( XBee_ReadAtResponsePacket( xbp, &frameID, &command, &status, &value ) )
      {
        char cmd[3];
        cmd[0] = *command;
        cmd[1] = *(command+1);
        cmd[2] = '\0';
        if( strcmp( cmd, "IR" ) == 0 )
        {
          snprintf( address, OSC_SCRATCH_SIZE, "/%s/samplerate", XBeeConfigOsc_Name );
          Osc_CreateMessage( channel, address, ",i", value );
        }
        else if( strcmp( cmd, "MY" ) == 0 )
        {
          snprintf( address, OSC_SCRATCH_SIZE, "/%s/address", XBeeConfigOsc_Name );
          Osc_CreateMessage( channel, address, ",i", value );
        }
        else if( strcmp( cmd, "CH" ) == 0 )
        {
          snprintf( address, OSC_SCRATCH_SIZE, "/%s/channel", XBeeConfigOsc_Name );
          Osc_CreateMessage( channel, address, ",i", value );
        }
        else if( strcmp( cmd, "ID" ) == 0 )
        {
          snprintf( address, OSC_SCRATCH_SIZE, "/%s/panid", XBeeConfigOsc_Name );
          Osc_CreateMessage( channel, address, ",i", value );
        }
        else
        {
          snprintf( address, OSC_SCRATCH_SIZE, "/%s/at-command", XBeeConfigOsc_Name );
          Osc_CreateMessage( channel, address, ",si", cmd, value );
        }
      }
      break;
    }
  }
  XBee_ResetPacket( xbp );
  return retval;
}

int XBeeOsc_Async( int channel )
{
  XBee_SetActive( 1 );
  int newMsgs = 0;

  if( !XBee_GetAutoSend( false ) || XBee->waitingForConfirm )
    return newMsgs;

  while( XBee_GetPacket( XBee->currentPkt, 0 ) )
  {
    XBeeOsc_HandleNewPacket( XBee->currentPkt, channel );
    newMsgs++;
  }
  return newMsgs;
}

#endif // OSC


