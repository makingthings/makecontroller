

#include "xbee.h"
#include "rtos.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
  Create a new XBeePacket object.
  Once you've created a new XBeePacket, it's recommended to reuse it across read and write
  operations, instead of creating a new one each time.
  
  @param serialChannel (optional) Which serial port to use - valid options are 0 or 1.  Defaults to 0.
  
  \b Example
  \code
  XBeePacket xb; // don't specify a serial channel, defaults to 0
  // or allocate one
  XBeePacket* xb = new XBeePacket(1); // use serial channel 1
  \endcode
*/
XBeePacket::XBeePacket( int serialChannel )
{
  ser = new Serial(serialChannel);
  reset();
}

/**
  Create a TX 16 packet.
*/
void XBeePacket::tx16( uint8 frameID, uint16 destination, uint8 options, uint8* data, uint8 datalength )
{
  packetData.apiId = XBEE_TX16;
  packetData.tx16.frameID = frameID;
  packetData.tx16.destination[0] = destination >> 8;
  packetData.tx16.destination[1] = destination & 0xFF;
  packetData.tx16.options = options;
  packetData.length = datalength + 5;
  uint8* ptr = packetData.tx16.data;
  while( datalength-- )
    *ptr++ = *data++;
}

/**
  A convenience function for creating an AT command packet.
  As per the XBee spec, AT Command packets that want to set/write a value must have 4 bytes of data.  
  Packets that query the value of an AT parameter (as opposed to writing it) must send 0 bytes of data.
  When you're sending your packet, the data length is 4 if you've included a value and 0 if you're just querying.
  
  Make sure you're in API mode before creating & sending packets - see setPacketApiMode( ).
  See the XBee documentation for the official list of AT commands that the XBee modules understand.
  @param frameID The frame ID for this packet that subsequent response/status messages can refer to.  Note 
  that you must include a frame ID (can be arbitrary) if you want to get a response back from the module.
  @param cmd The 2-character AT command.  
  @param value The value to be sent.  If you're querying, you can just leave this out.

  
  \b Example - Writing
  \code
  XBeePacket packet;
  packet.atCmd( 0, "IR", 1000 ); // set our sampling rate to 1000
  packet.send( 4 ); // we're sending 4 bytes of data (our value of 1000)
  \endcode

  \b Example - Reading
  \code
  XBeePacket packet;
  packet.atCmd( 0x53, "IR" ); // query the sampling rate of the IO pins - note the non-zero frameID
  packet.send(0);
  // now use get() to receive the response packet
  \endcode
*/
void XBeePacket::atCmd( uint8 frameID, const char* cmd, int value )
{
  packetData.apiId = XBEE_ATCOMMAND;
  packetData.atCommand.frameID = frameID;
  uint8* ptr = packetData.atCommand.command;
  *ptr++ = *cmd++;
  *ptr++ = *cmd;
  ptr = packetData.atCommand.parameters;
  if( value ) // add the value in big endian
  {
    *ptr++ = (value >> 24) & 0xFF;
    *ptr++ = (value >> 16) & 0xFF;
    *ptr++ = (value >> 8) & 0xFF;
    *ptr++ = value & 0xFF;
  }
}

XBeePacket::~XBeePacket( )
{
  delete ser;
}

