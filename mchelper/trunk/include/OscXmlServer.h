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
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QThread>

#include "MainWindow.h"
#include "MsgType.h"
#include "Osc.h"
#include "Board.h"

#ifdef MCHELPER_TEST_SUITE
#include "TestXmlServer.h";
#endif

class OscXmlServer;
class OscMessage;
class MainWindow;
class Board;

class OscXmlClient : public QThread
{
  Q_OBJECT
  public:
    OscXmlClient(int socketDescriptor, MainWindow *mainWindow, QObject *parent = 0);
    ~OscXmlClient( ) { }
    void run();
    void sendCrossDomainPolicy();

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
    QXmlStreamWriter xmlWriter;
    QXmlStreamReader xmlReader;
    QTcpSocket *socket;
    bool shuttingDown;

    OscMessage* currentMessage;
    QString currentDestination;
    int currentPort;
    QList<OscMessage*> oscMessageList;

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

  protected:
    void incomingConnection(int handle);

  private:
    MainWindow *mainWindow;
    int listenPort;

  #ifdef MCHELPER_TEST_SUITE
  friend class TestXmlServer;
  #endif
};

#endif // OSC_XML_SERVER_H
