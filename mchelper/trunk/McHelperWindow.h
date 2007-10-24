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
#include "ui_mchelperPrefs.h"

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
#include "OutputWindow.h"
#include "OscXmlServer.h"
#include "AppUpdater.h"

class Board;
class BoardListModel;
class UsbMonitor;
class NetworkMonitor;
class SambaMonitor;
class McHelperApp;
class OscXmlServer;
class aboutMchelper;
class mchelperPrefs;

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
		void message( QStringList strings, MessageEvent::Types type, QString from );
		
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
		QList<Board*> getConnectedBoards( );
		bool summaryTabIsActive( );
		void updateSummaryInfo( );
		void updateDeviceList( );
		void newXmlPacketReceived( QList<OscMessage*> messageList, QString address );
		void sendXmlPacket( QList<OscMessage*> messageList, QString srcAddress );
		void xmlServerBoardInfoUpdate( Board* board );
		bool findNetBoardsEnabled( );
		
		void setNoUI( bool val );
		void uiLessUpload( char* filename, bool bootFlash );
		OutputWindow* outputModel;
		
#ifdef Q_WS_WIN // Windows-only
		void usbRemoved( HANDLE deviceHandle );
#endif
		
		int appUdpListenPort;
		int appXmlListenPort;
		bool findEthernetBoardsAuto;
		
	protected:
		void closeEvent( QCloseEvent *qcloseevent );
		
	private:
		QApplication* application;
		SambaMonitor* samba;
		UsbMonitor* usb;
		NetworkMonitor* udp;
		OscXmlServer *xmlServer;
		QTimer* monitorTimer;
		QTimer summaryTimer;
		QTimer outputWindowTimer;
		aboutMchelper* aboutDialog;
		mchelperPrefs* prefsDialog;
		AppUpdater* appUpdater;
		QHash<QString, Board*> connectedBoards;
		TableEntry* createOutputWindowEntry( QString string, MessageEvent::Types type, QString from );
		QList<TableEntry*> outputWindowQueue;
		QMutex outputWindowQueueMutex;
		
		void readSettings();
		void writeFileSettings();
		void writeUdpSettings();
		void writeUsbSettings();
		void initUpdate( );
		
		void message( QString string, MessageEvent::Types type, QString from );
		
    void setupOutputWindow();
    
		bool noUI;
		int lastTabIndex;
		bool hideOSCMessages;
		
		public slots:
			void restoreDefaultPrefs( );
			void udpPortPrefsChanged( );
			void xmlPortPrefsChanged( );
			void findNetBoardsPrefsChanged( bool state );
			void updaterPrefsChanged( bool state );
    
		private slots:
			// Uploader functions
			void fileSelectButtonClicked();
			void uploadButtonClicked();
			void checkForNewDevices( );
			
			void commandLineEvent( );
			void postMessages( );
			
			// Summary tab editing
			void systemNameChanged( );
			void systemSerialNumberChanged( );
			void ipAddressChanged( );
			void dhcpChanged( bool newState );
			void webserverChanged( bool newState );
			void udpListenChanged( );
			void udpSendChanged( );
			
			// Devices list view functions
			void deviceSelectionChanged ( const QModelIndex & current, const QModelIndex & previous );
			void tabIndexChanged(int index);
			void sendSummaryMessage( );
					
			void openMchelperHelp( );
			void openOSCTuorial( );
			void outWindowHideOSCMessages( bool hide );
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

class aboutMchelper : public QDialog
{
		Q_OBJECT
	public:
		aboutMchelper( );
			
	private:
		QPushButton *okButton;
		QLabel title, version, icon;
		QLabel *description;
		QPixmap *mchelperIcon;
		QVBoxLayout *topLevelLayout;
		QHBoxLayout *buttonLayout;
	
};

class mchelperPrefs : public QDialog, public Ui::mchelperPrefs
{
		Q_OBJECT
	public:
		mchelperPrefs( McHelperWindow *mainWindow );
		void setUdpPortDisplay( int port );
		void setXmlPortDisplay( int port );
		void setFindNetBoardsDisplay( bool state );
	private slots:
			
	private:
		McHelperWindow *mainWindow;
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
