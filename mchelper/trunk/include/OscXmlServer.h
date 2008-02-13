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

#ifndef OSC_XML_SERVER_H
#define OSC_XML_SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QXmlSimpleReader>
#include <QXmlDefaultHandler>
#include <QDomDocument>
#include <QMutex>

#include "McHelperWindow.h"
#include "MessageEvent.h"
#include "Board.h"
#include "Osc.h"

class OscXmlServer;
class OscXmlClient;
class Osc;
class Board;

class XmlHandler : public QObject, public QXmlDefaultHandler
{
	Q_OBJECT
	public:
		XmlHandler( McHelperWindow *mainWindow, OscXmlClient *xmlClient );
		bool endElement( const QString & namespaceURI, const QString & localName, const QString & qName );
		bool startElement( const QString & namespaceURI, const QString & localName, 
												const QString & qName, const QXmlAttributes & atts );
		bool error (const QXmlParseException & exception);
		bool fatalError (const QXmlParseException & exception);
	
	signals:
		void newMessage( QStringList strings, MessageEvent::Types type, QString from );
												
	private:
		McHelperWindow *mainWindow;
		OscXmlClient *xmlClient;
		OscMessage* currentMessage;
		QString currentDestination;
		int currentPort;
		QList<OscMessage*> oscMessageList;
};

class OscXmlClient : public QThread
{
	Q_OBJECT
	public:
		OscXmlClient( QTcpSocket *socket, McHelperWindow *mainWindow, QObject *parent = 0 );
		~OscXmlClient( ) { }
    void run();
		void resetParser( );
	
	public slots:
		void boardListUpdate( QList<Board*> boardList, bool arrived );
		void boardInfoUpdate( Board* board );
		void wroteBytes( qint64 bytes );
		void sendXmlPacket( QList<OscMessage*> messageList, QString srcAddress, int srcPort );

	private:
    int socketDescriptor;
		McHelperWindow *mainWindow;
		bool lastParseComplete;
		QXmlSimpleReader xml;
		QXmlInputSource xmlInput;
		XmlHandler *handler;
		QTcpSocket *socket;
		QList<QString> uiMessages;
		QMutex msgMutex;
		QString peerAddress;
		bool shuttingDown;
		
		bool isConnected( );
		void writeXmlDoc( QDomDocument doc );
	
	private slots:
		void processData( );
		void disconnected( );
};

class OscXmlServer : public QTcpServer
{
	Q_OBJECT
	public:
		OscXmlServer( McHelperWindow *mainWindow, int port, QObject *parent = 0 );
		void run( );
		bool changeListenPort( int port );
	
	private slots:
		void openNewConnection( );
				
	private:
		McHelperWindow *mainWindow;
		int listenPort;
};

#endif // OSC_XML_SERVER_H


