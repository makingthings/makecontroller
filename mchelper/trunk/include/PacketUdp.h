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

#ifndef PACKETUDP_H
#define PACKETUDP_H

#include <QUdpSocket>
#include <QHostAddress>
#include <QHostInfo>
#include <QList>

#include "Board.h"
#include "MsgType.h"
#include "PacketInterface.h"

#define PING_TIMEOUT 3000

class PacketUdp : public QUdpSocket, public PacketInterface
{
  Q_OBJECT

public:
  PacketUdp(QHostAddress remoteAddress, int send_port);

  // From PacketInterface
  bool sendPacket( const char* packet, int length );
  QString key( void );
  void newMessage( const QByteArray & message );
  void setBoard(Board *b) {this->board = b;}

  void setSendPort(int port) {send_port = port;}

signals:
  void msg(QString message, MsgType::Type type, QString from);
  void timeout(QString key);

private slots:
  void onTimeOut( );

private:
  Board* board;
  QHostAddress remoteAddress;
  int send_port;
  QTimer pingTimer;
};

#endif



