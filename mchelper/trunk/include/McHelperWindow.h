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

#ifndef MCHELPERWINDOW_H
#define MCHELPERWINDOW_H

#include "ui_mchelper.h"

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
#include "McHelperPrefs.h"

class Board;
class UsbMonitor;
class NetworkMonitor;
class SambaMonitor;
class McHelperApp;
class OscXmlServer;
class aboutMchelper;
class mchelperPrefs;
class PacketUdp;


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
		
		void messageThreadSafe( QString string );
		void messageThreadSafe( QString string, MessageEvent::Types type );
		void messageThreadSafe( QString string, MessageEvent::Types type, QString from );
		void messageThreadSafe( QStringList strings, MessageEvent::Types type, QString from );
		void statusMessage( const QString & msg, int duration );
	
		void customEvent( QEvent* event );
		void progress( int value );
		void removeDevice( QString key );
		void removeDeviceThreadSafe( QString key );
		
		void usbBoardsArrived( QList<PacketInterface*> arrived );
		void udpBoardsArrived( QList<PacketUdp*> arrived );
		void sambaBoardsArrived( QList<UploaderThread*> arrived );
		Board* getCurrentBoard( );
		QList<Board*> getConnectedBoards( );
		bool summaryTabIsActive( );
		void updateSummaryInfo( );
		void setBoardName( QString key, QString name );
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
		
		// preferences window stuff
		int appUdpListenPort;
		int appUdpSendPort;
		int appXmlListenPort;
		bool findEthernetBoardsAuto;
		int maxOutputWindowMessages;
		
	protected:
		void closeEvent( QCloseEvent *qcloseevent );
	
	signals:
		void boardInfoUpdate( Board* board );
		void boardListUpdate( QList<Board*> boardList, bool added );
		void xmlPacket( QList<OscMessage*> messageList, QString srcAddress, int destPort );
		
	private:		
		QApplication* application;
		SambaMonitor* samba;
		UsbMonitor* usb;
		NetworkMonitor* udp;
		OscXmlServer *xmlServer;
		QTimer summaryTimer;
		QTimer outputWindowTimer;
		aboutMchelper* aboutDialog;
		mchelperPrefs* prefsDialog;
		AppUpdater* appUpdater;
		QHash<QString, Board*> connectedBoards;
		QList<TableEntry> outputWindowQueue;
		QMutex outputWindowQueueMutex;
		QListWidgetItem listWidgetPlaceholder;

		void setSummaryTabLabelsForegroundRole( QPalette::ColorRole role );
		
		void readSettings();
		void writeFileSettings();
		void writeUdpSettings();
		void writeUsbSettings();
		void initUpdate( );
    void setupOutputWindow();
    
		bool noUI;
		int lastTabIndex;
		bool hideOSCMessages;
		
		public slots:
			void restoreDefaultPrefs( );
			void setNewPrefs( );
    
		private slots:
			// Uploader functions
			void fileSelectButtonClicked();
			void uploadButtonClicked();
			
			void commandLineEvent( );
			void postMessages( );
			
			// Summary tab editing
			void onAnySummaryValueEdited( QString text );
			void onAnySummaryValueEdited( bool state );
			void onApplyChanges( );
			void onRevertChanges( );
			
			// Devices list view functions
			void deviceSelectionChanged ( const QModelIndex & current, const QModelIndex & previous );
			void tabIndexChanged(int index);
			void sendSummaryMessage( );
			void updateSummaryInfoInternal( );
					
			void openMchelperHelp( );
			void openOSCTuorial( );
			void outWindowHideOSCMessages( bool hide );
			void onActionCheckForUpdates( );
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

class BoardEvent : public QEvent
{
	public:
		BoardEvent( QString string );
		QString message;
};

class BoardNameEvent : public QEvent
{
	public:
		BoardNameEvent( QString key, QString name ) : QEvent( (Type)11112 )
		{
			this->key = key;
			this->name = name;
		}
		QString key, name;
};

class UpdateEvent : public QEvent
{
	public:
		UpdateEvent( ) : QEvent( (Type)11111 ) { }
		~UpdateEvent( ) { }
};

#endif // MCHELPERWINDOW_H
