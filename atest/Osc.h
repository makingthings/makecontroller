/****************************************************************************
**
** OSC
** MakingThings 2006.
**
****************************************************************************/

#ifndef OSC_H
#define OSC_H

#define OSC_MAX_MESSAGE 2048

#include "PacketInterface.h"
#include "MessageInterface.h"

#include <stdarg.h>
#include <stdlib.h>

class OscMessage
{
	public:
	  char* address;
	  char* s;
	  int   i;
	  float f;
	  
	  OscMessage()
	  {
	  	address = 0;
	  }
	  
	  ~OscMessage()
	  {
	  	if ( address != 0 )
	  	  free( address );
	  }
};

class Osc
{	
	public:
		enum Status { OK, ERROR_SENDING_TEXT_MESSAGE, ERROR_SENDING_COMPLEX_MESSAGE, 
			            ERROR_NO_PACKET, ERROR_CREATING_BUNDLE, ERROR_PACKET_LENGTH_0 };				

		Osc( PacketInterface* packetInterface, MessageInterface* messageInterface );

		Status createMessage( char* textMessage ); 
		Status createMessage( char* address, char* format, ... );
		Status sendPacket( );
		bool isMessageWaiting();
		Status receive( OscMessage* message ); 
	  		
	private:
		char* findDataTag( char* message, int length );
		void receivePacket( char* packet, int length,  OscMessage* message );
		void receiveMessage( char* message, int length, OscMessage* message );
		int extractData( char* buffer, OscMessage* message );
	 
		char* createBundle( char* buffer, int* length, int a, int b );
		char* createMessageInternal( char* bp, int* length, char* inputString );
		char* createMessageInternal( char* bp, int* length, char* address, char* format, va_list args );
		char* writePaddedString( char* buffer, int* length, char* string );
		//char* writenPaddedString( char* buffer, int* remaining, char* string, int length );
		char* writeTimetag( char* buffer, int* length, int a, int b );
		void resetOutBuffer( );
		
		// utility
		unsigned int endianSwap( unsigned int a );
				
		PacketInterface* packetInterface;
		MessageInterface* messageInterface;
		
		char outBuffer[ OSC_MAX_MESSAGE ];
	  char* outBufferPointer;
	  int outBufferRemaining;
	  int outMessageCount;
};

#endif	
