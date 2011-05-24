/*********************************************************************************

 Copyright 2006-2009 MakingThings

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
#include <QSettings>
#include "Preferences.h" // for the DEFAULT_XML_LISTEN_PORT

#define FROM_STRING "XML Server"

OscXmlServer::OscXmlServer(MainWindow *mainWindow, QObject *parent)
  : QTcpServer(parent),
    mainWindow (mainWindow)
{
  connect(this, SIGNAL(msg(QString, MsgType::Type, QString)), mainWindow, SLOT(message(QString, MsgType::Type, QString)));
  QSettings settings;
  setListenPort(settings.value("xml_listen_port", DEFAULT_XML_LISTEN_PORT).toInt(), false);
}

/*
  Called when a new TCP connection has been made.
  Create a new thread for the client, and set it up.
*/
void OscXmlServer::incomingConnection(int handle)
{
  OscXmlClient *client = new OscXmlClient(handle, mainWindow);
  connect(client, SIGNAL(finished()), client, SLOT(deleteLater()));
  connect(this, SIGNAL(newXmlPacket(QList<OscMessage*>, QString)), client, SLOT(sendXmlPacket(QList<OscMessage*>, QString)));
  connect(this, SIGNAL(boardInfoUpdate(Board*)), client, SLOT(boardInfoUpdate(Board*)));
  connect(this, SIGNAL(boardListUpdated(QList<Board*>, bool)), client, SLOT(boardListUpdate(QList<Board*>, bool)));
  client->start();
}

bool OscXmlServer::setListenPort( int port, bool announce )
{
  if(listenPort == port) // don't need to do anything
    return true;
  close( );
  if( !listen( QHostAddress::Any, port ) ) {
    emit msg( tr("Error - can't listen on port %1.  Make sure it's available.").arg(port), MsgType::Error, FROM_STRING );
    return false;
  }
  else {
    listenPort = port;
    if(announce)
      emit msg( tr("Now listening on port %1 for XML connections.").arg(port), MsgType::Notice, FROM_STRING );
    return true;
  }
}

/*
  A packet has been received from a board.
  Send it to all connected TCP clients.
*/
void OscXmlServer::sendPacket(const QList<OscMessage*> & msgs, const QString & srcAddress)
{
  emit newXmlPacket(msgs, srcAddress);
}

/*
  Boards have been connected/removed.
  Send this to all our connected clients.
*/
void OscXmlServer::sendBoardListUpdate(QList<Board*> boardList, bool arrived)
{
  emit boardListUpdated(boardList, arrived);
}

/************************************************************************************

                                    OscXmlClient

  Represents a connection to an XML peer over TCP - a Flash movie, or anybody else
  who wants to talk to us.  A new thread is created for each client, in which we wait
  around for data, and parse it into OSC messages when it arrives, sending the info
  on to be sent out via UDP or USB to a board.

************************************************************************************/
OscXmlClient::OscXmlClient(int socketDescriptor, MainWindow *mainWindow, QObject *parent )
  : QThread(parent),
    socketDescriptor(socketDescriptor),
    mainWindow(mainWindow)
{
  qRegisterMetaType<MsgType::Type>("MsgType::Type");
  connect(this, SIGNAL(msg(QString, MsgType::Type, QString)), mainWindow, SLOT(message(QString, MsgType::Type, QString)));
  connect(mainWindow, SIGNAL(boardInfoUpdate(Board*)), this, SLOT(boardInfoUpdate(Board*)));
  handler = new XmlHandler( mainWindow, this );
  xml.setContentHandler( handler );
  xml.setErrorHandler( handler );
  lastParseComplete = true;
  shuttingDown = false;
}

/*
  This is the main loop for the separate thread that each TCP client sits in.
  Set up our connections and wait around for data.
*/
void OscXmlClient::run()
{
  this->socket = new QTcpSocket();
  if (!socket->setSocketDescriptor(socketDescriptor)) {
    qWarning() << "couldn't figure new server connection, bailing.";
    return;
  }
  connect(socket, SIGNAL(readyRead()), this, SLOT(processData()));
  connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
  //connect( socket, SIGNAL(bytesWritten(qint64)), this, SLOT(wroteBytes(qint64)), Qt::DirectConnection);
  qRegisterMetaType< QList<OscMessage*> >("QList<OscMessage*>");

  peerAddress = socket->peerAddress( ).toString( );
  emit msg( tr("New connection from peer at %1").arg(peerAddress), MsgType::Notice, FROM_STRING );

  xmlWriter.setDevice(socket);
  sendCrossDomainPolicy();
  boardListUpdate(mainWindow->getConnectedBoards(), true);

  exec(); // run the thread, listening for and sending messages, until we call exit( )
}

