/*********************************************************************************
 
 Copyright 2006-2008 MakingThings
 
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

/*
 NetworkMonitor manages the discovery of new devices via Bonjour.
 and also sends/receives all UDP traffic based on the register of devices it knows about via Bonjour
*/
NetworkMonitor::NetworkMonitor( MainWindow* mainWindow ) : QUdpSocket( )
{
	QSettings settings("MakingThings", "mchelper");
  listen_port = settings.value("udp_listen_port", 10000).toInt();
  this->mainWindow = mainWindow;
  
  bonjourBrowser = new BonjourServiceBrowser(this);
  bonjourResolver = new BonjourServiceResolver(this);
  connect( bonjourBrowser, SIGNAL(currentBonjourRecordsChanged(const QList<BonjourRecord> &)),
          this, SLOT(updateRecords(const QList<BonjourRecord> &)));
  connect( bonjourResolver, SIGNAL(bonjourRecordResolved(const QHostInfo &, int, const BonjourRecord &)),
          this, SLOT(recordResolved(const QHostInfo &, int, const BonjourRecord &)));
  
  connect( this, SIGNAL(deviceArrived(QList<PacketInterface*>)), 
          mainWindow, SLOT(onEthernetDeviceArrived(QList<PacketInterface*>)));
  connect( this, SIGNAL(deviceRemoved(QString)), mainWindow, SLOT(onDeviceRemoved(QString)));
	connect( this, SIGNAL(readyRead()), this, SLOT( processPendingDatagrams() ) );
  connect( this, SIGNAL(msg(QString, MsgType::Type, QString)), mainWindow, SLOT(message(QString, MsgType::Type, QString)));
  bonjourBrowser->browseForServiceType(QLatin1String("_daap._tcp")); // just browse for itunes for now
  if (!bind(listen_port))
  {
    abort();
    QString str = QString("Error: Can't listen on port %1 - make sure it's not already in use.").arg( listen_port );
    emit msg( str, MsgType::Error, "Ethernet" );
  }
}

bool NetworkMonitor::setListenPort( int port )
{
	if(listen_port == port) // don't need to do anything
    return true;
  close( );
	if( !bind( port, QUdpSocket::ShareAddress ) )
	{
		QString str = QString( "Error: Can't listen on port %1 - make sure it's not already in use.").arg( port );
    emit msg( str, MsgType::Error, "Ethernet" );
		return false;
	}
	else
	{
		listen_port = port;
    QSettings settings("MakingThings", "mchelper");
    settings.setValue("udp_listen_port", listen_port);
    QString str = QString( "Now listening on port %1 for UDP messages.").arg( listen_port );
		emit msg( str, MsgType::Notice, "Ethernet" );
		return true;
	}
}

/*
 New data has arrived.
 Read the data and pass it on to the packet interface, according to where it came from.
*/
void NetworkMonitor::processPendingDatagrams()
{
  while( hasPendingDatagrams() )
  {
    QByteArray datagram;
    QHostAddress sender;
    datagram.resize( pendingDatagramSize() );
    readDatagram( datagram.data(), datagram.size(), &sender );
    if( connectedDevices.contains( sender.toString() ) ) // pass the packet through to the packet interface
    	connectedDevices.value( sender.toString() )->newMessage( datagram );
  }
}

/*
 This is called back when the Bonjour register has changed.
 Check to see if records have been added or removed, and update our list accordingly.
 */
void NetworkMonitor::updateRecords(const QList<BonjourRecord> &list)
{
  // first check if any of these records are new
  foreach (BonjourRecord record, list)
  {
    if(!connectedDevices.contains(record.remoteHost().toString()))
      bonjourResolver->resolveBonjourRecord(record);    
  }
  // then check if we have any old records that are no longer in the list
  foreach(PacketUdp *device, connectedDevices)
  {
    if(!list.contains(*device->record()))
    {
      connectedDevices.remove(device->key());
      emit deviceRemoved(device->key()); // device is deleted when board is removed from UI
    }
  }
}

/*
 Called back when a new Bonjour record has been resolved.
 Check to see if we know about this board and if not, create
 a new PacketUdp for it.
*/
void NetworkMonitor::recordResolved(const QHostInfo &hostInfo, int port, BonjourRecord record)
{
  QList<QHostAddress> a = hostInfo.addresses();
  if(a.count())
  {
    if(!connectedDevices.contains(a.first().toString()))
     {
       BonjourRecord *br = new BonjourRecord(record);
       br->hostInfo = hostInfo;
       br->port = port;
       PacketUdp *device = new PacketUdp(br);
       connectedDevices.insert( br->remoteHost().toString(), device );  // stick it in our own list of boards we know about
       connect(device, SIGNAL(msg(QString, MsgType::Type, QString)), mainWindow, SLOT(message(QString, MsgType::Type, QString)));
       QList<PacketInterface*> pi;
       pi << device;
       emit deviceArrived(pi);
     }
  }
}









