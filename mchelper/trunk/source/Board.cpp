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

QString Board::location( )
{
	switch( _type )
	{
		case BoardType::UsbSamba:
			return tr("Unprogrammed Board");
		case BoardType::UsbSerial:
		{
		  QString k = _key;
		  if( k.startsWith("\\\\.\\"))
		     k.remove("\\\\.\\");
		  return QString( "USB (%1)" ).arg(k);
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
void Board::msgReceived(QByteArray packet)
{
	QStringList messageList;
	QList<OscMessage*> oscMessageList = osc.processPacket( packet.data(), packet.size() );
  bool new_info = false;
		
	foreach(OscMessage *oscMsg, oscMessageList)
	{
		if( oscMsg->addressPattern == QString( "/system/info-internal-a" ) )
      new_info = extractSystemInfoA( oscMsg );
			
		else if( oscMsg->addressPattern == QString( "/system/info-internal-b" ) )
			new_info = extractSystemInfoB( oscMsg );
    
    else if( oscMsg->addressPattern == QString( "/network/find" ) )
			new_info = extractNetworkFind( oscMsg );
			
		else if( oscMsg->addressPattern.contains( "error", Qt::CaseInsensitive ) )
			emit msg( oscMsg->toString(), MsgType::Warning, location() );
      
		else
			messageList.append( oscMsg->toString( ) );
	}
	if( messageList.count( ) > 0 )
	{
		oscXmlServer->sendPacket( oscMessageList, key() );
		emit msgs( messageList, MsgType::Response, location() );
	}
  if(new_info)
    emit newInfo(this);
	qDeleteAll( oscMessageList );
}

bool Board::extractSystemInfoA( OscMessage* msg )
{
	QList<OscData*> msgData = msg->data;
  bool newInfo = false;
	
	for( int i = 0; i < msg->data.count( ); i++ )
	{
		if( msgData.at( i ) == 0 )
			break;
		switch( i ) // we're counting on the board to send the pieces of data in this order
		{
			case 0:
        if( name != msgData.at( i )->s() )
				{
          name = msgData.at( i )->s(); //name
					emit newBoardName(_key, name);
          newInfo = true;
				}
				break;
			case 1:
        if(serialNumber != QString::number( msgData.at(i)->i() ))
        {
          serialNumber = QString::number( msgData.at(i)->i() ); // serial number
          newInfo = true;
        }
				break;
			case 2:
        if(ip_address != msgData.at( i )->s())
        {
          ip_address = msgData.at( i )->s(); // IP address
          newInfo = true;
        }
				break;
			case 3:
        if(firmwareVersion != msgData.at(i)->s())
        {
          firmwareVersion = msgData.at(i)->s();
          newInfo = true;
        }
				break;
			case 4:
        if(freeMemory != QString::number( msgData.at(i)->i() ))
        {
          freeMemory = QString::number( msgData.at(i)->i() );
          newInfo = true;
        }
				break;
		}
	}
  return newInfo;
}

bool Board::extractSystemInfoB( OscMessage* msg )
{
	QList<OscData*> msgData = msg->data;
  bool newInfo = false;
	
	for( int j = 0; j < msg->data.count( ); j++ )
	{
		if( msgData.at( j ) == 0 )
			break;
		switch( j ) // we're counting on the board to send the pieces of data in this order
		{
			case 0:
        if(dhcp != msgData.at( j )->i())
        {
          dhcp = msgData.at( j )->i();
          newInfo = true;
        }
				break;
			case 1:
        if(webserver != msgData.at( j )->i())
        {
          webserver = msgData.at( j )->i();
          newInfo = true;
        }
				break;
			case 2:
        if(gateway != msgData.at( j )->s())
        {
          gateway = msgData.at( j )->s();
          newInfo = true;
        }
				break;
			case 3:
        if(netMask != msgData.at( j )->s())
        {
          netMask = msgData.at( j )->s();
          newInfo = true;
        }
				break;
			case 4:
        if(udp_listen_port != QString::number( msgData.at( j )->i() ))
        {
          udp_listen_port = QString::number( msgData.at( j )->i() );
          newInfo = true;
        }
				break;
			case 5:
        if(udp_send_port != QString::number( msgData.at( j )->i() ))
				{
          udp_send_port = QString::number( msgData.at( j )->i() );
          newInfo = true;
        }
				break;
		}
	}
  return newInfo;
}

bool Board::extractNetworkFind( OscMessage* msg )
{
	QList<OscData*> msgData = msg->data;
	bool newInfo = false;
	
	for( int j = 0; j < msgData.count( ); j++ )
	{
		if( msgData.at( j ) == 0 )
			break;
		switch( j ) // we're counting on the board to send the pieces of data in this order
		{
			case 0:
        if( ip_address != QString( msgData.at( j )->s() ) )
				{
          ip_address = QString( msgData.at( j )->s() ); // IP address
					newInfo = true;
				}
				break;
			case 1:
        if( udp_listen_port != QString::number( msgData.at( j )->i() ) )
				{
          udp_listen_port = QString::number( msgData.at( j )->i() );
					newInfo = true;
				}
				break;
			case 2:
        if( udp_send_port != QString::number( msgData.at( j )->i() ) )
				{
          udp_send_port = QString::number( msgData.at( j )->i() );
					newInfo = true;
				}
				break;
			case 3:
        if( name != QString( msgData.at( j )->s() ) )
				{
          name = QString( msgData.at( j )->s() );
          emit newBoardName(_key, name);
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
void Board::sendMessage( QString rawMessage )
{
	if( packetInterface && !rawMessage.isEmpty() )
	{		
		QByteArray packet = osc.createPacket( rawMessage );
		if( !packet.isEmpty( ) )
			packetInterface->sendPacket( packet.data( ), packet.size( ) );
	}
}

void Board::sendMessage( QList<OscMessage*> messageList )
{	
	if( packetInterface && messageList.count() )
  {
    QByteArray packet = osc.createPacket( messageList );
    if( !packet.isEmpty( ) )
      packetInterface->sendPacket( packet.data( ), packet.size( ) );
  }
}

void Board::sendMessage( QStringList messageList )
{
	if( packetInterface && messageList.count() )
  {
    QByteArray packet = osc.createPacket( messageList );
    if( !packet.isEmpty( ) )
      packetInterface->sendPacket( packet.data( ), packet.size( ) );
  }
}

