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

class McHelperWindow;


class OscMessageData
{
	public:
		enum { OmdString, OmdInt, OmdFloat, OmdBlob } omdType;
	  char* s;
	  void* b;
	  int   i;
	  float f;
	  
	  OscMessageData()
	  {
	  	s = 0;
	  	b = 0;
	  }
	  
	  ~OscMessageData()
	  {
	  	if ( s != 0 )
	  	  free( s );
	  	if( b!= 0 )
	  		free( b );
	  }
};

class OscMessage
{
	public:
	  char* address;
		QList<OscMessageData*> data;
	  
	  OscMessage()
	  {
	  	address = 0;
	  }
	  
	  ~OscMessage()
	  {
	  	if ( address != 0 )
	  	  free( address );
	  	qDeleteAll( data );
	  	data.clear( );
	  }
	  QString toString( );
};

class Osc : public QObject
{	
	Q_OBJECT
		
	public:
		enum Status { OK, ERROR_SENDING_TEXT_MESSAGE, ERROR_SENDING_COMPLEX_MESSAGE, 
			            ERROR_NO_PACKET, ERROR_CREATING_REQUEST, ERROR_CREATING_BUNDLE, ERROR_PACKET_LENGTH_0 };				

		Osc( );

		Status createMessage( char* textMessage ); 
		Status createMessage( char* address, char* format, ... );
		Status createOneRequest( char* buffer, int *length, char* message );
		Status sendPacket( );
		bool isMessageWaiting();
		Status receive( QList<OscMessage*>* oscMessageList = 0 );
		void setInterfaces( PacketInterface* packetInterface, MessageInterface* messageInterface, QApplication* application );
    	void setPreamble( const char* preamble ) { this->preamble = preamble; }
    	const char* getPreamble( );

	  void sleepMs( int ms );
	  void progress( int value );
		
	public slots:
		void uiSendPacket( QString rawString );
	  		
	private:
		char* findDataTag( char* message, int length );
		void receivePacket( char* packet, int length,  QList<OscMessage*>* oscMessageList );
		void receiveMessage( char* message, int length, QList<OscMessage*>* oscMessageList );
		int extractData( char* buffer, OscMessage* message );
	 
		char* createBundle( char* buffer, int* length, int a, int b );
		char* createMessageInternal( char* bp, int* length, char* inputString );
		char* createMessageInternal( char* bp, int* length, char* address, char* format, va_list args );
		char* writePaddedString( char* buffer, int* length, char* string );
		char* writeTimetag( char* buffer, int* length, int a, int b );
		void resetOutBuffer( );

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

#endif	