/*
  New data has arrived on our TCP connection.
  Read it and kick off the parsing process.
*/
void OscXmlClient::processData( )
{
  // if there's more than one XML document, we expect them to be delimited by \0
  QList<QByteArray> newDocuments = socket->readAll( ).split( '\0' );
  bool status;

  foreach( QByteArray document, newDocuments ) {
    if( document.size( ) ) {
      xmlInput.setData( document );
      if( lastParseComplete ) {
        lastParseComplete = false; // this will get reset in the parsing process if we get a complete message
        status = xml.parse( &xmlInput, true );
      }
      else
        status = xml.parseContinue( );

      if( !status ) {
        // there was a problem parsing.  now the next time we come through, it will start
        // a new parse, discarding anything that was left from the last socket read
        lastParseComplete = true;
        qDebug() <<  "XML parse error:" << handler->errorString();
      }
    }
  }
}

/*
  Called when our TCP peer disconnects.
  Clean up.
*/
void OscXmlClient::disconnected( )
{
  shuttingDown = true;
  emit msg( tr("Peer at %1 disconnected.").arg(peerAddress), MsgType::Notice, FROM_STRING );
  disconnect( ); // don't want to respond to any more signals
  socket->abort( );
  socket->deleteLater( ); // these will get deleted when control returns to the main event loop
  handler->deleteLater( );
  exit( ); // shut this thread down
}

void OscXmlClient::wroteBytes( qint64 bytes )
{
  qDebug() << tr("XML, wrote %1 bytes to %2").arg(bytes).arg(peerAddress);
}

bool OscXmlClient::isConnected( )
{
  if( socket != NULL && !shuttingDown )
    return ( socket->state( ) == QAbstractSocket::ConnectedState );
  return false;
}

/*
  Called when some element of the board's info has changed.
  Pass the information on to our TCP peer.
*/
void OscXmlClient::boardInfoUpdate( Board* board )
{
  if (!board)
    return;
  xmlWriter.writeStartDocument();
  xmlWriter.writeStartElement("BOARD_INFO");
  xmlWriter.writeStartElement("BOARD");
  xmlWriter.writeAttribute("LOCATION", board->key());
  xmlWriter.writeAttribute("NAME", board->name);
  xmlWriter.writeAttribute("SERIALNUMBER", board->serialNumber);
  xmlWriter.writeEndElement(); // BOARD
  xmlWriter.writeEndElement(); // BOARD_INFO
  xmlWriter.writeEndDocument();
  char terminator = '\0';
  socket->write(&terminator, 1);
}

