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

#include <QFileDialog>
#include <QSettings>
#include "Preferences.h"

Preferences::Preferences(MainWindow *mw, NetworkMonitor *nm, OscXmlServer *oxs) : QDialog( 0 )
{
  setupUi(this);
  mainWindow = mw;
  networkMonitor = nm;
  oscXmlServer = oxs;

  connect(okButton, SIGNAL(accepted()), this, SLOT(applyChanges()));
  connect(defaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
  #if defined (Q_OS_WIN) || defined (Q_OS_MAC)
  // only need to know the sam7 path on *nix - remove this from the prefs window
  uploaderLabel->setParent(0);
  uploaderEdit->setParent(0);
  #endif
  resize(vboxLayout->sizeHint()); // resize the window based on the size hint from the top level layout
}

// read the current settings, load them into the preferences form and then display it
void Preferences::loadAndShow( )
{
  QSettings settings;
  udpListenEdit->setText(QString::number(settings.value("udp_listen_port", DEFAULT_UDP_LISTEN_PORT).toInt()));
  udpSendEdit->setText(QString::number(settings.value("udp_send_port", DEFAULT_UDP_SEND_PORT).toInt()));
  xmlListenEdit->setText(QString::number(settings.value("xml_listen_port", DEFAULT_XML_LISTEN_PORT).toInt()));
  maxMsgsEdit->setText(QString::number(settings.value("max_messages", DEFAULT_ACTIVITY_MESSAGES).toInt()));
  uploaderEdit->setText(settings.value("sam7_path", DEFAULT_SAM7_PATH).toString());
  bool cs = settings.value("check_updates", DEFAULT_CHECK_UPDATES).toBool();
  updatesCheckBox->setChecked(cs);
  cs = settings.value("networkDiscovery", DEFAULT_NETWORK_DISCOVERY).toBool();
  netDiscoveryCheckBox->setChecked(cs);
  this->show( );
}

// rip through the preferences items, see if any have changed
// and call the mainwindow back if it needs to be updated
void Preferences::applyChanges( )
{
  QSettings settings;
  int udp_listen_port = udpListenEdit->text().toInt();
  settings.setValue("udp_listen_port", udp_listen_port);
  networkMonitor->setListenPort(udp_listen_port);

  int udp_send_port = udpSendEdit->text().toInt();
  settings.setValue("udp_send_port", udp_send_port);
  networkMonitor->setSendPort(udp_send_port);

  int xml_listen_port = xmlListenEdit->text().toInt();
  settings.setValue("xml_listen_port", xml_listen_port);
  oscXmlServer->setListenPort(xml_listen_port);

  int max_messages = maxMsgsEdit->text().toInt();
  settings.setValue("max_messages", max_messages);
  mainWindow->setMaxMessages(max_messages);

  settings.setValue("sam7_path", uploaderEdit->text());

  settings.setValue("checkForUpdatesOnStartup", updatesCheckBox->isChecked());

  bool cs = netDiscoveryCheckBox->isChecked();
  settings.setValue("networkDiscovery", cs);
  networkMonitor->setDiscoveryMode( cs );
}

/*
 Set the default values into the UI.
 We'll only actually change anything once the OK button is pressed.
*/
void Preferences::restoreDefaults( )
{
  udpListenEdit->setText(QString::number(DEFAULT_UDP_LISTEN_PORT));
  udpSendEdit->setText(QString::number(DEFAULT_UDP_SEND_PORT));
  xmlListenEdit->setText(QString::number(DEFAULT_XML_LISTEN_PORT));
  maxMsgsEdit->setText(QString::number(DEFAULT_ACTIVITY_MESSAGES));
  uploaderEdit->setText(DEFAULT_SAM7_PATH);
  updatesCheckBox->setChecked(true);
}




