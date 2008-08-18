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

#ifdef Q_WS_WIN
  #define DEFAULT_SAM7_PATH "C:\something\sam7.exe"
#else
  #define DEFAULT_SAM7_PATH "/usr/bin/sam7"
#endif

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