/*
  Called when boards have arrived or been removed.
  Pass the info on to our TCP peer.
*/
void OscXmlClient::boardListUpdate( const QList<Board*> & boardList, bool arrived )
{
  if (boardList.isEmpty())
    return;

  xmlWriter.writeStartDocument();
  xmlWriter.writeStartElement(arrived ? "BOARD_ARRIVAL" : "BOARD_REMOVAL");
  foreach (Board *board, boardList) {
    xmlWriter.writeStartElement("BOARD");
    if (board->type() == BoardType::UsbSerial )
      xmlWriter.writeAttribute("TYPE", "USB");
    else if( board->type() == BoardType::Ethernet )
      xmlWriter.writeAttribute("TYPE", "Ethernet");
    xmlWriter.writeAttribute("LOCATION", board->key());
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement(); // board arrival/removal
  xmlWriter.writeEndDocument();
  char terminator = '\0';
  socket->write(&terminator, 1);
}

/*
  Flash requires a cross-domain policy to appease its security system.
  Just allow anybody to connect to us.

  http://kb2.adobe.com/cps/142/tn_14213.html
*/
void OscXmlClient::sendCrossDomainPolicy()
{
  xmlWriter.writeStartDocument();
  xmlWriter.writeDTD("<!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\">");

  xmlWriter.writeStartElement("cross-domain-policy");
  xmlWriter.writeStartElement("allow-access-from");
  xmlWriter.writeAttribute("domain", "*");
  xmlWriter.writeAttribute("to-ports", "*");
  xmlWriter.writeEndElement(); // allow-access-from
  xmlWriter.writeEndElement(); // cross-domain-policy

  xmlWriter.writeEndDocument();
  char terminator = '\0';
  socket->write(&terminator, 1);
}

void OscXmlClient::sendXmlPacket( const QList<OscMessage*> & messageList, const QString & srcAddress )
{
  if (!isConnected() || messageList.isEmpty())
    return;

  xmlWriter.writeStartDocument();
  xmlWriter.writeStartElement("OSCPACKET");
  xmlWriter.writeAttribute("ADDRESS", srcAddress);
  xmlWriter.writeAttribute("TIME", 0);

  foreach (OscMessage* oscMsg, messageList) {
    xmlWriter.writeStartElement("MESSAGE");
    xmlWriter.writeAttribute("NAME", oscMsg->addressPattern);
    foreach (const QVariant & d, oscMsg->data) {
      xmlWriter.writeStartElement("ARGUMENT");
      switch(d.type()) {
        case QVariant::String:
          xmlWriter.writeAttribute("TYPE", "s");
          xmlWriter.writeAttribute("VALUE", d.toString());
          break;
        case QVariant::Int:
          xmlWriter.writeAttribute("TYPE", "i");
          xmlWriter.writeAttribute("VALUE", d.toString());
          break;
        case QMetaType::Float: // QVariant doesn't have a float type
          xmlWriter.writeAttribute("TYPE", "f");
          xmlWriter.writeAttribute("VALUE", d.toString());
          break;
        case QVariant::ByteArray: {
          QString blobstring;
          unsigned char* blob = (unsigned char*)d.toByteArray().data();
          int blob_len = d.toByteArray().size();
          while (blob_len--) {
            // break each byte into 4-bit chunks so they don't get misinterpreted
            // by any casts to ASCII, etc. and send a string composed of single chars from 0-f
            blobstring.append(QString::number((*blob >> 4) & 0x0f, 16 ));
            blobstring.append(QString::number(*blob++ & 0x0f, 16 ));
          }
          xmlWriter.writeAttribute("TYPE", "b");
          xmlWriter.writeAttribute("VALUE", blobstring);
          break;
        }
        default:
          break;
      }
      xmlWriter.writeEndElement(); // ARGUMENT
    }
    xmlWriter.writeEndElement(); // MESSAGE
  }

  xmlWriter.writeEndElement(); // OSCPACKET
  xmlWriter.writeEndDocument();
  char terminator = '\0';
  socket->write(&terminator, 1);
}

/************************************************************************************

                                    XmlHandler

  When parsing an XML packet, XmlHandler will call back on start and end of packets
  so we can create OSC messages out of them.  When complete messages have been assembled,
  send them out to the appropriate boards, and print the messages to the activity window.

************************************************************************************/

XmlHandler::XmlHandler( MainWindow *mainWindow, OscXmlClient *xmlClient ) : QXmlDefaultHandler( )
{
  this->mainWindow = mainWindow;
  this->xmlClient = xmlClient;
  currentMessage = NULL;
  connect(this, SIGNAL(msg(QStringList, MsgType::Type, QString)), mainWindow, SLOT(message(QStringList, MsgType::Type, QString)));
}

bool XmlHandler::fatalError (const QXmlParseException & exception)
{
  error( exception );
  return true;
}

bool XmlHandler::error (const QXmlParseException & exception)
{
   qDebug() << tr("incoming XML error on line, %1, column %2 : %3").arg(
            exception.lineNumber()).arg(exception.columnNumber()).arg(exception.message());
   return false;
}

bool XmlHandler::startElement( const QString & namespaceURI, const QString & localName,
                        const QString & qName, const QXmlAttributes & atts )
{
  (void) namespaceURI;
  (void) qName;

  if( localName == "OSCPACKET" ) {
    currentDestination = atts.value( "ADDRESS" );
    currentPort = atts.value( "PORT" ).toInt( );
    if( currentDestination.isEmpty( ) )
      return false;
  }
  else if( localName == "MESSAGE" ) {
    currentMessage = new OscMessage( );
    currentMessage->addressPattern = atts.value( "NAME" );
  }
  else if( localName == "ARGUMENT" ) {
    QString type = atts.value( "TYPE" );
    QString val = atts.value( "VALUE" );
    if( type.isEmpty( ) || val.isEmpty( ) )
      return false;

    QVariant msgData;
    switch(type.at(0).toAscii())
    {
      case 'i':
        msgData.setValue(val.toInt());
        break;
      case 'f':
        msgData.setValue(val.toFloat());
        break;
      case 's':
        msgData.setValue(val);
        break;
      case 'b': // TODO, unpack this appropriately
        msgData.setValue(val.toAscii());
        break;
    }
    if(msgData.isValid()) currentMessage->data.append( msgData );
  }
  return true;
}

bool XmlHandler::endElement( const QString & namespaceURI, const QString & localName, const QString & qName )
{
  (void) namespaceURI;
  (void) qName;

  if( localName == "OSCPACKET" ) {
    mainWindow->newXmlPacketReceived( oscMessageList, currentDestination );
    QStringList strings;
    foreach( OscMessage* msg, oscMessageList )
      strings << msg->toString( );
    emit msg( strings, MsgType::XMLMessage, FROM_STRING);
    qDeleteAll( oscMessageList );
    oscMessageList.clear( );
    xmlClient->resetParser( );
  }
  else if( localName == "MESSAGE" )
    oscMessageList.append( currentMessage );

  return true;
}
