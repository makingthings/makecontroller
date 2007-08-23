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
	//connect( &messagePostTimer, SIGNAL(timeout()), this, SLOT( postMessagesToUI() ) );
	//messagePostTimer.start( 25 );
}

Board::~Board( )
{
  delete osc; 
}

void Board::setPacketInterface( PacketInterface* packetInterface )
{
	this->packetInterface = packetInterface;
	osc->setInterfaces( packetInterface, messageInterface, application );
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
	QList<OscMessage*> oscMessageList;
	QStringList messageList;
	osc->receive( &oscMessageList );
	
	int messageCount = oscMessageList.size( ), i;
	bool sysInfoReceived = false;
	
	for( i = 0; i < messageCount; i++ )
	{
		QString msg = oscMessageList.at(i)->toString( );
		if( strcmp( oscMessageList.at(i)->address, "/system/info-internal-a" ) == 0 )
		{ 
      extractSystemInfoA( oscMessageList.at(i) );
			sysInfoReceived = true;
		}
		else if( strcmp( oscMessageList.at(i)->address, "/system/info-internal-b" ) == 0 )
		{ 
      extractSystemInfoB( oscMessageList.at(i) );
			sysInfoReceived = true;
		}
		else if( strcmp( oscMessageList.at(i)->address, "/network/find" ) == 0 )
		{
			if( oscMessageList.at(i)->data.count( ) > 0 ) // might be another mchelper sending out pings
			{
				ip_address = QString( oscMessageList.at(i)->data.at( 0 )->s ); // IP address
				udp_listen_port = QString::number( oscMessageList.at(i)->data.at( 1 )->i );
	      udp_send_port = QString::number( oscMessageList.at(i)->data.at( 2 )->i );
	      name = QString( oscMessageList.at(i)->data.at( 3 )->s ); //name
	      sysInfoReceived = true;
			}
		}
		else if( QString(oscMessageList.at(i)->address).contains( "error", Qt::CaseInsensitive ) )
			messageInterface->messageThreadSafe( msg, MessageEvent::Warning, locationString( ) );
		else
			messageList.append( msg );
	}
	if( messageList.count( ) > 0 )
	{
		mainWindow->sendXmlPacket( oscMessageList, locationString( ) );
		messageInterface->messageThreadSafe( messageList, MessageEvent::Response, locationString( ) );
	}
		
	if( sysInfoReceived )
	{
		if( this == mainWindow->getCurrentBoard( ) )
		{
			this->setText( QString( "%1 : %2" ).arg(name).arg(locationString()) );
			mainWindow->updateDeviceList( );
		}
	}
	qDeleteAll( oscMessageList );
}

void Board::extractSystemInfoA( OscMessage* msg )
{
	QList<OscMessageData*> msgData = msg->data;
	int dataCount = msg->data.count( );
	int i;
	
	for( i = 0; i < dataCount; i++ )
	{
		if( msgData.at( i ) == 0 )
			break;
		switch( i ) // we're counting on the board to send the pieces of data in this order
		{
			case 0:
				name = QString( msgData.at( i )->s ); //name
				break;
			case 1:
				serialNumber = QString::number( msgData.at( i )->i ); // serial number
				break;
			case 2:
				ip_address = QString( msgData.at( i )->s ); // IP address
				break;
			case 3:
				firmwareVersion = QString( msgData.at( i )->s );
				break;
			case 4:
				freeMemory = QString::number( msgData.at( i )->i );
				break;
		}
	}
}

void Board::extractSystemInfoB( OscMessage* msg )
{
	QList<OscMessageData*> msgData = msg->data;
	int dataCount = msg->data.count( );
	int i;
	
	for( i = 0; i < dataCount; i++ )
	{
		if( msgData.at( i ) == 0 )
			break;
		switch( i ) // we're counting on the board to send the pieces of data in this order
		{
			case 0:
				dhcp = msgData.at( 0 )->i;
				break;
			case 1:
				webserver = msgData.at( 1 )->i;
				break;
			case 2:
				gateway = QString( msgData.at( 2 )->s );
				break;
			case 3:
				netMask = QString( msgData.at( 3 )->s );
				break;
			case 4:
				udp_listen_port = QString::number( msgData.at( 4 )->i );
				break;
			case 5:
				udp_send_port = QString::number( msgData.at( 5 )->i );
				break;
		}
	}
}

void Board::sendMessage( QString rawMessage )
{
	if( packetInterface == NULL || !packetInterface->isOpen( ) )
		return;
	else
		osc->uiSendPacket( rawMessage );
}

void Board::sendMessage( QList<OscMessage*> messageList )
{
	int msgCount = messageList.count( );
	for( int i = 0; i < msgCount; i++ )
		osc->createMessage( messageList.at(i) );
	if( msgCount > 0 )
		osc->sendPacket( );

}

