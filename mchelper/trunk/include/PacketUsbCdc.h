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

#ifndef PACKETUSBCDC_H
#define PACKETUSBCDC_H

#include <QThread>
#include <QList>
#include <QMutex>
#include <QByteArray>

#include "UsbSerial.h"
#include "PacketInterface.h"
#include "MessageInterface.h"
#include "PacketReadyInterface.h"
#include "MonitorInterface.h"

class UsbSerial;

class PacketUsbCdc : public QThread, public PacketInterface
{
	Q_OBJECT
	public:
		PacketUsbCdc( );
		~PacketUsbCdc( );
		void run( );
		// from PacketInterface
		Status open( );	
		Status close( );
		Status sendPacket( char* packet, int length );
		bool isPacketWaiting( void );
		int pendingPacketSize( void );
		bool isOpen( void );
		int receivePacket( char* buffer, int size );
		QString getKey( void );
		char* location( void );
		void setPortName( QString name );
		void setInterfaces( MessageInterface* messageInterface, QApplication* application, MonitorInterface* monitor );
		void setPacketReadyInterface( PacketReadyInterface* packetReadyInterface);
		#ifdef Q_WS_WIN
		void setWidget( QMainWindow* window );
		#endif
			
	public slots:
		void readyRead( );
								
	private:
		QByteArray currentPacket;
		QMutex packetMutex;
		QByteArray slipRxPacket;
		UsbSerial *port;
		void sleepMs( int ms );

		PacketReadyInterface* packetReadyInterface;
		MessageInterface *messageInterface;
		QApplication* application;
		MonitorInterface* monitor;
		int slipReceive( QByteArray *packet );
		int getMoreBytes( void );
		bool exit;
};

#endif // PACKETUSBCDC_H

