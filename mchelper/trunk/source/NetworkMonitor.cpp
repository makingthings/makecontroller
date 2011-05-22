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

#include <QSettings>
#include "NetworkMonitor.h"
#include "Preferences.h" // for DEFAULT_UDP_LISTEN_PORT and DEFAULT_UDP_SEND_PORT
#include "Osc.h"

/*
 NetworkMonitor manages the discovery of new devices via Bonjour.
 and also sends/receives all UDP traffic based on the register of devices it knows about via Bonjour
*/
NetworkMonitor::NetworkMonitor( MainWindow* mainWindow ) : QUdpSocket( )
{
  QSettings settings;
  int listen = settings.value("udp_listen_port", DEFAULT_UDP_LISTEN_PORT).toInt();
  send_port = settings.value("udp_send_port", DEFAULT_UDP_SEND_PORT).toInt();
  sendDiscoveryPackets = settings.value("networkDiscovery", DEFAULT_NETWORK_DISCOVERY).toBool();
  this->mainWindow = mainWindow;
  sendLocal = false;
  QHostInfo::lookupHost( QHostInfo::localHostName(), this, SLOT(lookedUp(QHostInfo)));

  connect( this, SIGNAL(deviceArrived(PacketInterface*)), mainWindow, SLOT(onEthernetDeviceArrived(PacketInterface*)));
  connect( this, SIGNAL(deviceRemoved(QString)), mainWindow, SLOT(onDeviceRemoved(const QString &)));
  connect( this, SIGNAL(readyRead()), this, SLOT( processPendingDatagrams() ) );
  connect( this, SIGNAL(msg(QString, MsgType::Type, QString)), mainWindow, SLOT(message(QString, MsgType::Type, QString)));
  connect( &pingTimer, SIGNAL( timeout() ), this, SLOT( sendPing() ) );
  broadcastPing = OscMessage("/network/find").toByteArray(); // our constant OSC ping
  setListenPort( listen, false );
  pingTimer.start(1000);
}

bool NetworkMonitor::setListenPort( int port, bool announce )
{
  if(listen_port == port) // don't need to do anything
    return true;
  close( );
  if( !bind( port, QUdpSocket::ShareAddress ) ) {
    QString str = tr( "Error: Can't listen on port %1 - make sure it's not already in use.").arg( port );
    emit msg( str, MsgType::Error, "Ethernet" );
    return false;
  }
  else {
    listen_port = port;
    QSettings settings;
    settings.setValue("udp_listen_port", listen_port);
    if( announce ) {
      QString str = tr( "Now listening on port %1 for UDP messages.").arg( listen_port );
      emit msg( str, MsgType::Notice, "Ethernet" );
    }
    return true;
  }
}

/*
 New data has arrived.
 If this is from an address we don't know about, create a new PacketUdp for it.
 Otherwise, read the data and pass it on to the appropriate PacketUdp.
*/
void NetworkMonitor::processPendingDatagrams()
{
  while( hasPendingDatagrams() ) {
    QByteArray datagram;
    QHostAddress remoteClient;
    datagram.resize( pendingDatagramSize() );
    readDatagram( datagram.data(), datagram.size(), &remoteClient );

    if(datagram == broadcastPing) // filter out broadcasts from other mchelpers
      return;

    QString sender = remoteClient.toString();
    if( connectedDevices.contains( sender ) ) // pass the packet through to the packet interface
      connectedDevices.value( sender )->newMessage( datagram );
    else {
      PacketUdp *udp = new PacketUdp(remoteClient, send_port);
      connectedDevices.insert( sender, udp );
      connect(udp, SIGNAL(timeout(QString)), this, SLOT(onDeviceRemoved(QString)));
      emit deviceArrived(udp);
    }
  }
}

void NetworkMonitor::sendPing( )
{
  if( sendDiscoveryPackets ) {
    // normally we'll be set to send on QHostAddress::Broadcast, but if that fails, just try
    // to send on the local broadcast address
    QHostAddress dest = ( sendLocal ) ? localBroadcastAddress : QHostAddress::Broadcast;
    if( writeDatagram( broadcastPing.data(), broadcastPing.size(), dest, send_port ) < 0 && !sendLocal ) {
      writeDatagram( broadcastPing.data(), broadcastPing.size(), localBroadcastAddress, send_port );
      sendLocal = true;
    }
  }
}

void NetworkMonitor::lookedUp(const QHostInfo &host)
{
  if (host.error() != QHostInfo::NoError) // lookup failed
    return;

  // find out our address, and then replace the last byte with 0xFF
  // this will be a local broadcast address
  foreach(QHostAddress addr, host.addresses()) {
    QStringList addrlist = addr.toString( ).split( "." );
    if(addrlist.count() == 4) { // expecting an address string in the form of xxx.xxx.xxx.xxx
      addrlist.replace( 3, "255" );
      localBroadcastAddress = addrlist.join( "." );
    }
  }
}

/*
  Called when a board has been removed.
  Remove it from our internal list, and remove it from the UI.
*/
void NetworkMonitor::onDeviceRemoved(const QString & key)
{
  if(connectedDevices.contains(key)) {
    connectedDevices.remove(key);
    emit deviceRemoved(key);
  }
}










