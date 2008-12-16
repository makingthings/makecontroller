

#include "xbee_.h"
#include "rtos_.h"
#include <stdlib.h>
#include "string.h"

XBeePacket::XBeePacket( )
{
  commonInit();
}

/**
  Create a TX 16 packet.
*/
XBeePacket::XBeePacket( uint8 frameID, uint16 destination, uint8 options, uint8* data, uint8 datalength )
{
  commonInit();
  p.apiId = XBEE_TX16;
  p.tx16.frameID = frameID;
  p.tx16.destination[0] = destination >> 8;
  p.tx16.destination[1] = destination & 0xFF;
  p.tx16.options = options;
  p.length = datalength + 5;
  uint8* pp = p.tx16.data;
  while( datalength-- )
    *pp++ = *data++;
}

/**
  Create an AT command packet.
*/
XBeePacket::XBeePacket( uint8 frameID, char* cmd, int value )
{
  commonInit();
  p.apiId = XBEE_ATCOMMAND;
  p.atCommand.frameID = frameID;
  uint8* pp = p.atCommand.command;
  *pp++ = *cmd++;
  *pp++ = *cmd;
  pp = p.atCommand.parameters;
  if( value ) // add the value in big endian
  {
    *pp++ = (value >> 24) & 0xFF;
    *pp++ = (value >> 16) & 0xFF;
    *pp++ = (value >> 8) & 0xFF;
    *pp++ = value & 0xFF;
  }
}

