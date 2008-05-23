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


#include <QFileDialog>
#include <QSettings>
#include "Preferences.h"

Preferences::Preferences(MainWindow *mainWindow) : QDialog( 0 )
{
	this->mainWindow = mainWindow;
	setupUi(this);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(applyChanges()));
	connect(browseWorkspaceButton, SIGNAL(clicked()), this, SLOT(browseWorkspace()));
}

// static
QString Preferences::workspace( )
{
	QSettings settings("MakingThings", "mcbuilder");
	#ifdef Q_WS_MAC
	QString workspace = QDir::home().path() + "/Documents/mcbuilder";
	#else
	QString workspace = QDir::home().path() + "/mcbuilder";
	#endif
	workspace = settings.value("workspace", workspace).toString();
	// always make sure the workspace directory exists
	QDir dir(workspace);
	if(!dir.exists())
	{
		dir.cdUp();
		dir.mkdir("mcbuilder");
	}
	return workspace;
}

// static
QString Preferences::boardType( )
{
	QSettings settings("MakingThings", "mcbuilder");
	// select Make Controller by default
	return settings.value("boardType", "Make Controller").toString();
}

QString Preferences::toolsPath( ) // static
{
  QSettings settings("MakingThings", "mcbuilder");
  QString path = settings.value("toolsPath").toString();
  if(path.isEmpty())
    return QDir::currentPath() + "/resources/tools";
  else
    return path;
}

QString Preferences::makePath( ) // static
{
  QSettings settings("MakingThings", "mcbuilder");
  QString path = settings.value("makePath").toString();
  if(path.isEmpty())
    return QDir::currentPath() + "/resources/tools";
  else
    return path;
}

QString Preferences::sam7Path( ) // static
{
  QSettings settings("MakingThings", "mcbuilder");
  QString path = settings.value("sam7Path").toString();
  if(path.isEmpty())
    return QDir::currentPath() + "/resources/tools";
  else
    return path;
}

// read the current settings, load them into the preferences form and then display it
void Preferences::loadAndShow( )
{
	QSettings settings("MakingThings", "mcbuilder");
  workspaceEdit->setText( workspace() );
  makePathEdit->setText( settings.value("makePath").toString() );
  toolsPathEdit->setText( settings.value("toolsPath").toString() );
  sam7PathEdit->setText( settings.value("sam7Path").toString() );
  
	tabWidth->setText(QString::number(settings.value("tabWidth", 2).toInt()));
	show( );
}

/*
  The browse button has been clicked.
*/
void Preferences::browseWorkspace( )
{
	QString dummy;
	QString newProjDir = QFileDialog::getOpenFileName(this, tr("Select Project Directory"), 
																					Preferences::workspace(), "", &dummy, QFileDialog::ShowDirsOnly);
	if( !newProjDir.isNull() ) // will be null if user hit cancel
		workspaceEdit->setText(newProjDir);
}

// rip through the preferences items, see if any have changed 
// and call the mainwindow back if it needs to be updated
void Preferences::applyChanges( )
{
	QSettings settings("MakingThings", "mcbuilder");
	
	settings.setValue("workspace", workspaceEdit->text());
  settings.setValue("makePath", makePathEdit->text());
  settings.setValue("toolsPath", toolsPathEdit->text());
  settings.setValue("sam7Path", sam7PathEdit->text());
	
	int oldTabWidth = settings.value("tabWidth").toInt();
	if( oldTabWidth != tabWidth->text().toInt() )
	{
		settings.setValue("tabWidth", tabWidth->text().toInt());
		mainWindow->setTabWidth( tabWidth->text().toInt() );
	}
}