/**
  Receive an incoming XBee packet.
  A single call to this will continue to read from the serial port as long as there are characters 
  to read or until it times out.  If a packet has not been completely received, call it repeatedly
  with the same packet.

  Once you've received a packet, be sure to clear it out with a call to reset( ) before reading into it again.
  @param timeout (optional) The number of milliseconds to wait for a packet to arrive.  Defaults to 0.
  @return True if a complete packet has been received, false if not.
  @see setPacketApiMode( )

  \b Example
  \code
  // we're inside a task here...
  XBeePacket xb;
  xb.setPacketApiMode(true); // make sure we're receiving packets
  while( 1 )
  {
    if( xb.get() )
    {
      // ... process the new packet ...
      xb.reset(); // then clear it out before reading again
    }
    Task::sleep(10);
  }
  \endcode
*/
bool XBeePacket::get( int timeout )
{
  int time = RTOS::ticksSinceBoot( );
  do
  {
    ser->clearErrors( );
    while( ser->bytesAvailable( ) )
    {
      int newChar = ser->read( );
      if( newChar == -1 )
        break;
  
      switch( packetData.rxState )
      {
        case XBEE_PACKET_RX_START:
          if( newChar == XBEE_PACKET_STARTBYTE )
            packetData.rxState = XBEE_PACKET_RX_LENGTH_1;
          break;
        case XBEE_PACKET_RX_LENGTH_1:
          packetData.length = newChar;
          packetData.length <<= 8;
          packetData.rxState = XBEE_PACKET_RX_LENGTH_2;
          break;
        case XBEE_PACKET_RX_LENGTH_2:
          packetData.length += newChar;
          if( packetData.length > XBEE_MAX_PACKET_SIZE ) // in case we somehow get some garbage
            packetData.rxState = XBEE_PACKET_RX_START;
          else
            packetData.rxState = XBEE_PACKET_RX_PAYLOAD;
          packetData.crc = 0;
          break;
        case XBEE_PACKET_RX_PAYLOAD:
          *packetData.dataPtr++ = newChar;
          if( ++packetData.index >= packetData.length )
            packetData.rxState = XBEE_PACKET_RX_CRC;
          packetData.crc += newChar;
          break;
        case XBEE_PACKET_RX_CRC:
          packetData.crc += newChar;
          packetData.rxState = XBEE_PACKET_RX_START;
          if( packetData.crc == 0xFF )
            return true;
          else
          {
            reset( );
            return false;
          }
      }
    }
    if ( timeout > 0 )
      Task::sleep( 1 );
  } while( ( RTOS::ticksSinceBoot( ) - time ) < timeout );
  return false;
}

/**
  Send an XBee packet.
  The following methods make it easy to create packets to be sent:
  - tx16( ) - create a data packet to be sent out wirelessly with a 16-bit address
  - tx64( ) - create a data packet to be sent out wirelessly with a 64-bit address
  - atCmd( ) - create an AT command to configure an attached XBee module
  
  @param datalength The length of the data being sent
  @return True on success, otherwise false.
  @see setPacketApiMode( )

  \b Example
  \code
  XBeePacket packet;
  uint8 data[] = "ABC"; // 3 bytes of data
  packet.tx16( 0, 0, 0, data, 3 );
  packet.send( 3 );
  \endcode
*/
bool XBeePacket::send( int datalength )
{
  ser->write( XBEE_PACKET_STARTBYTE );
  int size = datalength;
  switch( packetData.apiId )
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

  ser->write( (size >> 8) & 0xFF ); // send the most significant bit
  ser->write(  size & 0xFF ); // then the LSB
  packetData.crc = 0; // just in case it hasn't been initialized.
  uint8* ptr = (uint8*)&packetData;
  while( size-- )
  {
    ser->write( *ptr );
    packetData.crc += *ptr++;
  }
  //uint8 test = 0xFF - packetData.crc;
  ser->write( 0xFF - packetData.crc );
  return true;
}

/**
  Re-initialize a packet before reading into it.
  Be sure to do this once you've received or sent a packet before reading or writing again.
  @see Take a look at the example for get()
*/
void XBeePacket::reset( )
{
  packetData.dataPtr = (uint8*)&packetData;
  packetData.crc = 0;
  packetData.rxState = XBEE_PACKET_RX_START;
  packetData.length = 0;
  packetData.index = 0;
  packetData.apiId = 0;
}

/**
  Read what kind of packet this is.
  Once you've received a packet, you'll want to check what kind it is so you know how to 
  process it.
  @return The type of packet.
*/
XBeePacketType XBeePacket::type()
{
  return (XBeePacketType)packetData.apiId;
}

