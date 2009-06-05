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
    QString uploaderName;

  private slots:
    void filterOutput();
    void filterError();
    void uploadStarted();
    void uploadFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onError(QProcess::ProcessError error);
    void onProgressDialogFinished(int result);
};

#endif // UPLOADER_H

