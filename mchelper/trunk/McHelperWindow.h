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

#ifndef MCHELPERWINDOW_H
#define MCHELPERWINDOW_H

#include "ui_mchelper.h"

//Qt includes
#include <QMainWindow>
#include <QThread>

#include "UploaderThread.h"
#include "PacketUdp.h"
#include "Osc.h"
#include "Samba.h"
#include "PacketUsbCdc.h"

class UploaderThread;
class PacketUdp;
class Osc;


class McHelperWindow : public QMainWindow, private Ui::McHelperWindow, public MessageInterface
{
	Q_OBJECT
	
	char fileNameBuffer[ 512 ];
	QString lastDirectory;
	
	public:
		enum Status { OK, ERROR_COULDNT_CONNECT, ERROR_COULDNT_DOWNLOAD, ERROR_COULDNT_SWITCH,
	  	            ERROR_WEIRD_CHIP, ERROR_NO_BIN_FILE, ERROR_SAMBA_ERROR,
	  	            ERROR_CANT_OPEN_SOCKET, ERROR_ALREADY_PROGRAMMED,
	  	            ERROR_INCORRECT_RESPONSE, ERROR_NO_RESPONSE};
	
    McHelperWindow( QApplication* application );
	  void message( int level, char *format, ... );
		void sleepMs( int ms );
		void flash( );
		Status requestErase( );
		Status checkForTestProgram();
		void customEvent( QEvent* event );
		void customMessage( char* text );
		void progress( int value );
		
		void setNoUI( bool val );
		void uiLessUpload( char* filename, bool bootFlash );
		
	private:
	  QApplication* application;
	  UploaderThread* uploaderThread;
		PacketUdp* udp;
		Osc* oscUdp;
		Osc* oscUsb;
		Samba* samba;
		PacketUsbCdc* usb;
		
		void readSettings();
		void writeFileSettings();
		void writeUdpSettings();
		void writeUsbSettings();
		
		bool noUI;
  
	public slots:
	  void about( );

	private slots:
		void fileSelectButtonClicked();
	  void uploadButtonClicked();
		void commandLineEvent( );
		void commandLineUsbEvent( );
		void newLocalPort( );
		void newRemotePort( );
		void newHostAddress( );
};

#endif
