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

#ifndef OSC_H
#define OSC_H

#include "MessageInterface.h"
#include "McHelperWindow.h"

class McHelperWindow;

class OscMessageData
{
	public:
		enum { OmdString, OmdInt, OmdFloat, OmdBlob } type;
		OscMessageData( ) { };
		OscMessageData( int i );
		OscMessageData( float f );
		OscMessageData( QString s );
		OscMessageData( QByteArray b );
	  QString s;
	  QByteArray b;
	  int i;
	  float f;
};

class OscMessage
{
	public:
	  QString addressPattern;
		QList<OscMessageData*> data;
	  QString toString( );
		QByteArray toByteArray( );
		~OscMessage( ) { qDeleteAll( data ); }
};

class Osc
{	
	public:
		enum Status { OK, ERROR_SENDING_TEXT_MESSAGE, ERROR_SENDING_COMPLEX_MESSAGE, 
			            ERROR_NO_PACKET, ERROR_CREATING_REQUEST, ERROR_CREATING_BUNDLE, ERROR_PACKET_LENGTH_0 };				

		Osc( ) { };
		static QByteArray writePaddedString( char *string );
		static QByteArray writePaddedString( QString str );
		static QByteArray writeTimetag( int a, int b );
		static QByteArray createOneRequest( char* message );
		QList<OscMessage*> processPacket( char* data, int size );
		QByteArray createPacket( QStringList strings );
		QByteArray createPacket( QList<OscMessage*> msgs );
		QByteArray createPacket( QString msg );
		
		Status createMessage( OscMessage* message );
		void setInterfaces( MessageInterface* messageInterface );
    void setPreamble( QString preamble ) { this->preamble = preamble; }
    QString getPreamble( );
		bool createMessage( QString msg, OscMessage *msg );
	  		
	private:
		char* findDataTag( char* message, int length );
		QString getTypeTag( char* message );
		void receivePacket( char* packet, int length,  QList<OscMessage*>* oscMessageList );
		void receiveMessage( char* message, int length, QList<OscMessage*>* oscMessageList );
		int extractData( char* buffer, OscMessage* message );

		MessageInterface* messageInterface;
		QString preamble;
};

#endif	


