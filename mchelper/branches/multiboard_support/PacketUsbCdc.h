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

#ifndef PACKETUSBCDC_H
#define PACKETUSBCDC_H

#include <QThread>
#include <QList>
#include <QMutex>

#include "UsbSerial.h"
#include "PacketInterface.h"
#include "MessageInterface.h"
#include "PacketReadyInterface.h"

#define MAX_MESSAGE 2048

class OscUsbPacket
{
  public:
	  char packetBuf[MAX_MESSAGE];
		int length;
};

class PacketUsbCdc : public QThread, public UsbSerial, public PacketInterface
{
    public:
			PacketUsbCdc( );
			~PacketUsbCdc( );
			void run( );
			// from PacketInterface
			Status open( );	
		  	Status close( );
			int sendPacket( char* packet, int length );
			bool isPacketWaiting( );
			int receivePacket( char* buffer, int size );
			QString getKey( void );
			char* location( void );
			void setInterfaces( MessageInterface* messageInterface, QApplication* application );
			void setPacketReadyInterface( PacketReadyInterface* packetReadyInterface);
			#ifdef Q_WS_WIN
			void setWidget( QMainWindow* window );
			#endif
								
                
    private:
		  QList<OscUsbPacket*> packetList;
		  OscUsbPacket* currentPacket;
		  QMutex packetListMutex;
		  void sleepMs( int ms );
			int packetCount;
			int packetState;
			enum State { START, DATASTART, DATA };
			
		  PacketReadyInterface* packetReadyInterface;
		  QApplication* application;
		  int slipReceive( char* buffer, int length );
		  bool exit;

};

#endif // PACKETUSBCDC_H

