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

/*
  Some code in here modified from Thomas Keller's guitone project
  http://guitone.thomaskeller.biz
  Author - Thomas Keller, me@thomaskeller.biz
*/

#include "AppUpdater.h"

#include <QDesktopServices>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QtDebug>
#include <QPushButton>

#define UPDATE_URL "http://www.makingthings.com/updates/mchelper.xml"

AppUpdater::AppUpdater(QWidget * parent)
  : QDialog(parent),
    checkingInBackground(true) // hidden by default
{
  ui.setupUi(this);
//  setModal(true);

  connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(&netAccess, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishedRead(QNetworkReply*)));
}

void AppUpdater::checkForUpdates(bool inBackground)
{
  checkingInBackground = inBackground;
  netAccess.get(QNetworkRequest(QUrl(UPDATE_URL)));
}


void AppUpdater::finishedRead(QNetworkReply* reply)
{
  if (reply->error() != QNetworkReply::NoError) {
    reply->deleteLater();
    if (!checkingInBackground) {
      ui.headlineLabel->setText(tr("<font size=4>Couldn't contact the update server...</font>"));
      ui.detailsLabel->setText(tr("Maybe you're not connected to the internet?"));
      ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
      setBrowserAndIgnoreButtonVisible(false);
      this->show();
    }
    return;
  }

  QPair<QString, QString> latest(MCHELPER_VERSION, "");
  bool updateAvailable = false;

  xmlReader.setDevice(reply);
  while (!xmlReader.atEnd() && !updateAvailable) {
    if (xmlReader.readNext() == QXmlStreamReader::StartElement) {
      if (xmlReader.name() == "description") {
        // store the desc since it comes first
        latest.second = xmlReader.readElementText();
      }
      else if (xmlReader.name() == "enclosure") {
        QString version = xmlReader.attributes().value("sparkle:version").toString();
        if (!version.isEmpty() && versionCompare(version, latest.first) > 0) {
          latest.first = version;
          updateAvailable = true;
          break;
        }
      }
    }
  }

  reply->deleteLater();

  // add the appropriate elements/info depending on whether an update is available
  if (updateAvailable) {
    ui.headlineLabel->setText(tr("<font size=4>A new version of mchelper is available!</font>") );
    QString d = tr("mchelper %1 is now available (you have %2).  Would you like to download it?")
                          .arg(latest.first).arg(MCHELPER_VERSION);
    ui.detailsLabel->setText(d);
    ui.browser->clear();
    ui.browser->appendHtml(latest.second);
    ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Visit Download Page"));
    ui.buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Not Right Now"));
    setBrowserAndIgnoreButtonVisible(true);
    this->show();
  }
  else {
    if (!checkingInBackground) {
      ui.headlineLabel->setText(tr("<font size=4>You're up to date!</font>"));
      ui.detailsLabel->setText(tr("You're running the latest version of mchelper, version %1.").arg(MCHELPER_VERSION));
      ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
      setBrowserAndIgnoreButtonVisible(false);
      this->show();
    }
  }
}

void AppUpdater::setBrowserAndIgnoreButtonVisible(bool visible)
{
  ui.browser->setVisible(visible);
  ui.buttonBox->button(QDialogButtonBox::Cancel)->setVisible(visible);
  resize(ui.gridLayout->sizeHint());
}

int AppUpdater::versionCompare(const QString & left, const QString & right)
{
    QStringList leftParts = left.split(".");
    QStringList rightParts = right.split(".");

    int leftCount = leftParts.size();
    int rightCount = rightParts.size();
    int maxCount = leftCount > rightCount ? leftCount : rightCount;

    for (int i=0, j=maxCount; i<j; i++) {
      unsigned int l = 0;
      if (i < leftCount) { l = leftParts.at(i).toUInt(); }
      unsigned int r = 0;
      if (i < rightCount) { r = rightParts.at(i).toUInt(); }

      if (l == r) continue;
      if (l > r) return 1;
      return -1;
    }

    return 0;
}

void AppUpdater::visitDownloadsPage( )
{
  QDesktopServices::openUrl(QUrl("http://www.makingthings.com/resources/downloads"));
  accept();
}