void XBeePacket::commonInit()
{
  ser = new Serial(0);
  reset();
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
  
      switch( p.rxState )
      {
        case XBEE_PACKET_RX_START:
          if( newChar == XBEE_PACKET_STARTBYTE )
            p.rxState = XBEE_PACKET_RX_LENGTH_1;
          break;
        case XBEE_PACKET_RX_LENGTH_1:
          p.length = newChar;
          p.length <<= 8;
          p.rxState = XBEE_PACKET_RX_LENGTH_2;
          break;
        case XBEE_PACKET_RX_LENGTH_2:
          p.length += newChar;
          if( p.length > XBEE_MAX_PACKET_SIZE ) // in case we somehow get some garbage
            p.rxState = XBEE_PACKET_RX_START;
          else
            p.rxState = XBEE_PACKET_RX_PAYLOAD;
          p.crc = 0;
          break;
        case XBEE_PACKET_RX_PAYLOAD:
          *p.dataPtr++ = newChar;
          if( ++p.index >= p.length )
            p.rxState = XBEE_PACKET_RX_CRC;
          p.crc += newChar;
          break;
        case XBEE_PACKET_RX_CRC:
          p.crc += newChar;
          p.rxState = XBEE_PACKET_RX_START;
          if( p.crc == 0xFF )
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
  switch( p.apiId )
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
  p.crc = 0; // just in case it hasn't been initialized.
  uint8* pp = (uint8*)&p;
  while( size-- )
  {
    ser->write( *pp );
    p.crc += *pp++;
  }
  //uint8 test = 0xFF - p.crc;
  ser->write( 0xFF - p.crc );
  return true;
}

void XBeePacket::reset( )
{
  p.dataPtr = (uint8*)&p;
  p.crc = 0;
  p.rxState = XBEE_PACKET_RX_START;
  p.length = 0;
  p.index = 0;
  p.apiId = 0;
  memset( p.payload, 0, 100 );
}

XBeeApiId XBeePacket::type()
{
  return (XBeeApiId)p.apiId;
}

bool XBeePacket::unpackRX16( uint16* srcAddress, uint8* sigstrength, uint8* options, uint8** data, uint8* datalength )
{
  if( p.apiId != XBEE_RX16 )
    return false;

  if( srcAddress )
  {
    *srcAddress = p.rx16.source[0];
    *srcAddress <<= 8;
    *srcAddress += p.rx16.source[1];
  }
  if( sigstrength )
    *sigstrength = p.rx16.rssi;
  if( options )
    *options = p.rx16.options;
  if( data )
    *data = p.rx16.data;
  if( datalength )
    *datalength = p.length - 5;
  return true;
}

bool XBeePacket::unpackRX64( uint64* srcAddress, uint8* sigstrength, uint8* options, uint8** data, uint8* datalength  )
{
  if( p.apiId != XBEE_RX64 )
    return false;

  int i;
  if( srcAddress )
  {
    for( i = 0; i < 8; i++ )
    {
      *srcAddress <<= i*8;
      *srcAddress += p.rx64.source[i];
    }
  }
  if( sigstrength )
    *sigstrength = p.rx64.rssi;
  if( options )
    *options = p.rx64.options;
  if( data )
    *data = p.rx64.data;
  if( datalength )
    *datalength = p.length - 11;
  return true;
}

bool XBeePacket::unpackIO16( uint16* srcAddress, uint8* sigstrength, uint8* options, int* samples )
{
  if( p.apiId != XBEE_IO16 )
    return false;
  if( srcAddress )
  {
    *srcAddress = p.io16.source[0];
    *srcAddress <<= 8;
    *srcAddress += p.io16.source[1];
  }
  if( sigstrength )
    *sigstrength = p.io16.rssi;
  if( options )
    *options = p.io16.options;
  if( samples )
  {
    if( !getIOValues( samples ) )
      return false;
  }
  return true;
}

bool XBeePacket::unpackIO64( uint64* srcAddress, uint8* sigstrength, uint8* options, int* samples )
{
  if( p.apiId != XBEE_RX64 )
    return false;
  if( srcAddress )
  {
    int i;
    for( i = 0; i < 8; i++ )
    {
      *srcAddress <<= i*8;
      *srcAddress += p.io64.source[i];
    }
  }
  if( sigstrength )
    *sigstrength = p.io64.rssi;
  if( options )
    *options = p.io64.options;
  if( samples )
  {
    if( !getIOValues( samples ) )
      return false;
  }
  return true;
}

bool XBeePacket::unpackAtResponse( uint8* frameID, char** command, uint8* status, int* datavalue )
{
  if( p.apiId != XBEE_ATCOMMANDRESPONSE )
    return false;
  if( frameID )
    *frameID = p.atResponse.frameID;
  if( command )
    *command = (char*)p.atResponse.command;
  if( status )
    *status = p.atResponse.status;
  if( datavalue )
  {
    uint8 *dataPtr = p.atResponse.value;
    int i;
    int datalength = p.length - 5; // data comes after apiID, frameID, 2-bytes of cmd, and 1-byte status
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
  if( p.apiId != XBEE_TXSTATUS )
    return false;
  if( frameID )
    *frameID = p.txStatus.frameID;
  if( status )
    *status = p.txStatus.status;
  return true;
}

bool XBeePacket::getIOValues( int *inputs )
{
  if( p.apiId == XBEE_IO16 || p.apiId == XBEE_IO64 )
  {
    int i;
    static bool enabled;
    int digitalins = 0;
    uint8* pp;
    int channelIndicators;
    if( p.apiId == XBEE_IO16 )
    {
      pp = p.io16.data;
      channelIndicators = (p.io16.channelIndicators[0] << 0x08) | p.io16.channelIndicators[1];
    }
    else // p.apiId == XBEE_IO64
    {
      pp = p.io64.data;
      channelIndicators = (p.io64.channelIndicators[0] << 0x08) | p.io64.channelIndicators[1];
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
            int dig0 = *pp++ << 0x08;
            digitalins = dig0 | *pp++;
          }
          inputs[i] = ((digitalins >> i) & 1) * 1023;
        }
      }
      else // analog ins
      {
        if( enabled )
        {
          int ain_msb = *pp++ << 0x08;
          inputs[i-9] = ain_msb | *pp++;
        }
      }
    }
    return true;
  }
  else
    return false;
}






