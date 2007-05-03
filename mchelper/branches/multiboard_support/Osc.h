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

#ifndef OSC_H
#define OSC_H

#define OSC_MAX_MESSAGE 2048

#include "PacketInterface.h"
#include "MessageInterface.h"
#include "PacketReadyInterface.h"
#include "McHelperWindow.h"

#include <QObject>

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

class Osc : public QObject, public PacketReadyInterface, public MessageInterface
{	
	Q_OBJECT
		
	public:
		enum Status { OK, ERROR_SENDING_TEXT_MESSAGE, ERROR_SENDING_COMPLEX_MESSAGE, 
			            ERROR_NO_PACKET, ERROR_CREATING_BUNDLE, ERROR_PACKET_LENGTH_0 };				

		Osc( );

		Status createMessage( char* textMessage ); 
		Status createMessage( char* address, char* format, ... );
		Status sendPacket( );
		bool isMessageWaiting();
		Status receive( OscMessage* message = 0 );
		void packetWaiting( );	//from PacketReadyInterface
		void setInterfaces( PacketInterface* packetInterface, MessageInterface* messageInterface, QApplication* application );
    void setPreamble( const char* preamble ) { this->preamble = preamble; }

	  void message( int level, char* format, ... );
	  void sleepMs( int ms );
	  void progress( int value );
		
	public slots:
		void uiSendPacket( QString rawString );
	  		
	private:
		char* findDataTag( char* message, int length );
		void receivePacket( char* packet, int length,  OscMessage* message );
		void receiveMessage( char* message, int length, OscMessage* message );
		int extractData( char* buffer, OscMessage* message );
	 
		char* createBundle( char* buffer, int* length, int a, int b );
		char* createMessageInternal( char* bp, int* length, char* inputString );
		char* createMessageInternal( char* bp, int* length, char* address, char* format, va_list args );
		char* writePaddedString( char* buffer, int* length, char* string );
		char* writeTimetag( char* buffer, int* length, int a, int b );
		void resetOutBuffer( );
		void customEvent( QEvent* event );

		// utility
		unsigned int endianSwap( unsigned int a );
				
		PacketInterface* packetInterface;
		MessageInterface* messageInterface;
		QApplication* application;
		
		const char* preamble;
		char outBuffer[ OSC_MAX_MESSAGE ];
	  char* outBufferPointer;
	  int outBufferRemaining;
	  int outMessageCount;
};

class OscMessageEvent : public QEvent
{
  public:
	  OscMessageEvent( int level, char* message );
	  ~OscMessageEvent( );
	  
	int level;
	char* message;
};

#endif	


