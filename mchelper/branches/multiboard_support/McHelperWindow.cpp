/*********************************************************************************

 Copyright 2006 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#include "McHelperWindow.h"

#include <QFileDialog>
#include <QSettings>
#include <QHostAddress>
#include <QMessageBox>
#include "Osc.h"

#define DEVICE_SCAN_FREQ 1000

McHelperWindow::McHelperWindow( McHelperApp* application ) : QMainWindow( 0 )
{
	this->application = application; 
	setupUi(this);
	
	#ifdef Q_WS_WIN
	application->setMainWindow( this );
	#endif
	
	#ifdef Q_WS_MAC
	setWindowIcon( QIcon("IconPackageOSX.icns") );
	#endif
	
	noUI = false;
	
	boardModel = new BoardListModel( application, this, this );
	udp = new NetworkMonitor( ); 
	samba = new SambaMonitor( application, this, boardModel );
	usb = new UsbMonitor( );
	
	#ifdef Q_WS_WIN
	usb->setWidget( this );
	#endif
	 
	udp->setInterfaces( this, application, boardModel );
	usb->setInterfaces( this, application, boardModel );
  
  // Create the BoardListModel
  
  
  // Some stuff about trying to enable drag/drop
  // list reordering...
  ///////////////////////////////////////////////
  //listViewDevices->setMovement(QListView::Free);
  listViewDevices->setSelectionMode(QAbstractItemView::SingleSelection);
  //listViewDevices->setDragEnabled(true);
  //listViewDevices->setDropIndicatorShown(true);
  //listViewDevices->setAcceptDrops(true);
  //listViewDevices->setAlternatingRowColors(true);
  //listViewDevices->setDragDropMode(QAbstractItemView::InternalMove);
  ///////////////////////////////////////////////
  
  // Connect the board list model to the devices listing view
  listViewDevices->setModel( boardModel );
  
  // Wire up the selection changed signal from the
  // model to be handled here
  connect( listViewDevices->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
           this,                                SLOT(deviceSelectionChanged(const QModelIndex &, const QModelIndex &)));
  
  ////////////////////////////////////////////////////////////////////////////
	
	progressBar->setRange( 0, 1000 );
	progressBar->setValue( 0 );
	
	readSettings( );
	lastTabIndex = 0;
	
	// if ( udp->open( ) != PacketUdp::OK )
		// mainConsole->insertPlainText( "udp> Cannot open socket.\n" );
	connect( tabWidget, SIGNAL( currentChanged(int) ), this, SLOT( tabIndexChanged(int) ) );
	
	//USB signals/slots
	connect( commandLine->lineEdit(), SIGNAL(returnPressed()), this, SLOT(commandLineEvent()) );
	
	//setup the pushbuttons
	connect( fileSelectButton, SIGNAL( clicked() ), this, SLOT( fileSelectButtonClicked() ) );
	connect( uploadButton, SIGNAL( clicked() ), this, SLOT( uploadButtonClicked() ) );
	
	// setup the menu
	connect( actionAboutMchelper, SIGNAL( triggered() ), this, SLOT( about( ) ) );
	connect( actionClearOutput, SIGNAL( triggered() ), this, SLOT( clearOutputWindow( ) ) );
	actionClearOutput->setShortcut(tr("Ctrl+X"));
	actionClearOutput->setShortcutContext( Qt::ApplicationShortcut ); // this doesn't seem to have much effect
	
	// after all is set up, fire up the mechanism that looks for new boards
	checkForNewDevices( ); // initially check for any devices out there
	monitorTimer = new QTimer(this); // then set up a timer to check for others periodically
    connect(monitorTimer, SIGNAL(timeout()), this, SLOT( checkForNewDevices() ) );
    monitorTimer->start( DEVICE_SCAN_FREQ ); // check for new devices once a second...more often?
  
  // Init the status bar and let the user know the app is loaded
  statusBar()->showMessage( tr("Ready."), 2000);
}

void McHelperWindow::checkForNewDevices( )
{
	// first check for USB boards
	QList<PacketInterface*> newBoards;
	usb->scan( &newBoards );
	int newBoardCount = newBoards.count( );
	if( newBoardCount > 0 )
	{
		int i;
		for( i=0; i<newBoardCount; i++ )
		{
		  Board *board = new Board( this, application );
          board->key = newBoards.at(i)->getKey();
	      board->type = Board::UsbSerial;
	      board->setPacketInterface( newBoards.at(i) );
	      board->com_port = newBoards.at(i)->location();
	      boardModel->addBoard( board );
		}
	}

	// then for UDP boards
	QList<PacketUdp*> udpBoards;
	udp->scan( &udpBoards );
	newBoardCount = udpBoards.count( );
	if( newBoardCount > 0 )
	{
		int i;
		for( i=0; i<newBoardCount; i++ )
		{
		  Board *board = new Board( this, application );
		  board->setPacketInterface( udpBoards.at(i) );
		  board->key = udpBoards.at(i)->getKey();
	      board->type = Board::Udp;
	      boardModel->addBoard( board );
		}
	} 

	QList<UploaderThread*> sambaBoards;
	samba->scan( &sambaBoards );
	newBoardCount = sambaBoards.count( );
	if( newBoardCount > 0 )
	{
		int i;
		for( i=0; i<newBoardCount; i++ )
		{
		  Board *board = new Board( this, application );
		  board->key = sambaBoards.at(i)->getDeviceKey();
	      board->name = "Samba Board";
	      board->type = Board::UsbSamba;
	      board->setUploaderThread( sambaBoards.at(i) );
	      boardModel->addBoard( board );
		}
	}
}

void McHelperWindow::deviceSelectionChanged ( const QModelIndex & current, const QModelIndex & previous )
{
  QString name = boardModel->data( current, Qt::DisplayRole ).toString();
  QString com_port = boardModel->data( current, BoardListModel::COMPortRole ).toString();
  QString status_bar_text = boardModel->data( current, Qt::StatusTipRole ).toString();
  
  if ( boardModel->flags(current) & Qt::ItemIsEnabled ) {
    boardModel->setActiveBoardIndex( current );
    statusBar()->showMessage(status_bar_text, 1000);
  }
  
  switch( boardModel->getActiveBoard()->type )
  {
  	case Board::UsbSerial: // same for both
  	case Board::Udp:
  		tabWidget->setCurrentIndex( lastTabIndex );
  		tabWidget->setTabEnabled( 0, 1 );
  		tabWidget->setTabEnabled( 1, 1 );
  		tabWidget->setTabEnabled( 2, 0 );
  		break;
  	case Board::UsbSamba:
  		tabWidget->setCurrentIndex( 2 );
  		tabWidget->setTabEnabled( 0, 0 );
  		tabWidget->setTabEnabled( 1, 0 );
  		tabWidget->setTabEnabled( 2, 1 );
  		break;
  }
  // fill up the line edits in the Summary tab with the new board's info
  updateSummaryInfo( boardModel->getActiveBoard() );
}

void McHelperWindow::updateSummaryInfo( Board* board )
{
	systemName->setText( board->name );
	systemSerialNumber->setText( board->serialNumber );
	//systemFirmwareVersion->setText( );
	//systemFreeMemory->setText( );
	netAddressLineEdit->setText( board->ip_address );
	/*
	netMaskLineEdit->setText( );
	netGatewayLineEdit->setText( );
	netMACLineEdit->setText( );
	dhcpCheckBox->setCheckState( );
	webserverCheckBox->setCheckState( ); // Qt::Checked, Qt::Unchecked
	*/
}

