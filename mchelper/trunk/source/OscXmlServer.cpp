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

OscXmlServer::OscXmlServer( MainWindow *mainWindow, QObject *parent ) : QTcpServer( parent )
{
  this->mainWindow = mainWindow;
  connect( this, SIGNAL( newConnection() ), this, SLOT( openNewConnection( ) ) );
  connect(this, SIGNAL(msg(QString, MsgType::Type, QString)), mainWindow, SLOT(message(QString, MsgType::Type, QString)));
  QSettings settings;
  setListenPort( settings.value("xml_listen_port", DEFAULT_XML_LISTEN_PORT).toInt(), false );
}

/*
  Called when a new TCP connection has been made.
  Create a new thread for the client, and set it up.
*/
void OscXmlServer::openNewConnection( )
{
  OscXmlClient *client = new OscXmlClient( nextPendingConnection( ), mainWindow );
  client->sendCrossDomainPolicy();
  connect( client, SIGNAL(finished()), client, SLOT(deleteLater()));
  connect(this, SIGNAL(newXmlPacket(QList<OscMessage*>, QString)), client, SLOT(sendXmlPacket(QList<OscMessage*>, QString)));
  connect(this, SIGNAL(boardInfoUpdate(Board*)), client, SLOT(boardInfoUpdate(Board*)));
  connect(this, SIGNAL(boardListUpdated(QList<Board*>, bool)), client, SLOT(boardListUpdate(QList<Board*>, bool)));
  client->start( );

  // tell Flash about the boards we have connected
  client->boardListUpdate(mainWindow->getConnectedBoards( ), true);
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
OscXmlClient::OscXmlClient( QTcpSocket *socket, MainWindow *mainWindow, QObject *parent )
  : QThread(parent)
{
  this->mainWindow = mainWindow;
  this->socket = socket;
  qRegisterMetaType<MsgType::Type>("MsgType::Type");
  connect(this, SIGNAL(msg(QString, MsgType::Type, QString)), mainWindow, SLOT(message(QString, MsgType::Type, QString)));
  connect(mainWindow, SIGNAL(boardInfoUpdate(Board*)), this, SLOT(boardInfoUpdate(Board*)));
  handler = new XmlHandler( mainWindow, this );
  xml.setContentHandler( handler );
  xml.setErrorHandler( handler );
  lastParseComplete = true;
  socket = NULL;
  shuttingDown = false;
}

/*
  This is the main loop for the separate thread that each TCP client sits in.
  Set up our connections and wait around for data.
*/
void OscXmlClient::run( )
{
  // these connections need to be direct since we have a pointer to the tcpsocket in our class
  // which means that it will live by default in the server thread (the main GUI thread),
  // and we want to process in our own thread
  connect( socket, SIGNAL(readyRead()), this, SLOT(processData()), Qt::DirectConnection);
  connect( socket, SIGNAL(disconnected()), this, SLOT(disconnected()), Qt::DirectConnection);
  //connect( socket, SIGNAL(bytesWritten(qint64)), this, SLOT(wroteBytes(qint64)), Qt::DirectConnection);
  qRegisterMetaType< QList<OscMessage*> >("QList<OscMessage*>");

  peerAddress = socket->peerAddress( ).toString( );
  emit msg( tr("New connection from peer at %1").arg(peerAddress), MsgType::Notice, FROM_STRING );
  exec( ); // run the thread, listening for and sending messages, until we call exit( )
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
  if(!board)
    return;
  QDomDocument doc;
  QDomElement boardUpdate = doc.createElement( "BOARD_INFO" );
  doc.appendChild( boardUpdate );

  QDomElement boardElement = doc.createElement( "BOARD" );
  boardElement.setAttribute( "LOCATION", board->key() );
  boardElement.setAttribute( "NAME", board->name );
  boardElement.setAttribute( "SERIALNUMBER", board->serialNumber );
  boardUpdate.appendChild( boardElement );

  writeXmlDoc( doc );
}

/*
  Called when boards have arrived or been removed.
  Pass the info on to our TCP peer.
*/
void OscXmlClient::boardListUpdate( const QList<Board*> & boardList, bool arrived )
{
  if(!boardList.count())
    return;
  QDomDocument doc;
  QDomElement boardUpdate;
  if( arrived )
    boardUpdate = doc.createElement( "BOARD_ARRIVAL" );
  else
    boardUpdate = doc.createElement( "BOARD_REMOVAL" );

  doc.appendChild( boardUpdate );
  foreach( Board *board, boardList) {
    QDomElement boardElem = doc.createElement( "BOARD" );
    if( board->type() == BoardType::UsbSerial )
      boardElem.setAttribute( "TYPE", "USB" );
    else if( board->type() == BoardType::Ethernet )
      boardElem.setAttribute( "TYPE", "Ethernet" );
    boardElem.setAttribute( "LOCATION", board->key() );
    boardUpdate.appendChild( boardElem );
  }
  writeXmlDoc( doc );
}

void OscXmlClient::writeXmlDoc( const QDomDocument & doc )
{
  if( isConnected( ) )
  {
    //qDebug("sending %s", qPrintable(doc.toString(2)));
    socket->write( doc.toByteArray(0).append( '\0' ) ); // Flash wants XML followed by a zero byte
  }
}

/*
  Flash requires a cross-domain policy to appease its security system.
  Just allow anybody to connect to us.
*/
void OscXmlClient::sendCrossDomainPolicy()
{
  QDomImplementation impl;
  QDomDocumentType doctype = impl.createDocumentType("cross-domain-policy", QString(), "/xml/dtds/cross-domain-policy.dtd");
  QDomDocument doc(doctype);
  QDomProcessingInstruction instr = doc.createProcessingInstruction("xml","version=\"1.0\"");
  doc.appendChild(instr);
  QDomElement pol = doc.createElement("cross-domain-policy");
  doc.appendChild( pol );
  QDomElement access = doc.createElement("allow-access-from");
  access.setAttribute( "domain", "*" );
  access.setAttribute( "to-ports", "*" );
  pol.appendChild( access );
  writeXmlDoc(doc);
}

void OscXmlClient::sendXmlPacket( const QList<OscMessage*> & messageList, const QString & srcAddress )
{
  if( !isConnected( ) || messageList.count() < 1 )
    return;

  QDomDocument doc;
  QDomElement oscPacket = doc.createElement( "OSCPACKET" );
  oscPacket.setAttribute( "ADDRESS", srcAddress );
  oscPacket.setAttribute( "TIME", 0 );
  doc.appendChild( oscPacket );

  foreach( OscMessage* oscMsg, messageList )
  {
    QDomElement msg = doc.createElement( "MESSAGE" );
    msg.setAttribute( "NAME", oscMsg->addressPattern );
    oscPacket.appendChild( msg );

    foreach( QVariant d, oscMsg->data ) {
      QDomElement argument = doc.createElement( "ARGUMENT" );
      switch( d.type() )
      {
        case QVariant::String:
          argument.setAttribute( "TYPE", "s" );
          argument.setAttribute( "VALUE", d.toString() );
          break;
        case QVariant::Int:
          argument.setAttribute( "TYPE", "i" );
          argument.setAttribute( "VALUE", d.toString() );
          break;
        case QMetaType::Float: // QVariant doesn't have a float type
          argument.setAttribute( "TYPE", "f" );
          argument.setAttribute( "VALUE", d.toString() );
          break;
        case QVariant::ByteArray:
        {
          QString blobstring;
          unsigned char* blob = (unsigned char*)d.toByteArray().data();
          int blob_len = d.toByteArray().size();
          while( blob_len-- ) {
            // break each byte into 4-bit chunks so they don't get misinterpreted
            // by any casts to ASCII, etc. and send a string composed of single chars from 0-f
            blobstring.append( QString::number( (*blob >> 4) & 0x0f, 16 ) );
            blobstring.append( QString::number(*blob++ & 0x0f, 16 ) );
          }
          argument.setAttribute( "TYPE", "b" );
          argument.setAttribute( "VALUE", blobstring );
          break;
        }
        default: break;
      }
      msg.appendChild( argument );
    }
  }
  writeXmlDoc( doc );
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








