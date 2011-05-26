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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QContextMenuEvent>

#include "Board.h"
#include "Inspector.h"
#include "OscXmlServer.h"
#include "Uploader.h"
#include "About.h"
#include "PacketInterface.h"
#include "MsgType.h"
#include "BoardType.h"
#include "UsbMonitor.h"
#include "AppUpdater.h"

#ifdef MCHELPER_TEST_SUITE
#include "TestXmlServer.h"
#endif

class Inspector;
class NetworkMonitor;
class UsbMonitor;
class Preferences;
class OscXmlServer;
class Uploader;
class Board;

// subclassed so we have access to the context menu events
class DeviceList : public QListWidget
{
  Q_OBJECT
public:
  DeviceList(QWidget *parent = 0) : QListWidget(0)
  {
    setParent(parent);
  }
  void contextMenuEvent(QContextMenuEvent *event);
};

#include "ui_mainwindow.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  MainWindow(bool no_ui);
  bool noUi() {return no_ui;}
  void setMaxMessages(int msgs);
  Board* getCurrentBoard();
  QList<Board*> getConnectedBoards();
  // make these available to the device list
  QAction* uploadAction() { return ui.actionUpload; }
  QAction* inspectorAction() { return ui.actionInspector; }
  QAction* resetAction() { return ui.actionResetBoard; }
  QAction* sambaAction() { return ui.actionEraseBoard; }
  void statusMsg(const QString & msg, int duration = 3500);
  void newXmlPacketReceived( const QList<OscMessage*> & msgs, const QString & destination );

public slots:
  void onEthernetDeviceArrived(PacketInterface* pi);
  void onUsbDeviceArrived(const QStringList & keys, BoardType::Type type);
  void onDeviceRemoved(const QString & key);
  void message(const QString & msg, MsgType::Type type, const QString & from = "mchelper");
  void message(const QStringList & msgs, MsgType::Type type, const QString & from = "mchelper");
  void setBoardName( const QString & key, const QString & name );
  void updateBoardInfo(Board *board);

private:
  Ui::MainWindowUi ui;
  Inspector *inspector;
  NetworkMonitor *networkMonitor;
  UsbMonitor *usbMonitor;
  Preferences *preferences;
  OscXmlServer *oscXmlServer;
  Uploader *uploader;
  About *about;
  AppUpdater* appUpdater;
  QListWidgetItem deviceListPlaceholder;
  QTextCharFormat grayText, blackText;
  bool no_ui;
  bool hideOscMsgs;
  void readSettings();
  void writeSettings();
  void closeEvent( QCloseEvent *qcloseevent );
  void boardInit(Board *board);
  QColor msgColor(MsgType::Type type);
  QString msgColorStr(MsgType::Type type);
  void addMessage(const QString & time, const QString & msg, const QString & tofrom, const QString & bkgnd);
  bool messagesEnabled( MsgType::Type type );

private slots:
  void onDeviceSelectionChanged();
  void onCommandLine();
  void onDoubleClick();
  void onDeviceResetRequest();
  void onEraseRequest();
  void onHideOsc(bool checked);
  void onCheckForUpdates(bool inBackground = false);
  void onHelp();
  void onOscTutorial();

signals:
  void boardInfoUpdate(Board* board);

  #ifdef MCHELPER_TEST_SUITE
  friend class TestXmlServer;
  #endif
};

#include "NetworkMonitor.h"
#include "Preferences.h"

#endif // MAINWINDOW_H


