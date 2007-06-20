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

Board::Board( MessageInterface* messageInterface, QApplication* application )
{
  osc = new Osc( );
  this->messageInterface = messageInterface;
  this->application = application;
}

Board::~Board( )
{
  delete osc;
}

void Board::setPacketInterface( PacketInterface* packetInterface )
{
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
	osc->receive( &oscMessageList );
	
	int messageCount = oscMessageList.size( ), i;
	for (i = 0; i < messageCount; i++)
	{
		if( strcmp( oscMessageList.at(i)->address, "/system/info" ) == 0 )
		{ // we're counting on the board to send the pieces of data in this order
			name = QString( oscMessageList.at(i)->data.at( 0 )->s ); //name
			serialNumber = QString::number( oscMessageList.at(i)->data.at( 1 )->i ); // serial number
			ip_address = QString( oscMessageList.at(i)->data.at( 2 )->s ); // IP address
		}
		else // just print them out
		{
			QString msg = oscMessageList.at(i)->toString( );
			msg.prepend( osc->getPreamble( ) );
			messageInterface->messageThreadSafe( msg );
		}
	}
}

void Board::sendMessage( QString rawMessage )
{
	osc->uiSendPacket( rawMessage );
}

