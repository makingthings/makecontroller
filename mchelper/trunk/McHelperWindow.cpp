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

McHelperWindow::McHelperWindow( QApplication* application ) : QMainWindow( 0 )
{
	this->application = application;
	setupUi(this);
	
	#ifdef Q_WS_MAC
	setWindowIcon( QIcon("IconPackageOSX.icns") );
	#endif
	
	noUI = false;
	
	udp = new PacketUdp( );
	oscUdp = new Osc( );
	oscUsb = new Osc( );
	samba = new Samba( );
	usb = new PacketUsbCdc( );
	
	oscUdp->setInterfaces( udp, this, application );
	oscUdp->setPreamble( "OscUdp" );
	oscUsb->setInterfaces( usb, this, application );
	oscUsb->setPreamble( "OscUsb" );
	udp->setInterfaces( oscUdp, this );
	usb->setInterfaces( oscUsb, oscUsb );
	
	usb->start( );
	
	progressBar->setRange( 0, 1000 );
	progressBar->setValue( 0 );
	
	bitFlipSwitch->setCheckState( Qt::Checked );
	
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
	
	uploaderThread = 0;
	uploaderThread = new UploaderThread( application, this, samba );
	samba->setMessageInterface( uploaderThread );
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
		
	uploaderThread->setBinFileName( fileNameBuffer );
  
  // If the check box is checked, also flip the bit so that the board boots from flash on reboot
	if( bitFlipSwitch->checkState() == Qt::Checked )
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
	writeUsbSettings();
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
/*
int McHelperWindow::checkForTestProgram()
{
	// Check for a reaction
	osc->createMessage( "/ctestee/active" );
	osc->sendPacket();
	
	//messageInterface->sleepMs( 100 );
	
 	OscMessage oscMessage;
	Osc::Status s = osc->receive( &oscMessage );
	if ( s != Osc::OK )
	{
    messageInterface->message( 2, "  No response - Not programmed\n" );
	  return ERROR_NO_RESPONSE;
	}
	
  messageInterface->message( 2, "Testee Already Programmed\n" );
	return OK;
}
*/

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
