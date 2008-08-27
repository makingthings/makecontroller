
#include <QFileInfo>
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
	uploaderProgress = new QProgressDialog();
	connect(uploaderProgress, SIGNAL(canceled()), this, SLOT(kill()));
	connect(uploaderProgress, SIGNAL(finished(int)), this, SLOT(onProgressDialogFinished(int)));
  connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(filterOutput()));
  connect(this, SIGNAL(readyReadStandardError()), this, SLOT(filterError()));
	connect(this, SIGNAL(started()), this, SLOT(uploadStarted()));
	connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(uploadFinished(int, QProcess::ExitStatus)));
  connect(this, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onError(QProcess::ProcessError)));
}

void Uploader::upload(QString filename)
{
  QSettings settings("MakingThings", "mchelper");
  QString uploaderName = settings.value("sam7_path", DEFAULT_SAM7_PATH).toString();
  currentFile = filename;
  QStringList uploaderArgs;
  uploaderArgs << "-e" << "set_clock";
  uploaderArgs << "-e" << "unlock_regions";
  uploaderArgs << "-e" << QString("flash -show_progress %1").arg(currentFile);
  uploaderArgs << "-e" << "boot_from_flash";
  uploaderArgs << "-e" << "reset";
  start(uploaderName, uploaderArgs);
}

void Uploader::filterOutput( )
{
  QString output(readAllStandardOutput());
  QRegExp re("upload progress: (\\d+)%");
  int pos = 0;
  bool matched = false;
  while((pos = re.indexIn(output, pos)) != -1)
  {
    int progress = re.cap(1).toInt();
    if(progress != uploaderProgress->value())
      uploaderProgress->setValue(progress);
	  pos += re.matchedLength();
    matched = true;
  }
  if(!matched)
    qDebug("upload output: %s", qPrintable(output));
}

void Uploader::filterError( )
{
  QString err(readAllStandardError());
  if(err.startsWith("can't find boot agent"))
    mainWindow->statusMsg(tr("Error - couldn't find an unprogrammed board to upload to."));
  else
    qDebug("upload err: %s", qPrintable(err));
}

void Uploader::uploadStarted( )
{
	QFileInfo fi(currentFile);
  uploaderProgress->setLabelText(QString("Uploading %1...").arg(fi.fileName()));
  uploaderProgress->show();
}

void Uploader::uploadFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	(void)exitCode;
	(void)exitStatus;
	uploaderProgress->reset();
}

/*
  The uploader has reported an error.
  Check what kind it was and print out a message to the user.
*/
void Uploader::onError(QProcess::ProcessError error)
{
  QString msg;
  switch(error)
  {
    case QProcess::FailedToStart:
      msg = tr("uploader failed to start.  sam7 is either missing, or doesn't have the correct permissions");
      break;
    case QProcess::Crashed:
      msg = tr("uploader (sam7) was canceled or crashed.");
      break;
    case QProcess::Timedout:
      msg = tr("uploader (sam7) timed out.");
      break;
    case QProcess::WriteError:
      msg = tr("uploader (sam7) reported a write error.");
      break;
    case QProcess::ReadError:
      msg = tr("uploader (sam7) reported a read error.");
      break;
    case QProcess::UnknownError:
      msg = tr("uploader (sam7) - unknown error type.");
      break;
  }
  mainWindow->statusMsg(tr("Error - ") + msg);
}

/*
  The progress dialog was clicked out of, instead of the the cancel button being hit.
  Cancel it just the same.
*/
void Uploader::onProgressDialogFinished(int result)
{
  if(result == QDialog::Rejected && state() == QProcess::Running)
    kill();
}




