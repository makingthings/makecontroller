/*********************************************************************************

 Copyright 2006-2009 MakingThings

 Licensed under the Apache License,
 Version 2.0 (the "License"); you may not use this file except in compliance
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#include <QFileInfo>
#include <QSettings>
#include "Uploader.h"
#include "Preferences.h"
#include <QDir>
#include <QFileDialog>
#include <QtDebug>

#ifdef Q_OS_MAC
#include <CoreFoundation/CFURL.h>
#include <CoreFoundation/CFBundle.h>
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

  QSettings settings;
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
  QSettings settings;
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
  filename.replace(" ", "\\ "); // escape any spaces in the filename
  qDebug() << "uploading" << filename;

  QStringList uploaderArgs;
  uploaderArgs << "-e" << "set_clock";
  uploaderArgs << "-e" << "unlock_regions";
  uploaderArgs << "-e" << QString("flash -show_progress %1").arg(filename);
  uploaderArgs << "-e" << "boot_from_flash";
  uploaderArgs << "-e" << "reset";
  uploader.start(sam7Path(), uploaderArgs);
  QFileInfo fi(filename);
  setWindowTitle(tr("Uploader - uploading %1").arg(fi.fileName()));
}

QString Uploader::sam7Path( )
{
  QString uploaderName;
  #ifdef Q_OS_MAC // get the path within the app bundle
  CFURLRef pluginRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
  CFStringRef macPath = CFURLCopyFileSystemPath(pluginRef, kCFURLPOSIXPathStyle);
  QDir appBundle( CFStringGetCStringPtr(macPath, CFStringGetSystemEncoding()) );
  uploaderName = appBundle.filePath( "Contents/Resources/sam7" );
  #elif defined (Q_OS_WIN)
  QDir d = QDir::current();
  if(!d.exists("sam7.exe")) // in dev mode, we're one dir down in 'bin'
    d.cdUp();
  uploaderName = d.filePath("sam7.exe");
  #else
  QSettings settings;
  QDir dir( settings.value("sam7_path", DEFAULT_SAM7_PATH).toString() );
  uploaderName = dir.filePath("sam7");
  #endif
  return uploaderName;
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
    qDebug() << tr("upload output:") << output;
}

void Uploader::filterError( )
{
  QString err(uploader.readAllStandardError());
  if(err.startsWith("can't find boot agent"))
    mainWindow->message(tr("Couldn't find an unprogrammed board to upload to."), MsgType::Error, tr("Uploader") );
  else
    qDebug() << tr("upload err:") << err;
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
  setWindowTitle(tr("Uploader"));
  mainWindow->message( msg, MsgType::Error, tr("Uploader"));
  hide();
}

void Uploader::onDialogClosed( )
{
  if(uploader.state() != QProcess::NotRunning )
    uploader.kill();
}






