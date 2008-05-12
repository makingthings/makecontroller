
#include <QTextStream>
#include <QSettings>
#include "Uploader.h"
#include "Preferences.h"

/**
	Uploader handles uploading a binary image to a board.  It reads the board profile for the 
	currently selected board to determine which uploader to use.  Then it fires up a QProcess
	and runs the uploader with flags determined by settings in Preferences.  It prints output
	from the upload process back to the console output in the MainWindow.
*/
Uploader::Uploader(MainWindow *mainWindow) : QProcess( )
{
	this->mainWindow = mainWindow;
	uploaderProgress = new QProgressDialog("Uploading...", "Cancel", 0, 100);
	connect(uploaderProgress, SIGNAL(canceled()), this, SLOT(terminate()));
	connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(readOutput()));
	connect(this, SIGNAL(readyReadStandardError()), this, SLOT(readError()));
	connect(this, SIGNAL(started()), this, SLOT(uploadStarted()));
	connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(uploadFinished(int, QProcess::ExitStatus)));
}

void Uploader::upload(QString filename)
{
  QSettings settings("MakingThings", "mchelper");
  QString uploaderName = settings.value("sam7_path", DEFAULT_SAM7_PATH).toString();
  QStringList uploaderArgs;
  uploaderArgs << "flash " + filename << "boot_from_flash";
  start(uploaderName, uploaderArgs);
}

void Uploader::readOutput( )
{
	//mainWindow->printOutput(readAll());
  printf( "sam7: %s", qPrintable(QString(readAll())));
//	QTextStream in(uploader);
//	QString line = in.readLine();
//	while (!line.isNull())
//	{
//		mainWindow->printOutput(line.append("\n"));
//		line = in.readLine();
//	} 
}

void Uploader::readError( )
{
	//mainWindow->printOutputError(readAll());
//	QTextStream in(uploader);
//	QString line = in.readLine();
//	while (!line.isNull())
//	{
//		mainWindow->printOutputError(line.append("\n"));
//		line = in.readLine();
//	} 
}

void Uploader::uploadStarted( )
{
	uploaderProgress->show();
}

void Uploader::uploadFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	(void)exitCode;
	(void)exitStatus;
	uploaderProgress->hide();
	uploaderProgress->setValue(0);
}
