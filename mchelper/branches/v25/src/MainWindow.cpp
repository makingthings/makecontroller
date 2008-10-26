
#include <QSettings>
#include <QHostInfo>
#include <QTextStream>
#include <QDir>
#include <QFileDialog>
#include <QTextBlock>
#include <QTime>
#include "MainWindow.h"
#include "PacketUsbSerial.h"

/**
	MainWindow represents, not surprisingly, the main window of the application.
	It handles all the menu items and the UI.
*/
MainWindow::MainWindow(bool no_ui) : QMainWindow( 0 )
{
	setupUi(this);
  this->no_ui = no_ui;
  readSettings( );
  
  // add an item to the list as a UI cue that no boards were found.
  // remove when boards have been discovered
  deviceListPlaceholder = QListWidgetItem( tr("No Make Controllers found...") );
  deviceListPlaceholder.setData( Qt::ForegroundRole, Qt::gray );
  deviceList->addItem( &deviceListPlaceholder );
  
  // default these to off until we see a board
  actionUpload->setEnabled(false);
  actionInspector->setEnabled(false);
  actionResetDevice->setEnabled(false);
  actionSAMBA->setEnabled(false);
  
  // initializations
  inspector = new Inspector(this);
  oscXmlServer = new OscXmlServer(this);
  usbMonitor = new UsbMonitor(this);
  networkMonitor = new NetworkMonitor(this);
  preferences = new Preferences(this, networkMonitor, oscXmlServer);
  uploader = new Uploader(this);
  about = new About();
  
  // device list connections
  connect( deviceList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onDoubleClick()));
  connect( deviceList, SIGNAL(itemSelectionChanged()), this, SLOT(onDeviceSelectionChanged()));
  
  // menu connections
  connect( actionInspector, SIGNAL(triggered()), inspector, SLOT(loadAndShow()));
  connect( actionPreferences, SIGNAL(triggered()), preferences, SLOT(loadAndShow()));
  connect( actionUpload, SIGNAL(triggered()), uploader, SLOT(show()));
  connect( actionClearConsole, SIGNAL(triggered()), outputConsole, SLOT(clear()));
  connect( actionAbout, SIGNAL(triggered()), about, SLOT(show()));
  connect( actionResetDevice, SIGNAL(triggered()), this, SLOT(onDeviceResetRequest()));
  connect( actionSAMBA, SIGNAL(triggered()), this, SLOT(onSamBaRequest()));
  
  // command line connections
  connect( commandLine->lineEdit(), SIGNAL(returnPressed()), this, SLOT(onCommandLine()));
  connect( sendButton, SIGNAL(clicked()), this, SLOT(onCommandLine()));
  
  #ifndef Q_WS_WIN
  #ifndef Q_WS_MAC
  // the USB monitor runs in a separate thread...start it up.
  // only need to do this on non-Windows/OSX machines since automatic device detection is not implemented
  usbMonitor->start();
  #endif
  #endif
}

void MainWindow::readSettings()
{
	QSettings settings("MakingThings", "mchelper");
	
	QSize size = settings.value( "size" ).toSize( );
	if( size.isValid( ) )
		resize( size );
  
  QPoint mainWinPos = settings.value("mainwindow_pos").toPoint();
  if(!mainWinPos.isNull())
    move(mainWinPos);
	
	QList<QVariant> splitterSettings = settings.value( "splitterSizes" ).toList( );
	QList<int> splitterSizes;
	if( !splitterSettings.isEmpty( ) )
	{
		foreach( QVariant setting, splitterSettings )
			splitterSizes << setting.toInt( );
		splitter->setSizes( splitterSizes );
	}
  
  QStringList commands = settings.value( "commands" ).toStringList( );
  foreach( QString cmd, commands )
    commandLine->addItem(cmd);
  if(!commandLine->count()) // always want to have a space on the end of our list of commands
    commandLine->addItem( "" );
  else
    commandLine->setCurrentIndex(commandLine->count() - 1 );
  
  setMaxMessages(settings.value("max_messages", DEFAULT_ACTIVITY_MESSAGES).toInt( ));
}

