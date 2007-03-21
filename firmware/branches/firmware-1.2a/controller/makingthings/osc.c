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

/** @defgroup OSC
	Communicate with the Make Controller Kit via OSC.
	
	\section osc OSC
	"Open Sound Control (OSC) is a protocol for communication among computers, sound synthesizers, 
	and other multimedia devices that is optimized for modern networking technology."  
	With OSC implemented on the Make Controller Kit, it can already talk to 
	a wide variety of environments and devices like Java, Max/MSP, Pd, Flash, Processing, SuperCollider,
	and many others.
	
	OSC is based on the notion of \b messages, which are composed of an address, and the data to 
	be sent to that address.  The address looks a lot like a URL that you might type into
	your internet browser.  Each element in the address is like a directory, with other 
	elements inside it, and each element is separated from the next by a slash (/).  Each OSC
	message must also start with a slash.  
	
	\par Example:
	For instance, the OSC address
	\code /make/controller/kit \endcode
	says, "start in the 'make' directory, go down to the 'controller' directory, and 
	finally to the 'kit' directory.  
	
	Any number of \b argument \b values can be sent to that address by including them in the message
	after the address.  These values can be integers (ints), floats, or strings.  
	
	\par Example:
	If we wanted to send the value 35.4 to the address above, we would create the message
	\code /make/controller/kit 35.4 \endcode
	with a space between the address and the data.\n\n
	Additional data can be added, each separated by a space
	\code /make/controller/kit 35.4 lawn 12 \endcode
	
	\section osc_mck OSC & the Make Controller Kit
	Many devices on the Make Controller Kit can be addressed via OSC.  In sending messages to them,
	we need to know the OSC address, and the appropriate argument values to send.
	
	The Make Controller Kit is orgranized, for OSC, into \b subsystems.  Each subsystem has one or more
	\b devices, and each device has one or more \b properties.  To address a particular device, you'll
	need to create an OSC message specifying the address in that format: 
	\code /subsystem/device/property \endcode
	Each of the modules above provide the details for each of the subsystems on the board, along with their
	devices and properties.  A simple example is given below.
	
	\par Example:
	To create an OSC message to turn an LED on, first identify the appropriate \b subsystem.
	In this case, the subsystem is called \b appled.\n\n
	There are 4 LEDs, so we need to specify which one to control.
	The LEDs are numbered 0 -3, so choosing the first LED means the \b device value is 0.\n\n
	The \b property of the LED that turns it on and off is its 'state'.\n\n
	Lastly, we must specify what the state should actually be, by including an \b argument value after the address.
	To turn it on, this value should be 1.  0 would turn it off.\n
	The complete OSC message looks like
	\code /appled/0/state 1 \endcode
*/

#include "config.h"
#include "osc.h"
#include "network.h"
#include "rtos.h"
#include "debug.h"
#include "types.h"

// These for the unwrapped semaphore
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>

#define OSC_CHANNEL_COUNT    2
#define OSC_MAX_MESSAGE_IN   200
#define OSC_MAX_MESSAGE_OUT  400

typedef struct OscChannel_
{
  char buffer[ OSC_MAX_MESSAGE_OUT ];
  char incoming[ OSC_MAX_MESSAGE_IN ];
  char* bufferPointer;
  int  bufferRemaining;
  int  messages;
  int replyAddress;
  int replyPort;
  int (*sendMessage)( char* packet, int length, int replyAddress, int replyPort );
  xSemaphoreHandle semaphore;
  int running;
}OscChannel;

typedef struct OscSubsystem_
{
  const char* name;
  int (*receiveMessage)( int channel, char* buffer, int length );  
  int (*poll)( int channel );
}OscSubsystem;

struct Osc_
{
  int users;
  int running;
  int subsystemHighest;
  void* sendSocket;
  void* UsbTaskPtr;
  void* UdpTaskPtr;
  OscChannel* channel[ OSC_CHANNEL_COUNT ];
  OscSubsystem* subsystem[ OSC_SUBSYSTEM_COUNT ];
  int registeredSubsystems;
};

struct Osc_* Osc;

int Osc_Lock( OscChannel* ch );
void Osc_Unlock( OscChannel *ch );

int Osc_Quicky( int channel, char* preamble, char* string );
char* Osc_WritePaddedString( char* buffer, int* length, char* string );
char* Osc_WritePaddedBlob( char* buffer, int* length, char* blob, int blen );
char* Osc_WriteTimetag( char* buffer, int* length, int a, int b );
int Osc_EndianSwap( int a );
int Osc_ReceiveMessage( int channel, char* message, int length );
char* Osc_CreateBundle( char* buffer, int* length, int a, int b );
int Osc_PropertyLookup( char** properties, char* property );
int Osc_ReadInt( char* buffer );
float Osc_ReadFloat( char* buffer );

int Osc_UdpPacketSend( char* packet, int length, int replyAddress, int replyPort );
int Osc_UsbPacketSend( char* packet, int length, int replyAddress, int replyPort );
int Osc_TcpPacketSend( char* packet, int length, int replyAddress, int replyPort );

void Osc_ResetChannel( OscChannel* ch );

int Osc_SendPacketInternal( OscChannel* ch );

int Osc_SetReplyAddress( int channel, int replyAddress );
int Osc_SetReplyPort( int channel, int replyPort );

int Osc_SendMessage( int channel, char* message, int length );
int Osc_ReceivePacket( int channel, char* packet, int length );
int Osc_Poll( int channel, char* buffer, int maxLength, int* length );

