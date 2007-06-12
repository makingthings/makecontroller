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
	
	udp = new PacketUdp( );
	oscUdp = new Osc( );
	oscUsb = new Osc( ); 
	samba = new Samba( );
	usb = new UsbMonitor( );
	
	#ifdef Q_WS_WIN
	usb->setWidget( this );
	#endif
	 
	oscUdp->setInterfaces( udp, this, application );
	oscUdp->setPreamble( "OscUdp" );
	//oscUsb->setInterfaces( usb, this, application );
	oscUsb->setPreamble( "OscUsb" );
	udp->setInterfaces( oscUdp, this );
	usb->setMessageInterface( this );
  
  // Create the BoardListModel
  boardModel = new BoardListModel( this );
  
  // Some stuff about trying to enable drag/drop
  // list reordering...
  ///////////////////////////////////////////////
  //listViewDevices->setMovement(QListView::Free);
  listViewDevices->setSelectionMode(QAbstractItemView::SingleSelection);
  listViewDevices->setDragEnabled(true);
  //listViewDevices->setDropIndicatorShown(true);
  listViewDevices->setAcceptDrops(true);
  listViewDevices->setAlternatingRowColors(true);
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
	
	if ( udp->open( ) != PacketUdp::OK )
		mainConsole->insertPlainText( "udp> Cannot open socket.\n" );
	
	// UDP signals/slots
	connect( textLocalPort, SIGNAL( editingFinished() ), this, SLOT( newLocalPort( ) ) );
	connect( textRemotePort, SIGNAL( editingFinished() ), this, SLOT( newRemotePort( ) ) );
	connect( textIPAddress, SIGNAL( editingFinished() ), this, SLOT( newHostAddress( ) ) );
	connect( sendButton, SIGNAL( clicked() ), this, SLOT( commandLineEvent( ) ) );
	connect( commandLine->lineEdit(), SIGNAL( returnPressed() ), sendButton, SLOT( click() ) );
	
	//USB signals/slots
	connect( commandLineUsb->lineEdit(), SIGNAL( returnPressed() ), sendButtonUsb, SLOT( click() ) );
	connect( sendButtonUsb, SIGNAL( clicked() ), this, SLOT( commandLineUsbEvent( ) ) );
	
	//setup the pushbuttons
	connect( fileSelectButton, SIGNAL( clicked() ), this, SLOT( fileSelectButtonClicked() ) );
	connect( uploadButton, SIGNAL( clicked() ), this, SLOT( uploadButtonClicked() ) );
	
	// setup the menu
	connect( actionAboutMchelper, SIGNAL( triggered() ), this, SLOT( about( ) ) );
	connect( actionClearOutput, SIGNAL( triggered() ), this, SLOT( clearOutputWindow( ) ) );
	actionClearOutput->setShortcut(tr("Ctrl+X"));
	actionClearOutput->setShortcutContext( Qt::ApplicationShortcut ); // this doesn't seem to have much effect
	
	uploaderThread = 0;
	uploaderThread = new UploaderThread( application, this, samba );
	samba->setMessageInterface( uploaderThread );
	
	// after all is set up, fire up the mechanism that looks for new boards
	checkForNewDevices( ); // initially check for any devices out there
	monitorTimer = new QTimer(this); // then set up a timer to check for others periodically
    connect(monitorTimer, SIGNAL(timeout()), this, SLOT( checkForNewDevices() ) );
    monitorTimer->start( 1000 ); // check for new devices once a second...more often?
  
  // Init the status bar and let the user know the app is loaded
  statusBar()->showMessage( tr("Ready."), 2000);
}

