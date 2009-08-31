/*********************************************************************************

 Copyright 2006-2009 MakingThings

 Licensed under the Apache License,
 Version 2.0 (the "License"); you may not use this file except in compliance
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

 *********************************************************************************/

#ifndef PACKET_USB_SERIAL_H
#define PACKET_USB_SERIAL_H

#include <QList>
#include <QByteArray>

#include "Board.h"
#include "qextserialport.h"
#include "MainWindow.h"
#include "PacketInterface.h"

class PacketUsbSerial : public QObject, public PacketInterface
{
  Q_OBJECT
public:
  PacketUsbSerial(const QString & portName);
  ~PacketUsbSerial( );
  bool open( );
  void close( ) { port->close( ); }
  bool sendPacket( const char* packet, int length );
  void setBoard( Board *board ) {this->board = board;}
  bool isOpen( ) { return port->isOpen(); }
  QString key( ) { return port->portName(); }

private slots:
  void processNewData( );

private:
  QByteArray currentPacket;
  Board* board;
  MainWindow *mainWindow;
  QextSerialPort *port;
  void slipDecode( QByteArray & data );
};

#endif // PACKET_USB_SERIAL_H



