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

Board::Board(MainWindow *mw, PacketInterface* pi, OscXmlServer *oxs, BoardType::Type type, QString key) : QListWidgetItem()
{
  mainWindow = mw;
  packetInterface = pi;
  oscXmlServer = oxs;
  _type = type;
  _key = key;
  if(packetInterface)
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

QString Board::friendlyKey( )
{
  QString k = _key;
  #ifdef Q_OS_WIN
  k.remove("\\\\.\\"); // in case it's above com9
  #endif
  return k;
}

QString Board::location( )
{
  switch( _type )
  {
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
  QList<OscMessage*> oscMessageList = osc.processPacket( packet.data(), packet.size() );
  bool new_info = false;

  foreach(OscMessage *oscMsg, oscMessageList)
  {
    if( oscMsg->addressPattern == "/system/info-internal-a" )
      new_info = extractSystemInfoA( oscMsg );

    else if( oscMsg->addressPattern == "/system/info-internal-b" )
      new_info = extractSystemInfoB( oscMsg );

    else if( oscMsg->addressPattern == "/network/find" )
      new_info = extractNetworkFind( oscMsg );

    else if( oscMsg->addressPattern.contains( "error", Qt::CaseInsensitive ) )
      emit msg( oscMsg->toString(), MsgType::Warning, location() );

    else
      messageList.append( oscMsg->toString( ) );
  }
  if( messageList.count( ) > 0 ) {
    oscXmlServer->sendPacket( oscMessageList, key() );
    emit msgs( messageList, MsgType::Response, location() );
  }
  if(new_info)
    emit newInfo(this);
  qDeleteAll( oscMessageList );
}

bool Board::extractSystemInfoA( OscMessage* msg )
{
  QList<OscData*>* msgData = &msg->data;
  bool newInfo = false;

  int datalen = msgData->size();
  for( int i = 0; i < datalen; i++ )
  {
    OscData* data = msgData->at( i );
    if( !data )
      break;
    switch( i ) // we're counting on the board to send the pieces of data in this order
    {
      case 0:
        if( name != data->s() ) {
          name = data->s(); //name
          emit newBoardName(_key, (name + " : " + location()));
          newInfo = true;
        }
        break;
      case 1:
        if(serialNumber != data->s()) {
          serialNumber = data->s(); // serial number
          newInfo = true;
        }
        break;
      case 2:
        if(ip_address != data->s()) {
          ip_address = data->s(); // IP address
          newInfo = true;
        }
        break;
      case 3:
        if(firmwareVersion != data->s()) {
          firmwareVersion = data->s();
          newInfo = true;
        }
        break;
      case 4:
        if(freeMemory != data->s()) {
          freeMemory = data->s();
          newInfo = true;
        }
        break;
    }
  }
  return newInfo;
}

bool Board::extractSystemInfoB( OscMessage* msg )
{
  QList<OscData*>* msgData = &msg->data;
  bool newInfo = false;

  int datalen = msgData->size();
  for( int j = 0; j < datalen; j++ )
  {
    OscData* data = msgData->at( j );
    if(!data)
      break;
    switch( j ) // we're counting on the board to send the pieces of data in this order
    {
      case 0:
        if(dhcp != data->i()) {
          dhcp = data->i();
          newInfo = true;
        }
        break;
      case 1:
        if(webserver != data->i()) {
          webserver = data->i();
          newInfo = true;
        }
        break;
      case 2:
        if(gateway != data->s()) {
          gateway = data->s();
          newInfo = true;
        }
        break;
      case 3:
        if(netMask != data->s()) {
          netMask = data->s();
          newInfo = true;
        }
        break;
      case 4:
        if(udp_listen_port != data->s()) {
          udp_listen_port = data->s();
          newInfo = true;
        }
        break;
      case 5:
        if(udp_send_port != data->s()) {
          udp_send_port = data->s();
          newInfo = true;
        }
        break;
    }
  }
  return newInfo;
}

bool Board::extractNetworkFind( OscMessage* msg )
{
  QList<OscData*>* msgData = &msg->data;
  bool newInfo = false;

  int datalen = msgData->size();
  for( int j = 0; j < datalen; j++ )
  {
    OscData* data = msgData->at( j );
    if( !data )
      break;
    switch( j ) // we're counting on the board to send the pieces of data in this order
    {
      case 0:
        if( ip_address != data->s() ) {
          ip_address = data->s(); // IP address
          newInfo = true;
        }
        break;
      case 1:
        if( udp_listen_port != data->s() ) {
          udp_listen_port = data->s();
          newInfo = true;
        }
        break;
      case 2:
        if( udp_send_port != data->s() ) {
          udp_send_port = data->s();
          newInfo = true;
        }
        break;
      case 3:
        if( name != data->s() ) {
          name = data->s();
          emit newBoardName(_key, (name + " : " + location()));
          newInfo = true;
        }
        break;
    }
  }
  return newInfo;
}

/*
  Send a message to this board.
  Create an appropriate message and send it out
  through our packet interface.
*/
void Board::sendMessage( const QString & rawMessage )
{
  if( packetInterface && !rawMessage.isEmpty() ) {
    QByteArray packet = osc.createPacket( rawMessage );
    if( !packet.isEmpty( ) )
      packetInterface->sendPacket( packet.data( ), packet.size( ) );
  }
}

void Board::sendMessage( const QList<OscMessage*> & messageList )
{
  if( packetInterface && messageList.count() ) {
    QByteArray packet = osc.createPacket( messageList );
    if( !packet.isEmpty( ) )
      packetInterface->sendPacket( packet.data( ), packet.size( ) );
  }
}

void Board::sendMessage( const QStringList & messageList )
{
  if( packetInterface && messageList.count() ) {
    QByteArray packet = osc.createPacket( messageList );
    if( !packet.isEmpty( ) )
      packetInterface->sendPacket( packet.data( ), packet.size( ) );
  }
}

