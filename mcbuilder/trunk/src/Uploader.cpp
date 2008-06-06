/*********************************************************************************

 Copyright 2008 MakingThings

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
#include "Uploader.h"

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
  connect(uploaderProgress, SIGNAL(canceled()), this, SLOT(kill()));
  connect(uploaderProgress, SIGNAL(finished(int)), this, SLOT(onProgressDialogFinished(int)));
  connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(readOutput()));
  connect(this, SIGNAL(readyReadStandardError()), this, SLOT(readError()));
  connect(this, SIGNAL(started()), this, SLOT(uploadStarted()));
  connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(uploadFinished(int, QProcess::ExitStatus)));
  connect(this, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onError(QProcess::ProcessError)));
}

bool Uploader::upload(QString boardProfileName, QString filename)
{
  bool retval = false;
  // read the board profile and find which uploader we should use
  QDir dir = QDir::current().filePath("resources/board_profiles");
  QDomDocument doc;
  QFile file(dir.filePath(boardProfileName));
  currentFile = QDir::toNativeSeparators(filename);
  if(doc.setContent(&file))
  {
	QDomNodeList nodes = doc.elementsByTagName("uploader");
    if(nodes.count())
      uploaderName = QDir::toNativeSeparators(nodes.at(0).toElement().text());
    QStringList uploaderArgs;
    uploaderArgs << "-e" << "set_clock";
    uploaderArgs << "-e" << "unlock_regions";
    uploaderArgs << "-e" << QString("flash %1").arg(currentFile);
    uploaderArgs << "-e" << "boot_from_flash";
    QDir sam7dir(Preferences::sam7Path());
    start(sam7dir.filePath(uploaderName), uploaderArgs);
    retval = true;
    file.close();
  }
  else
    retval = false;
  return retval;
}

void Uploader::readOutput( )
{
  mainWindow->printOutput(readAllStandardOutput());
}

void Uploader::readError( )
{
  mainWindow->printOutputError(readAllStandardError());
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
  uploaderProgress->hide();
  uploaderProgress->setValue(0);
}

void Uploader::onError(QProcess::ProcessError error)
{
  QString msg;
  switch(error)
  {
    case QProcess::FailedToStart:
      msg = QString("'%1' failed to start.  It's either missing, or doesn't have the correct permissions").arg(uploaderName);
      break;
    case QProcess::Crashed:
      msg = QString("'%1' was canceled or crashed.").arg(uploaderName);
      break;
    case QProcess::Timedout:
      msg = QString("'%1' timed out.").arg(uploaderName);
      break;
    case QProcess::WriteError:
      msg = QString("'%1' reported a write error.").arg(uploaderName);
      break;
    case QProcess::ReadError:
      msg = QString("'%1' reported a read error.").arg(uploaderName);
      break;
    case QProcess::UnknownError:
      msg = QString("'%1' - unknown error type.").arg(uploaderName);
      break;
  }
  mainWindow->printOutputError("Error - uploader: " + msg);
}

void Uploader::onProgressDialogFinished(int result)
{
  if(result == QDialog::Rejected)
    kill();
}




