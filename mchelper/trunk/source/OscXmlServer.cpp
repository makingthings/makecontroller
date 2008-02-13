/*********************************************************************************

 Copyright 2006-2007 MakingThings

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
#include <QMutexLocker>

#define FROM_STRING "XML Server"

OscXmlServer::OscXmlServer( McHelperWindow *mainWindow, int port, QObject *parent ) : QTcpServer( parent )
{
	this->mainWindow = mainWindow;
	listenPort = port;
}

void OscXmlServer::incomingConnection( int socketDescriptor )
{
	OscXmlClient *client = new OscXmlClient( socketDescriptor, mainWindow );
	connect( client, SIGNAL(finished()), client, SLOT(deleteLater()));
	client->start( );
	// tell Flash about the boards we have connected
	client->boardListUpdate( mainWindow->getConnectedBoards( ), true );
}

bool OscXmlServer::changeListenPort( int port )
{
	close( );
	if( !listen( QHostAddress::Any, port ) )
	{
		mainWindow->messageThreadSafe( QString( "Error - can't listen on port %1.  Make sure it's available." ).arg( port ), 
																		MessageEvent::Error, FROM_STRING );
		return false;
	}
	else
	{
		listenPort = port;
		mainWindow->messageThreadSafe( QString( "Now listening on port %1 for XML connections." ).arg( port ), 
																		MessageEvent::Info, FROM_STRING );
		return true;
	}
}

/************************************************************************************
																		
																		OscXmlClient
																		
************************************************************************************/

OscXmlClient::OscXmlClient( int socketDescriptor, McHelperWindow *mainWindow, QObject *parent )
	: QThread(parent), socketDescriptor( socketDescriptor )
{	
	this->mainWindow = mainWindow;
	handler = new XmlHandler( mainWindow, this );	
	xml.setContentHandler( handler );
	xml.setErrorHandler( handler );
	resetParser( );
	socket = NULL;
}

void OscXmlClient::run( )
{
	//moveToThread(QThread::currentThread() );
	socket = new QTcpSocket( );
	// these connections need to be direct since we have a pointer to the tcpsocket in our class
	// which means that it will live in the server thread, which is the main GUI thread, 
	// and we want to process in our own thread
	connect( socket, SIGNAL(readyRead()), this, SLOT(processData()), Qt::DirectConnection);
	connect( socket, SIGNAL(disconnected()), this, SLOT(disconnected()), Qt::DirectConnection);
	//connect( socket, SIGNAL(bytesWritten(qint64)), this, SLOT(wroteBytes(qint64)), Qt::DirectConnection);
	connect( mainWindow, SIGNAL(boardInfoUpdate(Board*)), this, SLOT(boardInfoUpdate(Board*)), Qt::DirectConnection);
	qRegisterMetaType< QList<OscMessage*> >("QList<OscMessage*>");
	connect( mainWindow, SIGNAL(boardListUpdate(QList<Board*>, bool)), 
						this, SLOT(boardListUpdate(QList<Board*>, bool)), Qt::DirectConnection);
	connect( mainWindow, SIGNAL(xmlPacket(QList<OscMessage*>, QString, int)), 
						this, SLOT(sendXmlPacket(QList<OscMessage*>, QString, int)), Qt::DirectConnection);
	
	if( !socket->setSocketDescriptor( socketDescriptor ) )
	{
		mainWindow->messageThreadSafe( "Error opening connection to client", MessageEvent::Error, FROM_STRING );
		return;
	}
	peerAddress = socket->peerAddress( ).toString( );
	mainWindow->messageThreadSafe( QString( "New connection from XML peer at %1").arg( peerAddress ), 
																	MessageEvent::Info, FROM_STRING );
	exec( ); // run the thread, listening for and sending messages, until we call exit( )
}

void OscXmlClient::processData( )
{
	// if there's more than one XML document, we expect them to be delimited by \0
	QList<QByteArray> newDocuments = socket->readAll( ).split( '\0' );
	bool status;
	for( int i = 0; i < newDocuments.size( ); i++ )
	{
		if( newDocuments.at( i ).size( ) )
		{
			//printf( "string: %s\n", newDocuments.at( i ).data() );
			xmlInput.setData( newDocuments.at( i ) );
		
			if( lastParseComplete )
			{
				lastParseComplete = false; // this will get reset in the parsing process if we get a complete message
				status = xml.parse( &xmlInput, true );
			}
			else
				status = xml.parseContinue( );
			
			if( !status ) 
			{
				// there was a problem parsing.  now the next time we come through, it will start
				// a new parse, discarding anything that was left from the last socket read
				resetParser( );
				printf( "XML parse error: %s\n", handler->errorString().toAscii().data() );
			}
		}
	}
}

void OscXmlClient::resetParser( )
{
	lastParseComplete = true;
}

void OscXmlClient::disconnected( )
{
	disconnect( ); // don't want to respond to any more signals
	socket->abort( );
	socket->deleteLater( ); // these will get deleted when control returns to the main event loop
	handler->deleteLater( );
	QString msg = QString( "XML peer at %1 disconnected." ).arg( peerAddress );
	mainWindow->messageThreadSafe( msg, MessageEvent::Info, FROM_STRING );
	exit( ); // shut this thread down
}

void OscXmlClient::wroteBytes( qint64 bytes )
{
	printf( "XML, wrote %d bytes to %s\n", (int)bytes, peerAddress.toAscii().data() );
}

