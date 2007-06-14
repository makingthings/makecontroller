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

#include "PacketUsbCdc.h"
#include <QMutexLocker>

// SLIP codes
#define END             0300    // indicates end of packet 
#define ESC             0333    // indicates byte stuffing 
#define ESC_END         0334    // ESC ESC_END means END data byte 
#define ESC_ESC         0335    // ESC ESC_ESC means ESC data byte 

PacketUsbCdc::PacketUsbCdc( ) : QThread( )
{
	packetCount = 0;
	oscTranslator = new Osc();
	packetReadyInterface = oscTranslator;
}

void PacketUsbCdc::run()
{
	OscUsbPacket* currentPacket = NULL;
  
  oscTranslator->setInterfaces( this, messageInterface, application );
  
	open( );
	while( 1 )
	{
	  if( !deviceOpen ) // if it's not open, try to open it
			usbOpen( );

		if( deviceOpen ) // then, if open() succeeded, try to read
		{
			currentPacket = new OscUsbPacket( );
			int packetLength = slipReceive( currentPacket->packetBuf, OSC_MAX_MESSAGE );
			if( packetLength > 0 )
			{
				currentPacket->length = packetLength;
				{
					QMutexLocker locker( &packetListMutex );
					packetList.append( currentPacket );
					packetCount++;
				}
				packetReadyInterface->packetWaiting( );
			}
			else
				this->sleepMs( 1 ); // usb is still open, but we didn't receive anything last time
		}
		else // usb isn't open...chill out.
			this->sleepMs( 50 );
	}
	close( ); // should never get here...
}

PacketUsbCdc::Status PacketUsbCdc::open()
{
	//if( UsbSerial::OK != usbOpen( ) )
		//return PacketInterface::ERROR_NOT_OPEN;
	//else
		return PacketInterface::OK;
}

PacketUsbCdc::Status PacketUsbCdc::close()
{
	usbClose( );
	return PacketInterface::OK;
}

int PacketUsbCdc::sendPacket( char* packet, int length )
{
  char buf[ length * 2 ]; // make it twice as long, as worst case scenario is ALL escape characters
	buf[0] = END;  // Flush out any spurious data that may have accumulated
	char* ptr = buf + 1; 
	int size = length;

  while( size-- )
  {
    switch(*packet)
		{
			// if it's the same code as an END character, we send a special 
			//two character code so as not to make the receiver think we sent an END
			case END:
				*ptr++ = ESC;
				*ptr++ = ESC_END;
				break;
				
				// if it's the same code as an ESC character, we send a special 
				//two character code so as not to make the receiver think we sent an ESC
			case ESC:
				*ptr++ = ESC;
				*ptr++ = ESC_ESC;
				break;
				//otherwise, just send the character
			default:
				*ptr++ = *packet;
		}
		packet++;
	}
	// tell the receiver that we're done sending the packet
	*ptr++ = END;
	usbWrite( buf, (ptr - buf) );
	
	return 0;
}

void PacketUsbCdc::uiSendPacket( QString rawString )
{
  oscTranslator->uiSendPacket(rawString);
}

int PacketUsbCdc::slipReceive( char* buffer, int length )
{
  int started = 0, count = 0, justGot = 0;
  char tempBuffer[length];
  char *bufferPtr = buffer, *tempPtr = tempBuffer;

  while ( true )
  {
    int available = numberOfAvailableBytes( );
    if( available > 0 )
    {
    	justGot = usbRead( tempBuffer, available );
    	tempPtr = tempBuffer;
    	if( justGot < 0 )
			close( );
    }
	
    int i;
    for( i = 0; i < justGot; i++ )
    {
      switch( *tempPtr )
      {
        case END:
          if( started && count ) // it was the END byte
			return count; // We're done now if we had received any characters
          else // skipping all starting END bytes
            started = true;
          break;					
        case ESC:
          // if it's the same code as an ESC character, we just want to skip it and 
          // stick the next byte in the packet
          tempPtr++;
          // no break here, just stick it in the packet		
        default:
          *bufferPtr++ = *tempPtr;
          count++;
      }
      tempPtr++;
    }
    if( justGot == 0 ) // if we didn't get anything, sleep...otherwise just rip through again
    	Sleep( 1 );
    justGot = 0; // reset our count for the next run through
  }
  return IO_ERROR; // should never get here
}

bool PacketUsbCdc::isPacketWaiting( )
{
  if( packetList.isEmpty() )
	  return false;
	else
	  return true;
}

int PacketUsbCdc::receivePacket( char* buffer, int size )
{
	// Need to protect the packetList structure from multithreaded diddling
	QMutexLocker locker( &packetListMutex );
	int length;
	
	if( !packetList.size() )
	{
	  messageInterface->message( 1, "usb> Error receiving packet.\n" );
		
		return 0;
	} 
	else
	{
  	OscUsbPacket* packet = packetList.takeAt( 0 );
	  packetCount--;
	  length = packet->length;
		if ( length <= size )
		{
	    buffer = (char*)memcpy( buffer, packet->packetBuf, length );
  	  delete packet;
  	  return length;
		}
		else
		{
			delete packet;
			return 0; 
		}
	}
}

char* PacketUsbCdc::location( )
{
	return portName;
}

void PacketUsbCdc::setInterfaces( MessageInterface* messageInterface, QApplication* application )
{
	this->messageInterface = messageInterface;
	this->application = application;
}

#ifdef Q_WS_WIN
void PacketUsbCdc::setWidget( QMainWindow* window )
{
	this->mainWindow = window;
}
#endif

void PacketUsbCdc::sleepMs( int ms )
{
  #ifdef Q_WS_WIN
  Sleep( ms );
  #endif
  #ifdef Q_WS_MAC
  usleep( ms*1000 );
  #endif
}



