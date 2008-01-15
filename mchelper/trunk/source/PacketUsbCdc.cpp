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

#include "PacketUsbCdc.h"
#include <QMutexLocker>

// SLIP codes
#define END             0300    // indicates end of packet 
#define ESC             0333    // indicates byte stuffing 
#define ESC_END         0334    // ESC ESC_END means END data byte 
#define ESC_ESC         0335    // ESC ESC_ESC means ESC data byte 

PacketUsbCdc::PacketUsbCdc( ) : QThread( )
{
	packetReadyInterface = NULL;
	currentPacket = NULL;
	exit = false;
}

PacketUsbCdc::~PacketUsbCdc( )
{
	if( currentPacket != NULL )
		delete currentPacket;
}

void PacketUsbCdc::run()
{
	while( 1 )
	{
	  if( exit == true )
	  	return;
	  	
	  if( !deviceOpen ) // if it's not open, try to open it
			usbOpen( );

		if( deviceOpen ) // then, if open() succeeded, try to read
		{
			currentPacket = new OscUsbPacket( );
			int packetLength = slipReceive( currentPacket->packetBuf, MAX_MESSAGE );
			if( packetLength > 0 && packetReadyInterface )
			{
				currentPacket->length = packetLength;
				{
					QMutexLocker locker( &packetListMutex );
					packetList.append( currentPacket );
				}
				packetReadyInterface->packetWaiting( );
			}
			else
			{
				delete currentPacket;
				msleep( 1 ); // usb is still open, but we didn't receive anything last time
			}
		}
		else // usb isn't open...chill out.
			msleep( 50 );
	}
	close( ); // should never get here...
}

PacketUsbCdc::Status PacketUsbCdc::open()
{
	if( UsbSerial::OK == usbOpen( ) )
		return PacketInterface::OK;
	else
		return PacketInterface::ERROR_NOT_OPEN;
}

PacketUsbCdc::Status PacketUsbCdc::close( )
{
	exit = true;
	while( isRunning( ) )
		msleep( 5 ); // wait a second before returning, because we'll be deleted right after we're removed form the GUI
	usbClose( );
	return PacketInterface::OK;
}

PacketUsbCdc::Status PacketUsbCdc::sendPacket( char* packet, int length )
{
  if( exit == true )
		return PacketInterface::IO_ERROR;
		
	QByteArray outgoingPacket;
	outgoingPacket.append( END ); // Flush out any spurious data that may have accumulated
	int size = length;

  while( size-- )
  {
    switch(*packet)
		{
			// if it's the same code as an END character, we send a special 
			//two character code so as not to make the receiver think we sent an END
			case END:
				outgoingPacket.append( ESC );
				outgoingPacket.append( ESC_END );
				break;
				// if it's the same code as an ESC character, we send a special 
				//two character code so as not to make the receiver think we sent an ESC
			case ESC:
				outgoingPacket.append( ESC );
				outgoingPacket.append( ESC_ESC );
				break;
				//otherwise, just send the character
			default:
				outgoingPacket.append( *packet );
		}
		packet++;
	}
	// tell the receiver that we're done sending the packet
	outgoingPacket.append( END );
	if( UsbSerial::OK != usbWrite( outgoingPacket.data( ), outgoingPacket.size( ) ) )
	{
		monitor->deviceRemoved( QString(portName) ); // shut ourselves down
		return PacketInterface::IO_ERROR;
	}
	else
		return PacketInterface::OK;
}

int PacketUsbCdc::getMoreBytes( )
{
	if( slipRxPacket.size( ) < 1 ) // if there's nothing left over from last time
	{
		int available = numberOfAvailableBytes( );
		if( available < 0 )
			return -1;
		if( available > 0 )
		{
			slipRxPacket.resize( available );
			int read = usbRead( slipRxPacket.data( ), slipRxPacket.size( ) );
			if( read < 0 )
				return -1;
		}
	}
	return PacketInterface::OK;
}

int PacketUsbCdc::slipReceive( char* buffer, int length )
{
  int started = 0, count = 0, finished = 0, i;
  char *bufferPtr = buffer;

  while ( true )
  {
		if( exit == true )
	  	return -1;
	  	
		int status = getMoreBytes( );
		if( status != PacketInterface::OK )
			return -1;
			
		if( slipRxPacket.size( ) )
		{
			int size = (slipRxPacket.size( ) > length) ? length : slipRxPacket.size( );
			for( i = 0; i < size; i++ )
			{
				char c = *slipRxPacket.data( );
				switch( c )
				{
					case END:
						if( started && count ) // it was the END byte
							finished = true; // We're done now if we had received any characters
						else // skipping all starting END bytes
							started = true;
						break;					
					case ESC:
						// if it's the same code as an ESC character, we just want to skip it and 
						// stick the next byte in the packet
						slipRxPacket.remove( 0, 1 );
						c = *slipRxPacket.data( );
						// no break here, just stick it in the packet		
					default:
						if( started )
						{
							*bufferPtr++ = c;
							count++;
						}
						break;
				}
				slipRxPacket.remove( 0, 1 );
				if( finished )
					return count;
			}
		}
    else // if we didn't get anything, sleep...otherwise just rip through again
    	msleep( 1 );
  }
  return PacketInterface::IO_ERROR; // should never get here
}

bool PacketUsbCdc::isPacketWaiting( )
{
  if( packetList.isEmpty() )
	  return false;
	else
	  return true;
}

bool PacketUsbCdc::isOpen( )
{
  return deviceOpen;
}

int PacketUsbCdc::receivePacket( char* buffer, int size )
{
	// Need to protect the packetList structure from multithreaded diddling
	int length;
	int retval = 0;
	
	if( !packetList.size() )
	{
		QString msg = QString( "Error receiving packet.");
		messageInterface->messageThreadSafe( msg, MessageEvent::Error);
		return 0;
	} 
	else
	{
  	int listSize = packetList.size( );
  	if( listSize > 0 )
  	{
  		QMutexLocker locker( &packetListMutex );
  		for( int i = 0; i < listSize; i++ )
  		{
  			OscUsbPacket* packet = packetList.takeAt( i );
	  		length = packet->length;
	  		if ( length <= size )
				{
			    buffer = (char*)memcpy( buffer, packet->packetBuf, length );
		  	  retval = length;
				}
				else
					retval = 0;
				delete packet;
  		}
  	}
	}
	return retval;
}

char* PacketUsbCdc::location( )
{
	#ifdef Q_WS_WIN
	return portName;
	#else
	return "USB";
	#endif
}

QString PacketUsbCdc::getKey( )
{
	return QString( portName );
}

void PacketUsbCdc::setInterfaces( MessageInterface* messageInterface, QApplication* application, MonitorInterface* monitor )
{
	this->messageInterface = messageInterface;
	this->application = application;
	this->monitor = monitor;
}

void PacketUsbCdc::setPacketReadyInterface( PacketReadyInterface* packetReadyInterface)
{
	this->packetReadyInterface = packetReadyInterface;
}

#ifdef Q_WS_WIN
void PacketUsbCdc::setWidget( QMainWindow* window )
{
	this->mainWindow = window;
}
#endif





