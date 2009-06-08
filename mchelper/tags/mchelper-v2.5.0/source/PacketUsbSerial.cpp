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

#include "PacketUsbSerial.h"
#include <QDebug>

// SLIP codes
#define END     0300 // indicates end of packet
#define ESC     0333 // indicates byte stuffing
#define ESC_END 0334 // ESC ESC_END means END data byte
#define ESC_ESC 0335 // ESC ESC_ESC means ESC data byte

PacketUsbSerial::PacketUsbSerial(QString portName)
{
  port = new QextSerialPort(portName, QextSerialPort::EventDriven);
  connect(port, SIGNAL(readyRead()), this, SLOT(processNewData()));
  pkt_started = false;
}

PacketUsbSerial::~PacketUsbSerial( )
{
  port->close();
  delete port;
}

/*
 New data has arrived on the serial port.
 Read it out, then decode it as SLIP data.
*/
void PacketUsbSerial::processNewData( )
{
  QByteArray newData;
  int avail = port->bytesAvailable();
  if(avail > 0 ) {
    newData.resize(avail);
    if(port->read(newData.data(), newData.size()) > 0) {
      readBuffer += newData;
      slipDecode();
    }
  }
}

bool PacketUsbSerial::open( )
{
  return port->open(QIODevice::ReadWrite);
}

/*
 SLIP encode the packet and send it out.
*/
bool PacketUsbSerial::sendPacket( const char* packet, int length )
{
  QByteArray out;
  out.append( END ); // Flush out any spurious data that may have accumulated
  while( length-- )
  {
    const char c = *packet++;
    switch(c)
    {
      // if it's the same code as an END character, we send a special
      //two character code so as not to make the receiver think we sent an END
      case END:
        out.append( ESC );
        out.append( ESC_END );
        break;
        // if it's the same code as an ESC character, we send a special
        //two character code so as not to make the receiver think we sent an ESC
      case ESC:
        out.append( ESC );
        out.append( ESC_ESC );
        break;
        //otherwise, just send the character
      default:
        out.append( c );
    }
  }
  // tell the receiver that we're done sending the packet
  out.append( END );
  if(port->write(out) < 0) {
    //monitor->deviceRemoved( port->portName() ); // shut ourselves down
    return false;
  }
  else
    return true;
}

/*
 SLIP decode the readBuffer.
 When we get a packet, pass it to the board to deal with it.
*/
void PacketUsbSerial::slipDecode( )
{
  while(!readBuffer.isEmpty())
  {
    unsigned char c = *(readBuffer.data());
    readBuffer.remove(0, 1);
    switch( c )
    {
      case END:
        if( pkt_started && currentPacket.size() ) { // it was the END byte
          board->msgReceived(currentPacket);
          currentPacket.clear();
          pkt_started = false;
        }
        else // skipping all starting END bytes
          pkt_started = true;
        break;
      case ESC:
        // we got an ESC character - get the next byte.
        // if it's not an ESC_END or ESC_ESC, it's a malformed packet.
        // http://tools.ietf.org/html/rfc1055 says just drop it in the packet in this case
        if(readBuffer.isEmpty())
          return;
        else {
          c = *(readBuffer.data());
          readBuffer.remove(0, 1);
          if( pkt_started ) {
            if( c == ESC_END ) {
              currentPacket.append(END);
              break;
            }
            else if( c == ESC_ESC ) {
              currentPacket.append(ESC);
              break;
            }
          }
        }
        // no break here
      default:
        if( pkt_started )
          currentPacket.append( c );
        break;
    }
  }
}







