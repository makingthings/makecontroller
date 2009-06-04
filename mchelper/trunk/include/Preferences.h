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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>
#include "ui_preferences.h"
#include "MainWindow.h"
#include "OscXmlServer.h"
#include "NetworkMonitor.h"

#define DEFAULT_UDP_LISTEN_PORT 10000
#define DEFAULT_UDP_SEND_PORT 10000
#define DEFAULT_XML_LISTEN_PORT 11000
#define DEFAULT_ACTIVITY_MESSAGES 150
#define DEFAULT_CHECK_UPDATES true
#define DEFAULT_NETWORK_DISCOVERY true

#define DEFAULT_SAM7_PATH "/usr/bin/sam7" // only relevant for *nix

class MainWindow;

class Preferences : public QDialog, private Ui::PreferencesUi
{
  Q_OBJECT
public:
  Preferences(MainWindow *mw, NetworkMonitor *nm, OscXmlServer *oxs);

private:
  MainWindow *mainWindow;
  NetworkMonitor *networkMonitor;
  OscXmlServer *oscXmlServer;

public slots:
  void loadAndShow( );

private slots:
  void applyChanges( );
  void restoreDefaults( );
};

#endif // PREFERENCES_H



