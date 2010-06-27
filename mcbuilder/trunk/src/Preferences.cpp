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

/*
  The dialog that pops up when "preferences" is clicked in the menu.
*/
Preferences::Preferences(MainWindow *mainWindow) : QDialog( 0 )
{
  this->mainWindow = mainWindow;
  ui.setupUi(this);
  connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(applyChanges()));
  connect(ui.browseWorkspaceButton, SIGNAL(clicked()), this, SLOT(browseWorkspace()));
  connect(ui.fontButton, SIGNAL(clicked()), this, SLOT(getNewFont()));
  connect(ui.makePathButton, SIGNAL(clicked()), this, SLOT(onMakePathButton()));
  connect(ui.armelfPathButton, SIGNAL(clicked()), this, SLOT(onArmElfPathButton()));
  connect(ui.sam7PathButton, SIGNAL(clicked()), this, SLOT(onSam7Button()));

  // initialize the parts that the main window needs to know about
  QSettings settings;
  QString editorFont = settings.value("editorFont", DEFAULT_FONT).toString();
  int editorFontSize = settings.value("editorFontSize", DEFAULT_FONT_SIZE).toInt();
  currentFont = QFont(editorFont, editorFontSize);
  tempFont = currentFont;
  mainWindow->setEditorFont(editorFont, editorFontSize);
  ui.fontBox->setText(QString("%1, %2pt").arg(editorFont).arg(editorFontSize));
  mainWindow->setTabWidth( settings.value("tabWidth", DEFAULT_TAB_WIDTH).toInt() );

  ui.mcVersionComboBox->addItem("Version 1.x");
  ui.mcVersionComboBox->addItem("Version 2.0");
  QString version = settings.value("mcVersion").toString();
  int idx = ui.mcVersionComboBox->findText( version );
  if( idx > 0 )
    ui.mcVersionComboBox->setCurrentIndex(idx);

  ui.appVersionComboBox->addItem("Version 1.0");
  ui.appVersionComboBox->addItem("Version 2.0");
  version = settings.value("appVersion").toString();
  idx = ui.appVersionComboBox->findText( version );
  if( idx > 0 )
    ui.appVersionComboBox->setCurrentIndex(idx);

  resize(ui.gridLayout->sizeHint());
}

// static
QString Preferences::workspace( )
{
  QSettings settings;
  #ifdef Q_WS_MAC
  QString workspace = QDir::home().filePath("Documents/mcbuilder");
  #elif defined(Q_WS_WIN)
  // would be nice to use home dir, but can't have spaces in file path
  QString workspace = QDir::current().filePath("workspace");
  #else
  QString workspace = QDir::home().path() + "/mcbuilder";
  #endif // Q_WS_MAC
  workspace = settings.value("workspace", workspace).toString();
  // always make sure the workspace directory exists
  QDir dir(workspace);
  if (!dir.exists()) {
    dir.cdUp();
    #ifdef Q_WS_WIN
    dir.mkdir("workspace");
    #else
    dir.mkdir("mcbuilder");
    #endif // Q_WS_WIN
  }
  return QDir::toNativeSeparators(workspace);
}

// static
QString Preferences::boardType()
{
  QSettings settings;
  return settings.value("boardType").toString();
}

/*
  Each of the options in the "tools" tab are empty by default.
  If this is the case, use the tools shipped with mcbuilder.
  Otherwise, use the tools specified by the user.
*/
QString Preferences::toolsPath( ) // static
{
  QSettings settings;
  return settings.value("toolsPath", QDir::current().filePath("resources/tools/arm-none-eabi/bin")).toString();
}

QString Preferences::makePath( ) // static
{
  QSettings settings;
  #if (defined Q_OS_MAC) || (defined Q_OS_WIN)
  QString _makepath = "resources/tools";
  #else
  QString _makepath = "";
  #endif
  return settings.value("makePath", QDir::current().filePath(_makepath)).toString();
}

QString Preferences::sam7Path( ) // static
{
  QSettings settings;
  return settings.value("sam7Path", QDir::current().filePath("resources/tools")).toString();
}

