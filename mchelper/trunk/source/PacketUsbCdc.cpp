/*********************************************************************************

 Copyright 2006-2008 MakingThings

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

PacketUsbCdc::PacketUsbCdc( McHelperWindow* mainWindow, QApplication* application, MonitorInterface* monitor ) : QThread( )
{
	this->mainWindow = mainWindow;
	this->application = application;
	this->monitor = monitor;
	packetReadyInterface = NULL;
	exit = false;
	currentPacket.clear( );
	port = new UsbSerial( mainWindow );
}

PacketUsbCdc::~PacketUsbCdc( )
{
	port->close( );
	delete port;
}

void PacketUsbCdc::readyRead( )
{

}

void PacketUsbCdc::run()
{
	while( 1 )
	{
	  if( exit == true )
	  	return;
	  	
	  if( !port->isOpen() ) // if it's not open, try to open it
			port->open( );

		if( port->isOpen() ) // then, if open() succeeded, try to read
		{
			int packetLength = slipReceive( &currentPacket );
			if( packetLength > 0 && packetReadyInterface )
				packetReadyInterface->packetWaiting( );
			else
			{
				currentPacket.clear( );
				msleep( 1 ); // usb is still open, but we didn't receive anything last time
			}
		}
		else // usb isn't open...chill out.
			msleep( 50 );
	}
	close( ); // should never get here...
}

int PacketUsbCdc::pendingPacketSize( )
{
	QMutexLocker locker( &packetMutex );
	return currentPacket.size( );
}

PacketUsbCdc::Status PacketUsbCdc::open( )
{
	if( UsbSerial::OK == port->open( ) )
		return PacketInterface::OK;
	else
		return PacketInterface::ERROR_NOT_OPEN;
}

void PacketUsbCdc::setPortName( QString name )
{
	port->setPortName( name );
}

PacketUsbCdc::Status PacketUsbCdc::close( )
{
	exit = true;
	while( isRunning( ) )
		msleep( 5 ); // wait a second before returning, because we'll be deleted right after we're removed form the GUI
	port->close( );
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
	if( UsbSerial::OK != port->write( outgoingPacket.data( ), outgoingPacket.size( ) ) )
	{
		monitor->deviceRemoved( port->name() ); // shut ourselves down
		return PacketInterface::IO_ERROR;
	}
	else
		return PacketInterface::OK;
}

int PacketUsbCdc::getMoreBytes( )
{
	if( slipRxPacket.size( ) < 1 ) // if there's nothing left over from last time
	{
		int available = port->bytesAvailable( );
		if( available < 0 )
			return -1;
		if( available > 0 )
		{
			slipRxPacket.resize( available );
			int read = port->read( slipRxPacket.data( ), slipRxPacket.size( ) );
			if( read < 0 )
				return -1;
		}
	}
	return PacketInterface::OK;
}

int PacketUsbCdc::slipReceive( QByteArray *packet )
{
  int started = 0, count = 0, finished = 0, i;

  while ( true )
  {
		if( exit == true )
	  	return -1;
	  	
		int status = getMoreBytes( );
		if( status != PacketInterface::OK )
			return -1;
		
		int size = slipRxPacket.size( );
		if( size > 0 )
		{
			QMutexLocker locker( &packetMutex );
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
							packet->append( c );
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
  QMutexLocker locker( &packetMutex );
	  return currentPacket.size( ) > 0;
}

bool PacketUsbCdc::isOpen( )
{
  return port->isOpen();
}

int PacketUsbCdc::receivePacket( char* buffer, int size )
{
	int retval = 0;
	QMutexLocker locker( &packetMutex );
	
	if( !currentPacket.size() || currentPacket.size( ) > size )
	{
		QString msg = QString( "Error receiving packet.");
		mainWindow->messageThreadSafe( msg, MessageEvent::Error);
		return 0;
	} 
	else
	{
  	memcpy( buffer, currentPacket.data( ), currentPacket.size( ) );
		retval = currentPacket.size( );
		currentPacket.clear( );
	}
	return retval;
}

char* PacketUsbCdc::location( )
{
	#ifdef Q_WS_WIN
	return port->name( ).toAscii( ).data( );
	#else
	return "USB";
	#endif
}

QString PacketUsbCdc::getKey( )
{
	return port->name( );
}

void PacketUsbCdc::setPacketReadyInterface( PacketReadyInterface* packetReadyInterface)
{
	this->packetReadyInterface = packetReadyInterface;
}

#ifdef Q_WS_WIN
void PacketUsbCdc::setDeviceHandle( HANDLE deviceHandle )
{
	port->deviceHandle = deviceHandle;
}

HANDLE PacketUsbCdc::getDeviceHandle( )
{
	if( port )
		return port->deviceHandle;
	else
		return INVALID_HANDLE_VALUE;
}
#endif






