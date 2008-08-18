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
		void upload(QString filename);
		
	private:
		MainWindow *mainWindow;
		QProgressDialog *uploaderProgress;
		QString currentFile;
		
	private slots:
		void filterOutput();
		void filterError();
		void uploadStarted();
		void uploadFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onError(QProcess::ProcessError error);
    void onProgressDialogFinished(int result);
};

#endif // UPLOADER_H