void MainWindow::setMaxMessages(int msgs)
{
  outputConsole->setMaximumBlockCount(msgs);
}

void MainWindow::writeSettings()
{
	QSettings settings("MakingThings", "mchelper");

	settings.setValue("size", size() );
  settings.setValue("mainwindow_pos", pos());
  settings.setValue("inspector_pos", inspector->pos());
  
	QList<QVariant> splitterSettings;
	QList<int> splitterSizes = splitter->sizes();
	foreach( int splitterSize, splitterSizes )
		splitterSettings << splitterSize;
	settings.setValue("splitterSizes", splitterSettings );
  
  QStringList commands;
  for( int i = 0; i < commandLine->count(); i++ )
    commands << commandLine->itemText(i);
  settings.setValue("commands", commands );
}

/*
 Called as the app is shutting down.
 Save the current app settings.
*/
void MainWindow::closeEvent( QCloseEvent *qcloseevent )
{
	(void)qcloseevent;
	writeSettings( );
}

#ifdef Q_WS_WIN
bool MainWindow::winEvent( MSG* msg, long* result )
{
  if ( msg->message == WM_DEVICECHANGE )
    usbMonitor->onDeviceChangeEventWin( msg->wParam, msg->lParam );
  return false;
}
#endif

/*
 Called back when we get a right click on the device list.
 If it's a SAMBA device, offer to upload firmware,
 otherwise just offer to view the inspector.
*/
void DeviceList::contextMenuEvent(QContextMenuEvent *event)
{
  Board *brd = (Board*)itemAt(event->pos());
  if(brd)
  {
    QMenu menu(this);
    MainWindow *mw = brd->mainWindowRef();
    if(brd->type() == BoardType::UsbSamba)
      menu.addAction(mw->uploadAction());
    else if(brd->type() == BoardType::Ethernet || brd->type() == BoardType::UsbSerial)
    {
      menu.addAction(mw->inspectorAction());
      menu.addAction(mw->resetAction());
      menu.addAction(mw->sambaAction());
    }
    menu.exec(event->globalPos());
  }
}

/*
  A board in the list has been double clicked.
  If it's a sam-ba board, bring up the uploader,
  otherwise bring up the inspector.
*/
void MainWindow::onDoubleClick()
{
  Board *brd = getCurrentBoard( );
  if( brd )
  {
    if( brd->type() == BoardType::UsbSamba )
    {
      if(!uploader->isVisible())
        uploader->show();
      uploader->raise();
      uploader->activateWindow();
    }
    else
    {
      if(!inspector->isVisible())
        inspector->loadAndShow();
      inspector->raise();
      inspector->activateWindow();
    }
  }
}

/*
 Called back when the selection in the device list changes.
 Update the inspector with the selected device's info.
 If the device isn't a SAM-BA device, disable the upload functionality
*/
void MainWindow::onDeviceSelectionChanged()
{
  Board *brd = getCurrentBoard( );
  if(brd)
  {
    if(brd->type() == BoardType::UsbSamba)
    {
      actionUpload->setEnabled(true);
      actionInspector->setEnabled(false);
      actionResetDevice->setEnabled(false);
      actionSAMBA->setEnabled(false);
    }
    else
    {
      inspector->setData(brd);
      actionInspector->setEnabled(true);
      actionUpload->setEnabled(false);
      actionResetDevice->setEnabled(true);
      actionSAMBA->setEnabled(true);
    }
  }
  else
    inspector->clear();
}

/*
 An Ethernet device has been discovered.
 Create an entry for it in the device list.
*/
void MainWindow::onEthernetDeviceArrived(PacketInterface* pi)
{
  QList<Board*> boardList;
  Board *board = new Board(this, pi, oscXmlServer, BoardType::Ethernet, pi->key());
  board->setText(pi->key());
  board->setIcon(QIcon(":icons/network_icon.gif"));
  board->setToolTip(tr("Ethernet Device: ") + pi->key());
  
  if(noUi())
  {
    QTextStream out(stdout);
    out << tr("network device discovered: ") + pi->key() << endl;
  }
  
  boardInit(board);
  boardList.append(board);
  oscXmlServer->sendBoardListUpdate(boardList, true);
}

