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
#define SLIP_END     0300 // indicates end of packet
#define SLIP_ESC     0333 // indicates byte stuffing
#define SLIP_ESC_END 0334 // ESC ESC_END means END data byte
#define SLIP_ESC_ESC 0335 // ESC ESC_ESC means ESC data byte

#define SLIP_MAX_SANE_PKT 16384

PacketUsbSerial::PacketUsbSerial(const QString & portName)
{
  port = new QextSerialPort(portName, QextSerialPort::EventDriven);
  instream = new QDataStream(port);
  connect(port, SIGNAL(readyRead()), this, SLOT(processNewData()));
}

PacketUsbSerial::~PacketUsbSerial()
{
  port->close();
  delete instream;
  delete port;
}

/*
 New data has arrived on the serial port.
 Read it out, then decode it as SLIP data.
*/
void PacketUsbSerial::processNewData()
{
  if (port->bytesAvailable() > 0)
    slipDecode();
}

bool PacketUsbSerial::open()
{
  return port->open(QIODevice::ReadWrite);
}

/*
 SLIP encode the packet and send it out.
*/
bool PacketUsbSerial::sendPacket(const char* packet, int length)
{
  QByteArray out(1, SLIP_END); // Flush out any spurious data that may have accumulated
  char c;
  while (length--) {
    c = *packet++;
    switch (c) {
      // if it's the same code as an END character, we send a special
      //two character code so as not to make the receiver think we sent an END
      case SLIP_END:
        out.append(SLIP_ESC);
        out.append(SLIP_ESC_END);
        break;
        // if it's the same code as an ESC character, we send a special
        //two character code so as not to make the receiver think we sent an ESC
      case SLIP_ESC:
        out.append(SLIP_ESC);
        out.append(SLIP_ESC_ESC);
        break;
        //otherwise, just send the character
      default:
        out.append(c);
    }
  }
  // tell the receiver that we're done sending the packet
  out.append(SLIP_END);
  return (port->write(out) > 0);
}

/*
 SLIP decode the readBuffer.
 When we get a packet, pass it to the board to deal with it.
*/
void PacketUsbSerial::slipDecode()
{
  quint8 c;
  while (!instream->atEnd()) {
    *instream >> c;
    switch (c) {
      case SLIP_END:
        if (!currentPacket.isEmpty()) {
          board->msgReceived(currentPacket);
          currentPacket.clear();
        }
        break;
      case SLIP_ESC:
        // we got an ESC character - get the next byte.
        // if it's not an ESC_END or ESC_ESC, it's a malformed packet.
        // http://tools.ietf.org/html/rfc1055 says just drop it in the packet in this case
        if (instream->atEnd()) break;
        *instream >> c;
        if (c == SLIP_ESC_END)
          c = SLIP_END;
        else if (c == SLIP_ESC_ESC)
          c = SLIP_ESC;
        // no break here
      default:
        currentPacket.append(c);
        if (currentPacket.size() > SLIP_MAX_SANE_PKT) {
          qDebug() << "clearing packet, size is greater than" << SLIP_MAX_SANE_PKT;
          currentPacket.clear();
        }
        break;
    }
  }
}
