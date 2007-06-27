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
#include <QTableWidget>
#include <QTableWidgetItem> 
#include <QHeaderView> 
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
	
	//listWidget = new BoardListModel( application, this, this );
	udp = new NetworkMonitor( ); 
	samba = new SambaMonitor( application, this );
	usb = new UsbMonitor( );
	 
	udp->setInterfaces( this, this );
	usb->setInterfaces( this, application, this );
  
  setupOutputTable();

  // Wire up the selection changed signal from the
  // model to be handled here
  connect( listWidget->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
           this,                                SLOT(deviceSelectionChanged(const QModelIndex &, const QModelIndex &)));
  
  ////////////////////////////////////////////////////////////////////////////
	
	progressBar->setRange( 0, 1000 );
	progressBar->setValue( 0 );
	
	readSettings( );
	lastTabIndex = 0;
	
	// if ( udp->open( ) != PacketUdp::OK )
	connect( tabWidget, SIGNAL( currentChanged(int) ), this, SLOT( tabIndexChanged(int) ) );
	
	//USB signals/slots
	connect( commandLine->lineEdit(), SIGNAL(returnPressed()), this, SLOT(commandLineEvent()) );
	
	//setup the pushbuttons
	connect( fileSelectButton, SIGNAL( clicked() ), this, SLOT( fileSelectButtonClicked() ) );
	connect( uploadButton, SIGNAL( clicked() ), this, SLOT( uploadButtonClicked() ) );
    
	// setup the menu
	connect( actionAboutMchelper, SIGNAL( triggered() ), this, SLOT( about( ) ) );
	connect( actionClearOutput, SIGNAL( triggered() ), this, SLOT( clearOutputWindow( ) ) );
	//actionClearOutput->setShortcut(tr("Ctrl+X"));
	//actionClearOutput->setShortcutContext( Qt::ApplicationShortcut ); // this doesn't seem to have much effect
	
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
    Board *board;
    
	// first check for USB boards
	QList<PacketInterface*> newBoards;
	usb->scan( &newBoards );
	int newBoardCount = newBoards.count( );
	if( newBoardCount > 0 )
	{
		int i;
		for( i=0; i<newBoardCount; i++ )
		{
		  board = new Board( this, this, application );
          board->key = newBoards.at(i)->getKey();
	      board->type = Board::UsbSerial;
	      board->setPacketInterface( newBoards.at(i) );
	      board->com_port = newBoards.at(i)->location();
	      board->setText( QString( ":%1" ).arg( board->typeString() ) );
	      connectedBoards.insert( board->key, board );
	      listWidget->addItem( board ); 
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
		  board = new Board( this, this, application );
		  board->setPacketInterface( udpBoards.at(i) );
		  board->key = udpBoards.at(i)->getKey();
	      board->type = Board::Udp;
	      connectedBoards.insert( board->key, board );
	      listWidget->addItem( board );
		}
	} 
	/*
	QList<UploaderThread*> sambaBoards;
	samba->scan( &sambaBoards );
	newBoardCount = sambaBoards.count( );
	if( newBoardCount > 0 )
	{
		int i;
		for( i=0; i<newBoardCount; i++ )
		{
		  board = new Board( this, this, application );
		  board->key = sambaBoards.at(i)->getDeviceKey();
	      board->name = "Samba Board";
	      board->type = Board::UsbSamba;
	      board->setUploaderThread( sambaBoards.at(i) );
	      connectedBoards.insert( board->key, board );
	      listWidget->addItem( board );
		}
	}
	*/
	if( listWidget->currentRow( ) < 0 && listWidget->count( ) > 0 )
		listWidget->setCurrentRow( 0 ); // if nothing is selected, just select the first item
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
		connectedBoards.remove( key );
		delete removed;
	}
}

