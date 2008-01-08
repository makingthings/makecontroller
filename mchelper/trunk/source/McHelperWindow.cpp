/*********************************************************************************

 Copyright 2006-2007 MakingThings

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
#include <QMessageBox> 
#include <QHeaderView>
#include <QCheckBox>
#include <QDesktopServices>
#include <QSizePolicy>
#include "Osc.h"
#include "BoardArrivalEvent.h"

#ifdef Q_WS_MAC
#include "carbon_cocoa.h"
#endif // Q_WS_MAC

#define DEVICE_SCAN_FREQ 1000
#define SUMMARY_MESSAGE_FREQ 2000

McHelperWindow::McHelperWindow( McHelperApp* application ) : QMainWindow( 0 )
{
	this->application = application;
	setupUi(this);
	readSettings( );
	aboutDialog = new aboutMchelper( );
	prefsDialog = new mchelperPrefs( this );
	appUpdater = new AppUpdater( );
	
	#ifdef Q_WS_WIN
	application->setMainWindow( this );
	#endif
	
	#ifdef Q_WS_MAC
	setWindowIcon( QIcon("IconPackageOSX.icns") );
	#endif
	
	noUI = false;
	splitter->setChildrenCollapsible( false );
	initUpdate( );
	
	//listWidget = new BoardListModel( application, this, this );
	udp = new NetworkMonitor( appUdpListenPort, appUdpSendPort ); 
	samba = new SambaMonitor( application, this );
	usb = new UsbMonitor( );
	xmlServer = new OscXmlServer( this, appXmlListenPort );
	 
	udp->setInterfaces( this, this, application );
	usb->setInterfaces( this, application, this );
	
	outputModel = new OutputWindow( maxOutputWindowMessages );
	outputView->setModel( outputModel );
  
  setupOutputWindow();

  // Wire up the selection changed signal from the
  // model to be handled here
	qRegisterMetaType<QModelIndex>("QModelIndex");
  connect( listWidget->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
           this,                                SLOT(deviceSelectionChanged(const QModelIndex &, const QModelIndex &)));
  
  ////////////////////////////////////////////////////////////////////////////
	
	progressBar->setRange( 0, 1000 );
	progressBar->setValue( 0 );
	lastTabIndex = 0;
	
	connect( tabWidget, SIGNAL( currentChanged(int) ), this, SLOT( tabIndexChanged(int) ) );
	
	//USB signals/slots
	connect( commandLine->lineEdit(), SIGNAL(returnPressed()), this, SLOT(commandLineEvent( ) ) );
	connect( sendButton, SIGNAL( clicked( ) ), this, SLOT(commandLineEvent( ) ) );
	
	//setup the pushbuttons
	connect( fileSelectButton, SIGNAL( clicked() ), this, SLOT( fileSelectButtonClicked() ) );
	connect( uploadButton, SIGNAL( clicked() ), this, SLOT( uploadButtonClicked() ) );
	
	// setup the Summary tab
	connect( systemName, SIGNAL( textEdited( QString ) ), this, SLOT( onAnySummaryValueEdited( QString ) ) );
	connect( systemSerialNumber, SIGNAL( textEdited( QString ) ), this, SLOT( onAnySummaryValueEdited( QString ) ) );
	connect( netAddressLineEdit, SIGNAL( textEdited( QString ) ), this, SLOT( onAnySummaryValueEdited( QString ) ) );
	connect( netMaskLineEdit, SIGNAL( textEdited( QString ) ), this, SLOT( onAnySummaryValueEdited( QString ) ) );
	connect( netGatewayLineEdit, SIGNAL( textEdited( QString ) ), this, SLOT( onAnySummaryValueEdited( QString ) ) );
	connect( udpListenPort, SIGNAL( textEdited( QString ) ), this, SLOT( onAnySummaryValueEdited( QString ) ) );
	connect( udpSendPort, SIGNAL( textEdited( QString ) ), this, SLOT( onAnySummaryValueEdited( QString ) ) );
	connect( dhcpCheckBox, SIGNAL( clicked( bool ) ), this, SLOT( onAnySummaryValueEdited( bool ) ) );
	connect( webserverCheckBox, SIGNAL( clicked( bool ) ), this, SLOT( onAnySummaryValueEdited( bool ) ) );
	connect( applyChangesButton, SIGNAL( clicked() ), this, SLOT( onApplyChanges() ) );
	connect( revertChangesButton, SIGNAL( clicked() ), this, SLOT( onRevertChanges() ) );
    
	// setup the menu
	connect( actionAboutMchelper, SIGNAL( triggered() ), aboutDialog, SLOT( show( ) ) );
	connect( actionCheckForUpdates, SIGNAL( triggered() ), this, SLOT( onActionCheckForUpdates( ) ) );
	connect( actionPreferences, SIGNAL( triggered() ), prefsDialog, SLOT( show( ) ) );
	connect( actionClearOutputWindow, SIGNAL( triggered() ), outputModel, SLOT( clear( ) ) );
	connect( actionMchelper_Help, SIGNAL( triggered() ), this, SLOT( openMchelperHelp( ) ) );
	connect( actionOSC_Tutorial, SIGNAL( triggered() ), this, SLOT( openOSCTuorial( ) ) );
	connect( actionHide_OSC_Messages, SIGNAL( triggered(bool) ), this, SLOT( outWindowHideOSCMessages(bool) ) );
	
	connect( &summaryTimer, SIGNAL(timeout()), this, SLOT( sendSummaryMessage() ) );
	connect( &outputWindowTimer, SIGNAL(timeout()), this, SLOT( postMessages( ) ) );
	
	// after all is set up, fire up the mechanisms that look for new boards
  usb->start( );
  udp->start( );
  samba->start( );
	outputWindowTimer.start( 50 );
}

void McHelperWindow::usbBoardsArrived( QList<PacketInterface*> arrived )
{
	Board* board;
	QList<Board*> boardList;
	int i;
	for( i=0; i<arrived.count( ); i++ )
	{
	  board = new Board( this, this, application );
    board->key = arrived.at(i)->getKey();
    board->type = Board::UsbSerial;
    board->setPacketInterface( arrived.at(i) );
    board->location = QString( arrived.at(i)->location( ) );
    board->setText( QString( board->locationString() ) );
    connectedBoards.insert( board->key, board );
    listWidget->addItem( board );
		boardList.append( board );
	}
	xmlServer->boardListUpdate( boardList, true );
}

void McHelperWindow::udpBoardsArrived( QList<PacketUdp*> arrived )
{
	Board* board;
	QList<Board*> boardList;
	int i;
	for( i=0; i<arrived.count( ); i++ )
	{
	  board = new Board( this, this, application );
	  board->setPacketInterface( arrived.at(i) );
	  board->key = arrived.at(i)->getKey();
		board->location = QString( arrived.at(i)->location( ) );
    board->type = Board::Udp;
    connectedBoards.insert( board->key, board );
    board->setText( QString( board->locationString() ) );
    listWidget->addItem( board );
    board->sendMessage( "/system/info-internal" );
		boardList.append( board );
	}
	xmlServer->boardListUpdate( boardList, true );
}

void McHelperWindow::sambaBoardsArrived( QList<UploaderThread*> arrived )
{
	Board* board;
	int i;
	for( i=0; i<arrived.count( ); i++ )
	{
	  board = new Board( this, this, application );
  	board->key = arrived.at(i)->getDeviceKey();
    board->name = "Samba Board";
    board->type = Board::UsbSamba;
    board->setUploaderThread( arrived.at(i) );
    connectedBoards.insert( board->key, board );
    board->setText( board->name );
    listWidget->addItem( board );
	}
}

Board* McHelperWindow::getCurrentBoard( )
{
	Board* board = (Board*)listWidget->currentItem( );
	return board;
}

QList<Board*> McHelperWindow::getConnectedBoards( )
{
	QList<Board*> boardList;
	for( int i = 0; i < listWidget->count( ); i++ )
		boardList.append( (Board*)listWidget->item( i ) );

	return boardList;
}

bool McHelperWindow::summaryTabIsActive( )
{
	if( tabWidget->currentIndex() == 1 )
		return true;
	else
		return false;
}

void McHelperWindow::removeDeviceThreadSafe( QString key )
{
	BoardEvent* boardEvent = new BoardEvent( key );
	application->postEvent( this, boardEvent );
}

void McHelperWindow::removeDevice( QString key )
{
	if( connectedBoards.contains( key ) )
	{
		int row = listWidget->row( connectedBoards.value(key) );
		Board* removed = (Board*)listWidget->takeItem( row );
		QList<Board*> boardList;
		boardList.append( removed );
		xmlServer->boardListUpdate( boardList, false );
		delete removed;
	}
}

void McHelperWindow::deviceSelectionChanged ( const QModelIndex & current, const QModelIndex & previous )
{
  (void)current;
	(void)previous;
	Board* board = (Board*)listWidget->currentItem();
  if( board == NULL )
  	return;
  	
  switch( board->type )
  {
  	case Board::UsbSerial:
  	case Board::Udp:
  		tabWidget->setCurrentIndex( lastTabIndex );
  		tabWidget->setTabEnabled( 0, 1 );
  		tabWidget->setTabEnabled( 1, 1 );
  		tabWidget->setTabEnabled( 2, 0 );
  		if( summaryTabIsActive( ) )
  			board->sendMessage( "/system/info-internal" );
  			
  		updateSummaryInfo( );
			tabWidget->setTabToolTip( 2, "Short the ERASE jumper on your board to enable the uploader." );
  		break;
        
  	case Board::UsbSamba:
  		tabWidget->setCurrentIndex( 2 );
  		tabWidget->setTabEnabled( 0, 0 );
  		tabWidget->setTabEnabled( 1, 0 );
  		tabWidget->setTabEnabled( 2, 1 );
			tabWidget->setTabToolTip( 2, NULL );
  		break;
  }
}

void McHelperWindow::updateDeviceList( )
{
	UpdateEvent* event = new UpdateEvent( );
  application->postEvent( this, event );
}

void McHelperWindow::updateSummaryInfo( )
{
	Board* board = (Board*)listWidget->currentItem();
  if( board == NULL )
  	return;
	
	if( systemName->text() != board->name )
		systemName->setText( board->name );
		
	if( systemSerialNumber->text() != board->serialNumber )
		systemSerialNumber->setText( board->serialNumber );
		
	if( systemFirmwareVersion->text() != board->firmwareVersion )
		systemFirmwareVersion->setText( board->firmwareVersion );
		
	if( systemFreeMemory->text() != board->freeMemory )
		systemFreeMemory->setText( board->freeMemory );
		
	if( netAddressLineEdit->text() != board->ip_address )
		netAddressLineEdit->setText( board->ip_address );
		
	if( netMaskLineEdit->text() != board->netMask )
		netMaskLineEdit->setText( board->netMask );
		
	if( netGatewayLineEdit->text() != board->gateway )
		netGatewayLineEdit->setText( board->gateway );
		
	if( udpListenPort->text() != board->udp_listen_port )
		udpListenPort->setText( board->udp_listen_port );
	
	if( udpSendPort->text() != board->udp_send_port )
		udpSendPort->setText( board->udp_send_port );
	
	Qt::CheckState boardDhcpState = (board->dhcp) ? Qt::Checked : Qt::Unchecked;
	if( dhcpCheckBox->checkState( ) != boardDhcpState )
		dhcpCheckBox->setCheckState( boardDhcpState );
		
	Qt::CheckState boardWebServerState = (board->webserver) ? Qt::Checked : Qt::Unchecked;
	if( webserverCheckBox->checkState( ) != boardWebServerState )
		webserverCheckBox->setCheckState( boardWebServerState );
}

void McHelperWindow::tabIndexChanged(int index)
{
	if( index < 2 )
		lastTabIndex = index;
	
	if( index == 1 )
	{
		Board* board = (Board*)listWidget->currentItem();
  	if( board != NULL )
  		board->sendMessage( "/system/info-internal" );
  		
  	if( !summaryTimer.isActive( ) )
  		summaryTimer.start( SUMMARY_MESSAGE_FREQ );
	}
	else
	{
		if( summaryTimer.isActive( ) )
			summaryTimer.stop( );
	}
}

void McHelperWindow::sendSummaryMessage( )
{
	Board* board = (Board*)listWidget->currentItem();
	if( board != NULL )
		board->sendMessage( "/system/info-internal" );
}

void McHelperWindow::closeEvent( QCloseEvent *qcloseevent )
{
	(void)qcloseevent;
	usb->closeAll( );
	QSettings settings("MakingThings", "mchelper");
	settings.setValue("mainWindowSize", size() );
	QList<QVariant> splitterSettings;
	QList<int> splitterSizes = splitter->sizes();
	for( int i = 0; i < splitterSizes.count( ); i++ )
		splitterSettings.append( splitterSizes.at(i) );
	settings.setValue("splitterSizes", splitterSettings );
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
	
	Board* board = (Board*)listWidget->currentItem();
	if( board == NULL )
		return;
		
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
  Board* board = (Board*)listWidget->currentItem();
  if( board == NULL )
  	return;
  
  QString cmd = commandLine->currentText();
  if( cmd.isEmpty() )
  	return;
  	
  messageThreadSafe( cmd, MessageEvent::Command, board->locationString() );
  board->sendMessage( cmd );
  
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
  writeUsbSettings( );
}

void McHelperWindow::newXmlPacketReceived( QList<OscMessage*> messageList, QString address )
{
	Board *board;
	for( int i = 0; i < listWidget->count( ); i++ )
	{
		board = (Board*)listWidget->item( i );
		if( board->key == address )
			board->sendMessage( messageList );
	}
}

void McHelperWindow::sendXmlPacket( QList<OscMessage*> messageList, QString srcAddress )
{
	if( xmlServer->isConnected( ) )
		xmlServer->sendXmlPacket( messageList, srcAddress, udp->getListenPort( ) );
}

void McHelperWindow::xmlServerBoardInfoUpdate( Board* board )
{
	xmlServer->boardInfoUpdate( board );
}

void McHelperWindow::customEvent( QEvent* event )
{
	switch( event->type() )
	{
		case 10000: //to get messages back from the uploader thread
		{
			McHelperEvent* mcHelperEvent = (McHelperEvent*)event;
			if ( !mcHelperEvent->statusBound )
				messageThreadSafe( mcHelperEvent->message );
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
			messageThreadSafe( messageEvent->message, messageEvent->type, messageEvent->from );
			
			break;
		}
		case 10004:
		{
			MessageEvent* messageEvent = (MessageEvent*)event;
			messageThreadSafe( messageEvent->messages, messageEvent->type, messageEvent->from );
			break;
		}
		case 10005: // a board was uploaded/removed
		{
			MessageEvent* messageEvent = (MessageEvent*)event;
			removeDevice( messageEvent->message );
			break;
		}
		case 11111: // there's new info about the board to shove into the UI
		{
			updateSummaryInfo( );
			listWidget->update( );
			break;
		}
		case 10010: // put a status message in the window
		{
			StatusEvent* statusEvent = (StatusEvent*)event;
			statusBar()->showMessage( statusEvent->message, statusEvent->duration );
			break;
		}
    case 10020: // a new board has been connected
    {
        BoardArrivalEvent* arrivalEvent = (BoardArrivalEvent*)event;
        switch( arrivalEvent->type )
        {
        	case Board::UsbSerial:
        		usbBoardsArrived( arrivalEvent->pInt );
        		break;
        	case Board::Udp:
        		udpBoardsArrived( arrivalEvent->pUdp );
        		break;
        	case Board::UsbSamba:
        		sambaBoardsArrived( arrivalEvent->uThread );
        		break;
        	default:
        		break;
        }
        break;
    }
		default:
			break;
	}
}

void McHelperWindow::statusMessage( const QString & msg, int duration )
{
	statusBar()->showMessage( msg, duration );
}

void McHelperWindow::messageThreadSafe( QString string  )
{ 
  // Default to an "Info" message if we don't know otherwise
  messageThreadSafe( string, MessageEvent::Info );
}

void McHelperWindow::messageThreadSafe( QString string, MessageEvent::Types type  )
{	
  // Default to coming from the "App" itself if we don't know otherwise
  messageThreadSafe( string, type, QString("mchelper") );
}

void McHelperWindow::messageThreadSafe( QString string, MessageEvent::Types type, QString from )
{ 
  if( hideOSCMessages )
	{
		if( type == MessageEvent::Response || type == MessageEvent::XMLMessage || type == MessageEvent::Warning )
			return;
	}
  
	TableEntry newItem = createOutputWindowEntry( string, type, from );
	QMutexLocker locker(&outputWindowQueueMutex);
	outputWindowQueue.append( newItem );
}

void McHelperWindow::messageThreadSafe( QStringList strings, MessageEvent::Types type, QString from )
{ 
  if( hideOSCMessages )
	{
		if( type == MessageEvent::Response || type == MessageEvent::XMLMessage || type == MessageEvent::Error )
			return;
	}
  
	QMutexLocker locker(&outputWindowQueueMutex); 
	for( int i = 0; i < strings.count( ); i ++ )
	{
		QString string = strings.at(i);
		TableEntry newItem = createOutputWindowEntry( string, type, from );
		outputWindowQueue.append( newItem );
	}
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

void McHelperWindow::postMessages( )
{
	if( outputWindowQueue.count( ) > 0 )
	{
		QMutexLocker locker(&outputWindowQueueMutex);
		outputModel->newRows( outputWindowQueue );
		outputWindowQueue.clear( );
		outputView->resizeColumnToContents( McHelperWindow::TO_FROM );
		outputView->resizeColumnToContents( McHelperWindow::TIMESTAMP );
		outputView->scrollToBottom( );
	}
}


TableEntry McHelperWindow::createOutputWindowEntry( QString string, MessageEvent::Types type, QString from )
{
	// create the to/from, message, and timestamp columns for the new message
	TableEntry newItem;
	newItem.column0 = from;
	newItem.column1 = string;
	newItem.column2 = QTime::currentTime().toString();
	newItem.type = type;
	return newItem;
}

void McHelperWindow::setupOutputWindow( )
{
	QHeaderView *headerHView = outputView->horizontalHeader();
	headerHView->setDefaultAlignment( Qt::AlignHCenter );
	headerHView->setClickable( false );
	headerHView->setMovable( false );
	headerHView->setStretchLastSection( false ); // we want the middle message section to be the one that does the stretching
	headerHView->setResizeMode( McHelperWindow::MESSAGE, QHeaderView::Stretch);
	headerHView->setResizeMode( McHelperWindow::TO_FROM, QHeaderView::Fixed);
	headerHView->hide( );
	
	QHeaderView *headerV = outputView->verticalHeader();
	headerV->setResizeMode(QHeaderView::Fixed);
	headerV->setDefaultSectionSize( 18 );
	headerV->hide();
}

// Read and write the last values used - address, ports, directory searched etc...
void McHelperWindow::readSettings()
{
	QSettings settings("MakingThings", "mchelper");
	
	lastDirectory = settings.value("directory", "/home").toString();
	fileSelectText->setEditText( lastDirectory );
	
	appUdpListenPort = settings.value( "appUdpListenPort", 10000 ).toInt( );
	appUdpSendPort = settings.value( "appUdpSendPort", 10000 ).toInt( );
	appXmlListenPort = settings.value( "appXmlListenPort", 11000 ).toInt( );
	findEthernetBoardsAuto = settings.value( "findEthernetBoardsAuto", true ).toBool( );
	
	hideOSCMessages = settings.value( "hideOSCMessages", false ).toBool( );
	actionHide_OSC_Messages->setChecked( hideOSCMessages );
	
	maxOutputWindowMessages = settings.value( "maxOutputWindowMessages", 1000 ).toInt( );
	
	QSize mainWindowSize = settings.value( "mainWindowSize" ).toSize( );
	if( mainWindowSize.isValid( ) )
		resize( mainWindowSize );
		
	QList<QVariant> splitterSettings = settings.value( "splitterSizes" ).toList( );
	QList<int> splitterSizes;
	if( !splitterSettings.isEmpty( ) )
	{
		for( int i = 0; i < splitterSettings.count( ); i++ )
			splitterSizes.append( splitterSettings.at(i).toInt( ) );
		splitter->setSizes( splitterSizes );
	}
	
	QStringList usbCmdList = settings.value( "usbCmdList", "" ).toStringList();
	commandLine->addItems( usbCmdList );
	if( usbCmdList.count() > 0 )
	{
		commandLine->insertItem( 9, "" );
		commandLine->setCurrentIndex( 9 );
	}
}

void McHelperWindow::writeFileSettings()
{
	QSettings settings("MakingThings", "mchelper");
	settings.setValue("directory", lastDirectory );
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
	(void)filename;
	(void)bootFlash;
}

void McHelperWindow::openMchelperHelp( )
{
	if( !QDesktopServices::openUrl( QString( "http://www.makingthings.com/documentation/tutorial/mchelper" ) ) )
		statusBar()->showMessage( "Help is online and requires an internet connection.", 3000 );
}

void McHelperWindow::openOSCTuorial( )
{
	if( !QDesktopServices::openUrl( QString( "http://www.makingthings.com/documentation/tutorial/osc" ) ) )
		statusBar()->showMessage( "Help is online and requires an internet connection.", 3000 );
}

void McHelperWindow::outWindowHideOSCMessages( bool hide )
{
	hideOSCMessages = hide;
	QSettings settings("MakingThings", "mchelper");
	settings.setValue("hideOSCMessages", hideOSCMessages );
}

aboutMchelper::aboutMchelper( ) : QDialog( )
{
	setModal( true );
	setWindowTitle( "About mchelper" );
	topLevelLayout = new QVBoxLayout( this );

	okButton = new QPushButton( this );
	okButton->setText( tr("OK") );
	okButton->setFixedWidth( 75 );
	connect( okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
	buttonLayout = new QHBoxLayout( );
	buttonLayout->addStretch( );
	buttonLayout->addWidget( okButton );
	buttonLayout->addStretch( );
	
	mchelperIcon = new QPixmap( ":mticon128.png" );
	icon.setPixmap( *mchelperIcon );
	icon.setAlignment( Qt::AlignHCenter );
	
	title.setText( "<font size=5>Make Controller Helper</font>" );
	title.setAlignment( Qt::AlignHCenter );
	version.setText( QString( "<font size=4>Version %1</font>" ).arg( MCHELPER_VERSION ) );
	version.setAlignment( Qt::AlignHCenter );
	description = new QLabel( "<br><b>mchelper</b> (Make Controller Helper) is part of the Make Controller Kit project - an \
	open source hardware platform for everybody.  mchelper can upload new firmware to your Make \
	Controller, and allow you to easily manage it. \
	<br><br> \
	mchelper is released under the <a href=\"http://www.apache.org/licenses/LICENSE-2.0.html\">Apache 2.0 license</a>, \
	and uses code from Erik Gilling's <a href=\"http://oss.tekno.us/sam7utils\"> sam7utils project</a>. \
	<br><br> \
	Copyright (C) 2006-2007 MakingThings LLC.  <a href=\"http://www.makingthings.com\">www.makingthings.com</a> \
	<br><br> \
	This program is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, \
	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.", this );
	description->setWordWrap( true );
	description->setFixedWidth( 400 );
	description->setOpenExternalLinks( true );
	
	topLevelLayout->addWidget( &icon );
	topLevelLayout->addWidget( &title );
	topLevelLayout->addWidget( &version );
	topLevelLayout->addWidget( description );
	topLevelLayout->addLayout( buttonLayout );
	topLevelLayout->setAlignment( Qt::AlignHCenter );
}

void McHelperWindow::restoreDefaultPrefs( )
{
	prefsDialog->setUdpListenPortDisplay( 10000 );
	prefsDialog->setUdpSendPortDisplay( 10000 );
	prefsDialog->setXmlPortDisplay( 11000 );
	prefsDialog->setFindNetBoardsDisplay( true );
	prefsDialog->setMaxMsgsDisplay( 1000 );
}

void McHelperWindow::setNewPrefs( )
{
	QSettings settings("MakingThings", "mchelper");
	if( appUdpListenPort != prefsDialog->udpListenPortPrefs->text().toInt( ) )
	{
		appUdpListenPort = prefsDialog->udpListenPortPrefs->text().toInt( );
		settings.setValue("appUdpListenPort", appUdpListenPort );
		udp->changeListenPort( appUdpListenPort );
	}
	
	if( appUdpSendPort != prefsDialog->udpSendPortPrefs->text().toInt( ) )
	{
		appUdpSendPort = prefsDialog->udpSendPortPrefs->text().toInt( );
		settings.setValue("appUdpSendPort", appUdpSendPort );
		udp->changeSendPort( appUdpSendPort );
	}
	
	if( appXmlListenPort != prefsDialog->xmlPortPrefs->text().toInt( ) )
	{
		appXmlListenPort = prefsDialog->xmlPortPrefs->text().toInt( );
		settings.setValue("appXmlListenPort", appXmlListenPort );
		xmlServer->changeListenPort( appXmlListenPort );	
	}
	
	if( findEthernetBoardsAuto != prefsDialog->getFindNetBoardsDisplay( ) )
	{
		findEthernetBoardsAuto = prefsDialog->getFindNetBoardsDisplay( );
		settings.setValue("findEthernetBoardsAuto", findEthernetBoardsAuto );
	}
	
	if( settings.value( "checkForUpdatesOnStartup" ).toBool( ) != prefsDialog->getUpdateCheckBoxDisplay( ) )
		settings.setValue( "checkForUpdatesOnStartup", prefsDialog->getUpdateCheckBoxDisplay( ) );
	
	if( maxOutputWindowMessages != prefsDialog->maxOutputMsgsLineEdit->text().toInt( ) )
	{
		maxOutputWindowMessages = prefsDialog->maxOutputMsgsLineEdit->text().toInt( );
		outputModel->setMaxMsgs( maxOutputWindowMessages );
		settings.setValue( "maxOutputWindowMessages", maxOutputWindowMessages );
	}
}

bool McHelperWindow::findNetBoardsEnabled( )
{
	return findEthernetBoardsAuto;
}

void McHelperWindow::initUpdate( )
{
	QSettings settings("MakingThings", "mchelper");
	if( settings.value( "checkForUpdatesOnStartup", true ).toBool( ) )
	{
		#ifdef Q_WS_MAC
		Sparkle::initialize(); // check for updates in background with Sparkle
		#else
		appUpdater->checkForUpdates( APPUPDATE_BACKGROUND );
		#endif // Q_WS_MAC
	}
}

void McHelperWindow::onActionCheckForUpdates( )
{
	#ifdef Q_WS_MAC
	Sparkle::checkForUpdates();
	#else
	appUpdater->checkForUpdates( APPUPDATE_FOREGROUND );
	#endif // Q_WS_MAC
}

void McHelperWindow::onAnySummaryValueEdited( bool state )
{
	(void)state;
	onAnySummaryValueEdited( QString( "this is not used" ) );
}

void McHelperWindow::onAnySummaryValueEdited( QString text )
{
	(void)text;
	if( listWidget->currentItem() == NULL ) return;
	setSummaryTabLabelsForegroundRole( QPalette::Mid );
}

void McHelperWindow::setSummaryTabLabelsForegroundRole( QPalette::ColorRole role )
{
	systemNameLabel->setForegroundRole( role );
	seriaNumberLabel->setForegroundRole( role );
	firmwareVersionLabel->setForegroundRole( role );
	freeMemoryLabel->setForegroundRole( role );
	ipAddressLabel->setForegroundRole( role );
	netMaskLabel->setForegroundRole( role );
	gatewayLabel->setForegroundRole( role );
	udpListenLabel->setForegroundRole( role );
	udpSendLabel->setForegroundRole( role );
	dhcpCheckBox->setForegroundRole( role ); // how to actually get at the text?
	webserverCheckBox->setForegroundRole( role ); // same...
}

void McHelperWindow::onRevertChanges( )
{
	setSummaryTabLabelsForegroundRole( QPalette::WindowText );
	updateSummaryInfo( );
}

// zip through the list of fields in the Summary tab and if any have changed, send
// the appropriate message to the board and notify the user of what has been changed in the output window
void McHelperWindow::onApplyChanges( )
{
	Board* board = (Board*)listWidget->currentItem();
	if( board == NULL )
		return;
	
	QStringList msgs;
	QStringList displayMsgs;
	
	QString newName = systemName->text();
	if( !newName.isEmpty() && board->name != newName )
	{
		msgs << QString( "/system/name %1" ).arg( QString( "\"%1\"" ).arg( newName ) );
		displayMsgs << QString( "Changed name to %1" ).arg( newName );
	}
		
	// serial number
	QString newNumber = systemSerialNumber->text();
	if( !newNumber.isEmpty() && board->serialNumber != newNumber )
	{
		msgs << QString( "/system/serialnumber %1" ).arg( newNumber );
		displayMsgs << QString( "Changed serial number to %1" ).arg( newNumber );
	}
		
	// IP address
	QString newAddress = netAddressLineEdit->text();
	if( !newAddress.isEmpty() && board->ip_address != newAddress )
	{
		msgs << QString( "/network/address %1" ).arg( newAddress );
		displayMsgs << QString( "Changed IP address to %1" ).arg( newAddress );
	}
		
	// dhcp
	bool newState = dhcpCheckBox->checkState( );
	if( newState == true && !board->dhcp )
	{
		msgs << QString( "/network/dhcp 1" );
		displayMsgs << QString( "Turned DHCP on" );
	}
	if( newState == false && board->dhcp )
	{
		msgs << QString( "/network/dhcp 0" );
		displayMsgs << QString( "Turned DHCP off" );
	}
		
	// webserver
	newState = webserverCheckBox->checkState( );
	if( newState == true && !board->webserver )
	{
		msgs << QString( "/network/webserver 1" );
		displayMsgs << QString( "Turned the webserver on" );
	}
	if( newState == false && board->webserver )
	{
		msgs << QString( "/network/webserver 0" );
		displayMsgs << QString( "Turned the webserver off" );
	}
		
	// udp listen port
	QString newPort = udpListenPort->text();
	if( !newPort.isEmpty() && board->udp_listen_port != newPort )
	{
		msgs << QString( "/network/osc_udp_listen_port %1" ).arg( newPort );
		displayMsgs << QString( "Changed the board to listen on port %1 for messages" ).arg( newPort );
	}
		
	// udp send port
	newPort = udpSendPort->text();
	if( !newPort.isEmpty() && board->udp_send_port != newPort )
	{
		msgs << QString( "/network/osc_udp_send_port %1" ).arg( newPort );
		displayMsgs << QString( "Changed the board to send messages on port %1" ).arg( newPort );
	}
		
	setSummaryTabLabelsForegroundRole( QPalette::WindowText );
	if( msgs.size( ) > 0 )
	{
		board->sendMessage( msgs );
		messageThreadSafe( displayMsgs, MessageEvent::Info, QString( "mchelper" ) );
		statusBar()->showMessage( tr("Changes have been applied."), 2000);
	}
}

#ifdef Q_WS_WIN
void McHelperWindow::usbRemoved( HANDLE deviceHandle )
{
	usb->removalNotification( deviceHandle );
}
#endif


BoardEvent::BoardEvent( QString string ) : QEvent( (Type)10005 )
{
	message = string;
}






