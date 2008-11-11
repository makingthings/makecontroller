
#include <QFileInfo>
#include <QSettings>
#include "Uploader.h"
#include "Preferences.h"
#include <QDir>
#include <QFileDialog>

#ifdef Q_WS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

/*
	Uploader handles uploading a binary image to a board.  It fires up a QProcess
	and runs the uploader with flags determined by settings in Preferences.
*/
Uploader::Uploader(MainWindow *mainWindow) : QDialog( 0 )
{
	this->mainWindow = mainWindow;
  setupUi(this);
  connect(this, SIGNAL(finished(int)), this, SLOT(onDialogClosed()));
  connect(&uploader, SIGNAL(readyReadStandardOutput()), this, SLOT(filterOutput()));
  connect(&uploader, SIGNAL(readyReadStandardError()), this, SLOT(filterError()));
	connect(&uploader, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(uploadFinished(int, QProcess::ExitStatus)));
  connect(&uploader, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onError(QProcess::ProcessError)));
  connect(browseButton, SIGNAL(clicked()), this, SLOT(onBrowseButton()));
  connect(uploadButton, SIGNAL(clicked()), this, SLOT(onUploadButton()));
  
  QSettings settings("MakingThings", "mchelper");
  QString lastFilePath = settings.value("last_firmware_upload", QDir::homePath()).toString();
  browseEdit->setText(lastFilePath);
  progressBar->reset();
  resize(gridLayout->sizeHint());
}

Uploader::~Uploader( )
{
  onDialogClosed( );
}

void Uploader::onBrowseButton()
{
  QSettings settings("MakingThings", "mchelper");
  QString lastFilePath = settings.value("last_firmware_upload", QDir::homePath()).toString();
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), lastFilePath, tr("Binaries (*.bin)"));
	if(!fileName.isNull()) // user canceled
  {
    settings.setValue("last_firmware_upload", fileName);
    browseEdit->setText(fileName);
  }
}

void Uploader::onUploadButton()
{
  QFileInfo fi(browseEdit->text());
  if(!fi.exists())
    return mainWindow->message( tr("%1 can't be found.").arg(fi.fileName()), MsgType::Error, tr("Uploader") );
  
  if( uploader.state() != QProcess::NotRunning )
    return mainWindow->message( tr("Uploader is currently busy...give it a second, then try again"), MsgType::Error, tr("Uploader") );
  
  upload(fi.filePath());
}

void Uploader::upload(QString filename)
{
  #ifdef Q_WS_MAC // get the path within the app bundle
  CFURLRef pluginRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
  CFStringRef macPath = CFURLCopyFileSystemPath(pluginRef, kCFURLPOSIXPathStyle);
  QDir appBundle( CFStringGetCStringPtr(macPath, CFStringGetSystemEncoding()) );
  QString uploaderName = appBundle.filePath( "Contents/Resources/sam7" );
  #elif defined (Q_WS_WIN)
  QString uploaderName = QDir::current().filePath("sam7");
  #else
  QSettings settings("MakingThings", "mchelper");
  QString uploaderName = settings.value("sam7_path", DEFAULT_SAM7_PATH).toString();
  #endif
  int offset = 0; // escape any spaces in the filename
  do
  {
    offset = filename.indexOf(" ", offset);
    if( offset != -1 )
    {
      filename.insert(offset, "\\");
      offset += 2; // step past the \ we inserted and the space we put it in front of
    }
  } while( offset != -1 );
  qDebug( "uploading %s", qPrintable(filename));
  
  QStringList uploaderArgs;
  uploaderArgs << "-e" << "set_clock";
  uploaderArgs << "-e" << "unlock_regions";
  uploaderArgs << "-e" << QString("flash -show_progress %1").arg(filename);
  uploaderArgs << "-e" << "boot_from_flash";
  uploaderArgs << "-e" << "reset";
  uploader.start(uploaderName, uploaderArgs);
  QFileInfo fi(filename);
  setWindowTitle(tr("Uploader - uploading %1").arg(fi.fileName()));
}

void Uploader::filterOutput( )
{
  QString output(uploader.readAllStandardOutput());
  QRegExp re("upload progress: (\\d+)%");
  int pos = 0;
  bool matched = false;
  while((pos = re.indexIn(output, pos)) != -1)
  {
    int progress = re.cap(1).toInt();
    if(progress != progressBar->value())
      progressBar->setValue(progress);
	  pos += re.matchedLength();
    matched = true;
  }
  if(!matched)
    qDebug("upload output: %s", qPrintable(output));
}

void Uploader::filterError( )
{
  QString err(uploader.readAllStandardError());
  if(err.startsWith("can't find boot agent"))
    mainWindow->message(tr("Couldn't find an unprogrammed board to upload to."), MsgType::Error, tr("Uploader") );
  else
    qDebug("upload err: %s", qPrintable(err));
}

void Uploader::uploadFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  if( exitCode == 0 && exitStatus == QProcess::NormalExit )
    mainWindow->message(tr("Upload complete"), MsgType::Notice, tr("Uploader") );
  else
    mainWindow->message(tr("Upload failed"), MsgType::Warning, tr("Uploader") );
	progressBar->reset();
  hide();
  setWindowTitle(tr("Uploader"));
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
      msg = tr("uploader (sam7) was canceled or crashed");
      break;
    case QProcess::Timedout:
      msg = tr("uploader (sam7) timed out");
      break;
    case QProcess::WriteError:
      msg = tr("uploader (sam7) reported a write error");
      break;
    case QProcess::ReadError:
      msg = tr("uploader (sam7) reported a read error");
      break;
    case QProcess::UnknownError:
      msg = tr("uploader (sam7) - unknown error type");
      break;
  }
  mainWindow->message( msg, MsgType::Error, tr("Uploader"));
  hide();
}

void Uploader::onDialogClosed( )
{
  if(uploader.state() != QProcess::NotRunning )
    uploader.kill();
}






