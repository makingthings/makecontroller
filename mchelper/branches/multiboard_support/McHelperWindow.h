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
#include <QTimer>
#include "Board.h"
#include "BoardListModel.h"
#include "NetworkMonitor.h"
#include "SambaMonitor.h"
#include "UsbMonitor.h"

class Board;
class BoardListModel;
class UsbMonitor;
class NetworkMonitor;
class SambaMonitor;
class McHelperApp;

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
	
    McHelperWindow( McHelperApp* application );
		
	  void message( int level, char *format, ... );
	  void messageThreadSafe( QString string );
	  void message( QString string );
		void sleepMs( int ms );
		void customEvent( QEvent* event );
		void customMessage( char* text );
		void progress( int value );
		
		void setNoUI( bool val );
		void uiLessUpload( char* filename, bool bootFlash );
		void usbRemoved( HANDLE deviceHandle );
	
	protected:
		void closeEvent( QCloseEvent *qcloseevent );
		
	private:
	  QApplication* application;
		SambaMonitor* samba;
		UsbMonitor* usb;
		NetworkMonitor* udp;
		QTimer* monitorTimer;
    	BoardListModel* boardModel;
		
		void readSettings();
		void writeFileSettings();
		void writeUdpSettings();
		void writeUsbSettings();
		void updateSummaryInfo( Board* board );
		
		bool noUI;
		int lastTabIndex;
  
	public slots:
    
	private slots:
    // Uploader functions
		void fileSelectButtonClicked();
	  void uploadButtonClicked();
	  void checkForNewDevices( );
    
    // Usb functions
    
    
    // Udp functions
		void commandLineEvent( );
		void newLocalPort( );
		void newRemotePort( );
		void newHostAddress( );
    
		// Menu functions
    void about( );
		void clearOutputWindow();
    
    // Devices list view functions
    void deviceSelectionChanged ( const QModelIndex & current, const QModelIndex & previous );
    void tabIndexChanged(int index);
};

class McHelperApp : public QApplication
{
  Q_OBJECT
  public:
    McHelperApp( int &argc, char **argv ) : QApplication( argc, argv ) {}
    ~McHelperApp() {}
    
    #ifdef Q_WS_WIN // Windows-only
    void setMainWindow( McHelperWindow* window );
    bool winEventFilter( MSG* msg, long* retVal );
    
  private:
    McHelperWindow* mchelper;
    #endif // Windows-only
};

class MessageEvent : public QEvent
{
	public:
	  MessageEvent( QString string );
	  ~MessageEvent( ) {}
	  
	QString message;
};


#endif // MCHELPERWINDOW_H
