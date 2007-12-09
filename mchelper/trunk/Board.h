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

#ifndef BOARD_H_
#define BOARD_H_

#include <QListWidgetItem>
#include "PacketReadyInterface.h"
#include "UploaderThread.h"
#include "PacketInterface.h"
#include "Osc.h"
#include "OutputWindow.h"

class UploaderThread;
class PacketInterface;
class Osc;
class OscMessage;

#include <QString>

class Board : public QObject, public QListWidgetItem, public PacketReadyInterface
{
  Q_OBJECT
	public:
    enum Types{ UsbSerial, UsbSamba, Udp };
    
    Board( MessageInterface* messageInterface, McHelperWindow* mainWindow, QApplication* application );
    ~Board( );
    void setPacketInterface( PacketInterface* packetInterface );
    void setUploaderThread( UploaderThread* uploaderThread );
    void packetWaiting( ); // from PacketReadyInterface
    void sendMessage( QString rawMessage );
		void sendMessage( QList<OscMessage*> messageList );
    bool setBinFileName( char* filename );
    void flash( );
    QString locationString( );
    
    QString key, location; 
    Board::Types type;
    
    // System properties
    QString name, serialNumber, firmwareVersion, freeMemory;
    
    // Network properties
    QString ip_address, netMask, gateway, udp_listen_port, udp_send_port;
    bool dhcp, webserver;

  private:
    MessageInterface* messageInterface;
    McHelperWindow* mainWindow;
    QApplication* application;
    PacketInterface* packetInterface;
    Osc* osc;
    UploaderThread* uploaderThread;
		QStringList messagesToPost;
		QTimer messagePostTimer;
		
		
		bool extractSystemInfoA( OscMessage* msg );
		bool extractSystemInfoB( OscMessage* msg );
		bool extractNetworkFind( OscMessage* msg );
};

#endif /*BOARD_H_*/
