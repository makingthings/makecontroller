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

#ifndef BOARD_H_
#define BOARD_H_

#include "PacketReadyInterface.h"
#include "UploaderThread.h"
#include "PacketInterface.h"
#include "Osc.h"

class UploaderThread;
class PacketInterface;
class Osc;

#include <QString>

class Board : public PacketReadyInterface
{
  public:
  
    enum Types{ UsbSerial, UsbSamba, Udp };
    
    Board( MessageInterface* messageInterface, QApplication* application );
    ~Board( );
    void setPacketInterface( PacketInterface* packetInterface );
    void setUploaderThread( UploaderThread* uploaderThread );
    void packetWaiting( ); // from PacketReadyInterface
    void sendMessage( QString rawMessage );
    bool setBinFileName( char* filename );
    void flash( );
    
    QString key;
    Board::Types type;
    QString com_port;
    
    // System properties
    QString name, serialNumber, firmwareVersion, freeMemory;
    
    // Network properties
    QString ip_address, netMask, gateway, mac;
    bool dhcp, webserver;

  private:
    MessageInterface* messageInterface;
    QApplication* application;
    PacketInterface* packetInterface;
    Osc* osc;
    UploaderThread* uploaderThread;
};

#endif /*BOARD_H_*/
