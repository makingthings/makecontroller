/*********************************************************************************

 Copyright 2008-2009 MakingThings

 Licensed under the Apache License,
 Version 2.0 (the "License"); you may not use this file except in compliance
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/


#include <QDir>
#include <QDomDocument>
#include <QTextStream>
#include <QDebug>
#include "Uploader.h"

#ifdef Q_WS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

/*
  Uploader handles uploading a binary image to a board.  It reads the board profile for the
  currently selected board to determine which uploader to use.  Then it fires up a QProcess
  and runs the uploader with flags determined by settings in Preferences.  It prints output
  from the upload process back to the console output in the MainWindow.
*/
Uploader::Uploader(MainWindow *mainWindow) : QProcess( )
{
  this->mainWindow = mainWindow;
  uploaderProgress = new QProgressDialog( );
  connect(uploaderProgress, SIGNAL(canceled()), this, SLOT(kill()));
  connect(uploaderProgress, SIGNAL(finished(int)), this, SLOT(onProgressDialogFinished(int)));
  connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(filterOutput()));
  connect(this, SIGNAL(readyReadStandardError()), this, SLOT(filterError()));
  connect(this, SIGNAL(started()), this, SLOT(uploadStarted()));
  connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(uploadFinished(int, QProcess::ExitStatus)));
  connect(this, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onError(QProcess::ProcessError)));
}

/*
  Upload a file to a board based on the given boardProfile.
  Extract the name of the uploader to use, then fire it off in a
  separate process with the appropriate arguments.
*/
bool Uploader::upload(const QString & boardProfileName, QString filename)
{
  bool retval = false;
  // read the board profile and find which uploader we should use
  QDir dir = QDir::current().filePath("resources/board_profiles");
  QDomDocument doc;
  QFile file(dir.filePath(boardProfileName));
  currentFile = filename;
  if(doc.setContent(&file))
  {
    QDomNodeList nodes = doc.elementsByTagName("uploader");
    if(nodes.count())
      uploaderName = nodes.at(0).toElement().text();

    filename.replace(" ", "\\ ");
    qDebug() << "uploading" << filename;

    QStringList uploaderArgs;
    uploaderArgs << "-e" << "set_clock";
    uploaderArgs << "-e" << "unlock_regions";
    uploaderArgs << "-e" << QString("flash -show_progress %1").arg(currentFile);
    uploaderArgs << "-e" << "boot_from_flash";
    uploaderArgs << "-e" << "reset";
    QDir sam7dir(Preferences::sam7Path());
    start(sam7dir.absoluteFilePath(uploaderName), uploaderArgs);
    retval = true;
    file.close();
  }
  else
    retval = false;
  return retval;
}

/*
  Filter the output from the uploader to make it more
  meaningful/useful for the user.  Check for upload progress
  And pass it to the upload dialog.
*/
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
    qDebug() << "upload output:" << output;
}

/*
  Filter the error output from the uploader to make it
  more useful to the user.
*/
void Uploader::filterError( )
{
  QString err(readAllStandardError());
  if(err.startsWith("can't find boot agent"))
  {
    mainWindow->printOutputError(tr("Error - couldn't find an unprogrammed board to upload to."));
    mainWindow->printOutputError(tr("  Make sure you've erased and unplugged/replugged your board."));
  }
  else
    qDebug() << "upload err" << err;
  //mainWindow->printOutputError(readAllStandardError());
}

/*
  The upload has started.
  Pop up the upload progress dialog.
*/
void Uploader::uploadStarted( )
{
  QFileInfo fi(currentFile);
  uploaderProgress->setLabelText(tr("Uploading %1...").arg(fi.fileName()));
  uploaderProgress->show();
}

/*
  The upload process has finished.
  Hide the progress dialog and reset its value.
*/
void Uploader::uploadFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  if( exitCode == 0 && exitStatus == QProcess::NormalExit)
    mainWindow->onUploadComplete(true);
  else
    mainWindow->onUploadComplete(false);
  uploaderProgress->reset();
}

/*
  The uploader has reported an error.
  Check what kind it was and print out a message to the user.
*/
void Uploader::onError(QProcess::ProcessError error)
{
  QString msg;
  QFileInfo up(uploaderName);
  QString uploader = up.fileName();
  switch(error)
  {
    case QProcess::FailedToStart:
      msg = tr("uploader failed to start.  '%1' is either missing, or doesn't have the correct permissions").arg(uploader);
      break;
    case QProcess::Crashed:
      msg = tr("uploader (%1) was canceled or crashed.").arg(uploader);
      break;
    case QProcess::Timedout:
      msg = tr("uploader (%1) timed out.").arg(uploader);
      break;
    case QProcess::WriteError:
      msg = tr("uploader (%1) reported a write error.").arg(uploader);
      break;
    case QProcess::ReadError:
      msg = tr("uploader (%1) reported a read error.").arg(uploader);
      break;
    case QProcess::UnknownError:
      msg = tr("uploader (%1) - unknown error type.").arg(uploader);
      break;
  }
  mainWindow->printOutputError(tr("Error - ") + msg);
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




