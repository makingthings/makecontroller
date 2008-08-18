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
  void statusMsg(QString msg, int duration = 0);
  
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
  QListWidgetItem deviceListPlaceholder;
  bool no_ui;
  void readSettings();
  void writeSettings();
  void closeEvent( QCloseEvent *qcloseevent );
  void boardInit(Board *board);
  QColor msgColor(MsgType::Type type);
  
private slots:
  void onDeviceSelectionChanged();
  void onUpload();
  void onCommandLine();
  void onDoubleClick();
};

#include "NetworkMonitor.h"
#include "Preferences.h"

#endif // MAINWINDOW_H


