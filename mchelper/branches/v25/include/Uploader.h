#ifndef UPLOADER_H
#define UPLOADER_H

#include <QProcess>
#include "MainWindow.h"
#include "ui_uploader.h"

class MainWindow;

class Uploader : public QDialog, private Ui::UploaderUi
{
	Q_OBJECT
	public:
		Uploader(MainWindow *mainWindow);
		void upload(QString filename);
    QProcess::ProcessState state() { return uploader.state(); }
		
	private:
		MainWindow *mainWindow;
    QProcess uploader;
		
	private slots:
		void filterOutput();
		void filterError();
		void uploadFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onError(QProcess::ProcessError error);
    void onBrowseButton();
    void onUploadButton();
};

#endif // UPLOADER_H


