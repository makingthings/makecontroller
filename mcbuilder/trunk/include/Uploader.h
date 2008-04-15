#ifndef UPLOADER_H
#define UPLOADER_H

#include <QProcess>
#include <QProgressDialog>
#include "MainWindow.h"

class MainWindow;

class Uploader : public QObject
{
	Q_OBJECT
	public:
		Uploader(MainWindow *mainWindow);
		bool upload(QString boardProfileName, QString filename);
		QProcess::ProcessState state( ) { return uploader->state(); }
		
	private:
		MainWindow *mainWindow;
		QProcess *uploader;
		QProgressDialog *uploaderProgress;
		QString currentFile;
		
	private slots:
		void readOutput();
		void readError();
		void uploadStarted();
		void uploadFinished(int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // UPLOADER_H