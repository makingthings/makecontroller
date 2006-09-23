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
	packetState = START;
}

void PacketUsbCdc::run()
{
  char justGot;
	OscUsbPacket* currentPacket = NULL;
	char* packetP = NULL;
	bool packetStarted = false;
  
	open( );
	while( usbIsOpen( ) )
	{
	  UsbStatus readResult = usbRead( &justGot, 1 );  //we're only ever going to read 1 character at a time
		if( readResult == GOT_CHAR ) //we got a character
		{
		  switch( justGot )
			{
			  case END:
				  if( packetStarted && currentPacket->length ) // it was the END byte
					{ 
						// Need to protect the packetList structure from multithreaded diddling
						{
						  QMutexLocker locker( &packetListMutex );
						
					    *packetP = '\0';
						  packetList.append( currentPacket );
  						packetCount++;
						}
						packetReadyInterface->packetWaiting( );
						packetStarted = false;
					} 
					else // it was the START byte
					{
					  currentPacket = new OscUsbPacket( );
						packetP = currentPacket->packetBuf;
						packetStarted = true;
					}
					break;
					
				// if it's the same code as an ESC character, get another character,
				// then figure out what to store in the packet based on that.
				case ESC:
					readResult = usbRead( &justGot, 1 );
					if( readResult == GOT_CHAR )
					{
					  switch( justGot )
						{
						  case ESC_END:
							  justGot = END;
								break;
							case ESC_ESC:
								justGot = ESC;
								break;
						}
					}
					else
					  break;
				// otherwise, just stick it in the packet		
				default:
				  if( packetP == NULL )
					  break;
				  *packetP = justGot;
					packetP++;
					currentPacket->length++;
			}
		}
		else
		{
		  #ifdef Q_WS_WIN
	    Sleep( 10 );
			#endif
			#ifdef Q_WS_MAC
			sleepMs( 10 );
			#endif
		}
	}
	close( );
}

PacketUsbCdc::Status PacketUsbCdc::open()
{
	usbOpen( );
	return PacketInterface::OK;
}

PacketUsbCdc::Status PacketUsbCdc::close()
{
	usbClose( );
	return PacketInterface::OK;
}

int PacketUsbCdc::sendPacket( char* packet, int length )
{
  int size = length;
  usbWriteChar( END ); // Flush out any spurious data that may have accumulated

  while( size-- )
  {
    switch(*packet)
		{
			// if it's the same code as an END character, we send a special 
			//two character code so as not to make the receiver think we sent an END
			case END:
				usbWriteChar( ESC );
				usbWriteChar( ESC_END );
				break;
				
				// if it's the same code as an ESC character, we send a special 
				//two character code so as not to make the receiver think we sent an ESC
			case ESC:
				usbWriteChar( ESC );
				usbWriteChar( ESC_ESC );
				break;
				//otherwise, just send the character
			default:
				usbWriteChar( *packet );
		}
		packet++;
	}
	
	// tell the receiver that we're done sending the packet
	usbWriteChar( END );
	return 0;
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

void PacketUsbCdc::setInterfaces( PacketReadyInterface* packetReadyInterface, MessageInterface* messageInterface )
{
	this->messageInterface = messageInterface;
	this->packetReadyInterface = packetReadyInterface;
}


