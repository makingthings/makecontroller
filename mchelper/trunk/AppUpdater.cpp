/*********************************************************************************

 Copyright 2006 MakingThings

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

#include <QDomDocument>

AppUpdater::AppUpdater( ) : QDialog( )
{
	setModal( true );
	setWindowTitle( "Software Update" );
	topLevelLayout = new QHBoxLayout( this );
	textLayout = new QVBoxLayout( this );
	
	downloadButton = new QPushButton( this );
	downloadButton->setText( tr("Visit Download Page") );
	ignoreButton = new QPushButton( this );
	ignoreButton->setText( tr("Not Right Now") );
	connect( downloadButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
	buttonLayout = new QHBoxLayout( );
	buttonLayout->addWidget( ignoreButton );
	buttonLayout->addStretch( );
	buttonLayout->addWidget( downloadButton );
	
	mchelperIcon = new QPixmap( ":mticon64.png" );
	icon.setPixmap( *mchelperIcon );
	icon.setAlignment( Qt::AlignHCenter );
	
	headline = new QLabel( );
	headline->setWordWrap( false );
	
	details = new QLabel( );
	details->setWordWrap( false );
	
	browser = new QTextEdit( );
	browser->setReadOnly( true );
	
	textLayout->addWidget( headline );
	textLayout->addWidget( details );
	textLayout->addWidget( browser );
	textLayout->addLayout( buttonLayout );
	topLevelLayout->addWidget( &icon );
	topLevelLayout->addLayout( textLayout );
	topLevelLayout->setAlignment( Qt::AlignHCenter );
	
	connect( &http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( finishedRead( int, bool ) ) );
}

void AppUpdater::on_actionCheckForUpdates( )
{
	checkForUpdates( );
}

void AppUpdater::checkForUpdates( )
{
	http.setHost("www.makingthings.com");
	GETid = http.get("/appcasts/mchelper.xml");
}

void AppUpdater::finishedRead( int id, bool errors )
{
	(void)errors; // ugh - deal with this at some point
	if( id != GETid )
		return;
	
	QDomDocument doc;
	QString err;
	int line, col;
	
	if (!doc.setContent(http.readAll(), true, &err, &line, &col))
	{
		return;
	}
	
	QDomElement channel = doc.documentElement().firstChild().toElement();
	QDomNodeList items = channel.elementsByTagName("item");
	QPair<QString, QString> latest(MCHELPER_VERSION, "");
	bool updateAvailable = false;
	
	for (int i=0, j=items.size(); i<j; i++)
	{
		QDomElement item = items.item(i).toElement();
		if( item.isNull() ) 
			continue;
		QDomNodeList enclosures = item.elementsByTagName("enclosure");
		
		for (int k=0, l=enclosures.size(); k<l; k++)
		{
			QDomElement enclosure = enclosures.item(k).toElement();
			if (enclosure.isNull()) continue;
			QString version = enclosure.attributeNS(
				"http://www.andymatuschak.org/xml-namespaces/sparkle", "version", "not-found" );
			
			// each item can have multiple enclosures, of which at least one
			// should have a version field
			if (version == "not-found") continue;
			
			if( versionCompare(version, latest.first) > 0 )
			{
				latest.first = version;
				QDomNodeList descs = item.elementsByTagName("description");
				//I(descs.size() == 1);
				QDomElement desc = descs.item(0).toElement();
				//I(!desc.isNull());
				latest.second = desc.text();
				updateAvailable = true;
			}
		}
	}
	
	if( updateAvailable )
	{
		headline->setText( "<font size=4>A new version of mchelper is available!</font>" );
		QString d = QString( "mchelper %1 is now available (you have %2).  Would you like to download it?" )
													.arg(latest.first).arg( MCHELPER_VERSION );
		details->setText( d );
		browser->setHtml( latest.second );
	}
	else
	{
		headline->setText( "<font size=4>You're up to date!</font>" );
		details->setText( QString( "You're running the latest version of mchelper, version %1." ).arg( MCHELPER_VERSION ) );
	}
	this->show( );
}

int AppUpdater::versionCompare(const QString & left, const QString & right)
{
    QStringList leftParts = left.split(".");
    QStringList rightParts = right.split(".");
    
    int leftCount = leftParts.size();
    int rightCount = rightParts.size();
    int maxCount = leftCount > rightCount ? leftCount : rightCount;
    
    for (int i=0, j=maxCount; i<j; i++)
    {
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



