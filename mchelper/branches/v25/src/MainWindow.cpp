
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
  
  // initializations
  inspector = new Inspector(this);
  networkMonitor = new NetworkMonitor(this);
  usbMonitor = new UsbMonitor(this);
  oscXmlServer = new OscXmlServer(this);
  preferences = new Preferences(this, networkMonitor, oscXmlServer);
  uploader = new Uploader(this);
  about = new About();
  
  // device list connections
  connect( deviceList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onDoubleClick()));
  connect( deviceList, SIGNAL(itemSelectionChanged()), this, SLOT(onDeviceSelectionChanged()));
  
  // menu connections
  connect( actionInspector, SIGNAL(triggered()), inspector, SLOT(loadAndShow()));
  connect( actionPreferences, SIGNAL(triggered()), preferences, SLOT(loadAndShow()));
  connect( actionUpload, SIGNAL(triggered()), this, SLOT(onUpload()));
  connect( actionClearConsole, SIGNAL(triggered()), outputConsole, SLOT(clear()));
  connect( actionAbout, SIGNAL(triggered()), about, SLOT(show()));
  
  // command line connections
  connect( commandLine->lineEdit(), SIGNAL(returnPressed()), this, SLOT(onCommandLine()));
  connect( sendButton, SIGNAL(clicked()), this, SLOT(onCommandLine()));
  
  // add an item to the list as a UI cue that no boards were found.
	// remove when boards have been discovered
	deviceListPlaceholder = QListWidgetItem( "No Make Controllers found..." );
	deviceListPlaceholder.setData( Qt::ForegroundRole, Qt::gray );
	deviceList->addItem( &deviceListPlaceholder );
  
  // the USB monitor runs in a separate thread...start it up
  usbMonitor->start();
}

void MainWindow::readSettings()
{
	QSettings settings("MakingThings", "mchelper");
	
	QSize size = settings.value( "size" ).toSize( );
	if( size.isValid( ) )
		resize( size );
	
	QList<QVariant> splitterSettings = settings.value( "splitterSizes" ).toList( );
	QList<int> splitterSizes;
	if( !splitterSettings.isEmpty( ) )
	{
		for( int i = 0; i < splitterSettings.count( ); i++ )
			splitterSizes.append( splitterSettings.at(i).toInt( ) );
		splitter->setSizes( splitterSizes );
	}
  
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
	QList<QVariant> splitterSettings;
	QList<int> splitterSizes = splitter->sizes();
	for( int i = 0; i < splitterSizes.count( ); i++ )
		splitterSettings.append( splitterSizes.at(i) );
	settings.setValue("splitterSizes", splitterSettings );
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
      menu.addAction(mw->inspectorAction());
    menu.exec(event->globalPos());
  }
}

void MainWindow::onDoubleClick()
{
  if(getCurrentBoard())
    inspector->loadAndShow();
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
    inspector->setData(brd);
    if(brd->type() == BoardType::UsbSamba)
      actionUpload->setEnabled(true);
    else
      actionUpload->setEnabled(false);
  }
}

/*
 The upload action has been triggered.
 Pop up a file dialog and try to upload the file selected by the user.
*/
void MainWindow::onUpload()
{
  QSettings settings("MakingThings", "mchelper");
  QString lastFilePath = settings.value("last_firmware_upload", QDir::homePath()).toString();
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), lastFilePath, tr("Binaries (*.bin)"));
	if(fileName.isNull()) // user canceled
		return;
  settings.setValue("last_firmware_upload", fileName);
  if(uploader->state() == QProcess::NotRunning)
    uploader->upload(fileName);
  else
    return statusBar()->showMessage( "Uploader is currently busy...give it a second, then try again.", 3500 );
}

/*
 An Ethernet device has been discovered.
 Create an entry for it in the device list.
*/
void MainWindow::onEthernetDeviceArrived(PacketInterface* pi)
{
  QList<Board*> boardList;
  Board *board = new Board(this, pi, oscXmlServer, BoardType::Ethernet);
  board->setText(pi->key());
  board->setIcon(QIcon(":icons/network_icon.gif"));
  board->setToolTip("Ethernet Device: " + pi->key());
  if(noUi())
  {
    QTextStream out(stdout);
    out << "network device discovered: " + pi->key() << endl;
  }
  deviceList->addItem(board);
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
      board = new Board(this, usb, oscXmlServer, type);
      usb->open();
      board->setText(key);
      board->setIcon(QIcon(":icons/usb_icon.gif"));
      board->setToolTip("USB Serial Device: " + key);
      noUiString = "usb device discovered: " + key;
    }
    else if(type == BoardType::UsbSamba)
    {
      board = new Board(this, 0, 0, type);
      board->setText("Unprogrammed Board");
      board->setIcon(QIcon(":icons/usb_icon.gif"));
      board->setToolTip("Unprogrammed device");
      noUiString = "sam-ba device discovered: " + key;
    }
    
    if(noUi())
    {
      QTextStream out(stdout);
      out << noUiString << endl;
    }
    deviceList->addItem(board);
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
    if(board->key() == key)
    {
      delete deviceList->takeItem(deviceList->row(board));
      if(noUi())
      {
        QTextStream out(stdout);
        out << "network device removed: " + key;
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
    QString tofrom("from");
    if(type == MsgType::Command || type == MsgType::XMLMessage)
      tofrom = "to";
    
    msg.prepend(QTime::currentTime().toString() + "    "); // todo - maybe make the time text gray
    msg += QString(" %1 %2").arg(tofrom).arg(from);
    outputConsole->appendPlainText(msg); // insert the message
    outputConsole->moveCursor(QTextCursor::End); // move the cursor to the end
    outputConsole->textCursor().setBlockFormat(format); // so that when we set the color, it colors the right block
    outputConsole->ensureCursorVisible();
  }
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
  if( commandLine->count() < 10 )
  	commandLine->addItem( cmd );
  else
  {
	  commandLine->removeItem( 0 );
	  commandLine->insertItem( 9, "" );
	  commandLine->insertItem( 8, cmd );
	  commandLine->setCurrentIndex( 9 );
  }
  
  commandLine->clearEditText();
}








