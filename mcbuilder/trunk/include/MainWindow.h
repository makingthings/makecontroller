/*********************************************************************************

 Copyright 2008 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFile>
#include <QMainWindow>
#include "ui_mainwindow.h"
#include "Highlighter.h"
#include "Preferences.h"
#include "ProjectInfo.h"
#include "Uploader.h"
#include "Builder.h"
#include "UsbConsole.h"
#include "FindReplace.h"
#include "About.h"
#include "ConsoleItem.h"
#include "AppUpdater.h"

class Preferences;
class Uploader;
class Builder;
class Properties;
class FindReplace;

class MainWindow : public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT
	public:
		MainWindow( );
		void setTabWidth( int width );
    void setEditorFont(QString family, int pointSize);
		void printOutput(QString text);
		void printOutputError(QString text);
    void printOutputError(ConsoleItem *item);
		QString currentProjectPath( ) { return currentProject; }
    QString currentBoardProfile( );
    bool findText(QString text, QTextDocument::FindFlags flags, bool forward);
    void replaceAll(QString find, QString replace, QTextDocument::FindFlags flags);
    void replace(QString rep);
    void onBuildComplete(bool success);
    void onCleanComplete();
    void buildingNow(QString file);
    void highlightLine(QString filepath, int linenumber, ConsoleItem::Type type);
    void removeFileFromProject(QString file);
		
	private:
		void openFile( const QString &path );
		void loadBoardProfiles( );
		void loadExamples( );
		void loadLibraries( );
		void loadRecentProjects( );
		void writeSettings();
		void readSettings();
		void closeEvent( QCloseEvent *qcloseevent );
		Highlighter *highlighter;
		Preferences *prefs;
		ProjectInfo *projInfo;
		Uploader *uploader;
		Builder *builder;
    UsbConsole *usbConsole;
    FindReplace *findReplace;
    About *about;
    AppUpdater *updater;
    QComboBox *currentFileDropDown;
		QActionGroup *boardTypeGroup;
		QString currentFile; // path of the file in the editor
		QString currentProject; // path of the current project directory
		void editorLoadFile( QString filepath );
		void createNewFile(QString path);
    bool addToProjectFile(QString projectPath, QString newFilePath, QString buildtype);
		void openProject(QString projectPath);
		void updateRecentProjects(QString newProject);
		void uploadFile(QString filename);
    bool maybeSave( );
    bool save( );
		
	private slots:
		void onCursorMoved( );
    void onDocumentModified( );
		void onNewFile( );
    void onAddExistingFile( );
		void onNewProject( );
		void onOpen( );
		void onSave( );
		void onSaveAs( );
		void onSaveProjectAs( );
		void openRecentProject(QAction* project);
		void onBuild( );
    void onStop( );
    void onClean( );
		void onProperties( );
		void onUpload( );
		void onUploadFile( );
		void onExample(QAction *example);
    void onLibrary(QAction *example);
		void onFileSelection(int index);
		void openMCReference( );
    void openManual( );
    void onConsoleDoubleClick(QListWidgetItem *item);
    void onUpdate();
};

#endif // MAINWINDOW_H