void McHelperWindow::checkForNewDevices( )
{
	QList<PacketInterface*> newBoards;
	usb->scan( &newBoards );
	int newBoardCount = newBoards.count( );
	if( newBoardCount > 0 )
	{
		int i;
		for( i=0; i<newBoardCount; i++ )
		{
			Board *board = new Board();
      		board->name = "UsbSerial Board";
      		board->type = Board::UsbSerial;
      		board->com_port = QString( newBoards.at(i)->location() );
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
    message( 1, "Switching to board: %s\n", name.toAscii().constData());

    statusBar()->showMessage(status_bar_text, 1000);
  }
}

void McHelperWindow::closeEvent( QCloseEvent *qcloseevent )
{
	usb->closeAll( );
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
 
void McHelperWindow::uploadButtonClicked()
{
	//Make sure there's actually something in the text field
	strcpy( fileNameBuffer, fileSelectText->currentText().toAscii().constData() );
	if( strlen( fileNameBuffer) < 0 )
		return;
	
	//if( usb->usbIsOpen( ) )
	  //usb->usbClose( );

	uploaderThread->setBinFileName( fileNameBuffer );
  
  // Let's assume we'll always want to boot from the flash image we just uploaded on reboot
  uploaderThread->setBootFromFlash( true );
	
	flash( );
	
	writeFileSettings();
}

void McHelperWindow::commandLineEvent( )
{
	QString cmd = commandLine->currentText();
	mainConsole->insertPlainText( "OscUdp< ");
	mainConsole->insertPlainText( cmd );
	mainConsole->insertPlainText( "\n" );
  oscUdp->uiSendPacket( cmd );
	commandLine->clearEditText();
	mainConsole->ensureCursorVisible( );
	writeUdpSettings( );
}

void McHelperWindow::commandLineUsbEvent( )
{
  QString cmd = commandLineUsb->currentText();
  mainConsole->insertPlainText( "OscUsb< ");
  mainConsole->insertPlainText( cmd );
  mainConsole->insertPlainText( "\n" );
  oscUsb->uiSendPacket( cmd );
	
  commandLineUsb->clearEditText();
  mainConsole->ensureCursorVisible( );
  writeUsbSettings();
}

void McHelperWindow::clearOutputWindow()
{
  mainConsole->clear( );
	mainConsole->ensureCursorVisible( );
}

void McHelperWindow::newLocalPort( )
{
	udp->setLocalPort( textLocalPort->text().toInt(), true );
}

void McHelperWindow::newRemotePort( )
{
	udp->setRemotePort( textRemotePort->text().toInt() );
}

void McHelperWindow::newHostAddress( )
{
	QHostAddress hostAddress = QHostAddress( textIPAddress->text() );
	udp->setHostAddress( hostAddress );
}

void McHelperWindow::flash()
{
	//if ( !uploaderThread->isFinished() )
    //return;
		
	uploaderThread->start( );
}

McHelperWindow::Status McHelperWindow::requestErase( )
{
	// Check for a reaction
	oscUdp->createMessage( "/ctestee/active", ",i", 0 );
	oscUdp->sendPacket();
	
	return OK;
}

McHelperWindow::Status McHelperWindow::checkForTestProgram()
{
	// Check for a reaction
	oscUdp->createMessage( "/ctestee/active" );
	oscUdp->sendPacket();
	
	//messageInterface->sleepMs( 100 ); ?????????
	
 	OscMessage oscMessage;
	Osc::Status s = oscUdp->receive( &oscMessage );
	if ( s != Osc::OK )
	  return ERROR_NO_RESPONSE;
	// if it returns ok, the chip is programmed and needs to be erased
	return OK;
}

void McHelperWindow::customEvent( QEvent* event )
{
	switch( event->type() )
	{
		case 10000: //to get messages back from the uploader thread
		{
			McHelperEvent* mcHelperEvent = (McHelperEvent*)event;
			if ( !mcHelperEvent->statusBound )
				customMessage( mcHelperEvent->message );
			break;
		}
		case 10001: //to get progress bar info back from the uploader thread
		{
			McHelperProgressEvent* mcHelperProgressEvent = (McHelperProgressEvent*)event;
			progress( mcHelperProgressEvent->progress );
			break;
		}
		default:
			break;
	}
}

void McHelperWindow::customMessage( char* text )
{
	if ( text != 0 )
    mainConsole->insertPlainText( text );
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
			if ( buffer != NULL )
				mainConsole->insertPlainText( buffer );
			else
				mainConsole->clear();
			mainConsole->ensureCursorVisible( );
		}
	}
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
	udp->setRemotePort( remotePort );	// set it in the UDP system
	QString remotePortString = QString::number( remotePort );	// turn it into a string
	textRemotePort->setText( remotePortString );	// and display it onscreen
	
	int localPort = settings.value("localPort", 10000 ).toInt();
	udp->setLocalPort( localPort, false );
	QString localPortString = QString::number( localPort );
	textLocalPort->setText( localPortString );
	
	QString addressString = settings.value("remoteHostAddress", "192.168.0.200").toString();
	QHostAddress hostAddress = QHostAddress( addressString );
	udp->setHostAddress( hostAddress );
	textIPAddress->setText( addressString );
	
	QStringList udpCmdList = settings.value( "udpCmdList", "" ).toStringList();
	commandLine->addItems( udpCmdList );
	
	QStringList usbCmdList = settings.value( "usbCmdList", "" ).toStringList();
	commandLineUsb->addItems( usbCmdList );
}

void McHelperWindow::writeFileSettings()
{
	QSettings settings("MakingThings", "mchelper");
	settings.setValue("directory", lastDirectory );
}

void McHelperWindow::writeUdpSettings()
{
	QSettings settings("MakingThings", "mchelper");
	
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
}

void McHelperWindow::writeUsbSettings()
{
	QSettings settings("MakingThings", "mchelper");
	
	QStringList usbCmdList;
	int i;
	for( i = 0; i < 10; i++ )
	{
	  QString cmd = commandLineUsb->itemText( i );
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
		
	uploaderThread->setBinFileName( fileNameBuffer );
  
	if( bootFlash )
		uploaderThread->setBootFromFlash( true );
	
	flash( );
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
void McHelperWindow::usbRemoved( )
{
	// usb->usbClose( );
}
#endif