void McHelperWindow::deviceSelectionChanged ( const QModelIndex & current, const QModelIndex & previous )
{
  Board* board = (Board*)listWidget->currentItem();
  if( board == NULL )
  	return;
  	
  switch( board->type )
  {
  	case Board::UsbSerial: // same for both
  	case Board::Udp:
  		tabWidget->setCurrentIndex( lastTabIndex );
  		tabWidget->setTabEnabled( 0, 1 );
  		tabWidget->setTabEnabled( 1, 1 );
  		tabWidget->setTabEnabled( 2, 0 );
        
        // fill up the line edits in the Summary tab with the new
        // board's info by sending off a "/system/info" command
        //
        // :NOTE: added a bit of sleep in here to help with the
        // case when you first start the app with a board attached
        // that gets autoselected and the device hasn't fully
        // connected yet. Once the "system info" checking
        // thread is in place, we should remove the sending of
        // the "/system/info" message here since isn't really very reliable.
        Sleep( 250 );
        board->sendMessage( QString("/system/info") );
  		break;
        
  	case Board::UsbSamba:
  		tabWidget->setCurrentIndex( 2 );
  		tabWidget->setTabEnabled( 0, 0 );
  		tabWidget->setTabEnabled( 1, 0 );
  		tabWidget->setTabEnabled( 2, 1 );
  		break;
  }
}

void McHelperWindow::updateSummaryInfo( QString key )
{
    // Make sure we know about this board ...
    if( connectedBoards.contains( key ) )
    {
        // ... and it's the currently selected one from the list
        Board* board = (Board*)listWidget->currentItem();
        if (board->key == key )
        {
            systemName->setText( board->name );
            systemSerialNumber->setText( board->serialNumber );
            systemFirmwareVersion->setText( board->firmwareVersion );
            systemFreeMemory->setText( board->freeMemory );
            netAddressLineEdit->setText( board->ip_address );
            netMaskLineEdit->setText( board->netMask );
            netGatewayLineEdit->setText( board->gateway );
            netMACLineEdit->setText( board->mac );
            
            /* :TODO: what and how should these be set
            dhcpCheckBox->setCheckState( );
            webserverCheckBox->setCheckState( ); // Qt::Checked, Qt::Unchecked
            */
        }
    }
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
  
  QString cmd = QString( "%1" ).arg(commandLine->currentText() );
  messageThreadSafe( cmd, MessageEvent::Command, board->key  );
  
  board->sendMessage( commandLine->currentText());
  
  // in order to get a readline-style history of commands via up/down arrows
  // we ened to keep an empty item at the end of the list so we have a context from which to up-arrow
  
  /*commandLine->removeItem( 0 );
  commandLine->insertItem( 8, commandLine->currentText() );
  commandLine->insertItem( 9, "" );
  commandLine->setCurrentIndex( 9 );
  */
  commandLine->clearEditText();
  writeUsbSettings();
}

void McHelperWindow::clearOutputWindow()
{
  int r, c;
  for( r = 0; r < outputTable->rowCount(); r++ )
  {
    for( c = 0; c < outputTable->columnCount(); c++ )
    {
      delete outputTable->item(r,c);
    }
  }
  
  //outputTable->clearContents();
  outputTable->setRowCount(0);
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
			message( messageEvent->message, messageEvent->type, messageEvent->from );
			//delete messageEvent;
			break;
		}
		case 10005: // a board was uploaded/removed
		{
			MessageEvent* messageEvent = (MessageEvent*)event;
			removeDevice( messageEvent->message );
			//delete messageEvent;
			break;
		}
		case 10010: // put a status message in the window
		{
			StatusEvent* statusEvent = (StatusEvent*)event;
			statusBar()->showMessage( statusEvent->message, statusEvent->duration );
            break;
		}
        case 10015: // put a status message in the window
        {
            BoardSummaryInfoUpdateEvent* boardSummaryInfoUpdateEvent = (BoardSummaryInfoUpdateEvent*)event;
            updateSummaryInfo( boardSummaryInfoUpdateEvent->key );
            break;
        }
		default:
			break;
	}
}

void McHelperWindow::messageThreadSafe( QString string  )
{ 
  // Default to an "Info" message if we don't know otherwise
  messageThreadSafe( string, MessageEvent::Info );
}

void McHelperWindow::messageThreadSafe( QString string, MessageEvent::Types type  )
{	
  // Default to coming from the "App" itself if we don't know otherwise
  messageThreadSafe( string, type, QString("App") );
}