/*
  A USB device has arrived.  It could be a UsbSerial or a Samba device.
  Because the UsbMonitor runs in a separate thread, we want to 
  create the packet interfaces here, in the main thread.
*/
void MainWindow::onUsbDeviceArrived(QStringList keys, BoardType::Type type)
{
  Board *board;
  QList<Board*> boardList;
  QString noUiString;
  foreach(QString key, keys)
  {
    if( type == BoardType::UsbSerial)
    {
      PacketUsbSerial *usb = new PacketUsbSerial(key);
      board = new Board(this, usb, oscXmlServer, type, key);
      usb->open();
      board->setText(key);
      board->setIcon(QIcon(":icons/usb_icon.gif"));
      board->setToolTip(tr("USB Serial Device: ") + key);
      noUiString = tr("usb device discovered: ") + key;
    }
    else if(type == BoardType::UsbSamba)
    {
      board = new Board(this, 0, 0, type, key);
      board->setText(tr("Unprogrammed Board"));
      board->setIcon(QIcon(":icons/usb_icon.gif"));
      board->setToolTip(tr("Unprogrammed device"));
      noUiString = tr("sam-ba device discovered: ") + key;
    }
    
    if(noUi())
    {
      QTextStream out(stdout);
      out << noUiString << endl;
    }
    
    boardInit(board);
    boardList.append(board);
  }
  oscXmlServer->sendBoardListUpdate(boardList, true);
}

/*
  Initialization common to any kind of board.
*/
void MainWindow::boardInit(Board *board)
{
  connect(board, SIGNAL(newInfo(Board*)), this, SLOT(updateBoardInfo(Board*)));
  // if the placeholder is in there, get rid of it
  int placeholderRow = deviceList->row( &deviceListPlaceholder );
  if( placeholderRow >= 0 )
    deviceList->takeItem( placeholderRow );
  deviceList->addItem(board);
  // if no other boards are selected, select this new one
  if( !getCurrentBoard() )
    deviceList->setCurrentRow (deviceList->count()-1);
  board->sendMessage("/system/info-internal"); // get the board's info
  onDeviceSelectionChanged(); // make sure menus get updated
}

/*
  Called when a board's info has changed.
  Update the Inspector and the OSC XML server with the new info.
*/
void MainWindow::updateBoardInfo(Board *board)
{
  //oscXmlServer->sendBoardListUpdate(board);
  inspector->setData(board);
}

void MainWindow::onDeviceRemoved(QString key)
{
  QList<Board*> boards = getConnectedBoards();
  foreach(Board *board, boards)
  {
    if( board->key() == key )
    {
      Board* brd = (Board*)deviceList->takeItem(deviceList->row(board));
      brd->deleteLater();
      if(noUi())
      {
        QTextStream out(stdout);
        out << tr("network device removed: ") + key;
      }
      break;
    }
  }
  // if no boards are left, put the placeholder back in
  if( !deviceList->count( ) )
    deviceList->addItem( &deviceListPlaceholder );
}

void MainWindow::message(QStringList msgs, MsgType::Type type, QString from)
{
  QStringList post;
  QString currentTime = QTime::currentTime().toString();
  QTextBlockFormat format;
  format.setBackground(msgColor(type));
  QString tofrom("from");
  if(type == MsgType::Command || type == MsgType::XMLMessage)
    tofrom = "to";
  foreach(QString msg, msgs)
    post << QString("%1    %2 %3 %4").arg(currentTime).arg(msg).arg(tofrom).arg(from);

  // because the format will be the same for all lines added via insertPlainText()
  // we need to add a blank line to set our format, then insert the message
  outputConsole->moveCursor(QTextCursor::End);
  if(outputConsole->blockCount())
    outputConsole->insertPlainText("\n");
  outputConsole->textCursor().setBlockFormat(format);
  outputConsole->insertPlainText(post.join("\n"));
  outputConsole->ensureCursorVisible();
}

