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

#include "PacketUdp.h"

#define PING_TIMEOUT 3000

PacketUdp::PacketUdp(QHostAddress remoteAddress, int send_port)
{
  this->remoteAddress = remoteAddress;
  this->send_port = send_port;
  connect( &pingTimer, SIGNAL(timeout()), this, SLOT(onTimeOut( )));
}

QString PacketUdp::key( )
{
  return remoteAddress.toString();
}

/*
 A board wants to send a message via UDP.
*/
bool PacketUdp::sendPacket( const char* packet, int length )
{
  qint64 result = writeDatagram( (const char*)packet, (qint64)length, remoteAddress, send_port);
  if( result < 0 )
  {
    emit msg( tr("Error - Couldn't send packet."), MsgType::Error, "Ethernet" );
    return false;
  }
  return true;
}

/*
 Called when new data has been received.
 Send it on to the board.
 */
void PacketUdp::newMessage( const QByteArray & message )
{
  pingTimer.start( PING_TIMEOUT ); // reset our timer
  if(board != NULL)
    board->msgReceived(message);
}

/*
  Our timer has expired...means the board has gone away.
*/
void PacketUdp::onTimeOut( )
{
  pingTimer.stop();
  emit timeout(remoteAddress.toString());
}