/**
  Set a module into packet API mode.  
  XBee modules are in transparent serial port mode by default.  To work with the XBee packet API, 
  you must enable it with this method.  You'll usually want to do this along with any other setup/init 
  you might be doing.

  When enabling packet mode, the XBee module must wait 1 second before it's ready to receive any AT 
  commands - this function will block for that amount of time.  If you disable packet mode,
  you won't get any responses to packets you send until you enable it again.
  @param enabled true to turn packet mode on, false to turn it off.

  \b Example
  \code
  MyTask( void * p )
  {
    XBeePacket xb;
    xb.setPacketApiMode( true ); // initialize the module to be in API mode
    while( 1 )
    {
      // your task here.
    }
  }
  \endcode
*/
bool XBeePacket::setPacketApiMode( bool enabled )
{
  if( enabled )
  {
    char buf[50];
    sprintf( buf, "+++" ); // enter command mode
    ser->write( buf, strlen(buf) );
    Task::sleep( 1025 ); // have to wait one second after +++ to actually get set to receive in AT mode
    sprintf( buf, "ATAP1,CN\r" ); // turn API mode on, and leave command mode
    ser->write( buf, strlen(buf) );
    Task::sleep(50);
    ser->flush( ); // rip the OKs out of there
  }
  else
  {
    atCmd( 0, "AP", 0 );
    send(4);
  }
  return true;
}

/**
  Unpack the info from an incoming packet with a 16-bit address.
  If you've just received a packet with type XBEE_RX16, this method makes it easy to
  extract the data from that packet.  Pass in pointers to the values that you want 
  to be updated with the info from the packet.
  @param srcAddress The 16-bit address of this packet.
  @param sigstrength (optional) The signal strength of this packet.
  @param options (optional) The XBee options for this packet.
  @param data (optional) A pointer that will be set to the data of this packet.
  @param datalength (optional) The length of data in this packet.
  @return True on success, false on failure.
  
  \b Example
  \code
  XBeePacket packet;
  if( packet.get( ) )
  {
    uint16 src;
    uint8 sigstrength;
    uint8* data;
    uint8 datalength;
    if( packet.unpackRX16( &src, &sigstrength, NULL, &data, &datalength ) )
    {
      // ... now those values have been updated with info from the packet
      packet.reset( ); // and clear it out before reading again
    }
  }
  \endcode
*/
bool XBeePacket::unpackRX16( uint16* srcAddress, uint8* sigstrength, uint8* options, uint8** data, uint8* datalength )
{
  if( packetData.apiId != XBEE_RX16 )
    return false;

  if( srcAddress )
  {
    *srcAddress = packetData.rx16.source[0];
    *srcAddress <<= 8;
    *srcAddress += packetData.rx16.source[1];
  }
  if( sigstrength )
    *sigstrength = packetData.rx16.rssi;
  if( options )
    *options = packetData.rx16.options;
  if( data )
    *data = packetData.rx16.data;
  if( datalength )
    *datalength = packetData.length - 5;
  return true;
}

/**
  Unpack the info from an incoming packet with a 64-bit address.
  If you've just received a packet with type XBEE_RX64, this method makes it easy to
  extract the data from that packet.  Pass in pointers to the values that you want 
  to be updated with the info from the packet.
  @param srcAddress The 64-bit address of this packet.
  @param sigstrength (optional) The signal strength of this packet.
  @param options (optional) The XBee options for this packet.
  @param data (optional) A pointer that will be set to the data of this packet.
  @param datalength (optional) The length of data in this packet.
  @return True on success, false on failure.
  
  \b Example
  \code
  XBeePacket packet;
  if( packet.get( ) )
  {
    uint64 src;
    uint8 sigstrength;
    uint8* data;
    uint8 datalength;
    if( packet.unpackRX64( &src, &sigstrength, NULL, &data, &datalength ) )
    {
      // ... now those values have been updated with info from the packet
      packet.reset( ); // and clear it out before reading again
    }
  }
  \endcode
*/
bool XBeePacket::unpackRX64( uint64* srcAddress, uint8* sigstrength, uint8* options, uint8** data, uint8* datalength  )
{
  if( packetData.apiId != XBEE_RX64 )
    return false;

  int i;
  if( srcAddress )
  {
    for( i = 0; i < 8; i++ )
    {
      *srcAddress <<= i*8;
      *srcAddress += packetData.rx64.source[i];
    }
  }
  if( sigstrength )
    *sigstrength = packetData.rx64.rssi;
  if( options )
    *options = packetData.rx64.options;
  if( data )
    *data = packetData.rx64.data;
  if( datalength )
    *datalength = packetData.length - 11;
  return true;
}

