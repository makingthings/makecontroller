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
#include "ui_aboutMchelper.h"

//Qt includes
#include <QMainWindow>
#include <QThread>
#include <QTimer>
#include "MessageEvent.h"
#include "Board.h"
#include "NetworkMonitor.h"
#include "SambaMonitor.h"
#include "UsbMonitor.h"
#include "PacketUdp.h"

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
	
        enum OutputColumn { TO_FROM, MESSAGE, TIMESTAMP };
        
        McHelperWindow( McHelperApp* application );
		
        void message( int level, char *format, ... );
        void message( QString string );
        
        void messageThreadSafe( QString string );
        void messageThreadSafe( QString string, MessageEvent::Types type );
        void messageThreadSafe( QString string, MessageEvent::Types type, QString from );
        void messageThreadSafe( QStringList strings, MessageEvent::Types type, QString from );
        
		void customEvent( QEvent* event );
		void progress( int value );
		void removeDevice( QString key );
		void removeDeviceThreadSafe( QString key );
		
		void usbBoardsArrived( QList<PacketInterface*>* arrived );
		void udpBoardsArrived( QList<PacketUdp*>* arrived );
		void sambaBoardsArrived( QList<UploaderThread*>* arrived );
		Board* getCurrentBoard( );
		bool summaryTabIsActive( );
		void updateSummaryInfo( );
		void updateDeviceList( );
        
		void setNoUI( bool val );
		void uiLessUpload( char* filename, bool bootFlash );
		
		#ifdef Q_WS_WIN // Windows-only
		void usbRemoved( HANDLE deviceHandle );
		#endif
	
	protected:
		void closeEvent( QCloseEvent *qcloseevent );
		
	private:
        QApplication* application;
        SambaMonitor* samba;
		UsbMonitor* usb;
		NetworkMonitor* udp;
		QTimer* monitorTimer;
		QTimer summaryTimer;
    	QDialog* aboutMchelper;
    	QHash<QString, Board*> connectedBoards;
		
		void readSettings();
		void writeFileSettings();
		void writeUdpSettings();
		void writeUsbSettings();
        
        void message( QString string, MessageEvent::Types type, QString from );
		
    void setupOutputWindow();
    
		bool noUI;
		int lastTabIndex;
  
	public slots:
    
	private slots:
        // Uploader functions
        void fileSelectButtonClicked();
        void uploadButtonClicked();
        void checkForNewDevices( );

		void commandLineEvent( );
    
		// Menu functions
        void about( );
        
        // Summary tab editing
        void systemNameChanged( );
        void systemSerialNumberChanged( );
        void ipAddressChanged( );
        void dhcpChanged( int newState );
        void webserverChanged( int newState );
        void udpListenChanged( );
        void udpSendChanged( );
        
        // Devices list view functions
        void deviceSelectionChanged ( const QModelIndex & current, const QModelIndex & previous );
        void tabIndexChanged(int index);
        void sendSummaryMessage( );
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


class BoardEvent : public QEvent
{
	public:
	  BoardEvent( QString string );
	  ~BoardEvent( ) { }
	  
	QString message;
};

class UpdateEvent : public QEvent
{
	public:
	  UpdateEvent( ) : QEvent( (Type)11111 ) { }
	  ~UpdateEvent( ) { }
};

#endif // MCHELPERWINDOW_H
