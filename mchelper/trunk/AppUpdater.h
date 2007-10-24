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

#ifndef APP_UPDATER_H
#define APP_UPDATER_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHttp>
#include <QTextEdit>

class AppUpdater : public QDialog
{
	Q_OBJECT
	public:
		AppUpdater( );
		~AppUpdater( ){ };
		void checkForUpdates( );
		bool checkingOnStartup;
	
	public slots:
		void on_actionCheckForUpdates( );
			
	private:
		QPushButton *acceptButton, *ignoreButton;
		QLabel icon;
		QLabel *headline, *details;
		QPixmap *mchelperIcon;
		QVBoxLayout *textLayout;
		QHBoxLayout *buttonLayout, *topLevelLayout;
		QTextEdit *browser;
		QHttp http;
		int GETid;
		int versionCompare(const QString & left, const QString & right);
	
	private slots:
		void finishedRead( int id, bool errors );
		void visitDownloadsPage( );
};

#endif // APP_UPDATER_H