void McHelperWindow::tabIndexChanged(int index)
{
	if( index < 2 )
		lastTabIndex = index;
}

void McHelperWindow::closeEvent( QCloseEvent *qcloseevent )
{
	usb->closeAll( );
	// samba->closeAll( );
}

void McHelperWindow::fileSelectButtonClicked()
{
	lastDirectory = QFileDialog::getOpenFileName( this, "Select a file", lastDirectory, "Binary Files (*.bin)");
	strcpy( fileNameBuffer, lastDirectory.toAscii().constData() );
	
	//stick the file path into the text field onscreen
	if( strlen( fileNameBuffer ) > 0 )
	{
		fileSelectText->clearEditText();
		fileSelectText->setEditText( fileNameBuffer );
	}
}
 
void McHelperWindow::uploadButtonClicked( )
{
	//Make sure there's actually something in the text field
	if( fileSelectText->currentText().isEmpty( ) || fileSelectText->currentText().isNull( ) )
		return;
	else
		strcpy( fileNameBuffer, fileSelectText->currentText().toAscii().constData() );
	
	Board* board = boardModel->getActiveBoard();
	if( board->type == Board::UsbSamba )
	{
		if( !board->setBinFileName( fileNameBuffer ) )
			return;
		board->flash( );
	}
	writeFileSettings();
}

