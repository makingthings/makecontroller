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

#ifndef APP_UPDATER_H
#define APP_UPDATER_H

#include <QNetworkAccessManager>
#include <QTextEdit>
#include <QXmlStreamReader>

#include "ui_appupdater.h"

#define APPUPDATE_BACKGROUND true
#define APPUPDATE_FOREGROUND false

class AppUpdater : public QDialog
{
  Q_OBJECT
  public:
    AppUpdater(QWidget* parent = 0);
    void checkForUpdates(bool inBackground);
    bool checkingInBackground;

  private:
    QNetworkAccessManager netAccess;
    QXmlStreamReader xmlReader;
    int versionCompare(const QString & left, const QString & right);
    void setBrowserAndIgnoreButtonVisible(bool visible);
    Ui::AppUpdaterUi ui;

  private slots:
    void finishedRead(QNetworkReply* reply);
    void visitDownloadsPage();
};

#endif // APP_UPDATER_H
