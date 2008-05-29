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
#include <QFontDialog>
#include "Preferences.h"

#ifdef Q_WS_MAC
#define DEFAULT_FONT "Monaco"
#define DEFAULT_FONT_SIZE 12
#else
#define DEFAULT_FONT "Courier"
#define DEFAULT_FONT_SIZE 10
#endif
#define DEFAULT_TAB_WIDTH 2
#define DEFAULT_BOARDTYPE "Make Controller"

/*
  The dialog that pops up when "preferences" is clicked in the menu.
*/
Preferences::Preferences(MainWindow *mainWindow) : QDialog( 0 )
{
	this->mainWindow = mainWindow;
	setupUi(this);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(applyChanges()));
	connect(browseWorkspaceButton, SIGNAL(clicked()), this, SLOT(browseWorkspace()));
  connect(fontButton, SIGNAL(clicked()), this, SLOT(getNewFont()));
  
  // initialize the parts that the main window needs to know about
  QSettings settings("MakingThings", "mcbuilder");
  QString editorFont = settings.value("editorFont", DEFAULT_FONT).toString();
  int editorFontSize = settings.value("editorFontSize", DEFAULT_FONT_SIZE).toInt();
  currentFont = QFont(editorFont, editorFontSize);
  tempFont = currentFont;
  mainWindow->setEditorFont(editorFont, editorFontSize);
  fontBox->setText(QString("%1, %2pt").arg(editorFont).arg(editorFontSize));
  mainWindow->setTabWidth( settings.value("tabWidth", DEFAULT_TAB_WIDTH).toInt() );
}

// static
QString Preferences::workspace( )
{
	QSettings settings("MakingThings", "mcbuilder");
	#ifdef Q_WS_MAC
	QString workspace = QDir::home().path() + "/Documents/mcbuilder";
	#elif #defined Q_WS_WIN
	// would be nice to use home dir, but can't have spaces in file path
	QString workspace = QDir::toNativeSeparators("C:/mcbuilder");
	#else
	QString workspace = QDir::home().path() + "/mcbuilder";
	#endif // Q_WS_MAC
	workspace = settings.value("workspace", workspace).toString();
	// always make sure the workspace directory exists
	QDir dir(workspace);
	if(!dir.exists())
	{
		dir.cdUp();
		dir.mkdir("mcbuilder");
	}
	return QDir::toNativeSeparators(workspace);
}

// static
QString Preferences::boardType( )
{
	QSettings settings("MakingThings", "mcbuilder");
	return settings.value("boardType", DEFAULT_BOARDTYPE).toString();
}

/*
  Each of the options in the "tools" tab are empty by default.
  If this is the case, use the tools shipped with mcbuilder.
  Otherwise, use the tools specified by the user.
*/
QString Preferences::toolsPath( ) // static
{
  QSettings settings("MakingThings", "mcbuilder");
  QString path = settings.value("toolsPath").toString();
  if(path.isEmpty())
    return QDir::currentPath() + "/resources/tools/arm-elf/bin";
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

/*
  Read the current settings, load them into the UI and then display it
*/
void Preferences::loadAndShow( )
{
	QSettings settings("MakingThings", "mcbuilder");
  workspaceEdit->setText( workspace() );
  makePathEdit->setText( settings.value("makePath").toString() );
  toolsPathEdit->setText( settings.value("toolsPath").toString() );
  sam7PathEdit->setText( settings.value("sam7Path").toString() );
  
  fontBox->setText(QString("%1, %2pt").arg(currentFont.family()).arg(currentFont.pointSize()));
	tabWidth->setText(QString::number(settings.value("tabWidth", DEFAULT_TAB_WIDTH).toInt()));
	show( );
}

/*
  The browse button has been clicked.
  Pop up a dialog to let the user select a new workspace.
*/
void Preferences::browseWorkspace( )
{
	QString newProjDir = QFileDialog::getExistingDirectory(this, tr("Select Workspace Directory"), 
																					Preferences::workspace(), QFileDialog::ShowDirsOnly);
	if( !newProjDir.isNull() ) // will be null if user hit cancel
		workspaceEdit->setText(newProjDir);
}

/*
  The user has clicked on the "choose" button to select a new font.
  Pop up a font dialog and store the selected font in an intermediate spot.
  Only apply this once the dialog has been accepted (applyChanges()).
*/
void Preferences::getNewFont( )
{
  bool ok = false;
  QFont newFont = QFontDialog::getFont(&ok, QFont(currentFont.family(), currentFont.pointSize()), this);
  if(ok) // the user clicked OK and font is set to the font the user selected
  {
    tempFont = newFont;
    fontBox->setText(QString("%1, %2pt").arg(tempFont.family()).arg(tempFont.pointSize()));
  }
}

/*
  The "ok" button has been clicked.
  Rip through the preferences items, see if any have changed 
  and call the mainwindow back if it needs to be updated.
*/
void Preferences::applyChanges( )
{
	QSettings settings("MakingThings", "mcbuilder");
	
	settings.setValue("workspace", workspaceEdit->text());
  settings.setValue("makePath", makePathEdit->text());
  settings.setValue("toolsPath", toolsPathEdit->text());
  settings.setValue("sam7Path", sam7PathEdit->text());
	
	int oldTabWidth = settings.value("tabWidth", DEFAULT_TAB_WIDTH).toInt();
	if( oldTabWidth != tabWidth->text().toInt() )
	{
		settings.setValue("tabWidth", tabWidth->text().toInt());
		mainWindow->setTabWidth( tabWidth->text().toInt() );
	}
  
  if(tempFont.family() != currentFont.family() || tempFont.pointSize() != currentFont.pointSize())
  {
    currentFont.setFamily(tempFont.family());
    currentFont.setPointSize(tempFont.pointSize());
    mainWindow->setEditorFont(currentFont.family(), currentFont.pointSize());
    settings.setValue("editorFont", currentFont.family());
    settings.setValue("editorFontSize", currentFont.pointSize());
  }
}