bool Osc_PatternMatch(const char * pattern, const char * test); 
 
int Osc_Quicky( int channel, char* preamble, char* string );
char* Osc_WritePaddedString( char* buffer, int* length, char* string );
char* Osc_WriteTimetag( char* buffer, int* length, int a, int b );
int Osc_EndianSwap( int a );
int Osc_ReceiveMessage( int channel, char* message, int length );
char* Osc_CreateBundle( char* buffer, int* length, int a, int b );
char* Osc_CreateMessageInternal( char* bp, int* length, char* address, char* format, va_list args );
int Osc_CreateMessageToBuf( char* bp, int* length, char* address, char* format, ... );

int Osc_ReadInt( char* buffer );
float Osc_ReadFloat( char* buffer );
int Osc_UdpPacketSend( char* packet, int length, int replyAddress, int replyPort );

//void Osc_ResetChannel( OscChannel* ch );

void Osc_UdpTask( void* parameters );
void Osc_UsbTask( void* parameters );
void Osc_TcpTask( void* parameters );


/**
	Osc_SetActive.
	Sets whether the Osc system active. \n
	@param state
	@return Zero on success.
*/
void Osc_SetActive( int state )
{
  if ( state && Osc == NULL )
  {
    Osc = Malloc( sizeof( struct Osc_ ) );
    Osc->subsystemHighest = 0;
    Osc->registeredSubsystems = 0;
    int i;
    for( i = 0; i < OSC_CHANNEL_COUNT; i++ )
      Osc->channel[ i ] = NULL;
    for( i = 0; i < OSC_SUBSYSTEM_COUNT; i++ )
      Osc->subsystem[ i ] = NULL;

    Osc->UdpTaskPtr = TaskCreate( Osc_UdpTask, "OSC-UDP", 500, (void*)OSC_CHANNEL_UDP, 3 );
    Osc->UsbTaskPtr = TaskCreate( Osc_UsbTask, "OSC-USB", 300, (void*)OSC_CHANNEL_USB, 3 );
    //TaskCreate( Osc_TcpTask, "Osc-Tcp", 500, (void*)OSC_CHANNEL_TCP, 3 );

    Osc->users = 1;
    Osc->running = true;
  }
  if( !state && Osc )
  {
    TaskDelete( Osc->UsbTaskPtr );
    TaskDelete( Osc->UdpTaskPtr );
    int i;
    for( i = 0; i < OSC_CHANNEL_COUNT; i++ )
      Free( Osc->channel[ i ] );
    for( i = 0; i < OSC_SUBSYSTEM_COUNT; i++ )
      Free( Osc->subsystem[ i ] );
    Free ( Osc );
    Osc = NULL;
  }
  return;
}

/**
	Osc_GetActive.
	Returns the state of the Osc system.
	@return Non-zero if active, zero if inactive.
*/
int Osc_GetActive( )
{
  return Osc != NULL;
}

int Osc_UsbPacketSend( char* packet, int length, int replyAddress, int replyPort )
{
  (void)replyAddress; // get rid of the spurious 'variable not used' warnings
  (void)replyPort;
  return Usb_SlipSend( packet, length );
}

int Osc_UdpPacketSend( char* packet, int length, int replyAddress, int replyPort )
{
  if ( replyAddress != 0 && replyPort != 0 )
    return DatagramSocketSend( Osc->sendSocket, replyAddress, replyPort, packet, length );
  else
    return CONTROLLER_ERROR_NO_ADDRESS;
}

int Osc_GetRunning( )
{
  if( Osc )
    return Osc->running;
  else
    return 0;
}

void Osc_UdpTask( void* parameters )
{
  int channel = (int)parameters;
  Osc->channel[ channel ] = Malloc( sizeof( OscChannel ) );

  OscChannel *ch = Osc->channel[ channel ];
  ch->running = false;
  Osc_SetReplyPort( channel, 10000 );

  ch->sendMessage = Osc_UdpPacketSend;

  Osc_ResetChannel( ch );

  // Chill until the Network is up
  while ( !Network_GetActive() )
    Sleep( 100 );

  ch->running = true;
  void* ds = DatagramSocket( NetworkOsc_GetUdpPort() );
  
  Osc->sendSocket = DatagramSocket( 0 );

  while ( true )
  {
    int address;
    int port;
    int length = DatagramSocketReceive( ds, NetworkOsc_GetUdpPort(), &address, &port, ch->incoming, OSC_MAX_MESSAGE_IN );

    Osc_SetReplyAddress( channel, address );

    Osc_ReceivePacket( channel, ch->incoming, length );
    Sleep( 1 );
  }
}
/*
void Osc_TcpTask( void* parameters )
{
  int channel = (int)parameters;
  OscChannel *ch = &Osc->channel[ channel ];
  //ch->sendMessage = Osc_TcpPacketSend;
  Osc_ResetChannel( ch );

  // Chill until the Network is up
  while ( !Network_GetActive() )
    Sleep( 100 );

  ch->running = true;
  struct netconn* sock = NULL;

  // TODO - check to see if the autoconnect property is set, and fire things up now if it is

  // now get down to business
  while ( true )
  {
    int address;
    int port;

    //int length = DatagramSocketReceive( ds, ch->replyPort, &address, &port, ch->incoming, OSC_MAX_MESSAGE_IN );

    //Osc_SetReplyAddress( channel, address );

    //Osc_ReceivePacket( channel, ch->incoming, length );
    Sleep( 1 );
  }
}
*/
void Osc_UsbTask( void* parameters )
{
  int channel = (int)parameters;
  Osc->channel[ channel ] = Malloc( sizeof( OscChannel ) );
  OscChannel *ch = Osc->channel[ channel ];

  ch->sendMessage = Osc_UsbPacketSend;

  Osc_ResetChannel( ch );

  // Chill until the USB connection is up
  while ( !Usb_GetActive() )
    Sleep( 100 );

  ch->running = true;

  //strcpy( data, "HELLO!!" );
  while ( true )
  {
    int length = Usb_SlipReceive( ch->incoming, OSC_MAX_MESSAGE_IN );

    if ( length > 0 )
      Osc_ReceivePacket( channel, ch->incoming, length );
  }
}

