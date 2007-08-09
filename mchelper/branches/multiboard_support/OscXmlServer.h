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

#ifndef OSC_XML_SERVER_H
#define OSC_XML_SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QXmlSimpleReader>
#include <QXmlDefaultHandler>
#include "McHelperWindow.h"
#include "MessageEvent.h"
#include "Osc.h"

class OscXmlServer;
class Osc;

class XmlHandler : public QXmlDefaultHandler
{
	public:
		XmlHandler( McHelperWindow *mainWindow, OscXmlServer *xmlServer );
		bool endElement( const QString & namespaceURI, const QString & localName, const QString & qName );
		bool startElement( const QString & namespaceURI, const QString & localName, 
												const QString & qName, const QXmlAttributes & atts );
												
	private:
		McHelperWindow *mainWindow;
		OscXmlServer *xmlServer;
		OscMessage* currentMessage;
		QHostAddress currentDestination;
		int currentPort;
};

class OscXmlServer : public QObject
{
	Q_OBJECT
	
	public:
		OscXmlServer( McHelperWindow *mainWindow );
		~OscXmlServer( );
		bool isConnected( );
		void sendXmlPacket( QList<OscMessage*> messageList, QString srcAddress, int srcPort );
		QList<OscMessage*> oscMessageList;
		QString fromString;
		
	private:
		QTcpServer *serverSocket;
		QTcpSocket *clientSocket;
		McHelperWindow *mainWindow;
		QXmlSimpleReader xml;
		QXmlInputSource *xmlInput;
		XmlHandler *handler;
	
	private slots:
		void openNewConnection( );
		void processClientData( );
		void clientDisconnected( );
		void clientError( QAbstractSocket::SocketError error );
};

#endif // OSC_XML_SERVER_H