void McHelperWindow::messageThreadSafe( QString string, MessageEvent::Types type, QString from )
{ 
  MessageEvent* messageEvent = new MessageEvent( string, type, from );
  application->postEvent( this, messageEvent );
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

// Deprecated...
// Use one of the messageThreadSafe methods
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
            if ( buffer != NULL && strlen(buffer) > 1 )
                message( QString(buffer) );
		}
	}
}

// Deprecated...
// Use one of the messageThreadSafe methods
void McHelperWindow::message( QString string )
{
	if( noUI )
		return;
        
	if ( !string.isEmpty( ) && !string.isNull( ) )
        message( string, MessageEvent::Info, QString("App") );
}

void McHelperWindow::message( QString string, MessageEvent::Types type, QString from )
{
    if( noUI )
        return;

    if ( !string.isEmpty( ) && !string.isNull( ) ) {
    
        // Set the background row color
        QColor bgColor;
        switch( type )
        {
            case MessageEvent::Command:
                bgColor = QColor(225, 225, 225, 127); // light-light gray
                break;
            
            case MessageEvent::Response:
                bgColor = QColor(98, 191, 142, 127); // Green
                break;
                
            case MessageEvent::Error:
                bgColor = QColor(219, 62, 0, 127); // Red
                break;
              
            case MessageEvent::Warning:
                bgColor = QColor(255, 176, 59, 127); // Orange
                break;
                
            case MessageEvent::Info:
            case MessageEvent::Notice:  
            default:
                bgColor = Qt::white;;
        } 
           
        // Increase the row count
        outputTable->setRowCount(outputTable->rowCount() + 1);
        
        // Timestamp
        QTableWidgetItem *timeStampItem = new QTableWidgetItem( QTime::currentTime().toString() );
        timeStampItem->setTextAlignment(Qt::AlignRight);
        timeStampItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        timeStampItem->setBackgroundColor( bgColor );
        outputTable->setItem(outputTable->rowCount() - 1, 0, timeStampItem);
        
        // From
        QTableWidgetItem *fromItem = new QTableWidgetItem(from);
        fromItem->setTextAlignment(Qt::AlignHCenter);
        fromItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        fromItem->setBackgroundColor( bgColor );
        outputTable->setItem(outputTable->rowCount() - 1, 1, fromItem );
        
        // Message
        QTableWidgetItem *messageItem = new QTableWidgetItem(string);
        messageItem->setTextAlignment(Qt::AlignLeft);
        messageItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        messageItem->setBackgroundColor( bgColor );
        outputTable->setItem(outputTable->rowCount() - 1, 2, messageItem);
        
        // Set the row height to allow more data to show than
        // the default height would and scroll down to the last item
        outputTable->verticalHeader()->resizeSection(outputTable->rowCount() - 1, 15);
        outputTable->scrollToItem(timeStampItem, QAbstractItemView::EnsureVisible);
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
void McHelperWindow::setAboutDialog( QDialog* about )
{
	this->aboutMchelper = about; 
}

// Setup the output table
void McHelperWindow::setupOutputTable()
{
  //outputTable->setRowCount(0);
  //outputTable->setColumnCount(3);
  //QStringList horizontalLabels;
  //horizontalLabels << "TimeStamp" << "From" << "Message";
  //outputTable->setHorizontalHeaderLabels(horizontalLabels);
  
  // The message column should stretch to fill the table
  QHeaderView *headerHView = outputTable->horizontalHeader();
  headerHView->setResizeMode(QHeaderView::Interactive);
  headerHView->resizeSection(0, 60);
  headerHView->resizeSection(1, 50);
  headerHView->setResizeMode(2, QHeaderView::Stretch);

  //QHeaderView *headerVView = outputTable->verticalHeader();
  //headerVView->setResizeMode(QHeaderView::ResizeToContents);
  
  // We don't want to show the vertical header column
  outputTable->verticalHeader()->hide();
}

void McHelperWindow::about( )  // set the version number here.
{
  aboutMchelper->show();
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

BoardSummaryInfoUpdateEvent::BoardSummaryInfoUpdateEvent( QString key ) : QEvent( (Type)10015 )
{
    // The key of the board that sent the event
    this->key = key;
}



