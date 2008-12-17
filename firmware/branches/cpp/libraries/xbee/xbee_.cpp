

#include "xbee_.h"
#include "rtos_.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

XBeePacket::XBeePacket( )
{
  ser = new Serial(0);
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
  Create an AT command packet.
*/
void XBeePacket::atCmd( uint8 frameID, char* cmd, int value )
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

void XBeePacket::reset( )
{
  packetData.dataPtr = (uint8*)&packetData;
  packetData.crc = 0;
  packetData.rxState = XBEE_PACKET_RX_START;
  packetData.length = 0;
  packetData.index = 0;
  packetData.apiId = 0;
}

XBeeApiId XBeePacket::type()
{
  return (XBeeApiId)packetData.apiId;
}

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






