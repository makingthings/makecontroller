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

#include "Board.h"
#include "MsgType.h"
#include <QStringList>
#include <QList>

#define OSC_MSG_NET_FIND_SIZE   4
#define OSC_MSG_SYSINFO_A_SIZE  5
#define OSC_MSG_SYSINFO_B_SIZE  6

Board::Board(MainWindow *mw, PacketInterface* pi, OscXmlServer *oxs, BoardType::Type type, const QString & key, QListWidget* parent)
  : QListWidgetItem(parent),
    mainWindow(mw),
    packetInterface(pi),
    oscXmlServer(oxs),
    _key(key),
    _type(type)
{
  if (packetInterface)
    packetInterface->setBoard(this);

  connect(this, SIGNAL(msg(QString, MsgType::Type, QString)),
          mainWindow, SLOT(message(QString, MsgType::Type, QString)));
  connect(this, SIGNAL(msgs(QStringList, MsgType::Type, QString)),
          mainWindow, SLOT(message(QStringList, MsgType::Type, QString)), Qt::DirectConnection);
  connect(this, SIGNAL(newBoardName(QString, QString)), mainWindow, SLOT(setBoardName(QString, QString)));
}

Board::~Board()
{
  delete packetInterface;
}

QString Board::friendlyKey()
{
  QString k = _key;
  #ifdef Q_OS_WIN
  k.remove("\\\\.\\"); // in case it's above com9
  #endif
  return k;
}

QString Board::location()
{
  switch( _type ) {
    case BoardType::UsbSamba:
      return tr("Unprogrammed Board");
    case BoardType::UsbSerial:
    {
      #ifdef Q_OS_WIN
        return QString( "USB (%1)" ).arg(friendlyKey());
      #else
        return "USB";
      #endif
    }
    case BoardType::Ethernet:
      return _key;
    default:
      return QString();
  }
}

/*
 Our packet interface has received a message.
 First check if there's any info we need to use internally.
 Then, print it to the screen, and send it to the XML server.
*/
void Board::msgReceived(const QByteArray & packet)
{
  QStringList messageList;
  QList<OscMessage*> oscMessageList = osc.processPacket(packet.data(), packet.size());
  bool new_info = false;

  foreach (OscMessage *oscMsg, oscMessageList) {
    if (oscMsg->addressPattern == "/system/info-internal-a")
      new_info = extractSystemInfoA(oscMsg);

    else if (oscMsg->addressPattern == "/system/info-internal-b")
      new_info = extractSystemInfoB(oscMsg);

    else if (oscMsg->addressPattern == "/network/find")
      new_info = extractNetworkFind(oscMsg);

    else if (oscMsg->addressPattern.contains("error", Qt::CaseInsensitive))
      emit msg(oscMsg->toString(), MsgType::Warning, location());

    else
      messageList.append(oscMsg->toString());
  }

  if (messageList.isEmpty()) {
    oscXmlServer->sendPacket(oscMessageList, key());
    emit msgs(messageList, MsgType::Response, location());
  }
  if (new_info)
    emit newInfo(this);
  qDeleteAll(oscMessageList);
}

bool Board::extractSystemInfoA(OscMessage* msg)
{
  bool newInfo = false;

  if (msg->data.size() >= OSC_MSG_SYSINFO_A_SIZE) {
    if (name != msg->data.at(0).toString()) {
      name = msg->data.at(0).toString(); //name
      emit newBoardName(_key, (name + " : " + location()));
      newInfo = true;
    }
    if (serialNumber != msg->data.at(1).toString()) {
      serialNumber = msg->data.at(1).toString(); // serial number
      newInfo = true;
    }
    if (ip_address != msg->data.at(2).toString()) {
      ip_address = msg->data.at(2).toString(); // IP address
      newInfo = true;
    }
    if (firmwareVersion != msg->data.at(3).toString()) {
      firmwareVersion = msg->data.at(3).toString();
      newInfo = true;
    }
    if (freeMemory != msg->data.at(4).toString()) {
      freeMemory = msg->data.at(4).toString();
      newInfo = true;
    }
  }
  return newInfo;
}

bool Board::extractSystemInfoB(OscMessage* msg)
{
  bool newInfo = false;

  if (msg->data.size() >= OSC_MSG_SYSINFO_B_SIZE) {
    if (dhcp != msg->data.at(0).toInt()) {
      dhcp = msg->data.at(0).toInt();
      newInfo = true;
    }
    if (webserver != msg->data.at(1).toInt()) {
      webserver = msg->data.at(1).toInt();
      newInfo = true;
    }
    if (gateway != msg->data.at(2).toString()) {
      gateway = msg->data.at(2).toString();
      newInfo = true;
    }
    if (netMask != msg->data.at(3).toString()) {
      netMask = msg->data.at(3).toString();
      newInfo = true;
    }
    if (udp_listen_port != msg->data.at(4).toString()) {
      udp_listen_port = msg->data.at(4).toString();
      newInfo = true;
    }
    if (udp_send_port != msg->data.at(5).toString()) {
      udp_send_port = msg->data.at(5).toString();
      newInfo = true;
    }
  }
  return newInfo;
}

bool Board::extractNetworkFind(OscMessage* msg)
{
  bool newInfo = false;

  if (msg->data.size() >= OSC_MSG_NET_FIND_SIZE) {
    if (ip_address != msg->data.at(0).toString()) {
      ip_address = msg->data.at(0).toString(); // IP address
      newInfo = true;
    }
    if (udp_listen_port != msg->data.at(1).toString()) {
      udp_listen_port = msg->data.at(1).toString();
      newInfo = true;
    }
    if (udp_send_port != msg->data.at(2).toString()) {
      udp_send_port = msg->data.at(2).toString();
      newInfo = true;
    }
    if (name != msg->data.at(3).toString()) {
      name = msg->data.at(3).toString();
      emit newBoardName(_key, (name + " : " + location()));
      newInfo = true;
    }
  }
  return newInfo;
}

/*
  Send a message to this board.
  Create an appropriate message and send it out
  through our packet interface.
*/
void Board::sendMessage(const QString & rawMessage)
{
  if (packetInterface && !rawMessage.isEmpty()) {
    QByteArray packet = OscMessage(rawMessage).toByteArray();
    if (!packet.isEmpty())
      packetInterface->sendPacket(packet.data(), packet.size());
  }
}

void Board::sendMessage(const QList<OscMessage*> & messageList)
{
  if (packetInterface && messageList.count()) {
    QByteArray packet = osc.createPacket(messageList);
    if (!packet.isEmpty())
      packetInterface->sendPacket(packet.data(), packet.size());
  }
}

void Board::sendMessage(const QStringList & messageList)
{
  if (packetInterface && messageList.count()) {
    QByteArray packet = osc.createPacket(messageList);
    if (!packet.isEmpty())
      packetInterface->sendPacket(packet.data(), packet.size());
  }
}
