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
    ~Uploader( );
    void upload(QString filename);
    QProcess::ProcessState state() { return uploader.state(); }

  private:
    MainWindow *mainWindow;
    QProcess uploader;
    QString sam7Path( );

  private slots:
    void filterOutput();
    void filterError();
    void uploadFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onError(QProcess::ProcessError error);
    void onBrowseButton();
    void onUploadButton();
    void onDialogClosed( );
};

#endif // UPLOADER_H


