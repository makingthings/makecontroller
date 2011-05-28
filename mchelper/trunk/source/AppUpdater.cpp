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

#define UPDATE_URL "http://www.makingthings.com/updates/mchelper.xml"

AppUpdater::AppUpdater(QWidget * parent) : QDialog(parent)
{
  setModal( true );
  setWindowTitle( tr("Software Update") );

  acceptButton.setDefault( true );
  ignoreButton.setText( tr("Not Right Now") );

  buttonLayout.addStretch( );
  buttonLayout.addWidget( &acceptButton );

  mchelperIcon.load( ":icons/mticon64.png" );
  icon.setPixmap( mchelperIcon );
  icon.setAlignment( Qt::AlignHCenter );

  headline.setWordWrap( false );
  details.setWordWrap( false );
  browser.setReadOnly( true );

  textLayout.addWidget( &headline );
  textLayout.addWidget( &details );
  textLayout.addLayout( &buttonLayout );
  topLevelLayout.addWidget( &icon );
  topLevelLayout.addLayout( &textLayout );
  topLevelLayout.setAlignment( Qt::AlignHCenter );

  this->setLayout( &topLevelLayout );
  checkingOnStartup = true; // hide the dialog by default
  connect( &netAccess, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishedRead(QNetworkReply*)) );
}

void AppUpdater::checkForUpdates( bool inBackground )
{
  checkingOnStartup = inBackground;
  netAccess.get(QNetworkRequest(QUrl(UPDATE_URL)));
}


void AppUpdater::finishedRead(QNetworkReply* reply)
{
  if (reply->error() != QNetworkReply::NoError || reply->url().toString() != UPDATE_URL) {
    headline.setText(tr("<font size=4>Couldn't contact the update server...</font>"));
    details.setText(tr("Make sure you're connected to the internet."));
    acceptButton.setText(tr("OK"));
    acceptButton.disconnect(); // make sure it wasn't connected by anything else previously
    connect(&acceptButton, SIGNAL(clicked()), this, SLOT(accept()));
    removeBrowserAndIgnoreButton();

    if (!checkingOnStartup)
      this->show();
    delete reply;
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
        if (!version.isEmpty() && versionCompare(version, latest.first) > 0 ) {
          latest.first = version;
          updateAvailable = true;
          break;
        }
      }
    }
  }

  delete reply;

  // add the appropriate elements/info depending on whether an update is available
  if( updateAvailable ) {
    headline.setText( tr("<font size=4>A new version of mchelper is available!</font>") );
    QString d = QString( tr("mchelper %1 is now available (you have %2).  Would you like to download it?") )
                          .arg(latest.first).arg( MCHELPER_VERSION );
    details.setText( d );
    browser.setHtml( latest.second );
    acceptButton.setText( tr("Visit Download Page") );
    acceptButton.disconnect( );
    ignoreButton.disconnect( );
    connect( &acceptButton, SIGNAL( clicked() ), this, SLOT( visitDownloadsPage() ) );
    connect( &ignoreButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
    if( textLayout.indexOf( &browser ) < 0 ) // if the browser's not in the layout, then insert it after the details line
      textLayout.insertWidget( textLayout.indexOf( &details ) + 1, &browser );
    if( buttonLayout.indexOf( &ignoreButton ) < 0 ) // put the ignore button on the left
      buttonLayout.insertWidget( 0, &ignoreButton );

    this->show( );
  }
  else {
    headline.setText( tr("<font size=4>You're up to date!</font>") );
    details.setText( QString( tr("You're running the latest version of mchelper, version %1.") ).arg( MCHELPER_VERSION ) );
    acceptButton.setText( tr("OK") );
    acceptButton.disconnect( );
    connect( &acceptButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
    removeBrowserAndIgnoreButton( );
    if(!checkingOnStartup)
      this->show( );
  }
}

void AppUpdater::removeBrowserAndIgnoreButton( )
{
  if( textLayout.indexOf( &browser ) >= 0 ) // if the browser's in the layout, rip it out
    textLayout.removeWidget( &browser );
  browser.setParent( NULL );

  if( textLayout.indexOf( &ignoreButton ) >= 0 ) // if the ignoreButton's in the layout, rip it out
    textLayout.removeWidget( &ignoreButton );
  ignoreButton.setParent( NULL );
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
  QDesktopServices::openUrl( QUrl( "http://www.makingthings.com/resources/downloads" ) );
  accept( );
}
