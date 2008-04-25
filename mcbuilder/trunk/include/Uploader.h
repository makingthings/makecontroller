#ifndef UPLOADER_H
#define UPLOADER_H

#include <QProcess>
#include <QProgressDialog>
#include "MainWindow.h"

class MainWindow;

class Uploader : public QProcess
{
	Q_OBJECT
	public:
		Uploader(MainWindow *mainWindow);
		bool upload(QString boardProfileName, QString filename);
		
	private:
		MainWindow *mainWindow;
		QProgressDialog *uploaderProgress;
		QString currentFile;
		
	private slots:
		void readOutput();
		void readError();
		void uploadStarted();
		void uploadFinished(int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // UPLOADER_H