bool OscXmlClient::isConnected( )
{
	if( socket != NULL )
	{
		if( socket->state( ) == QAbstractSocket::ConnectedState )
			return true;
	}
	return false;
}

void OscXmlClient::boardInfoUpdate( Board* board )
{		
	QDomDocument doc;
	QDomElement boardUpdate = doc.createElement( "BOARD_INFO" );
	doc.appendChild( boardUpdate );

	QDomElement boardElement = doc.createElement( "BOARD" );
	boardElement.setAttribute( "LOCATION", board->key );
	boardElement.setAttribute( "NAME", board->name );
	boardElement.setAttribute( "SERIALNUMBER", board->serialNumber );
	boardUpdate.appendChild( boardElement );
		
	writeXmlDoc( doc );
}

void OscXmlClient::boardListUpdate( QList<Board*> boardList, bool arrived )
{
	QDomDocument doc;
	QDomElement boardUpdate;
	if( arrived )
		boardUpdate = doc.createElement( "BOARD_ARRIVAL" );
	else
		boardUpdate = doc.createElement( "BOARD_REMOVAL" );
		
	doc.appendChild( boardUpdate );
	for( int i = 0; i < boardList.count( ); i++ )
	{
		Board* currentBoard = boardList.at( i );
		QDomElement board = doc.createElement( "BOARD" );
		if( currentBoard->type == Board::UsbSerial )
			board.setAttribute( "TYPE", "USB" );
		if( currentBoard->type == Board::Udp )
			board.setAttribute( "TYPE", "Ethernet" );
		board.setAttribute( "LOCATION", currentBoard->key );
		boardUpdate.appendChild( board );
	}
	
	writeXmlDoc( doc );
}

void OscXmlClient::writeXmlDoc( QDomDocument doc )
{
	if( isConnected( ) )
		socket->write( doc.toByteArray( ).append( '\0' ) ); // Flash wants XML followed by a zero byte
}

void OscXmlClient::sendXmlPacket( QList<OscMessage*> messageList, QString srcAddress, int srcPort )
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
		msg.setAttribute( "NAME", oscMsg->addressPattern );
		oscPacket.appendChild( msg );
		
		for( int j = 0; j < dataCount; j++ )
		{
			OscMessageData *data = oscMsg->data.at( j );
			QDomElement argument = doc.createElement( "ARGUMENT" );
			switch( data->type )
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
				{
					QString blobstring;
					unsigned char* blob = (unsigned char*)data->b.data( );
					int blob_len = qFromBigEndian( *(int*)blob );  // the first int should give us the length of the blob
					blob += sizeof(int); // step past the length
					while( blob_len-- )
					{
						// break each byte into 4-bit chunks so they don't get misinterpreted
						// by any casts to ASCII, etc. and send a string composed of single chars from 0-f
						blobstring.append( QString::number( (*blob >> 4) & 0x0f, 16 ) );
						blobstring.append( QString::number(*blob++ & 0x0f, 16 ) );
					}
					argument.setAttribute( "TYPE", "b" );
					argument.setAttribute( "VALUE", blobstring );
					break;
				}
			}
			msg.appendChild( argument );
		}
	}
	writeXmlDoc( doc );
}

/************************************************************************************
																		
																		XmlHandler
																		
************************************************************************************/

XmlHandler::XmlHandler( McHelperWindow *mainWindow, OscXmlClient *xmlClient ) : QXmlDefaultHandler( )
{
	this->mainWindow = mainWindow;
	this->xmlClient = xmlClient;
	currentMessage = NULL;
}

bool XmlHandler::fatalError (const QXmlParseException & exception)
{
	error( exception );
	return true;
}

bool XmlHandler::error (const QXmlParseException & exception)
{
	 printf( "incoming XML error on line, %d, column %d : %s\n",
	 					exception.lineNumber(), exception.columnNumber(), exception.message().toAscii().data() );
	 return false;
} 

bool XmlHandler::startElement( const QString & namespaceURI, const QString & localName, 
												const QString & qName, const QXmlAttributes & atts )
{
	(void) namespaceURI;
	(void) qName;
	
	if( localName == "OSCPACKET" )
	{
		currentDestination = atts.value( "ADDRESS" );
		currentPort = atts.value( "PORT" ).toInt( );
		if( currentDestination.isEmpty( ) )
			return false;
	}
	else if( localName == "MESSAGE" )
	{
		currentMessage = new OscMessage( );
		currentMessage->addressPattern = atts.value( "NAME" );
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
				msgData->type = OscMessageData::OmdInt;
				msgData->i = val.toInt( );
			}
			else if( type == "f" )
			{
				msgData->type = OscMessageData::OmdFloat;
				msgData->f = val.toFloat( );
			}
			else if( type == "s" )
			{
				msgData->type = OscMessageData::OmdString;
				msgData->s = val;
			}
			else if( type == "b" )
			{
				msgData->type = OscMessageData::OmdBlob;
				msgData->b = val.toAscii( );
			}
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
		mainWindow->newXmlPacketReceived( oscMessageList, currentDestination );
		QStringList strings;
		for( int i = 0; i < oscMessageList.count( ); i++ )
			strings << oscMessageList.at( i )->toString( );
		mainWindow->messageThreadSafe( strings, MessageEvent::XMLMessage, FROM_STRING);
		qDeleteAll( oscMessageList );
		oscMessageList.clear( );
		xmlClient->resetParser( );
	}
	else if( localName == "MESSAGE" )
		oscMessageList.append( currentMessage );

	return true;
}








