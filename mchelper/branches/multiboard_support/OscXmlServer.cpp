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

#include "OscXmlServer.h"
#include <QDomDocument>

OscXmlServer::OscXmlServer( McHelperWindow *mainWindow, int port )
{
	this->mainWindow = mainWindow;
	listenPort = port;
	serverSocket = new QTcpServer( );
	serverSocket->setMaxPendingConnections( 1 ); // just listen for 1 for now
	serverSocket->listen( QHostAddress::Any, listenPort );
	connect( serverSocket, SIGNAL( newConnection() ), this, SLOT( openNewConnection() ) );
	clientSocket = NULL;

	handler = new XmlHandler( mainWindow, this );
	xml.setContentHandler( handler );
	
	fromString = QString( "XML server" ); // what our messages to the UI will show as having come from
}

OscXmlServer::~OscXmlServer( )
{
	
}

void OscXmlServer::openNewConnection( )
{
	clientSocket = serverSocket->nextPendingConnection( );
	if( !clientSocket )
	{
		mainWindow->messageThreadSafe( "Couldn't make connection to socket", MessageEvent::Error, fromString );
		return;
	}
	else
		mainWindow->messageThreadSafe( "New XML connection", MessageEvent::Info, fromString );
	connect( clientSocket, SIGNAL( readyRead() ), this, SLOT( processClientData() ) );
	connect( clientSocket, SIGNAL( disconnected() ), this, SLOT( clientDisconnected() ) );
	connect( clientSocket, SIGNAL( error(  QAbstractSocket::SocketError ) ), this, SLOT( clientError( QAbstractSocket::SocketError) ) );
	xmlInput = new QXmlInputSource( clientSocket );
}

bool OscXmlServer::changeListenPort( int port )
{
	serverSocket->close( );
	if( !serverSocket->listen( QHostAddress::Any, port ) )
		return false;
	else
	{
		listenPort = port;
		return true;
	}
}

void OscXmlServer::processClientData( )
{
	// normally we'd read out of the clientSocket here, but because we've set it up as an xmlInputSource, we just parse from that
	xml.parse( xmlInput, false );
}

void OscXmlServer::clientDisconnected( )
{
	mainWindow->messageThreadSafe( "XML peer disconnected.", MessageEvent::Info, fromString );
	clientSocket->abort( ); // close the socket and flush it
	delete xmlInput;
	xmlInput = NULL;
}

void OscXmlServer::clientError( QAbstractSocket::SocketError error )
{
	(void) error;
}

bool OscXmlServer::isConnected( )
{
	if( clientSocket != NULL )
	{
		if( clientSocket->state( ) == QAbstractSocket::ConnectedState )
			return true;
	}
	
	return false;
}

void OscXmlServer::sendXmlPacket( QList<OscMessage*> messageList, QString srcAddress, int srcPort )
{
	int msgCount = messageList.count( );
	if( !isConnected( ) || msgCount < 1 )
		return;
	
	QDomDocument doc;
	QDomElement oscPacket = doc.createElement( "OSCPACKET" );
	oscPacket.setAttribute( "ADDRESS", srcAddress );
	oscPacket.setAttribute( "PORT", srcPort );
	oscPacket.setAttribute( "TIME", 0 );
	doc.appendChild( oscPacket );

	for( int i = 0; i < msgCount; i++ )
	{
		OscMessage *oscMsg = messageList.at( i );
		int dataCount = oscMsg->data.count( );
		
		QDomElement msg = doc.createElement( "MESSAGE" );
		msg.setAttribute( "NAME", QString( oscMsg->address ) );
		oscPacket.appendChild( msg );
		
		for( int j = 0; j < dataCount; j++ )
		{
			OscMessageData *data = oscMsg->data.at( j );
			QDomElement argument = doc.createElement( "ARGUMENT" );
			switch( data->omdType )
			{
				case OscMessageData::OmdString:
					argument.setAttribute( "TYPE", "s" );
					argument.setAttribute( "VALUE", QString( data->s ) );
					break;
				case OscMessageData::OmdInt:
					argument.setAttribute( "TYPE", "i" );
					argument.setAttribute( "VALUE", QString::number( data->i ) );
					break;
				case OscMessageData::OmdFloat:
					argument.setAttribute( "TYPE", "f" );
					argument.setAttribute( "VALUE", QString::number( data->f ) );
					break;
				case OscMessageData::OmdBlob:
					argument.setAttribute( "TYPE", "b" );
					argument.setAttribute( "VALUE", QString( (char*)data->b ) );
					break;
			}
			msg.appendChild( argument );
		}
	}
	clientSocket->write( doc.toByteArray( ) );
	clientSocket->write( "\0", 1 ); // Flash wants XML followed by a zero byte
}

XmlHandler::XmlHandler( McHelperWindow *mainWindow, OscXmlServer *xmlServer ) : QXmlDefaultHandler( )
{
	this->mainWindow = mainWindow;
	this->xmlServer = xmlServer;
}

bool XmlHandler::startElement( const QString & namespaceURI, const QString & localName, 
												const QString & qName, const QXmlAttributes & atts )
{
	(void) namespaceURI;
	(void) qName;
	
	if( localName == "OSCPACKET" )
	{
		QString destAddress = atts.value( "ADDRESS" );
		QString destPort = atts.value( "PORT" );
		if( destAddress.isEmpty( ) || destPort.isEmpty( ) )
			return false;
		currentDestination = QHostAddress( destAddress );
		currentPort = destPort.toInt( );
	}
	else if( localName == "MESSAGE" )
	{
		currentMessage = new OscMessage( );
		QString addr = atts.value( "NAME" );
		currentMessage->address = (char*)malloc( 256 );
		snprintf( currentMessage->address, 256, addr.toLatin1( ).data( ) );
	}
	else if( localName == "ARGUMENT" )
	{
		QString type = atts.value( "TYPE" );
		QString val = atts.value( "VALUE" );
		if( type.isEmpty( ) || val.isEmpty( ) )
			return false;
		if( type == "i" || type == "f" || type == "s" || type == "b" )
		{
			OscMessageData *msgData = new OscMessageData( );
			if( type == "i" )
			{
				msgData->omdType = OscMessageData::OmdInt;
				msgData->i = val.toInt( );
			}
			else if( type == "f" )
			{
				msgData->omdType = OscMessageData::OmdFloat;
				msgData->f = val.toFloat( );
			}
			else if( type == "s" )
			{
				msgData->omdType = OscMessageData::OmdString;
				msgData->s = (char*)malloc( 256 );
				snprintf( msgData->s, 256, val.toLatin1( ).data( ) );
			}
			/*
			else if( type == "b" )
			{
				msgData->omdType = OscMessageData::OmdBlob;
				msgData->i = val.toInt( );
			}
			*/
			currentMessage->data.append( msgData );
		}
	}

	return true;
}

bool XmlHandler::endElement( const QString & namespaceURI, const QString & localName, const QString & qName )
{
	(void) namespaceURI;
	(void) qName;
	
	if( localName == "OSCPACKET" )
	{
		mainWindow->newXmlPacketReceived( xmlServer->oscMessageList, currentDestination.toString() );
		QStringList strings;
		for( int i = 0; i < xmlServer->oscMessageList.count( ); i++ )
			strings << xmlServer->oscMessageList.at( i )->toString( );
		mainWindow->messageThreadSafe( strings, MessageEvent::XMLMessage, xmlServer->fromString);
		qDeleteAll( xmlServer->oscMessageList );
		xmlServer->oscMessageList.clear( );
	}
	else if( localName == "MESSAGE" )
		xmlServer->oscMessageList.append( currentMessage );

	return true;
}