int Osc_SetReplyAddress( int channel, int replyAddress )
{
  if ( channel < 0 || channel >= OSC_CHANNEL_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  OscChannel* ch =  Osc->channel[ channel ];

  ch->replyAddress = replyAddress;

  return CONTROLLER_OK;
}

int Osc_SetReplyPort( int channel, int replyPort )
{
  if ( channel < 0 || channel >= OSC_CHANNEL_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  OscChannel* ch =  Osc->channel[ channel ];

  ch->replyPort = replyPort;

  return CONTROLLER_OK;
}

void Osc_ResetChannel( OscChannel* ch )
{
  ch->bufferPointer = ch->buffer;
  ch->bufferRemaining = OSC_MAX_MESSAGE_OUT;
  ch->messages = 0;
}

int Osc_RegisterSubsystem( const char *name, int (*subsystem_ReceiveMessage)( int channel, char* buffer, int length ), int (*subsystem_Poll)( int channel ) )
{
  int subsystem = Osc->registeredSubsystems;
  if ( Osc->registeredSubsystems++ > OSC_SUBSYSTEM_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX; 

  Osc->subsystem[ subsystem ] = Malloc( sizeof( OscSubsystem ) );
  OscSubsystem* sub = Osc->subsystem[ subsystem ];
  sub->name = name;
  sub->receiveMessage = subsystem_ReceiveMessage;
  sub->poll = subsystem_Poll;
  return CONTROLLER_OK;
}

int Osc_SendMessage( int channel, char* message, int length )
{
  (void)channel;
  (void)message;
  (void)length;
  return CONTROLLER_OK;
}

int Osc_ReceivePacket( int channel, char* packet, int length )
{
  // Got a packet.  Unpacket.

  switch ( *packet )
  {
    case '/':
      Osc_ReceiveMessage( channel, packet, length );
      break;
    case '#':
      if ( strcmp( packet, "#bundle" ) == 0 )
      {
        // skip bundle text and timetag
        packet += 16;
        length -= 16;
        while ( length > 0 )
        {
          // read the length (pretend packet is a pointer to integer)
          int messageLength = Osc_EndianSwap( *((int*)packet) );
          packet += 4;
          length -= 4;
          if ( messageLength <= length )
            Osc_ReceivePacket( channel, packet, messageLength );
          length -= messageLength;
          packet += messageLength;
        }
      }
      break;
    default:
      // Something else?
      Osc_CreateMessage( channel, "/error", ",s", "Packet Error" );
      break;
  }

  return Osc_SendPacket( channel );
}

int Osc_ReceiveMessage( int channel, char* message, int length )
{
  // Got a packet.  Unpacket.

  // Confirm it's a message
  if ( *message == '/' )
  {
    // Plain message
    char* nextSlash = strchr( message + 1, '/' );
    if ( nextSlash != NULL )
      *nextSlash = 0;
    int i;
    int count = 0;
    for ( i = 0; i < Osc->registeredSubsystems; i++ )
    {
      OscSubsystem* sub = Osc->subsystem[ i ];
      if ( Osc_PatternMatch( message + 1, sub->name ) )
      {
        count++;
        if ( nextSlash )
          (sub->receiveMessage)( channel, nextSlash + 1, length - ( nextSlash - message ) - 1 );
        else
          (sub->receiveMessage)( channel, 0, 0 );
      }
    }
    if ( count == 0 )
    {
      char s[ OSC_SCRATCH_SIZE ];
      snprintf( s, OSC_SCRATCH_SIZE, "No Subsystem Match - %s", message + 1 ); 
      Osc_CreateMessage( channel, "/error", ",s", s );
    }
  }
  else
  {
    return CONTROLLER_ERROR_BAD_DATA;
  }
  return CONTROLLER_OK;
}

int Osc_SendPacket( int channel )
{
  if ( channel < 0 || channel >= OSC_CHANNEL_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  OscChannel* ch = Osc->channel[ channel ];

  if ( ch->messages == 0 )
    return CONTROLLER_OK;

  if ( Osc_Lock( ch ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_LOCK_ERROR;

  int ret = Osc_SendPacketInternal( ch );

  Osc_Unlock( ch );

  return ret;
}

int Osc_SendPacketInternal( OscChannel* ch )
{
  if ( ch->messages == 0 )
    return CONTROLLER_OK;

  // set the buffer and length up
  char* buffer = ch->buffer;
  int length = OSC_MAX_MESSAGE_OUT - ch->bufferRemaining;

  // see if we can dispense with the bundle business
  if ( ch->messages == 1 )
  {
    // skip 8 bytes of "#bundle" and 8 bytes of timetag and 4 bytes of size
    buffer += 20;
    // shorter too
    length -= 20;
  }

  (*ch->sendMessage)( buffer, length, ch->replyAddress, ch->replyPort );

  Osc_ResetChannel( ch );

  return CONTROLLER_OK;
}

int Osc_Poll( int channel, char* buffer, int maxLength, int* length )
{
  (void)buffer;
  (void)maxLength;
  (void)length;
  (void)channel;
  return CONTROLLER_OK;
}

int Osc_Quicky( int channel, char* preamble, char* string )
{
  return Osc_CreateMessage( channel, "/debug", ",ss", preamble, string );
}

int Osc_SubsystemError( int channel, char* subsystem, char* string )
{
  char address[ OSC_SCRATCH_SIZE ];
  snprintf( address, OSC_SCRATCH_SIZE, "/%s/error", subsystem ); 
  return Osc_CreateMessage( channel, address, ",s", string );
}

// An OSC message has arrived, check it out... although, don't change it - others need
// to use it.  If the original OSC packet was a BUNDLE - i.e. had many 
// messages in it, they will be unpacked elsewhere.  We'll only get 
// individual messages here.
// What is passed is the part after the first subsystem address.
// So "/dipswitch/active 1" would be passed to this routine as "active 1"
// Not literally like this, of course, but as OSC messages
// In order to do anything, we need some extra information from 
// the subsystem like what the names of its properties are, what 
// the getters and setters are, etc.
int Osc_BlobReceiverHelper( int channel, char* message, int length, 
                           char* subsystemName, 
                           int (*blobPropertySet)( int property, uchar* buffer, int length ),
                           int (*blobPropertyGet)( int property, uchar* buffer, int size ),
                           char* blobPropertyNames[] )
{
  if ( message == NULL )
    return CONTROLLER_ERROR_NO_PROPERTY;

  int propertyIndex = Osc_PropertyLookup( blobPropertyNames, message );
  if ( propertyIndex == -1 )
    return CONTROLLER_ERROR_UNKNOWN_PROPERTY;

  // Sometime after the address, the data tag begins - this is the description 
  // of the data in the rest of the message.  It starts with a comma.  Return
  // where it is into 'type'.  If there is no comma, this is bad.
  char* type = Osc_FindDataTag( message, length );
  if ( type == NULL )
    return CONTROLLER_ERROR_NO_TYPE_TAG;

  // We can tell if there's data by seeing if the character after the comma
  // is a zero or not.
  if ( type[ 1 ] != 0 )
  {
    if ( type[ 1 ] == 'b' )
    {
      unsigned char *buffer;
      int size;
      int count = Osc_ExtractData( type, "b", &buffer, &size );
      if ( count != 1 )
        return CONTROLLER_ERROR_BAD_DATA;
    
      (*blobPropertySet)( propertyIndex, buffer, size );
    }
    else
    {
      if ( type[ 1 ] == 's' )
      {
        unsigned char *buffer;
        int count = Osc_ExtractData( type, "s", &buffer );
        if ( count != 1 )
          return CONTROLLER_ERROR_BAD_DATA;
    
        (*blobPropertySet)( propertyIndex, buffer, strlen( (char*)buffer ) );
      }
      else
        return CONTROLLER_ERROR_BAD_DATA;
    }
      
  }
  else
  {
    // No data, then.  I guess it was a read.  The XXXXOsc getters
    // take the channel number and use it to call
    // Osc_CreateMessage() which adds a new message to the outgoing
    // stack
    uchar buffer[ OSC_SCRATCH_SIZE ];
    int size = (*blobPropertyGet)( propertyIndex, buffer, OSC_SCRATCH_SIZE );
    char address[ OSC_SCRATCH_SIZE ]; 
    snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", subsystemName, blobPropertyNames[ propertyIndex ] ); 
    Osc_CreateMessage( channel, address, ",b", buffer, size );
  }

  return CONTROLLER_OK;
}

// An OSC message has arrived, check it out... although, don't change it - others need
// to use it.  If the original OSC packet was a BUNDLE - i.e. had many 
// messages in it, they will be unpacked elsewhere.  We'll only get 
// individual messages here.
// What is passed is the part after the first subsystem address.
// So "/dipswitch/active 1" would be passed to this routine as "active 1"
// Not literally like this, of course, but as OSC messages
// In order to do anything, we need some extra information from 
// the subsystem like what the names of its properties are, what 
// the getters and setters are, etc.
int Osc_IntReceiverHelper( int channel, char* message, int length, 
                           char* subsystemName, 
                           int (*propertySet)( int property, int value ),
                           int (*propertyGet)( int property ),
                           char* propertyNames[] )
{
  if ( message == NULL )
    return CONTROLLER_ERROR_NO_PROPERTY;

  int propertyIndex = Osc_PropertyLookup( propertyNames, message );
  if ( propertyIndex == -1 )
    return CONTROLLER_ERROR_UNKNOWN_PROPERTY;

/*
  int bits;
  int number = Osc_NumberMatch( indexCount, message, &bits );
  if ( number == -1 && bits == -1 )
    return Osc_SubsystemError( channel, subsystemName, "Bad index" );
*/
  
  // Sometime after the address, the data tag begins - this is the description 
  // of the data in the rest of the message.  It starts with a comma.  Return
  // where it is into 'type'.  If there is no comma, this is bad.
  char* type = Osc_FindDataTag( message, length );
  if ( type == NULL )
    return CONTROLLER_ERROR_NO_TYPE_TAG;

  // We can tell if there's data by seeing if the character after the comma
  // is a zero or not.
  if ( type[ 1 ] == 'i' || type[ 1 ] == 'f' )
  {
    // If there was int or float data, it was a WRITE.
    // So, sort of scanff-like, go get the data.  Here we pass in where the data is
    // thanks to the previous routine and then specify what we expect to find there
    // in tag terms (i.e. "i", "s", "f" and others).  Finally we pass in a set 
    // of pointers to the data types we want to extract.  Osc_ExtractData()
    // will rummage around in the message magically grabbing values for you,
    // reporting how many it got.  It will convert ints and floats if necessary.
    int value;
    int count = Osc_ExtractData( type, "i", &value );
    if ( count != 1 )
      return CONTROLLER_ERROR_BAD_DATA;
  
    (*propertySet)( propertyIndex, value );
  }
  else
  {
    // No data, then.  I guess it was a read.  The XXXXOsc getters
    // take the channel number and use it to call
    // Osc_CreateMessage() which adds a new message to the outgoing
    // stack
    int value = (*propertyGet)( propertyIndex );
    char address[ OSC_SCRATCH_SIZE ]; 
    snprintf( address, OSC_SCRATCH_SIZE, "/%s/%s", subsystemName, propertyNames[ propertyIndex ] ); 
    Osc_CreateMessage( channel, address, ",i", value );
  }

  return CONTROLLER_OK;
}

/* Osc_GeneralReceiverHelper
 *
 * The General Helper passes the whole type/data string to the getters
 * and a channel to the setters so they can do their own message creation
 *
 * This is provided to for subsystems that have heterogenious data types
 *
 * This helper is called by the subsystem when an OSC message has arrived. 
 *
 * What is passed is the part after the first subsystem address.
 * So "/dipswitch/active 1" would be passed to this routine as "active 1"
 *
 * In order to do anything, we need some extra information from 
 * the subsystem like what the names of its properties are, what 
 * the getters and setters are, etc. so these are passed in by the
 * subsystem
 *
 */
int Osc_GeneralReceiverHelper( int channel, char* message, int length, 
                           char* subsystemName, 
                           int (*propertySet)( int property, char* typedata, int channel ),
                           int (*propertyGet)( int property, int channel ),
                           char* propertyNames[] )
{
  if ( message == NULL )
    return CONTROLLER_ERROR_NO_PROPERTY;

  int propertyIndex = Osc_PropertyLookup( propertyNames, message );
  if ( propertyIndex == -1 )
    return CONTROLLER_ERROR_UNKNOWN_PROPERTY;

/*
  int bits;
  int number = Osc_NumberMatch( indexCount, message, &bits );
  if ( number == -1 && bits == -1 )
    return Osc_SubsystemError( channel, subsystemName, "Bad index" );
*/
  
  // Sometime after the address, the data tag begins - this is the description 
  // of the data in the rest of the message.  It starts with a comma.  Return
  // where it is into 'type'.  If there is no comma, this is bad.
  char* type = Osc_FindDataTag( message, length );
  if ( type == NULL )
    return Osc_SubsystemError( channel, subsystemName, "No type tag" );

  // Debug( 1, "Osc General Type[1] = %d", type[ 1 ] );

  // We can tell if there's data by seeing if the character after the comma
  // is a zero or not.
  if ( type[ 1 ] != 0 )
  {
    // If there was data, it was a WRITE.
    int status;
    status = (*propertySet)( propertyIndex, type, channel );
    if ( status != CONTROLLER_OK )
      return CONTROLLER_ERROR_BAD_DATA;
  }
  else
  {
    // No data, then.  I guess it was a read.  The XXXXOsc getters
    // take the channel number and use it to call
    // Osc_CreateMessage() which adds a new message to the outgoing
    // stack

    (*propertyGet)( propertyIndex, channel );
  }

  return CONTROLLER_OK;
}

// An OSC message has arrived, check it out... although, don't change it - others need
// to use it.  If the original OSC packet was a BUNDLE - i.e. had many 
// messages in it, they will be unpacked elsewhere.  We'll only get 
// individual messages here.
// What is passed is the part after the first subsystem address.
// So "/appled/0/on 1" would be passed to this routine as "0/on 1"
// Not literally like this, of course, but as OSC messages
// In order to do anything, we need some extra information from 
// the subsystem like what the names of its properties are, what 
// the getters and setters are, etc.
// It's great to have this code in one central place.  It will be easier 
// to effect change from here.o
int Osc_IndexIntReceiverHelper( int channel, char* message, int length, 
                                int indexCount, char* subsystemName, 
                                int (*propertySet)( int index, int property, int value ),
                                int (*propertyGet)( int index, int property ),
                                char* propertyNames[] )
{
  // Look for the next slash - being the one that separates the index
  // from the property.  Note that this won't go off on a search through the buffer
  // since there will soon be a string terminator (i.e. a 0)
  char* prop = strchr( message, '/' );
  if ( prop == NULL )
    return CONTROLLER_ERROR_BAD_FORMAT;

  // Now that we know where the property is, we can see if we can find it.
  // This is a little cheap, since we're also implying that there are no 
  // more address terms after the property.  That is, if testing for "speed", while
  // "speed" would match, "speed/other_stuff" would not.
  int propertyIndex = Osc_PropertyLookup( propertyNames, prop + 1 );
  if ( propertyIndex == -1 )
    return CONTROLLER_ERROR_UNKNOWN_PROPERTY;

  // Here's where we try to understand what index we got.  In the world of 
  // OSC, this could be a pattern.  So while we could get "0/speed" we could 
  // also get "*/speed" or "[0-4]/speed".  This is kind of a drag, but it is 
  // quite nice from the user's perspective.
  // So to deal with this take a look at the text "0" or "{1,2}" or whatever
  // and produce either a nice integer in the simplest case or a set of bits 
  // where each bit corresponds to one of the indicies.  Clearly we don't have
  // to go crazy, since there are only a small finite number of them.
  // Osc_NumberMatch() does the work for us, producing either number = -1 and 
  // bits == -1 if there was no index match, or number != -1 for there was a single
  // number, or bits != -1 if there were several.

  // note that we tweak the string a bit here to make sure the next '/' is not 
  // mixed up with this.  Insert a string terminator.
  *prop = 0;

  int bits;
  int number = Osc_NumberMatch( indexCount, message, &bits );
  if ( number == -1 && bits == -1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;
  
  // We tweaked the '/' before - now put it back
  *prop = '/';

  // Sometime after the address, the data tag begins - this is the description 
  // of the data in the rest of the message.  It starts with a comma.  Return
  // where it is into 'type'.  If there is no comma, this is bad.
  char* type = Osc_FindDataTag( message, length );
  if ( type == NULL )
    return CONTROLLER_ERROR_NO_TYPE_TAG;

  // We can tell if there's data by seeing if the character after the comma
  // is a zero or not.
  if ( type[ 1 ] == 'i' || type[ 1 ] == 'f' )
  {
    // If there was int or float data, it was a WRITE.
    // So, sort of scanff-like, go get the data.  Here we pass in where the data is
    // thanks to the previous routine and then specify what we expect to find there
    // in tag terms (i.e. "i", "s", "f" and others).  Finally we pass in a set 
    // of pointers to the data types we want to extract.  Osc_ExtractData()
    // will rummage around in the message magically grabbing values for you,
    // reporting how many it got.  It will convert ints and floats if necessary.
    int value;
    int count = Osc_ExtractData( type, "i", &value );
    if ( count != 1 )
      return CONTROLLER_ERROR_INCORRECT_DATA_TYPE;
  
    // Now with the data we need to decide what to do with it.
    // Is there one or many here?
    if ( number != -1 )
      (*propertySet)( number, propertyIndex, value );
    else
    {
      int index = 0;
      while ( bits > 0 && index < indexCount )
      { 
        if ( bits & 1 )
          (*propertySet)( index, propertyIndex, value );
        bits >>= 1;
        index++;
      }
    }
  }
  else
  {
    // No data, then.  I guess it was a read.  The XXXXOsc getters
    // take the channel number and use it to call
    // Osc_CreateMessage() which adds a new message to the outgoing
    // stack
    if ( number != -1 )
    {
      int value = (*propertyGet)( number, propertyIndex );
      char address[ OSC_SCRATCH_SIZE ]; 
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%d/%s", subsystemName, number, propertyNames[ propertyIndex ] ); 
      Osc_CreateMessage( channel, address, ",i", value );
    }
    else
    {
      int index = 0;
      while ( bits > 0 && index < indexCount )
      { 
        if ( bits & 1 )
        {
          int value = (*propertyGet)( index, propertyIndex );
          char address[ OSC_SCRATCH_SIZE ]; 
          snprintf( address, OSC_SCRATCH_SIZE, "/%s/%d/%s", subsystemName, index, propertyNames[ propertyIndex ] ); 
          Osc_CreateMessage( channel, address, ",i", value );
        }
        bits >>= 1;
        index++;
      }
    }
  }

  return CONTROLLER_OK;
}

int Osc_SendError( int channel, char* subsystemName, int error )
{
  char* errorText;
  switch ( error )
  {
    case CONTROLLER_ERROR_UNKNOWN_PROPERTY:
      errorText = "Unknown Property";
      break;
    case CONTROLLER_ERROR_NO_PROPERTY:
      errorText = "No Property";
      break;
    case CONTROLLER_ERROR_INCORRECT_DATA_TYPE:
      errorText = "Incorrect Data Type";
      break;
    case CONTROLLER_ERROR_ILLEGAL_INDEX:
      errorText = "Bad Index";
      break;
    case CONTROLLER_ERROR_BAD_FORMAT:
      errorText = "Bad Format";
      break;
    case CONTROLLER_ERROR_NO_TYPE_TAG:
      errorText = "No Type Tag";
      break;
    case CONTROLLER_ERROR_BAD_DATA:
      errorText = "Bad Data";
      break;
    default:
      errorText = "Error";
      break;
  }
  return Osc_SubsystemError( channel, subsystemName, errorText );
}

// OSC_ExtractData takes a buffer (i.e. a point in the incoming OSC message)
// And a format e.g. "i" "bb", etc. and unpacks them to the var args
// The var args need to be pointers to memory ready to receive the values.
// In the case of blobs, there need to be three parameters: char** buffer, 
// and int* size on the param list.  The buffer gets a pointer into the 
// right place in the incoming buffer and the size value gets assigned
int Osc_ExtractData( char* buffer, char* format, ... )
{
  // Set up to iterate through the arguments
  va_list args;
  va_start( args, format );
  int count = 0;

  // figure out where the data starts
  int tagLen = strlen( buffer ) + 1;
  int pad = tagLen % 4;
  if ( pad != 0 )
    tagLen += ( 4 - pad );
  char* data = buffer + tagLen;

  // Going to be walking the tag string, the format string and the data
  char* fp;
  char* tp = buffer + 1; // need to skip the comma ','
  bool cont = true;
  for ( fp = format; *fp && cont; fp++ )
  {
    cont = false;
    switch ( *fp )
    {
      case 'i':
        if ( *tp == 'i' )
        {
          *(va_arg( args, int* )) = Osc_ReadInt( data );
          data += 4;
          count++;
          cont = true;
        }
        if ( *tp == 'f' )
        {
          *(va_arg( args, int* )) = (int)Osc_ReadFloat( data );
          data += 4;
          count++;
          cont = true;
        }
        
        break;
      case 'f':
        if ( *tp == 'f' )
        {
          *(va_arg( args, float* )) = Osc_ReadFloat( data );
          data += 4;
          count++;
          cont = true;
        }
        if ( *tp == 'i' )
        {
          *(va_arg( args, float* )) = (float)Osc_ReadInt( data );
          data += 4;
          count++;
          cont = true;
        }
        break;
      case 's':
        if ( *tp == 's' )
        {
          *(va_arg( args, char** )) = data;
          int len = strlen( data ) + 1;
          int pad = len % 4;
          if ( pad != 0 )
            len += ( 4 - pad );
          data += len;
          count++;
          cont = true;
        }
        break;
      case 'b':
        if ( *tp == 'b' )
        {
          int length = Osc_ReadInt( data );
          data += 4;
          *(va_arg( args, char** )) = data;
          *(va_arg( args, int* )) = length;
          int pad = length % 4;
          if ( pad != 0 )
            length += ( 4 - pad );
          data += length;
          count++;
          cont = true;
        }
        break;
    }
    tp++;
  }

  //va_end( args );

  return count;
}

int Osc_ReadInt( char* buffer )
{
  int v = *((int*)buffer);
  v = Osc_EndianSwap( v );
  return v;
}

float Osc_ReadFloat( char* buffer )
{
  int v = *((int*)buffer);
  v = Osc_EndianSwap( v );
  return  *(float*)&v;
}

/**
  Osc_CreateMessageNoLock
  Must put the "," as the first format letter
  */
int Osc_CreateMessage( int channel, char* address, char* format, ... )
{
  if ( address == NULL || format == NULL || *format != ',' )
    return CONTROLLER_ERROR_BAD_DATA;

  if ( channel < 0 || channel >= OSC_CHANNEL_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  OscChannel* ch = Osc->channel[ channel ];

  if ( !ch->running )
    return CONTROLLER_ERROR_SUBSYSTEM_INACTIVE;

  if ( channel == OSC_CHANNEL_UDP && ch->replyAddress == 0 )
    return CONTROLLER_ERROR_NO_ADDRESS;

  // Check for sender
  if ( ch->sendMessage == NULL )
    return CONTROLLER_ERROR_RESOURCE_MISSING;

  if ( Osc_Lock( ch ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_LOCK_ERROR;

  if ( ch->bufferPointer == NULL )
    Osc_ResetChannel( ch );

  // try to send this message - if there's a problem somewhere, 
  // send the existing buffer - freeing up space, then try (once) again.
  int count = 0;
  char *bp;
  do
  {  
    count++;

    char* buffer = ch->bufferPointer;
    int length = ch->bufferRemaining;
  
    bp = buffer;
  
    // First message in the buffer?
    if ( bp == ch->buffer )
    {
      bp = Osc_CreateBundle( bp, &length, 0, 0 );
      if ( bp == NULL )
        return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;
    }
  
    // Make room for the new message
    int* lp = (int *)bp;
    bp += 4;
    length -= 4;

    // remember the start of the message
    char* mp = bp;    

    if ( length > 0 )
    {      
      // Set up to iterate through the arguments
      va_list args;
      va_start( args, format );
    
      bp = Osc_CreateMessageInternal( bp, &length, address, format, args ); 

      //va_end( args );
    }
    else
      bp = 0;
      
    if ( bp != 0 )
    {
      // Set the size
      *lp = Osc_EndianSwap( bp - mp ); 
  
      ch->bufferPointer = bp;
      ch->bufferRemaining = length;
      ch->messages++;
    }
    else
    {
      Osc_SendPacketInternal( ch );
    }
  } while ( bp == 0 && count == 1 );

  Osc_Unlock( ch );

  return ( bp != 0 ) ? CONTROLLER_OK : CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;
}

int Osc_CreateMessageToBuf( char* bp, int* length, char* address, char* format, ... )
{
  if ( address == NULL || format == NULL || *format != ',' )
    return CONTROLLER_ERROR_BAD_DATA;

  va_list args;
  va_start( args, format );
    
  Osc_CreateMessageInternal( bp, length, address, format, args );
  return CONTROLLER_OK;
}

char* Osc_CreateMessageInternal( char* bp, int* length, char* address, char* format, va_list args )
{
  // do the address
  bp = Osc_WritePaddedString( bp, length, address );
  if ( bp == NULL )
    return 0;

  // do the type
  bp = Osc_WritePaddedString( bp, length, format );
  if ( bp == NULL )
    return 0;

  // Going to be walking the tag string, the format string and the data
  // skip the ',' comma
  char* fp;
  bool cont = true;
  for ( fp = format + 1; *fp && cont; fp++ )
  {
    switch ( *fp )
    {
      case 'i':
          *length -= 4;
          if ( *length >= 0 )
          {
            int v = va_arg( args, int );
            v = Osc_EndianSwap( v );
            *((int*)bp) = v;
            bp += 4;
          }
          else 
            cont = false;
        break;
      case 'f':
        *length -= 4;
        if ( *length >= 0 )
        {
          int v;
          *((float*)&v) = (float)( va_arg( args, double ) ); 
          v = Osc_EndianSwap( v );
          *((int*)bp) = v;
          bp += 4;
        }
        else 
          cont = false;
        break;
      case 's':
      {
        char* s = va_arg( args, char* );
        bp = Osc_WritePaddedString( bp, length, s );
        if ( bp == NULL )
          cont = false;
        break;
      }
      case 'b':
      {
        char* b = va_arg( args, char* );
        int blen = va_arg( args, int );
        bp = Osc_WritePaddedBlob( bp, length, b, blen  );
        if ( bp == NULL )
          cont = false;
        break;
      }
      default:
        cont = false;
    }
  }

  return ( cont ) ? bp : NULL;
}

char* Osc_CreateBundle( char* buffer, int* length, int a, int b )
{
  char *bp = buffer;

  // do the bundle bit
  bp = Osc_WritePaddedString( bp, length, "#bundle" );
  if ( bp == NULL )
    return 0;

  // do the timetag
  bp = Osc_WriteTimetag( bp, length, a, b );
  if ( bp == NULL )
    return 0;

  return bp;
}

int Osc_Lock( OscChannel* ch )
{
  // Check for semphore lock - Critical to avoid multiple creates
  TaskEnterCritical( );
  if ( !ch->semaphore )
      vSemaphoreCreateBinary( ch->semaphore );
  TaskExitCritical( );

  // Lock up this program segment to prevent multiple use
  if ( !xSemaphoreTake( ch->semaphore, 1000 ) )
    return CONTROLLER_ERROR_LOCK_ERROR;
  
  return CONTROLLER_OK;
}

void Osc_Unlock( OscChannel *ch )
{
  xSemaphoreGive( ch->semaphore );
}

int Osc_NumberMatch( int count, char* message, int* bits )
{
  int n = 0;
  int digits = 0;
  while ( isdigit( *message ) )
  {
    digits++;
    n = n * 10 + ( *message++ - '0' );
  }

  *bits = -1;
  if ( n >= count )
    return -1;

  switch ( *message )
  {
    case '*':
    case '?':
    case '[':
    case '{':
    {
      int i;
      int b = 0;
      char s[ 5 ];
      for ( i = count - 1; i >=0 ; i-- )
      {
        b <<= 1;
        sprintf( s, "%d", i );
        if ( Osc_PatternMatch( message, s ) )
          b |= 1;
      }
      *bits = b;
      return -1;
    }
    default:
      if ( digits == 0 )
        return -1;
      return n;
  }
}

// Looks the named property up, returning an index
// Note that we need to be careful - there may be other stuff there in the string
// Probably best to eventually do something better with it.
int Osc_PropertyLookup( char** properties, char* property )
{
  char** p = properties;
  int index = 0;
  while (*p != NULL )
  {
    if ( strcmp( property, *p++ ) == 0 )
      return index;
    index++;
  }
  return -1;
}

char *Osc_FindDataTag( char* message, int length )
{
  while ( *message != ',' && length-- > 0 )
    message++;
  if ( length <= 0 )
    return NULL;
  else
    return message;
}

char* Osc_WritePaddedString( char* buffer, int* length, char* string )
{
  int tagLen = strlen( string ) + 1;
  int tagPadLen = tagLen;
  int pad = ( tagPadLen ) % 4;
  if ( pad != 0 )
    tagPadLen += ( 4 - pad );
 
  *length -= tagPadLen;

  if ( *length >= 0 )
  {
    strcpy( buffer, string );
    int i;
    buffer += tagLen;
    for ( i = tagLen; i < tagPadLen; i++ ) 
      *buffer++ = 0;
  }
  else
    return NULL;

  return buffer;
}

char* Osc_WritePaddedBlob( char* buffer, int* length, char* blob, int blen )
{
  int i;
  int padLength = blen;
  int pad = ( padLength ) % 4;
  if ( pad != 0 )
    padLength += ( 4 - pad );
 
  if ( *length < ( padLength + 4 ) )
    return 0;

  // add the length of the blob
  int l = Osc_EndianSwap( blen );
  *((int*)buffer) = l;
  buffer += 4;
  *length -= 4;

  memcpy( buffer, blob, blen );
  buffer += blen;
  // reduce the remaining buffer size
  *length -= padLength;

  for ( i = blen; i < padLength; i++ ) 
      *buffer++ = 0;

  return buffer;
}

char* Osc_WriteTimetag( char* buffer, int* length, int a, int b )
{
  if ( *length < 8 )
    return NULL;

  *((int*)buffer) = Osc_EndianSwap( a );
  buffer += 4;
  *((int*)buffer) = Osc_EndianSwap( b );
  buffer += 4;
  *length -= 8;

  return buffer;
}

int Osc_EndianSwap( int a )
{
  return ( ( a & 0x000000FF ) << 24 ) |
         ( ( a & 0x0000FF00 ) << 8 )  | 
         ( ( a & 0x00FF0000 ) >> 8 )  | 
         ( ( a & 0xFF000000 ) >> 24 );

}