/**
  Unpack the info from an incoming IO packet with a 16-bit address.
  When an XBee module has been given a sample rate, it will sample its IO pins according to their 
  current configuration and send an IO packet with the sample data.  This function will extract 
  the sample info into an array of ints for you.  There are 9 IO pins on the XBee modules, 
  so be sure that the array you pass in has room for 9 ints.

  Pass \b NULL into any of the parameters you don't care about.
  @param srcAddress A uint16 that will be filled up with the 16-bit address of this packet.
  @param sigstrength (optional) A uint8 that will be filled up with the signal strength of this packet.
  @param options (optional) A uint8 that will be filled up with the XBee options for this packet.
  @param samples (optional) An array of ints that will be filled up with the sample values from this packet.
  @return True on success, false on failure.
  @see XBeeConfig_SetSampleRate( ), XBeeConfig_SetIOs( )
  
  \b Example
  \code
  XBeePacket packet;
  if( packet.get( 0 ) )
  {
    uint16 src;
    uint8 sigstrength;
    int samples[9];
    if( packet.unpackIO16( &src, &sigstrength, NULL, samples ) )
    {
      // ... now those values have been updated with info from the packet
      packet.reset( ); // and clear it out before reading again
    }
  }
  \endcode
*/
bool XBeePacket::unpackIO16( uint16* srcAddress, uint8* sigstrength, uint8* options, int* samples )
{
  if( packetData.apiId != XBEE_IO16 )
    return false;
  if( srcAddress )
  {
    *srcAddress = packetData.io16.source[0];
    *srcAddress <<= 8;
    *srcAddress += packetData.io16.source[1];
  }
  if( sigstrength )
    *sigstrength = packetData.io16.rssi;
  if( options )
    *options = packetData.io16.options;
  if( samples )
  {
    if( !getIOValues( samples ) )
      return false;
  }
  return true;
}

/**
  Unpack the info from an incoming IO packet with a 64-bit address.
  When an XBee module has been given a sample rate, it will sample its IO pins according to their current configuration
  and send an IO packet with the sample data.  This function will extract the sample info into an array of ints for you.
  There are 9 IO pins on the XBee modules, so be sure that the array you pass in has room for 9 ints.
  
  @param srcAddress A pointer to a uint64 that will be filled up with the 16-bit address of this packet.
  @param sigstrength (optional) A uint8 that will be filled up with the signal strength of this packet.
  @param options (optional) A uint8 that will be filled up with the XBee options for this packet.
  @param samples (optional) An array of ints that will be filled up with the sample values from this packet.
  @return True on success, false on failure.
  @see XBeeConfig_SetSampleRate( ), XBeeConfig_SetIOs( )
  
  \b Example
  \code
  XBeePacket packet;
  if( packet.get( 0 ) )
  {
    uint64 src;
    uint8 sigstrength;
    int samples[9];
    if( packet.unpackIO64( &src, &sigstrength, NULL, samples ) )
    {
      // ... now those values have been updated with info from the packet
      packet.reset( ); // and clear it out before reading again
    }
  }
  \endcode
*/
bool XBeePacket::unpackIO64( uint64* srcAddress, uint8* sigstrength, uint8* options, int* samples )
{
  if( packetData.apiId != XBEE_RX64 )
    return false;
  if( srcAddress )
  {
    int i;
    for( i = 0; i < 8; i++ )
    {
      *srcAddress <<= i*8;
      *srcAddress += packetData.io64.source[i];
    }
  }
  if( sigstrength )
    *sigstrength = packetData.io64.rssi;
  if( options )
    *options = packetData.io64.options;
  if( samples )
  {
    if( !getIOValues( samples ) )
      return false;
  }
  return true;
}

