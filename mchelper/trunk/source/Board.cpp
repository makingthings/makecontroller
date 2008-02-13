/*********************************************************************************

 Copyright 2006-2007 MakingThings

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
#include <QStringList>
#include <QList>

Board::Board( MessageInterface* messageInterface, McHelperWindow* mainWindow, QApplication* application )
{
  osc = new Osc( );
  this->messageInterface = messageInterface;
  this->mainWindow = mainWindow;
  this->application = application;
  packetInterface = NULL;
}

Board::~Board( )
{
  delete osc; 
}

void Board::setPacketInterface( PacketInterface* packetInterface )
{
	this->packetInterface = packetInterface;
	osc->setInterfaces( messageInterface );
	osc->setPreamble( packetInterface->location( ) );
	packetInterface->setPacketReadyInterface( this );
	packetInterface->open( );
}

void Board::setUploaderThread( UploaderThread* uploaderThread )
{
	this->uploaderThread = uploaderThread;
}

bool Board::setBinFileName( char* filename )
{
	if( type != Board::UsbSamba )
		return false;
	uploaderThread->setBinFileName( filename );
		return true;
}

QString Board::locationString( )
{
	switch( type )
	{
		case Board::UsbSamba:
			return QString( "Samba" );
		case Board::UsbSerial:
		{
			#ifdef Q_WS_WIN
				return QString( "USB (%1)" ).arg(location);
			#else
				return QString( "USB" );
			#endif
		}
		case Board::Udp:
			return location;
		default:
			return QString( "" );
	}
}

void Board::flash( )
{
	if( type != Board::UsbSamba )
		return;
	//if ( uploaderThread->isRunning() )
    	//return;
	uploaderThread->start( );
}

void Board::packetWaiting( )
{
	QStringList messageList;
	QByteArray packet;
	packet.resize( packetInterface->pendingPacketSize( ) );
	packetInterface->receivePacket( packet.data( ), packet.size( ) );
	QList<OscMessage*> oscMessageList = osc->processPacket( packet.data(), packet.size() );
	
	int messageCount = oscMessageList.size( ), i;
	bool newSysInfo = false;
	
	for( i = 0; i < messageCount; i++ )
	{
		OscMessage *msg = oscMessageList.at(i);
		if( msg->addressPattern == QString( "/system/info-internal-a" ) )
      newSysInfo = extractSystemInfoA( oscMessageList.at(i) );
			
		else if( msg->addressPattern == QString( "/system/info-internal-b" ) )
			newSysInfo = extractSystemInfoB( oscMessageList.at(i) );
			
		else if( msg->addressPattern == QString( "/network/find" ) )
			newSysInfo = extractNetworkFind( oscMessageList.at(i) );
			
		else if( msg->addressPattern.contains( "error", Qt::CaseInsensitive ) )
			messageInterface->messageThreadSafe( msg->toString( ), MessageEvent::Warning, locationString( ) );
		else
			messageList.append( msg->toString( ) );
	}
	if( messageList.count( ) > 0 )
	{
		mainWindow->sendXmlPacket( oscMessageList, key );
		messageInterface->messageThreadSafe( messageList, MessageEvent::Response, locationString( ) );
	}
		
	if( newSysInfo )
	{
		mainWindow->setBoardName( key, QString( "%1 : %2" ).arg(name).arg(locationString()) );
		mainWindow->updateSummaryInfo( );
		mainWindow->xmlServerBoardInfoUpdate( this );
	}
	//qDeleteAll( oscMessageList );
}

bool Board::extractSystemInfoA( OscMessage* msg )
{
	QList<OscMessageData*> msgData = msg->data;
	int dataCount = msg->data.count( );
	int i;
	bool newInfo = false;
	
	for( i = 0; i < dataCount; i++ )
	{
		if( msgData.at( i ) == 0 )
			break;
		switch( i ) // we're counting on the board to send the pieces of data in this order
		{
			case 0:
				if( msgData.at( i )->s == 0 ) break;
				if( name != QString( msgData.at( i )->s ) )
				{
					name = QString( msgData.at( i )->s ); //name
					newInfo = true;
				}
				break;
			case 1:
				if( serialNumber != QString::number( msgData.at( i )->i ) )
				{
					serialNumber = QString::number( msgData.at( i )->i ); // serial number
					newInfo = true; 
				}
				break;
			case 2:
				if( msgData.at( i )->s == 0 ) break;
				if( ip_address != QString( msgData.at( i )->s ) )
				{
					ip_address = QString( msgData.at( i )->s ); // IP address
					newInfo = true;
				}
				break;
			case 3:
				if( msgData.at( i )->s == 0 ) break;
				if( firmwareVersion != QString( msgData.at( i )->s ) )
				{
					firmwareVersion = QString( msgData.at( i )->s );
					newInfo = true;
				}
				break;
			case 4:
				if( freeMemory != QString::number( msgData.at( i )->i ) )
				{
					freeMemory = QString::number( msgData.at( i )->i );
					newInfo = true;
				}
				break;
		}
	}
	return newInfo;
}

bool Board::extractSystemInfoB( OscMessage* msg )
{
	QList<OscMessageData*> msgData = msg->data;
	int dataCount = msg->data.count( );
	int j;
	bool newInfo = false;
	
	for( j = 0; j < dataCount; j++ )
	{
		if( msgData.at( j ) == 0 )
			break;
		switch( j ) // we're counting on the board to send the pieces of data in this order
		{
			case 0:
				if( dhcp != msgData.at( j )->i )
				{
					dhcp = msgData.at( j )->i;
					newInfo = true;
				}
				break;
			case 1:
				if( webserver != msgData.at( j )->i )
				{
					webserver = msgData.at( j )->i;
					newInfo = true;
				}
				break;
			case 2:
				if( msgData.at( j )->s == 0 ) break;
				if( gateway != QString( msgData.at( j )->s ) )
				{
					gateway = QString( msgData.at( j )->s );
					newInfo = true;
				}
				break;
			case 3:
				if( msgData.at( j )->s == 0 ) break;
				if( netMask != QString( msgData.at( j )->s ) )
				{
					netMask = QString( msgData.at( j )->s );
					newInfo = true;
				}
				break;
			case 4:
				if( udp_listen_port != QString::number( msgData.at( j )->i ) )
				{
					udp_listen_port = QString::number( msgData.at( j )->i );
					newInfo = true;
				}
				break;
			case 5:
				if( udp_send_port != QString::number( msgData.at( j )->i ) )
				{
					udp_send_port = QString::number( msgData.at( j )->i );
					newInfo = true;
				}
				break;
		}
	}
	return newInfo;
}

bool Board::extractNetworkFind( OscMessage* msg )
{
	QList<OscMessageData*> msgData = msg->data;
	int dataCount = msg->data.count( );
	bool newInfo = false;
	
	for( int j = 0; j < dataCount; j++ )
	{
		if( msgData.at( j ) == 0 )
			break;
		switch( j ) // we're counting on the board to send the pieces of data in this order
		{
			case 0:
				if( msgData.at( j )->s == 0 ) break;
				if( ip_address != QString( msgData.at( j )->s ) )
				{
					ip_address = QString( msgData.at( j )->s ); // IP address
					newInfo = true;
				}
				break;
			case 1:
				if( udp_listen_port != QString::number( msgData.at( j )->i ) )
				{
					udp_listen_port = QString::number( msgData.at( j )->i );
					newInfo = true;
				}
				break;
			case 2:
				if( udp_send_port != QString::number( msgData.at( j )->i ) )
				{
					udp_send_port = QString::number( msgData.at( j )->i );
					newInfo = true;
				}
				break;
			case 3:
				if( msgData.at( j )->s == 0 ) break;
				if( name != QString( msgData.at( j )->s ) )
				{
					name = QString( msgData.at( j )->s );
					newInfo = true;
				}
				break;
		}
	}
	return newInfo;
}

void Board::sendMessage( QString rawMessage )
{
	if( packetInterface == NULL || !packetInterface->isOpen( ) )
		return;
	else
	{		
		QByteArray packet = osc->createPacket( rawMessage );
		if( !packet.isEmpty( ) )
			packetInterface->sendPacket( packet.data( ), packet.size( ) );
	}
}

void Board::sendMessage( QList<OscMessage*> messageList )
{	
	if( packetInterface == NULL || !packetInterface->isOpen( ) )
		return;
	if( messageList.count( ) > 0 )
	{		
		QByteArray packet = osc->createPacket( messageList );
		if( !packet.isEmpty( ) )
			packetInterface->sendPacket( packet.data( ), packet.size( ) );
	}
}

void Board::sendMessage( QStringList messageList )
{
	if( packetInterface == NULL || !packetInterface->isOpen( ) )
		return;
	if( messageList.count( ) > 0 )
	{		
		QByteArray packet = osc->createPacket( messageList );
		if( !packet.isEmpty( ) )
			packetInterface->sendPacket( packet.data( ), packet.size( ) );
	}
}

