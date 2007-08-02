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
		if( strcmp( oscMessageList.at(i)->address, "/system/info-internal" ) == 0 )
		{ 
      // we're counting on the board to send the pieces of data in this order
			name = QString( oscMessageList.at(i)->data.at( 0 )->s ); //name
      serialNumber = QString::number( oscMessageList.at(i)->data.at( 1 )->i ); // serial number
      ip_address = QString( oscMessageList.at(i)->data.at( 2 )->s ); // IP address
      firmwareVersion = QString( oscMessageList.at(i)->data.at( 3 )->s );
      freeMemory = QString::number( oscMessageList.at(i)->data.at( 4 )->i );
      dhcp = oscMessageList.at(i)->data.at( 5 )->i;
      webserver = oscMessageList.at(i)->data.at( 6 )->i;
      gateway = QString( oscMessageList.at(i)->data.at( 7 )->s );
      netMask = QString( oscMessageList.at(i)->data.at( 8 )->s );
      udp_listen_port = QString::number( oscMessageList.at(i)->data.at( 9 )->i );
      udp_send_port = QString::number( oscMessageList.at(i)->data.at( 10 )->i );

			sysInfoReceived = true;
		}
		else if( strcmp( oscMessageList.at(i)->address, "/network/find" ) == 0 )
		{
			ip_address = QString( oscMessageList.at(i)->data.at( 0 )->s ); // IP address
			udp_listen_port = QString::number( oscMessageList.at(i)->data.at( 1 )->i );
      udp_send_port = QString::number( oscMessageList.at(i)->data.at( 2 )->i );
      name = QString( oscMessageList.at(i)->data.at( 3 )->s ); //name
      sysInfoReceived = true;
		}
		else if( QString(oscMessageList.at(i)->address).contains( "error", Qt::CaseInsensitive ) )
			messageInterface->messageThreadSafe( msg, MessageEvent::Warning, QString( locationString( ) ) );
		else
			messageList.append( msg );
	}
	if( messageList.count( ) > 0 )
		messageInterface->messageThreadSafe( messageList, MessageEvent::Response, QString( locationString( ) ) );
		
	if( sysInfoReceived )
	{
		
		if( this == mainWindow->getCurrentBoard( ) )
		{
			this->setText( QString( "%1 : %2" ).arg(name).arg(locationString()) );
			mainWindow->updateDeviceList( );
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