/**
  Unpack the info from an incoming AT Command Response packet.
  When you send an AT Command packet querying a value, the module will respond with 
  an AT Command Response message.
  
  If you've just received a packet with type XBEE_ATCOMMANDRESPONSE, this method makes it easy to
  extract its data.  Pass in pointers to the values that you want 
  to be updated with the info from the packet.
  
  @param frameID A uint64 that will be filled up with the 16-bit address of this packet.
  @param command (optional) A uint8 that will be filled up with the signal strength of this packet.
  @param status (optional) A uint8 that will be filled up with the XBee options for this packet.
  @param datavalue (optional) The value of the requested command.
  @return True on success, false on failure.
  
  \b Example
  \code
  XBeePacket packet;
  if( packet.get() )
  {
    uint8 frameID;
    char* command;
    uint8 status;
    int value;
    if( packet.unpackAtResponse( &frameID, command, &status, &value ) )
    {
      // ... now those values have been updated with info from the packet
      packet.reset(); // and clear it out before reading again
    }
  }
  \endcode
*/
bool XBeePacket::unpackAtResponse( uint8* frameID, char** command, uint8* status, int* datavalue )
{
  if( packetData.apiId != XBEE_ATCOMMANDRESPONSE )
    return false;
  if( frameID )
    *frameID = packetData.atResponse.frameID;
  if( command )
    *command = (char*)packetData.atResponse.command;
  if( status )
    *status = packetData.atResponse.status;
  if( datavalue )
  {
    uint8 *dataPtr = packetData.atResponse.value;
    int i;
    int datalength = packetData.length - 5; // data comes after apiID, frameID, 2-bytes of cmd, and 1-byte status
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
  When you send a packet, the module sends back a TX Status message.  This indicates whether the packet was transmitted
  successfully or not.  If the message you sent had a frameID of 0, a TX status message will not be generated.

  If you've just received a packet with type XBEE_TXSTATUS, this method makes it easy to
  extract its data.  Pass in pointers to the values that you want to be updated with the info from the packet.
  
  @param frameID A uint8 that will be filled up with the frame ID of this packet.
  @param status (optional) A uint8 that will be filled up with the status of this packet.
  @return True on success, false on failure.
  
  \b Example
  \code
  XBeePacket packet;
  if( packet.get( ) )
  {
    uint8 frameID;
    uint8 status;
    if( packet.unpackTXStatus( &frameID, &status ) )
    {
      // ... now those values have been updated with info from the packet
      packet.reset( ); // and clear it out before reading again
    }
  }
  \endcode
*/
bool XBeePacket::unpackTXStatus( uint8* frameID, uint8* status )
{
  if( packetData.apiId != XBEE_TXSTATUS )
    return false;
  if( frameID )
    *frameID = packetData.txStatus.frameID;
  if( status )
    *status = packetData.txStatus.status;
  return true;
}

bool XBeePacket::getIOValues( int *inputs )
{
  if( packetData.apiId == XBEE_IO16 || packetData.apiId == XBEE_IO64 )
  {
    int i;
    static bool enabled;
    int digitalins = 0;
    uint8* ptr;
    int channelIndicators;
    if( packetData.apiId == XBEE_IO16 )
    {
      ptr = packetData.io16.data;
      channelIndicators = (packetData.io16.channelIndicators[0] << 0x08) | packetData.io16.channelIndicators[1];
    }
    else // packetData.apiId == XBEE_IO64
    {
      ptr = packetData.io64.data;
      channelIndicators = (packetData.io64.channelIndicators[0] << 0x08) | packetData.io64.channelIndicators[1];
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
            int dig0 = *ptr++ << 0x08;
            digitalins = dig0 | *ptr++;
          }
          inputs[i] = ((digitalins >> i) & 1) * 1023;
        }
      }
      else // analog ins
      {
        if( enabled )
        {
          int ain_msb = *ptr++ << 0x08;
          inputs[i-9] = ain_msb | *ptr++;
        }
      }
    }
    return true;
  }
  else
    return false;
}






