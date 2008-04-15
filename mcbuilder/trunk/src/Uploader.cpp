#include <QDir>
#include <QDomDocument>
#include <QTextStream>
#include "Uploader.h"

/**
	Uploader handles uploading a binary image to a board.  It reads the board profile for the 
	currently selected board to determine which uploader to use.  Then it fires up a QProcess
	and runs the uploader with flags determined by settings in Preferences.  It prints output
	from the upload process back to the console output in the MainWindow.
*/
Uploader::Uploader(MainWindow *mainWindow) : QObject( )
{
	this->mainWindow = mainWindow;
	uploader = new QProcess(this);
	uploaderProgress = new QProgressDialog("Uploading...", "Cancel", 0, 100);
	connect(uploaderProgress, SIGNAL(canceled()), uploader, SLOT(terminate()));
	connect(uploader, SIGNAL(readyReadStandardOutput()), this, SLOT(readOutput()));
	connect(uploader, SIGNAL(readyReadStandardError()), this, SLOT(readError()));
	connect(uploader, SIGNAL(started()), this, SLOT(uploadStarted()));
	connect(uploader, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(uploadFinished(int, QProcess::ExitStatus)));
}

bool Uploader::upload(QString boardProfileName, QString filename)
{
	bool retval = false;
	// read the board profile and find which uploader we should use
	QDir dir = QDir::current();
	dir.cd("boards");
	QDomDocument doc;
	QFile file(dir.filePath(boardProfileName));
	if(!file.exists())
		return false;
	if(file.open(QIODevice::ReadOnly))
	{
		if(doc.setContent(&file))
		{
			QString uploaderName = doc.elementsByTagName("uploader").at(0).toElement().text();
			QStringList uploaderArgs;
			uploaderArgs << filename;
			dir.cd("../uploaders"); // we're still in "boards" from above
			uploader->setWorkingDirectory(dir.path());
			uploaderName.prepend(dir.path() + QDir::separator());
			uploader->start(uploaderName, uploaderArgs);
			retval = true;
		}
		file.close();
	}
	else
		retval = false;
	return retval;
}

void Uploader::readOutput( )
{
	mainWindow->printOutput(uploader->readAll());
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
	mainWindow->printOutputError(uploader->readAll());
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
