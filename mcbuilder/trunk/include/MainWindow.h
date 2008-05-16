#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFile>
#include <QMainWindow>
#include "ui_mainwindow.h"
#include "Highlighter.h"
#include "Preferences.h"
#include "Properties.h"
#include "Uploader.h"
#include "Builder.h"
#include "UsbMonitor.h"
#include "FindReplace.h"

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
		void printOutput(QString text);
		void printOutputError(QString text);
		QString currentProjectPath( ) { return currentProject; }
    QString currentBoardProfile( );
    bool findText(QString text, bool ignoreCase, bool forward, bool wholeword);
    void replaceAll(QString find, QString replace);
    void onBuildComplete(bool success);
    void onCleanComplete();
    void buildingNow(QString file);
		
	private:
		void openFile( const QString &path );
		void setupEditor( );
		void loadBoardProfiles( );
		void loadExamples( );
		void loadLibraries( );
		void loadRecentProjects( );
		void writeSettings();
		void readSettings();
		void closeEvent( QCloseEvent *qcloseevent );
		Highlighter *highlighter;
		Preferences *prefs;
		Properties *props;
		Uploader *uploader;
		Builder *builder;
    UsbMonitor *usbMonitor;
    FindReplace *findReplace;
		QActionGroup *boardTypeGroup;
		QString currentFile; // path of the file in the editor
		QString currentProject; // path of the current project directory
		void editorLoadFile( QFile *file );
		void createNewFile(QString path);
		void openProject(QString projectPath);
		void updateRecentProjects(QString newProject);
		void uploadFile(QString filename);
    bool maybeSave( );
    bool save( );
		QHash<QString, QString> boardTypes; // board name and config filename
		
	private slots:
		void onCursorMoved( );
    void onDocumentModified( );
		void onNewFile( );
		void onNewProject( );
		void onOpen( );
		void onSave( );
		void onSaveAs( );
		void onSaveProjectAs( );
		void openRecentProject(QAction* project);
		void onBuild( );
    void onClean( );
		void onProperties( );
		void onUpload( );
		void onUploadFile( );
		void onExample(QAction *example);
    void onLibrary(QAction *example);
		void onFileSelection(QString filename);
		void openMCReference( );
};

#endif // MAINWINDOW_H


