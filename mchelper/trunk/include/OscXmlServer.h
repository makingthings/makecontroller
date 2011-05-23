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

#ifndef OSC_XML_SERVER_H
#define OSC_XML_SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QXmlSimpleReader>
#include <QXmlDefaultHandler>
#include <QXmlStreamWriter>
#include <QThread>

#include "MainWindow.h"
#include "MsgType.h"
#include "Osc.h"
#include "Board.h"

#ifdef MCHELPER_TEST_SUITE
#include "TestXmlServer.h";
#endif

class OscXmlServer;
class OscXmlClient;
class OscMessage;
class MainWindow;
class Board;

class XmlHandler : public QObject, public QXmlDefaultHandler
{
  Q_OBJECT
  public:
    XmlHandler( MainWindow *mainWindow, OscXmlClient *xmlClient );
    bool endElement( const QString & namespaceURI, const QString & localName, const QString & qName );
    bool startElement( const QString & namespaceURI, const QString & localName,
                        const QString & qName, const QXmlAttributes & atts );
    bool error (const QXmlParseException & exception);
    bool fatalError (const QXmlParseException & exception);

  signals:
    void msg(const QStringList & msg, MsgType::Type, const QString & from);

  private:
    MainWindow *mainWindow;
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
    OscXmlClient( QTcpSocket *socket, MainWindow *mainWindow, QObject *parent = 0 );
    ~OscXmlClient( ) { }
    void run();
    void sendCrossDomainPolicy();
    void resetParser() { lastParseComplete = true; }

  public slots:
    void boardListUpdate( const QList<Board*> & boardList, bool arrived );
    void boardInfoUpdate( Board* board );
    void wroteBytes( qint64 bytes );
    void sendXmlPacket( const QList<OscMessage*> & messageList, const QString & srcAddress );

  signals:
    void msg(const QString & msg, MsgType::Type, const QString & from);

  private:
    int socketDescriptor;
    MainWindow *mainWindow;
    bool lastParseComplete;
    QXmlStreamWriter xmlWriter;
    QXmlSimpleReader xml;
    QXmlInputSource xmlInput;
    XmlHandler *handler;
    QTcpSocket *socket;
    QList<QString> uiMessages;
    QString peerAddress;
    bool shuttingDown;

    bool isConnected( );

  private slots:
    void processData( );
    void disconnected( );

  #ifdef MCHELPER_TEST_SUITE
  friend class TestXmlServer;
  #endif
};

class OscXmlServer : public QTcpServer
{
  Q_OBJECT
  public:
    OscXmlServer( MainWindow *mainWindow, QObject *parent = 0 );
    bool setListenPort( int port, bool announce = true );
    void sendPacket(const QList<OscMessage*> & msgs, const QString & srcAddress);
    void sendBoardListUpdate(QList<Board*> boardList, bool arrived);

  signals:
    void msg(QString msg, MsgType::Type, QString from);
    void newXmlPacket(const QList<OscMessage*> & messageList, const QString & srcAddress);
    void boardInfoUpdate(Board *board);
    void boardListUpdated(QList<Board*> boardList, bool arrived);

  private slots:
    void openNewConnection( );

  private:
    MainWindow *mainWindow;
    int listenPort;

  #ifdef MCHELPER_TEST_SUITE
  friend class TestXmlServer;
  #endif
};

#endif // OSC_XML_SERVER_H
