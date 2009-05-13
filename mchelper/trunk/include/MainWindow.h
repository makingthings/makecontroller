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

class MainWindow : public QMainWindow, private Ui::MainWindowUi
{
  Q_OBJECT
public:
  MainWindow(bool no_ui);
  bool noUi() {return no_ui;}
  void setMaxMessages(int msgs);
  Board* getCurrentBoard();
  QList<Board*> getConnectedBoards();
  // make these available to the device list
  QAction* uploadAction() { return actionUpload; }
  QAction* inspectorAction() { return actionInspector; }
  QAction* resetAction() { return actionResetBoard; }
  QAction* sambaAction() { return actionEraseBoard; }
  void statusMsg(const QString & msg, int duration = 3500);
  void newXmlPacketReceived( const QList<OscMessage*> & msgs, const QString & destination );

public slots:
  void onEthernetDeviceArrived(PacketInterface* pi);
  void onUsbDeviceArrived(QStringList keys, BoardType::Type type);
  void onDeviceRemoved(QString key);
  void message(QString msg, MsgType::Type type, QString from = "mchelper");
  void message(QStringList msgs, MsgType::Type type, QString from = "mchelper");
  void setBoardName( QString key, QString name );
  void updateBoardInfo(Board *board);

private:
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
  #ifdef Q_WS_WIN
  bool winEvent( MSG* msg, long* result );
  #endif
  void addMessage( const QString & time, const QString & msg, const QString & tofrom, const QTextBlockFormat & bkgnd );
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