void McHelperWindow::commandLineEvent( )
{
  QString cmd = QString( "OscUsb< %1" ).arg(commandLine->currentText() );
  mainConsole->append( cmd );
    
  QModelIndex activeBoard = boardModel->getActiveBoardIndex();
  QString name = boardModel->data( activeBoard, Qt::DisplayRole ).toString();
  boardModel->getActiveBoard()->sendMessage( commandLine->currentText() );
  
  // in order to get a readline-style history of commands via up/down arrows
  // we ened to keep an empty item at the end of the list so we have a context from which to up-arrow
  commandLine->removeItem( 0 );
  commandLine->insertItem( 8, commandLine->currentText() );
  commandLine->insertItem( 9, "" );
  commandLine->setCurrentIndex( 9 );
  mainConsole->ensureCursorVisible( );
  commandLine->clearEditText();
  writeUsbSettings();
}

void McHelperWindow::clearOutputWindow()
{
  mainConsole->clear( );
	mainConsole->ensureCursorVisible( );
}

void McHelperWindow::newLocalPort( )
{
	//udp->setLocalPort( textLocalPort->text().toInt(), true );
}

void McHelperWindow::newRemotePort( )
{
	//udp->setRemotePort( textRemotePort->text().toInt() );
}

void McHelperWindow::newHostAddress( )
{
	//QHostAddress hostAddress = QHostAddress( textIPAddress->text() );
	//udp->setHostAddress( hostAddress );
}

void McHelperWindow::customEvent( QEvent* event )
{
	switch( event->type() )
	{
		case 10000: //to get messages back from the uploader thread
		{
			McHelperEvent* mcHelperEvent = (McHelperEvent*)event;
			if ( !mcHelperEvent->statusBound )
				message( mcHelperEvent->message );
			break;
		}
		case 10001: //to get progress bar info back from the uploader thread
		{
			McHelperProgressEvent* mcHelperProgressEvent = (McHelperProgressEvent*)event;
			progress( mcHelperProgressEvent->progress );
			break;
		}
		case 10003:
		{
			MessageEvent* messageEvent = (MessageEvent*)event;
			message( messageEvent->message );
			//delete messageEvent;
			break;
		}
		case 10005: // a board was uploaded/removed
		{
			MessageEvent* messageEvent = (MessageEvent*)event;
			boardModel->removeBoard( messageEvent->message );
			//delete messageEvent;
			break;
		}
		case 10010: // put a status message in the window
		{
			StatusEvent* statusEvent = (StatusEvent*)event;
			statusBar()->showMessage( statusEvent->message, statusEvent->duration );
		}
		default:
			break;
	}
}

void McHelperWindow::messageThreadSafe( QString string )
{	
	MessageEvent* messageEvent = new MessageEvent( string );
	application->postEvent( this, messageEvent );
}

void McHelperWindow::customMessage( char* text )
{
	if ( text != 0 )
    	mainConsole->append( text );
  	mainConsole->ensureCursorVisible( );
}

void McHelperWindow::progress( int value )
{
	if( value < 0 )
	{
		if ( noUI )
			application->quit( );
		else
			progressBar->reset();
	}
	else
		progressBar->setValue( value );
}

void McHelperWindow::message( int level, char *format, ... )
{
	va_list args;
	char buffer[ 1000 ];
	
	va_start( args, format );
	vsnprintf( buffer, 1000, format, args );
	va_end( args );
	
	if( level == 1 )
	{
		if( noUI )
			printf( buffer );
		else
		{
			mainConsole->ensureCursorVisible( );
			if ( buffer != NULL && strlen(buffer) > 1 )
				mainConsole->insertPlainText( buffer );
			mainConsole->ensureCursorVisible( ); // just to be sure...
		}
	}
}