void MainWindow::message(QString msg, MsgType::Type type, QString from)
{
  if(noUi())
  {
    QTextStream out(stdout);
    out << from + ": " + msg << endl;
  }
  else
  {
    QTextBlockFormat format;
    format.setBackground(msgColor(type));
    QString tofrom = tr("from");
    if(type == MsgType::Command || type == MsgType::XMLMessage)
      tofrom = tr("to");
    
    msg.prepend(QTime::currentTime().toString() + "    "); // todo - maybe make the time text gray
    msg += QString(" %1 %2").arg(tofrom).arg(from);
    outputConsole->appendPlainText(msg); // insert the message
    outputConsole->moveCursor(QTextCursor::End); // move the cursor to the end
    outputConsole->textCursor().setBlockFormat(format); // so that when we set the color, it colors the right block
    outputConsole->ensureCursorVisible();
  }
}

void MainWindow::statusMsg(QString msg, int duration)
{
  statusBar()->showMessage(msg, duration);
}

QColor MainWindow::msgColor(MsgType::Type type)
{
  switch(type)
  {
    case MsgType::Warning:
      return QColor(255, 228, 118, 255); // orange
    case MsgType::Error:
      return QColor(255, 221, 221, 255); // red
    case MsgType::Notice:
      return QColor(235, 235, 235, 235); // light gray
    case MsgType::Response:
      return Qt::white;
    case MsgType::Command:
      return QColor(229, 237, 247, 255); // light blue
    case MsgType::XMLMessage:
      return QColor(219, 250, 224, 255); // green
    default:
      return Qt::white;
  }
}

void MainWindow::setBoardName( QString key, QString name )
{
  QList<Board*> boardList = getConnectedBoards( );
  foreach(Board *board, boardList)
  {
    if( board->key() == key )
    {
      deviceList->item(deviceList->row(board))->setText(name);
      break;
    }
  }
}

/*
  Return the currently selected board in the UI list of boards,
  or NULL if none are selected.
*/
Board* MainWindow::getCurrentBoard( )
{
	if( deviceList->currentItem( ) == &deviceListPlaceholder )
		return 0;
	else
		return (Board*)deviceList->currentItem( );
}

QList<Board*> MainWindow::getConnectedBoards( )
{
	QList<Board*> boardList;
	for( int i = 0; i < deviceList->count( ); i++ )
	{
		if( deviceList->item( i ) != &deviceListPlaceholder )
			boardList.append( (Board*)deviceList->item( i ) );
	}
	return boardList;
}

/*
 The user has pressed return in the command line or clicked the send button.
 Grab the text, print it to the screen and send the message to the currently
 selected board.
*/
void MainWindow::onCommandLine()
{
  QString cmd = commandLine->currentText();
  Board* brd = getCurrentBoard();
  if(cmd.isEmpty() || brd == NULL)
    return;
  message(cmd, MsgType::Command, brd->key()); // print it to screen
  brd->sendMessage(cmd); // send it to the board
  
  // in order to get a readline-style history of commands via up/down arrows
  // we need to keep an empty item at the end of the list so we have a context from which to up-arrow
  if( commandLine->count() >= commandLine->maxCount() )
    commandLine->removeItem( 0 );
  commandLine->insertItem( commandLine->count() - 1, cmd );
  commandLine->setCurrentIndex( commandLine->count() - 1 );
  
  commandLine->clearEditText();
}

void MainWindow::onDeviceResetRequest()
{
  Board* brd = getCurrentBoard();
  if(brd)
  {
    brd->sendMessage("/system/reset 1");
    message (tr("Resetting Board"), MsgType::Notice, "mchelper");
  }
}

void MainWindow::onSamBaRequest()
{
  Board* brd = getCurrentBoard();
  if(brd)
  {
    brd->sendMessage("/system/samba 1");
    message (tr("Setting board into SAM-BA mode"), MsgType::Notice, "mchelper");
  }
}








