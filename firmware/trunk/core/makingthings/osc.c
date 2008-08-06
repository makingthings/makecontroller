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

/** @defgroup OSC OSC
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
#ifdef OSC

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
//#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#define OSC_MAX_MESSAGE_IN   200
#define OSC_MAX_MESSAGE_OUT  600

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
  int (*async)( int channel );
}OscSubsystem;

typedef struct Osc_
{
  int users;
  int running;
  int subsystemHighest;
  void* sendSocket;
  struct netconn* tcpSocket;
  void* UsbTaskPtr;
  void* UdpTaskPtr;
  void* TcpTaskPtr;
  void* AsyncTaskPtr;
  OscChannel* channel[ OSC_CHANNEL_COUNT ];
  OscSubsystem* subsystem[ OSC_SUBSYSTEM_COUNT ];
  int registeredSubsystems;
  char scratch1[ OSC_SCRATCH_SIZE ], scratch2[ OSC_SCRATCH_SIZE ];
  xSemaphoreHandle scratch1Semaphore, scratch2Semaphore;
}OscStruct;

OscStruct* Osc;

int Osc_Lock( OscChannel* ch );
void Osc_Unlock( OscChannel *ch );
int Osc_LockScratchBuf( xSemaphoreHandle scratchSemaphore );
int Osc_UnlockScratchBuf( xSemaphoreHandle scratchSemaphore );

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

void Osc_UdpTask( void* parameters );
void Osc_UsbTask( void* parameters );
void Osc_TcpTask( void* parameters );
void Osc_AsyncTask( void* p );
int Osc_UdpPacketSend( char* packet, int length, int replyAddress, int replyPort );
int Osc_UsbPacketSend( char* packet, int length, int replyAddress, int replyPort );
int Osc_TcpPacketSend( char* packet, int length, int replyAddress, int replyPort );

void Osc_ResetChannel( OscChannel* ch );
int Osc_SendPacketInternal( OscChannel* ch );

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

int OscBusy;


/**
	Osc_SetActive.
	Sets whether the Osc system active. \n
	@param state
  @param udptask Whether or not to start up the internal UDP-OSC task (only relevant on startup).
  @param usbtask Whether or not to start up the internal USB-OSC task (only relevant on startup).
  @param async Whether or not to start up the internal async task that monitors registered subsystems
  and automatically sends out messages from them (only relevant on startup).

  \b Example
  \code
  // Fire up the OSC system with everything running (as in heavy)
  Osc_SetActive( true, true, true, true );
  \endcode
*/
void Osc_SetActive( int state, bool udptask, bool usbtask, bool async )
{
  if ( state && Osc == NULL )
  {
		Osc = MallocWait( sizeof( OscStruct ), 100 );
    Osc->subsystemHighest = 0;
    Osc->registeredSubsystems = 0;
    Osc->sendSocket = NULL;
    int i;
    for( i = 0; i < OSC_CHANNEL_COUNT; i++ )
      Osc->channel[ i ] = NULL;
    for( i = 0; i < OSC_SUBSYSTEM_COUNT; i++ )
      Osc->subsystem[ i ] = NULL;

    if(udptask)
    {
      #ifdef MAKE_CTRL_NETWORK
        #ifdef CROSSWORKS_BUILD
        Osc->UdpTaskPtr = TaskCreate( Osc_UdpTask, "OSC-UDP", 1200, (void*)OSC_CHANNEL_UDP, 3 );
        #else // GnuArm, WinArm, YAGARTO, etc.
        Osc->UdpTaskPtr = TaskCreate( Osc_UdpTask, "OSC-UDP", 2500, (void*)OSC_CHANNEL_UDP, 3 );
        #endif // CROSSWORKS_BUILD
      Osc->TcpTaskPtr = NULL;
      #endif // MAKE_CTRL_NETWORK
    }
    
    if(usbtask)
    {
      #ifdef MAKE_CTRL_USB
        #ifdef CROSSWORKS_BUILD
        Osc->UsbTaskPtr = TaskCreate( Osc_UsbTask, "OSC-USB", 1000, (void*)OSC_CHANNEL_USB, 3 );
        #else
        Osc->UsbTaskPtr = TaskCreate( Osc_UsbTask, "OSC-USB", 1300, (void*)OSC_CHANNEL_USB, 3 );
        #endif // CROSSWORKS_BUILD
      #endif // MAKE_CTRL_USB
    }
    
    if(async)
    {
      #ifdef CROSSWORKS_BUILD
      Osc->AsyncTaskPtr = TaskCreate( Osc_AsyncTask, "OSC-ASYNC", 600, 0, 2 );
      #else
      Osc->AsyncTaskPtr = TaskCreate( Osc_AsyncTask, "OSC-ASYNC", 1100, 0, 2 );
      #endif // CROSSWORKS_BUILD
    }

    vSemaphoreCreateBinary( Osc->scratch1Semaphore );
    vSemaphoreCreateBinary( Osc->scratch2Semaphore );

    Osc->users = 1;
    Osc->running = true;
  }
  if( !state && Osc )
  {
    #ifdef MAKE_CTRL_USB
    TaskDelete( Osc->UsbTaskPtr );
    #endif
    #ifdef MAKE_CTRL_NETWORK
    TaskDelete( Osc->UdpTaskPtr );
    #endif
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

int Osc_GetRunning( )
{
  if( Osc )
    return Osc->running;
  else
    return 0;
}

#ifdef MAKE_CTRL_NETWORK
void Osc_UdpTask( void* parameters )
{
  int channel = (int)parameters;
  Osc->channel[ channel ] = MallocWait( sizeof( OscChannel ), 100 );
  OscChannel *ch = Osc->channel[ channel ];
  ch->running = false;
  Osc_SetReplyPort( channel, NetworkOsc_GetUdpSendPort() );
  ch->sendMessage = Osc_UdpPacketSend;
  Osc_ResetChannel( ch );

  // Chill until the Network is up
  while ( !Network_GetActive() )
    Sleep( 100 );

  void* ds = NULL;
  while( ds == NULL )
  {
  	ds = DatagramSocket( NetworkOsc_GetUdpListenPort() );
  	Sleep( 100 );
  }
  while( Osc->sendSocket == NULL )
  {
  	Osc->sendSocket = DatagramSocket( 0 );
  	Sleep( 100 );
  }
  
  ch->running = true;
  while ( true )
  {
    int address;
    int port;
    int length = DatagramSocketReceive( ds, NetworkOsc_GetUdpListenPort(), &address, &port, ch->incoming, OSC_MAX_MESSAGE_IN );
    if( length > 0 )
    {
      Osc_SetReplyAddress( channel, address );
      Osc_ReceivePacket( channel, ch->incoming, length );
    }
    TaskYield( );
  }
}

int Osc_UdpPacketSend( char* packet, int length, int replyAddress, int replyPort )
{
  if ( replyAddress != 0 && replyPort != 0 )
  {
    int retval = DatagramSocketSend( Osc->sendSocket, replyAddress, NetworkOsc_GetUdpSendPort(), packet, length );
    return retval;
  }
  else
    return CONTROLLER_ERROR_NO_ADDRESS;
}

void Osc_StartTcpTask( )
{
  Osc->TcpTaskPtr = TaskCreate( Osc_TcpTask, "OscTcp", 600, 0, 2 );
}

int Osc_TcpPacketSend( char* packet, int length, int replyAddress, int replyPort )
{
  (void)replyAddress;
  (void)replyPort;
  char len[ 4 ];
  //int endian = Osc_EndianSwap(length);
  sprintf( len, "%d", length );
      
  int result = SocketWrite( Osc->tcpSocket, len, 4 );
  result = SocketWrite( Osc->tcpSocket, packet, length );
  if( !result )
  {
    SocketClose( Osc->tcpSocket );
    Osc->tcpSocket = NULL;
  }
  return result;
}

void Osc_TcpTask( void* parameters )
{
  (void)parameters;
  Osc->channel[ OSC_CHANNEL_TCP ] = MallocWait( sizeof( OscChannel ), 100 );
  OscChannel *ch = Osc->channel[ OSC_CHANNEL_TCP ];
  ch->sendMessage = Osc_TcpPacketSend;
  Osc_ResetChannel( ch );

  // Chill until the Network is up
  while ( !Network_GetActive() )
    Sleep( 100 );

  int ledstate = 0;

  ch->running = true;
  ch->semaphore = NULL; // not sure why I need to do this here and not for the other channels...

  while( NetworkOsc_GetTcpRequested( ) )
  {
    // check to see if we have an open socket
    if( Osc->tcpSocket == NULL )
      Osc->tcpSocket = Socket( NetworkOsc_GetTcpOutAddress(), NetworkOsc_GetTcpOutPort() );

    if( Osc->tcpSocket != NULL )
    {
      int length = SocketRead( Osc->tcpSocket, ch->incoming, OSC_MAX_MESSAGE_IN );
      // should actually read the given length in the message, and use that.
      if( length > 0 )
      {
        Osc_ReceivePacket( OSC_CHANNEL_TCP, ch->incoming+4, length-4 );
        ledstate = !ledstate;
      }
        
      if( !length )
      {
        SocketClose( Osc->tcpSocket );
        Osc->tcpSocket = NULL;
      }
      TaskYield( );
    }
    else
      Sleep( 100 ); // Just so we don't bash the Socket( ) call constantly when we're not open
  } // while( )
  // now we shut down
  SocketClose( Osc->tcpSocket );
  Osc->tcpSocket = NULL;
  Osc_ResetChannel( ch );
  Free( Osc->channel[ OSC_CHANNEL_TCP ] );
  Osc->channel[ OSC_CHANNEL_TCP ] = NULL;
  TaskDelete( Osc->TcpTaskPtr );
}
#endif // MAKE_CTRL_NETWORK

#ifdef MAKE_CTRL_USB
void Osc_UsbTask( void* parameters )
{
  int channel = (int)parameters;
  Osc->channel[ channel ] = MallocWait( sizeof( OscChannel ), 100 );
  OscChannel *ch = Osc->channel[ channel ];
  ch->sendMessage = Osc_UsbPacketSend;
  Osc_ResetChannel( ch );

  // Chill until the USB connection is up
  while ( !Usb_GetActive() )
    Sleep( 100 );

  ch->running = true;

  while ( true )
  {
    int length = Usb_SlipReceive( ch->incoming, OSC_MAX_MESSAGE_IN );
    if ( length > 0 )
      Osc_ReceivePacket( channel, ch->incoming, length );
    Sleep( 1 );
  }
}

int Osc_UsbPacketSend( char* packet, int length, int replyAddress, int replyPort )
{
  (void)replyAddress;
  (void)replyPort;
  return Usb_SlipSend( packet, length );
}
#endif // MAKE_CTRL_USB

void Osc_AsyncTask( void* p )
{
  (void)p;
  int channel;
  int i;
  OscSubsystem* sub;
  int newMsgs = 0;
  while( 1 )
  {
    channel = System_GetAsyncDestination( );
    if( channel >= 0 )
    {
      for( i = 0; i < Osc->registeredSubsystems; i++ )
      {
        sub = Osc->subsystem[ i ];
        if( sub->async != NULL )
          newMsgs += (sub->async)( channel );   
      }
      if( newMsgs > 0 )
      {
        Osc_SendPacket( channel );
        newMsgs = 0;
      }
      Sleep( System_GetAutoSendInterval( ) );
    }
    else
      Sleep( 1000 );
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
  int status = -1;
  switch ( *packet )
  {
    case '/':
      status = Osc_ReceiveMessage( channel, packet, length );
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
    if( strlen(message) > (unsigned int)length )
      return CONTROLLER_ERROR_BAD_DATA;

    int i;
    char* nextChar = message + 1; // first, try to see if it was a "help" query
    if( *nextChar == '\0' || *nextChar == ' ' )
    {
      for ( i = 0; i < Osc->registeredSubsystems; i++ )
      {
        OscSubsystem* sub = Osc->subsystem[ i ];
        Osc_CreateMessage( channel, "/", ",s", sub->name );
      }
      return CONTROLLER_OK;
    }

    char* nextSlash = strchr( message + 1, '/' );
    if ( nextSlash != NULL )
      *nextSlash = 0;
    
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
        {
          char* noNextSlash = message + strlen(message);
          (sub->receiveMessage)( channel, noNextSlash, 0 );
        }
      }
    }
    if ( count == 0 )
    {
      Osc_LockScratchBuf( Osc->scratch1Semaphore );
      snprintf( Osc->scratch1, OSC_SCRATCH_SIZE, "No Subsystem Match - %s", message + 1 );
			Osc_CreateMessage( channel, "/error", ",s", Osc->scratch1 );
			Osc_UnlockScratchBuf( Osc->scratch1Semaphore );
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

/** @defgroup OSCAPI OSC API
	Make use of the OSC infrastructure for your own subsystems.
	You can use the existing OSC infrastructure to create your own OSC subsystems.  It's expected that you'll have
	a few things lined up to use this API:
	-# The name of your subsystem
	-# The properties that your subsystem will support.  This must be in an array with '0' as the last element.
	-# Getters and setters for each of those properties and functions to call them by property index.
	-# A function to call the correct helper when Osc calls you. 
	-# Finally, once you've done all this, you'll need to add your subsystem to the list that Osc knows about.
	
	\par Example
	So this might look something like:
	\code
	// our system name and a function to get it
	static char* MySubsystemOsc_Name = "my-system";
	const char* MySubsystemOsc_GetName( void )
	{
		return MySubsystemOsc_Name;
	}
	
	// our property names
	static char* MySubsystemOsc_PropertyNames[] = { "prop0", "prop1", 0 }; // must end with a zero
	
	// A getter and setter, each dealing with the property given to us by the OSC system
	int MyGPSOsc_PropertySet( int index, int property, int value )   
  {
    switch ( property )     
    {       
      case 0:         
        MySubsystem_SetProperty0( index, value );
        break;
      case 1:
        MySubsystem_SetProperty1( index, value );
        break;
     }
    return CONTROLLER_OK;
  }

  int MySubsystemOsc_PropertyGet( int index, int property )
  {
    int value;
     switch ( property )
     {
      case 0:
        value = MySubsystem_GetProperty0( index );
        break;
      case 1:
        value = MySubsystem_GetProperty1( index );
        break;
    }
    return value;
  }
	
	// this is called when the OSC system determines an incoming message is for you.
	int MySubsystemOsc_ReceiveMessage( int channel, char* message, int length )
	{  
     // depending on your subsystem, use one of the OSC helpers to parse the incoming message
		 return Osc_IndexGeneralReceiverHelper( channel, message, length,
                                        MySubsystemOsc_Name,
                                        MySubsystemOsc_PropertySet,
                                        MySubsystemOsc_PropertyGet,
                                        MySubsystemOsc_PropertyNames );
	}
	 
	// lastly, we'll need to register our system with OSC
	Run( ) // this is our startup task in make.c
	{
		// other startup stuff
		Osc_RegisterSubsystem( MySubsystemOsc_GetName(), MySubsystemOsc_ReceiveMessage, NULL );
	}
	\endcode
	
	Check the how-to at http://www.makingthings.com/documentation/how-to/create-your-own-osc-subsystem for details.
	
	\ingroup Core	
*/

/**	
	Register your subsystem with the OSC system.
	You'll usually want to do this on startup.
	@param name The name of your subsystem.
	@param subsystem_ReceiveMessage The function to call when an OSC message for this subsystem has arrived.
	@param subsystem_Async The function to be called by the OSC Async system.  This is a task that will call you at regular intervals,
	if enabled, so you can check for status changes and send a message out automatically if you like.  See the analog in source for 
	an example.  Pass in NULL if you don't want to use the Async system.
	\ingroup OSCAPI

  \par Example
  \code
	Run( ) // this is our startup task in make.c
	{
		// other startup stuff
		Osc_RegisterSubsystem( MySubsystemOsc_GetName(), MySubsystemOsc_ReceiveMessage, NULL );
	}
  \endcode
*/
int Osc_RegisterSubsystem( const char *name, int (*subsystem_ReceiveMessage)( int channel, char* buffer, int length ), int (*subsystem_Async)( int channel ) )
{
  int subsystem = Osc->registeredSubsystems;
  if ( Osc->registeredSubsystems++ > OSC_SUBSYSTEM_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX; 

  Osc->subsystem[ subsystem ] = MallocWait( sizeof( OscSubsystem ), 100 );
  OscSubsystem* sub = Osc->subsystem[ subsystem ];
  sub->name = name;
  sub->receiveMessage = subsystem_ReceiveMessage;
  sub->async = subsystem_Async;
  return CONTROLLER_OK;
}

/**	
	Send an error back via OSC from your subsystem.
	You'll usually want to call this when the OSC system calls you with a new message, you've tried to
	parse it, and you've gotten an error.
	@param channel An index for which OSC channel is being used (usually USB or Ethernet).  Usually provided for you 
	by the OSC system.
	@param subsystem The name of the subsystem sending the error.
	@param string The actual contents of the error message.
	\ingroup OSCAPI

  \par Example
  \code
	// this is where OSC calls us when an incoming message for us has arrived
	int MySubsystemOsc_ReceiveMessage( int channel, char* message, int length )
	{
		int status = Osc_IntReceiverHelper( channel, message, length,
																				MySubsystemOsc_Name,
																				MySubsystemOsc_PropertySet, MySubsystemOsc_PropertyGet, 
																				MySubsystemOsc_PropertyNames );

		if ( status != CONTROLLER_OK )
			Osc_SubsystemError( channel, MySubsystemOsc_Name, "Oh no. Bad data." );
	}
  \endcode
*/
int Osc_SubsystemError( int channel, char* subsystem, char* string )
{
  int retval;
  Osc_LockScratchBuf( Osc->scratch1Semaphore );
  snprintf( Osc->scratch1, OSC_SCRATCH_SIZE, "/%s/error", subsystem ); 
  retval = Osc_CreateMessage( channel, Osc->scratch1, ",s", string );
  Osc_UnlockScratchBuf( Osc->scratch1Semaphore );
  return retval;
}


/**	
	Receive an OSC blob for a subsystem with no indexes.
	You'll usually want to call this when the OSC system calls you with a new message. 
	@param channel An index for which OSC channel is being used (usually USB or Ethernet).  Usually provided for you 
	by the OSC system.
	@param message The OSC message being received.  Usually provided for you by the OSC system.
	@param length The length of the incoming message.  Usually provided for you by the OSC system.
	@param subsystemName The name of your subsystem.
	@param blobPropertySet A pointer to the function to be called in order to write a property of your subsystem.
	@param blobPropertyGet A pointer to the function to be called in order to read a property of your subsystem.
	@param blobPropertyNames An array of all the property names in your subsystem.
	\ingroup OSCAPI

  \par Example
  \code
	// this is where OSC calls us when an incoming message for us has arrived
	int MySubsystemOsc_ReceiveMessage( int channel, char* message, int length )
	{
		int status = Osc_BlobReceiverHelper( channel, message, length, 
																			MySubsystemOsc_Name,
																			MySubsystemOsc_BlobPropertySet, MySubsystemOsc_BlobPropertyGet, 
																			MySubsystemOsc_BlobPropertyNames );                        

		if ( status != CONTROLLER_OK )
			return Osc_SendError( channel, MySubsystemOsc_Name, status );
		return CONTROLLER_OK;
	}
  \endcode
*/
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
    Osc_LockScratchBuf( Osc->scratch1Semaphore );
    Osc_LockScratchBuf( Osc->scratch2Semaphore );
    int size = (*blobPropertyGet)( propertyIndex, (uchar*)Osc->scratch1, OSC_SCRATCH_SIZE );
    snprintf( Osc->scratch2, OSC_SCRATCH_SIZE, "/%s/%s", subsystemName, blobPropertyNames[ propertyIndex ] ); 
    Osc_CreateMessage( channel, Osc->scratch2, ",b", Osc->scratch1, size );
    Osc_UnlockScratchBuf( Osc->scratch1Semaphore );
    Osc_UnlockScratchBuf( Osc->scratch2Semaphore );
  }

  return CONTROLLER_OK;
}

/**	
	Receive an OSC blob for a subsystem with indexes.
	You'll usually want to call this when the OSC system calls you with a new message. 
	@param channel An index for which OSC channel is being used (usually USB or Ethernet).  Usually provided for you 
	by the OSC system.
	@param message The OSC message being received.  Usually provided for you by the OSC system.
	@param length The length of the incoming message.  Usually provided for you by the OSC system.
	@param indexCount The number of indexes in your subsystem.
	@param subsystemName The name of your subsystem.
	@param blobPropertySet A pointer to the function to be called in order to write a property of your subsystem.
	@param blobPropertyGet A pointer to the function to be called in order to read a property of your subsystem.
	@param blobPropertyNames An array of all the property names in your subsystem.
	\ingroup OSCAPI

  \par Example
  \code
	// this is where OSC calls us when an incoming message for us has arrived
	int MySubsystemOsc_ReceiveMessage( int channel, char* message, int length )
	{
		int status = Osc_IndexBlobReceiverHelper( channel, message, length, 2,
																				MySubsystemOsc_Name,
																				MySubsystemOsc_BlobPropertySet, MySubsystemOsc_BlobPropertyGet, 
																				MySubsystemOsc_BlobPropertyNames );

		if ( status != CONTROLLER_OK )
			return Osc_SendError( channel, MySubsystemOsc_Name, status );
		return CONTROLLER_OK;
	}
  \endcode
*/
int Osc_IndexBlobReceiverHelper( int channel, char* message, int length, 
                                int indexCount, char* subsystemName, 
                                int (*blobPropertySet)( int index, int property, uchar* buffer, int length ),
                                int (*blobPropertyGet)( int index, int property, uchar* buffer, int length ),
                                char* blobPropertyNames[] )
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
  int propertyIndex = Osc_PropertyLookup( blobPropertyNames, prop + 1 );
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
  if ( type[ 1 ] == 'b' || type[ 1 ] == 's' )
  {
    // If there was blob or string data, it was a WRITE.
    // So, sort of scanf-like, go get the data.  Here we pass in where the data is
    // thanks to the previous routine and then specify what we expect to find there
    // in tag terms (i.e. "b", "s").  Finally we pass in a set 
    // of pointers to the data types we want to extract.  Osc_ExtractData()
    // will rummage around in the message magically grabbing values for you,
    // reporting how many it got.  It will grab convert strings if necessary.
    unsigned char *buffer;
    int size;
    int count = Osc_ExtractData( type, "b", &buffer, &size );
    if ( count != 1 )
      return CONTROLLER_ERROR_INCORRECT_DATA_TYPE;
  
    // Now with the data we need to decide what to do with it.
    // Is there one or many here?
    if ( number != -1 )
      (*blobPropertySet)( number, propertyIndex, buffer, size );
    else
    {
      int index = 0;
      while ( bits > 0 && index < indexCount )
      { 
        if ( bits & 1 )
          (*blobPropertySet)( index, propertyIndex, buffer, size  );
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
      Osc_LockScratchBuf( Osc->scratch1Semaphore );
      Osc_LockScratchBuf( Osc->scratch2Semaphore );
      int size = (*blobPropertyGet)( number, propertyIndex, (uchar*)Osc->scratch1, OSC_SCRATCH_SIZE );
      snprintf( Osc->scratch2, OSC_SCRATCH_SIZE, "/%s/%d/%s", subsystemName, number, blobPropertyNames[ propertyIndex ] ); 
      Osc_CreateMessage( channel, Osc->scratch2, ",b", Osc->scratch1, size );
      Osc_UnlockScratchBuf( Osc->scratch1Semaphore );
      Osc_UnlockScratchBuf( Osc->scratch2Semaphore );
    }
    else
    {
      int index = 0;
      while ( bits > 0 && index < indexCount )
      { 
        if ( bits & 1 )
        {
          Osc_LockScratchBuf( Osc->scratch1Semaphore );
          Osc_LockScratchBuf( Osc->scratch2Semaphore );
          int size = (*blobPropertyGet)( index, propertyIndex, (uchar*)Osc->scratch1, OSC_SCRATCH_SIZE );
          snprintf( Osc->scratch2, OSC_SCRATCH_SIZE, "/%s/%d/%s", subsystemName, index, blobPropertyNames[ propertyIndex ] ); 
          Osc_CreateMessage( channel, Osc->scratch2, ",b", Osc->scratch1, size );
          Osc_UnlockScratchBuf( Osc->scratch1Semaphore );
          Osc_UnlockScratchBuf( Osc->scratch2Semaphore );
        }
        bits >>= 1;
        index++;
      }
    }
  }

  return CONTROLLER_OK;
}

/**	
	Receive an integer for a subsystem with no indexes.
	You'll usually want to call this when the OSC system calls you with a new message. 
	@param channel An index for which OSC channel is being used (usually USB or Ethernet).  Usually provided for you 
	by the OSC system.
	@param message The OSC message being received.  Usually provided for you by the OSC system.
	@param length The length of the incoming message.  Usually provided for you by the OSC system.
	@param subsystemName The name of your subsystem.
	@param propertySet A pointer to the function to be called in order to write a property of your subsystem.
	@param propertyGet A pointer to the function to be called in order to read a property of your subsystem.
	@param propertyNames An array of all the property names in your subsystem.
	\ingroup OSCAPI

  \par Example
  \code
	// this is where OSC calls us when an incoming message for us has arrived
	int MySubsystemOsc_ReceiveMessage( int channel, char* message, int length )
	{
		int status = Osc_IntReceiverHelper( channel, message, length,
																				MySubsystemOsc_Name,
																				MySubsystemOsc_PropertySet, MySubsystemOsc_PropertyGet, 
																				MySubsystemOsc_PropertyNames );

		if ( status != CONTROLLER_OK )
			return Osc_SendError( channel, MySubsystemOsc_Name, status );
		return CONTROLLER_OK;
	}
  \endcode
*/
int Osc_IntReceiverHelper( int channel, char* message, int length, 
                           char* subsystemName, 
                           int (*propertySet)( int property, int value ),
                           int (*propertyGet)( int property ),
                           char* propertyNames[] )
{
  if( *message == '\0' || *message == ' ' ) // first, try to see if it was a property "help" query
  {
    int i = 0;
    while( true )
    {
      if( propertyNames[i] != 0 )
      {
        Osc_LockScratchBuf( Osc->scratch1Semaphore );
        snprintf( Osc->scratch1, OSC_SCRATCH_SIZE, "/%s", subsystemName ); 
        Osc_CreateMessage( channel, Osc->scratch1, ",s", propertyNames[i] );
        Osc_UnlockScratchBuf( Osc->scratch1Semaphore );
        i++;
      }
      else
        return CONTROLLER_OK;
    }
  }

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
    Osc_LockScratchBuf( Osc->scratch1Semaphore );
    snprintf( Osc->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", subsystemName, propertyNames[ propertyIndex ] ); 
    Osc_CreateMessage( channel, Osc->scratch1, ",i", value );
    Osc_UnlockScratchBuf( Osc->scratch1Semaphore );
  }

  return CONTROLLER_OK;
}

/**	
	Receive data for a subsystem that receives a variety of different data types.
	An example of this kind of situation is the network system - you have a variety of different properties, 
	several of which are both ints and strings.
	
	You'll usually want to call this when the OSC system calls you with a new message. 
	@param channel An index for which OSC channel is being used (usually USB or Ethernet).  Usually provided for you 
	by the OSC system.
	@param message The OSC message being received.  Usually provided for you by the OSC system.
	@param length The length of the incoming message.  Usually provided for you by the OSC system.
	@param subsystemName The name of your subsystem.
	@param propertySet A pointer to the function to be called in order to write a property of your subsystem.
	@param propertyGet A pointer to the function to be called in order to read a property of your subsystem.
	@param propertyNames An array of all the property names in your subsystem.
	\ingroup OSCAPI

  \par Example
  \code
	// this is where OSC calls us when an incoming message for us has arrived
	int MySubsystemOsc_ReceiveMessage( int channel, char* message, int length )
	{
		int status = Osc_GeneralReceiverHelper( channel, message, length,
																				MySubsystemOsc_Name,
																				MySubsystemOsc_PropertySet, MySubsystemOsc_PropertyGet, 
																				MySubsystemOsc_PropertyNames );

		if ( status != CONTROLLER_OK )
			return Osc_SendError( channel, MySubsystemOsc_Name, status );
		return CONTROLLER_OK;
	}
  \endcode
*/
int Osc_GeneralReceiverHelper( int channel, char* message, int length, 
                           char* subsystemName, 
                           int (*propertySet)( int property, char* typedata, int channel ),
                           int (*propertyGet)( int property, int channel ),
                           char* propertyNames[] )
{
  if ( message == NULL )
    return CONTROLLER_ERROR_NO_PROPERTY;

  if( *message == '\0' || *message == ' ' ) // first, try to see if it was a property "help" query
  {
    int i = 0;
    while( true )
    {
      if( propertyNames[i] != 0 )
      {
        Osc_LockScratchBuf( Osc->scratch1Semaphore );
        snprintf( Osc->scratch1, OSC_SCRATCH_SIZE, "/%s", subsystemName ); 
        Osc_CreateMessage( channel, Osc->scratch1, ",s", propertyNames[i] );
        Osc_UnlockScratchBuf( Osc->scratch1Semaphore );
        i++;
      }
      else
        return CONTROLLER_OK;
    }
  }

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

/**	
	Receive integers for a subsystem with multiple indexes.
	An example of this kind of situation is the analog in system - you have 8 channels (indexes) and you're only 
	going to be receiving integers.  
	
	You'll usually want to call this when the OSC system calls you with a new message. 
	@param channel An index for which OSC channel is being used (usually USB or Ethernet).  Usually provided for you 
	by the OSC system.
	@param message The OSC message being received.  Usually provided for you by the OSC system.
	@param length The length of the incoming message.  Usually provided for you by the OSC system.
	@param indexCount The number of indexes in your subsystem.
	@param subsystemName The name of your subsystem.
	@param propertySet A pointer to the function to be called in order to write a property of your subsystem.
	@param propertyGet A pointer to the function to be called in order to read a property of your subsystem.
	@param propertyNames An array of all the property names in your subsystem.
	\ingroup OSCAPI

  \par Example
  \code
	// this is where OSC calls us when an incoming message for us has arrived
	int MySubsystemOsc_ReceiveMessage( int channel, char* message, int length )
	{
		int status = Osc_GeneralReceiverHelper( channel, message, length,
																				5, // our index count
																				MySubsystemOsc_Name,
																				MySubsystemOsc_PropertySet, MySubsystemOsc_PropertyGet, 
																				MySubsystemOsc_PropertyNames );

		if ( status != CONTROLLER_OK )
			return Osc_SendError( channel, MySubsystemOsc_Name, status );
		return CONTROLLER_OK;
	}
  \endcode
*/
int Osc_IndexIntReceiverHelper( int channel, char* message, int length, 
                                int indexCount, char* subsystemName, 
                                int (*propertySet)( int index, int property, int value ),
                                int (*propertyGet)( int index, int property ),
                                char* propertyNames[] )
{
  int i;
  if( *message == '\0' || *message == ' ' ) // first, try to see if it was an index "help" query
  {
    for ( i = 0; i < indexCount; i++ )
    {
      Osc_LockScratchBuf( Osc->scratch1Semaphore );
      snprintf( Osc->scratch1, OSC_SCRATCH_SIZE, "/%s", subsystemName ); 
      Osc_CreateMessage( channel, Osc->scratch1, ",i", i );
      Osc_UnlockScratchBuf( Osc->scratch1Semaphore );
    }
    return CONTROLLER_OK;
  }
  
  // Look for the next slash - being the one that separates the index
  // from the property.  Note that this won't go off on a search through the buffer
  // since there will soon be a string terminator (i.e. a 0)
  char* prop = strchr( message, '/' );
  char* propHelp = NULL;
  if ( prop == NULL )
    propHelp = message + strlen(message);


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

  //char* propHelp = prop + 1; // then, see if it was a property "help" query
  if( *propHelp == '\0' || *propHelp == ' ' ) // first, try to see if it was an index "help" query
  {
    i = 0;
    while( true )
    {
      if( propertyNames[i] != 0 )
      {
        Osc_LockScratchBuf( Osc->scratch1Semaphore );
        snprintf( Osc->scratch1, OSC_SCRATCH_SIZE, "/%s/%d", subsystemName, number ); 
        Osc_CreateMessage( channel, Osc->scratch1, ",s", propertyNames[i] );
        Osc_UnlockScratchBuf( Osc->scratch1Semaphore );
        i++;
      }
      else
        return CONTROLLER_OK;
    }
  }

  // Now that we know where the property is, we can see if we can find it.
  // This is a little cheap, since we're also implying that there are no 
  // more address terms after the property.  That is, if testing for "speed", while
  // "speed" would match, "speed/other_stuff" would not.
  int propertyIndex = Osc_PropertyLookup( propertyNames, prop + 1 );
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
      Osc_LockScratchBuf( Osc->scratch1Semaphore );
      snprintf( Osc->scratch1, OSC_SCRATCH_SIZE, "/%s/%d/%s", subsystemName, number, propertyNames[ propertyIndex ] ); 
      Osc_CreateMessage( channel, Osc->scratch1, ",i", value );
      Osc_UnlockScratchBuf( Osc->scratch1Semaphore );
    }
    else
    {
      int index = 0;
      while ( bits > 0 && index < indexCount )
      { 
        if ( bits & 1 )
        {
          int value = (*propertyGet)( index, propertyIndex );
          Osc_LockScratchBuf( Osc->scratch1Semaphore );
          snprintf( Osc->scratch1, OSC_SCRATCH_SIZE, "/%s/%d/%s", subsystemName, index, propertyNames[ propertyIndex ] ); 
          Osc_CreateMessage( channel, Osc->scratch1, ",i", value );
          Osc_UnlockScratchBuf( Osc->scratch1Semaphore );
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
        else
        {
          if ( *tp == 's' )
          {
            *(va_arg( args, char** )) = data;
            int len = strlen( data ) + 1;
            *(va_arg( args, int* )) = len;
            int pad = len % 4;
            if ( pad != 0 )
              len += ( 4 - pad );
            data += len;
            count++;
            cont = true;
          }
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

int Osc_LockScratchBuf( xSemaphoreHandle scratchSemaphore )
{
  // Lock up this program segment to prevent multiple use
  if ( !xSemaphoreTake( scratchSemaphore, 1000 ) )
    return CONTROLLER_ERROR_LOCK_ERROR;

  return CONTROLLER_OK;
}

int Osc_UnlockScratchBuf( xSemaphoreHandle scratchSemaphore )
{
  if( !xSemaphoreGive( scratchSemaphore ) )
    return CONTROLLER_ERROR_LOCK_ERROR;
  else
    return CONTROLLER_OK;
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

#endif // OSC