void McHelperWindow::message( QString string )
{
	if( noUI )
		return;
	mainConsole->ensureCursorVisible( );
	if ( !string.isEmpty( ) && !string.isNull( ) )
		mainConsole->append( string );
	mainConsole->ensureCursorVisible( ); // just to be sure...
}

void McHelperWindow::sleepMs( int ms )
{
  //usleep( ms * 1000 );	
}

// Read and write the last values used - address, ports, directory searched etc...
void McHelperWindow::readSettings()
{
	QSettings settings("MakingThings", "mchelper");
	
	lastDirectory = settings.value("directory", "/home").toString();
	fileSelectText->setEditText( lastDirectory );
	
	int remotePort = settings.value("remotePort", 10000 ).toInt();	//read the port back as an int
	//udp->setRemotePort( remotePort );	// set it in the UDP system
	QString remotePortString = QString::number( remotePort );	// turn it into a string
	//textRemotePort->setText( remotePortString );	// and display it onscreen
	
	int localPort = settings.value("localPort", 10000 ).toInt();
	//udp->setLocalPort( localPort, false );
	QString localPortString = QString::number( localPort );
	//textLocalPort->setText( localPortString );
	
	QString addressString = settings.value("remoteHostAddress", "192.168.0.200").toString();
	QHostAddress hostAddress = QHostAddress( addressString );
	//udp->setHostAddress( hostAddress );
	//textIPAddress->setText( addressString );
	
	//QStringList udpCmdList = settings.value( "udpCmdList", "" ).toStringList();
	//commandLine->addItems( udpCmdList );
	
	QStringList usbCmdList = settings.value( "usbCmdList", "" ).toStringList();
	commandLine->addItems( usbCmdList );
}

void McHelperWindow::writeFileSettings()
{
	QSettings settings("MakingThings", "mchelper");
	settings.setValue("directory", lastDirectory );
}

void McHelperWindow::writeUdpSettings()
{
	QSettings settings("MakingThings", "mchelper");
	/*
	QString remoteHostAddress = textIPAddress->text();
	settings.setValue("remoteHostAddress", remoteHostAddress );
	
	int remotePort = textRemotePort->text().toInt();
	settings.setValue("remotePort", remotePort );
	
	int localPort = textLocalPort->text().toInt();
	settings.setValue("localPort", localPort );
	
	QStringList udpCmdList;
	int i;
	for( i = 0; i < 10; i++ )
	{
	  QString cmd = commandLine->itemText( i );
		if( !cmd.isEmpty() )
		  udpCmdList.insert( i, cmd );
	}
	settings.setValue("udpCmdList", udpCmdList );
	*/
}

void McHelperWindow::writeUsbSettings()
{
	QSettings settings("MakingThings", "mchelper");
	
	QStringList usbCmdList;
	int i;
	for( i = 0; i < 10; i++ )
	{
	  QString cmd = commandLine->itemText( i );
		if( !cmd.isEmpty() )
		  usbCmdList.insert( i, cmd );
	}
	settings.setValue("usbCmdList", usbCmdList );
}

void McHelperWindow::setNoUI( bool val )
{
	if( val )
		noUI = true;
	else
		noUI = false;
}

void McHelperWindow::uiLessUpload( char* filename, bool bootFlash )
{
	strcpy( fileNameBuffer, filename );
		//fileNameBuffer = *filename;
		
	// uploaderThread->setBinFileName( fileNameBuffer );
  
	//if( bootFlash )
		//uploaderThread->setBootFromFlash( true );
	
	//flash( );
}

// actions for the menu
void McHelperWindow::about( )  // set the version number here.
{
  QMessageBox::about(this, tr("About mchelper"),
  tr("Make Controller Helper - version 2.0.0\n\n"
  "Making Things 2007\n\n"
  "www.makingthings.com") );
}

#ifdef Q_WS_WIN
void McHelperWindow::usbRemoved( HANDLE deviceHandle )
{
	usb->removalNotification( deviceHandle );
}
#endif

MessageEvent::MessageEvent( QString string ) : QEvent( (Type)10003 )
{
	message = string;
}



