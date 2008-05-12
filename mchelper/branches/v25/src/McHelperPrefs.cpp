/*********************************************************************************

Copyright 2006-2008 MakingThings

Licensed under the Apache License, 
Version 2.0 (the "License"); you may not use this file except in compliance 
with the License. You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for
the specific language governing permissions and limitations under the License.

*********************************************************************************/

#include "McHelperPrefs.h"

mchelperPrefs::mchelperPrefs( McHelperWindow *mainWindow ) : QDialog( )
{
	setupUi(this);
	this->mainWindow = mainWindow;
	
	connect( defaultsButton, SIGNAL( clicked() ), mainWindow, SLOT( restoreDefaultPrefs() ) );
	connect( okButton, SIGNAL( accepted( ) ), mainWindow, SLOT( setNewPrefs( ) ) );
	
	setUdpListenPortDisplay( mainWindow->appUdpListenPort );
	setUdpSendPortDisplay( mainWindow->appUdpSendPort );
	setXmlPortDisplay( mainWindow->appXmlListenPort );
	setFindNetBoardsDisplay( mainWindow->findEthernetBoardsAuto );
	setMaxMsgsDisplay( mainWindow->maxOutputWindowMessages );
	QSettings settings("MakingThings", "mchelper");
	if( settings.value( "checkForUpdatesOnStartup", true ).toBool( ) )
		updatesCheckBox->setCheckState( Qt::Checked );
	else
		updatesCheckBox->setCheckState( Qt::Unchecked );
	setFindNetBoardsDisplay( settings.value("findEthernetBoardsAuto", true ).toBool( ) );
}

void mchelperPrefs::setUdpListenPortDisplay( int port )
{
	udpListenPortPrefs->setText( QString::number( port ) );
}

void mchelperPrefs::setUdpSendPortDisplay( int port )
{
	udpSendPortPrefs->setText( QString::number( port ) );
}

void mchelperPrefs::setXmlPortDisplay( int port )
{
	xmlPortPrefs->setText( QString::number( port ) );
}

void mchelperPrefs::setMaxMsgsDisplay( int max )
{
	maxOutputMsgsLineEdit->setText( QString::number( max ) );
}

void mchelperPrefs::setFindNetBoardsDisplay( bool state )
{
	if( state )
		networkPingCheckBox->setCheckState( Qt::Checked );
	else
		networkPingCheckBox->setCheckState( Qt::Unchecked );
}

bool mchelperPrefs::getFindNetBoardsDisplay( )
{
	return networkPingCheckBox->checkState( );
}

bool mchelperPrefs::getUpdateCheckBoxDisplay( )
{
	return updatesCheckBox->checkState( );
}