/*
  Read the current settings, load them into the UI and then display it
*/
void Preferences::loadAndShow( )
{
  QSettings settings;
  ui.workspaceEdit->setText(QDir::toNativeSeparators(workspace()));
  ui.makePathEdit->setText( QDir::toNativeSeparators(makePath()));
  ui.toolsPathEdit->setText(QDir::toNativeSeparators(toolsPath()));
  ui.sam7PathEdit->setText( QDir::toNativeSeparators(sam7Path()));

  bool state = settings.value("checkForUpdates", true).toBool();
  ui.updaterBox->setChecked(state);

  ui.fontBox->setText(QString("%1, %2pt").arg(currentFont.family()).arg(currentFont.pointSize()));
  ui.tabWidth->setText(QString::number(settings.value("tabWidth", DEFAULT_TAB_WIDTH).toInt()));
  show();
}

/*
  The browse button has been clicked.
  Pop up a dialog to let the user select a new workspace.
*/
void Preferences::browseWorkspace( )
{
  QString newProjDir = QFileDialog::getExistingDirectory(this, tr("Select Workspace Directory"),
                                                           Preferences::workspace(), QFileDialog::ShowDirsOnly);
  if (!newProjDir.isNull()) // will be null if user hit cancel
    ui.workspaceEdit->setText(newProjDir);
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
  if (ok) { // the user clicked OK and font is set to the font the user selected
    tempFont = newFont;
    ui.fontBox->setText(QString("%1, %2pt").arg(tempFont.family()).arg(tempFont.pointSize()));
  }
}

/*
  The "ok" button has been clicked.
  Rip through the preferences items, see if any have changed
  and call the mainwindow back if it needs to be updated.
*/
void Preferences::applyChanges( )
{
  QSettings settings;

  settings.setValue("workspace", ui.workspaceEdit->text());
  settings.setValue("makePath", ui.makePathEdit->text());
  settings.setValue("toolsPath", ui.toolsPathEdit->text());
  settings.setValue("sam7Path", ui.sam7PathEdit->text());
  settings.setValue("checkForUpdates", (ui.updaterBox->isChecked()));

  settings.setValue("mcVersion", ui.mcVersionComboBox->itemText(ui.mcVersionComboBox->currentIndex()));
  settings.setValue("appVersion", ui.appVersionComboBox->itemText(ui.appVersionComboBox->currentIndex()));

  int oldTabWidth = settings.value("tabWidth", DEFAULT_TAB_WIDTH).toInt();
  if (oldTabWidth != ui.tabWidth->text().toInt()) {
    settings.setValue("tabWidth", ui.tabWidth->text().toInt());
    mainWindow->setTabWidth(ui.tabWidth->text().toInt());
  }

  if (tempFont.family() != currentFont.family() || tempFont.pointSize() != currentFont.pointSize()) {
    currentFont.setFamily(tempFont.family());
    currentFont.setPointSize(tempFont.pointSize());
    mainWindow->setEditorFont(currentFont.family(), currentFont.pointSize());
    settings.setValue("editorFont", currentFont.family());
    settings.setValue("editorFontSize", currentFont.pointSize());
  }
}

void Preferences::onMakePathButton()
{
  QString newMakeDir = QFileDialog::getExistingDirectory(this, tr("Select Directory Containing Make"),
                                                           makePath(), QFileDialog::ShowDirsOnly);
  if (!newMakeDir.isNull()) // will be null if user hit cancel
    ui.makePathEdit->setText(newMakeDir);
}

void Preferences::onArmElfPathButton()
{
  QString newArmElfDir = QFileDialog::getExistingDirectory(this, tr("Select Directory Containing GnuArm Tools"),
                                                           toolsPath(), QFileDialog::ShowDirsOnly);
  if (!newArmElfDir.isNull()) // will be null if user hit cancel
    ui.toolsPathEdit->setText(newArmElfDir);
}

void Preferences::onSam7Button()
{
  QString newSam7Dir = QFileDialog::getExistingDirectory(this, tr("Select Directory Containing GnuArm Tools"),
                                                           sam7Path(), QFileDialog::ShowDirsOnly);
  if (!newSam7Dir.isNull()) // will be null if user hit cancel
    ui.sam7PathEdit->setText(newSam7Dir);
}
