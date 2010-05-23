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


#include "ProjectInfo.h"
#include <QFileIconProvider>
#include <QHeaderView>
#include <QContextMenuEvent>

#define DEFAULT_VERSION "0.1.0"
#define DEFAULT_OPTLEVEL "Optimize For Size (-Os)"
#define DEFAULT_INCLUDE_DEBUG false
#define DEFAULT_INCLUDE_OSC false
#define DEFAULT_INCLUDE_USB false
#define DEFAULT_INCLUDE_NETWORK false

#define FILENAME_COLUMN 0
#define BUILDTYPE_COLUMN 1

#define FULLPATH_ROLE Qt::UserRole

/*
  ProjectInfo is a dialog box that pops up to manage per-project
  properties.  This includes build configuration and the file list.
*/
ProjectInfo::ProjectInfo(MainWindow *mainWindow) : QDialog( 0 )
{
  this->mainWindow = mainWindow;
  ui.setupUi(this);
  connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(applyChanges()));
  connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(accept()));
  connect(ui.defaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
  connect(ui.fileBrowser, SIGNAL(removeFileRequest(QString)), this, SLOT(onRemoveFileRequest(QString)));
  connect(ui.fileBrowser, SIGNAL(changeBuildType(QString, QString)), this, SLOT(onChangeBuildType(QString, QString)));

  QHeaderView *header = ui.fileBrowser->header();
  header->setResizeMode(FILENAME_COLUMN, QHeaderView::Stretch);
  header->setResizeMode(BUILDTYPE_COLUMN, QHeaderView::ResizeToContents);
  header->setStretchLastSection(false);
//  ui.locationLabel->setForegroundRole(QPalette::Disabled);
}

/*
  Read the project's ProjectInfo from the project file
  and load them into the UI.
*/
bool ProjectInfo::load(const QString & projectPath)
{
  if (projectPath.isEmpty())
    return false;
  QDir projectDir(projectPath);
  setWindowTitle(projectDir.dirName() + " - Project Info");
  ui.locationLabel->setText(projectPath);

  // read the ProjectInfo file
  QFile file(projectFilePath(projectPath));
  if (file.open(QIODevice::ReadOnly|QFile::Text)) {
    QDomDocument projectFile;
    if (projectFile.setContent(&file)) {
      ui.versionEdit->setText(projectFile.elementsByTagName("version").at(0).toElement().text());
      QString optlevel = projectFile.elementsByTagName("optlevel").at(0).toElement().text();
      ui.optLevelBox->setCurrentIndex(ui.optLevelBox->findText(optlevel));
      bool state = (projectFile.elementsByTagName("debuginfo").at(0).toElement().text() == "true");
      ui.debugInfoCheckbox->setChecked(state);

      state = (projectFile.elementsByTagName("include_osc").at(0).toElement().text() == "true");
      ui.oscBox->setChecked(state);

      state = (projectFile.elementsByTagName("include_usb").at(0).toElement().text() == "true");
      ui.usbBox->setChecked(state);

      state = (projectFile.elementsByTagName("include_network").at(0).toElement().text() == "true");
      ui.networkBox->setChecked(state);

      loadFileBrowser(&projectDir, &projectFile);
    }
  }
  else
    return false;
  return true;
}

/*
  Update the file browser to show files in the project.
  Read these from the project's project file.
*/
void ProjectInfo::loadFileBrowser(QDir *projectDir, QDomDocument *projectFile)
{
  // setup the file browser
  ui.fileBrowser->clear(); // get rid of anything previously in there
  QDomNodeList allFiles = projectFile->elementsByTagName("files").at(0).childNodes();
  QTreeWidgetItem *top = new QTreeWidgetItem(QStringList() << projectDir->dirName());
  top->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirIcon));
  ui.fileBrowser->addTopLevelItem(top);

  // only deals with files in the top level directory at the moment
  QFileIconProvider ip;
  for (int i = 0; i < allFiles.count(); i++) {
    QFileInfo fi(projectDir->filePath(allFiles.at(i).toElement().text()));
    if (!fi.fileName().isEmpty()) {
      if (projectDir->exists(fi.fileName())) {
        QString buildtype = allFiles.at(i).toElement().attribute("type");
        QTreeWidgetItem *child = new QTreeWidgetItem(QStringList() << fi.fileName() << buildtype);
        child->setData(FILENAME_COLUMN, FULLPATH_ROLE, fi.filePath());
        child->setToolTip(FILENAME_COLUMN, fi.filePath());
        child->setIcon(FILENAME_COLUMN, ip.icon(fi));
        top->addChild(child);
      }
    }
  }
  top->setExpanded(true);
}

// rip through the fields and see if any have changed
// update appropriately if they have
void ProjectInfo::applyChanges( )
{
  if( diffProjects( mainWindow->currentProjectPath(), true ) )
    emit projectInfoUpdated();
  accept();
}

/*
  Determine whether the ProjectInfo for a new project is
  different than the existing project, optionally saving the
  values currently in the UI to the project file.
*/
bool ProjectInfo::diffProjects( const QString & newProjectPath, bool saveUiToFile )
{
  if(ui.versionEdit->text().isEmpty()) // check the version box as a sample...if this is empty, we don't have anything loaded so don't bother checking
    return false;
  bool changed = false;

  QFile file(projectFilePath(newProjectPath));
  if (file.open(QIODevice::ReadWrite|QFile::Text)) {
    QDomDocument projectFile;
    if(projectFile.setContent(&file)) {
      // to get at the actual text of an element, you need to grab its child, which will be a QDomText node
      if(ui.versionEdit->text() != projectFile.elementsByTagName("version").at(0).toElement().text()) {
        projectFile.elementsByTagName("version").at(0).firstChild().setNodeValue(ui.versionEdit->text());
        changed = true;
      }

      if(ui.optLevelBox->currentText() != projectFile.elementsByTagName("optlevel").at(0).toElement().text()) {
        projectFile.elementsByTagName("optlevel").at(0).firstChild().setNodeValue(ui.optLevelBox->currentText());
        changed = true;
      }

      bool state = (projectFile.elementsByTagName("debuginfo").at(0).toElement().text() == "true");
      if(ui.debugInfoCheckbox->checkState() != state) {
        QString debugstr = ui.debugInfoCheckbox->isChecked() ? "true" : "false";
        projectFile.elementsByTagName("debuginfo").at(0).firstChild().setNodeValue(debugstr);
        changed = true;
      }

      state = (projectFile.elementsByTagName("include_osc").at(0).toElement().text() == "true");
      if(ui.oscBox->isChecked() != state) {
        QString str = ui.oscBox->isChecked() ? "true" : "false";
        projectFile.elementsByTagName("include_osc").at(0).firstChild().setNodeValue(str);
        changed = true;
      }

      state = (projectFile.elementsByTagName("include_usb").at(0).toElement().text() == "true");
      if(ui.usbBox->isChecked() != state) {
        QString str = ui.usbBox->isChecked() ? "true" : "false";
        projectFile.elementsByTagName("include_usb").at(0).firstChild().setNodeValue(str);
        changed = true;
      }

      state = (projectFile.elementsByTagName("include_network").at(0).toElement().text() == "true");
      if(ui.networkBox->isChecked() != state) {
        QString str = ui.networkBox->isChecked() ? "true" : "false";
        projectFile.elementsByTagName("include_network").at(0).firstChild().setNodeValue(str);
        changed = true;
      }

      if(saveUiToFile) {
        file.resize(0); // clear out the current contents so we can update them, since we opened as read/write
        file.write(projectFile.toByteArray(2));
      }
    }
    file.close();
  }
  return changed;
}

/*
  Return the path of the project file
  for the current project.
*/
QString ProjectInfo::projectFilePath(const QString & projectPath)
{
  QDir projectDir(projectPath);
  return projectDir.filePath(projectDir.dirName() + ".xml");
}

void ProjectInfo::restoreDefaults()
{
  ui.versionEdit->setText(DEFAULT_VERSION);
  ui.optLevelBox->setCurrentIndex(ui.optLevelBox->findText(DEFAULT_OPTLEVEL));
  ui.debugInfoCheckbox->setChecked(DEFAULT_INCLUDE_DEBUG);
  ui.oscBox->setChecked(DEFAULT_INCLUDE_OSC);
  ui.usbBox->setChecked(DEFAULT_INCLUDE_USB);
  ui.networkBox->setChecked(DEFAULT_INCLUDE_NETWORK);
}

/*
  We've gotten a right click in the file browser.
  Pop up a menu that offers to remove the file that was clicked
  or change its build type.
*/
void FileBrowser::contextMenuEvent(QContextMenuEvent *event)
{
  QTreeWidgetItem *item = itemAt(event->pos());
  if (item) {
    if (item->childCount()) // files shouldn't have any children
      return;
    setCurrentItem(item); // make sure we have the right item selected
    QMenu menu(this);
    menu.addAction(actionRemoveFromProject);
    actionSetBuildType->setText("Change build type to thumb");
    if (item->text(BUILDTYPE_COLUMN) == "thumb")
      actionSetBuildType->setText("Change build type to arm");
    menu.addAction(actionSetBuildType);
    menu.exec(event->globalPos());
  }
}

/*
  The user has triggered the action to remove a file from the project.
  Grab the file name and signal ProjectInfo to make the change.
*/
void FileBrowser::onRemoveRequest()
{
  QString filepath = currentItem()->text(FILENAME_COLUMN);
  QTreeWidgetItem *top = topLevelItem(0);
  delete top->takeChild(top->indexOfChild(currentItem()));
  emit removeFileRequest(filepath);
}

/*
  The user has triggered the action to change a file's build type.
  Grab the file name and signal ProjectInfo to make the change in the project file.
*/
void FileBrowser::onSetBuildType()
{
  QTreeWidgetItem *item = currentItem();
  QString filepath = item->data(FILENAME_COLUMN, FULLPATH_ROLE).toString();
  QString newtype;
  if(item->text(BUILDTYPE_COLUMN) == "thumb")
    newtype = "arm";
  else if(item->text(BUILDTYPE_COLUMN) == "arm")
    newtype = "thumb";

  item->setText(BUILDTYPE_COLUMN, newtype);
  emit changeBuildType(filepath, newtype);
}

/*
  Remove the file in the current project's project file.
  The file has already been removed from the filebrowser UI.
*/
void ProjectInfo::onRemoveFileRequest(const QString & filename)
{
  QFile projectFile(projectFilePath(mainWindow->currentProjectPath()));
  QDir projectDir(mainWindow->currentProjectPath());
  if(projectManager.removeFromProjectFile(projectDir.path(), filename))
    mainWindow->removeFileFromProject(projectDir.filePath(filename));
}

/*
  Toggle the file's build type in the project file.
  The filebrowser UI has already been updated.
*/
void ProjectInfo::onChangeBuildType(const QString & filename, const QString & newtype)
{
  projectManager.setFileBuildType(mainWindow->currentProjectPath(), filename, newtype);
}

void ProjectInfo::setIncludeOsc(bool osc)
{
  ui.oscBox->setChecked(osc);
}

void ProjectInfo::setIncludeUsb(bool usb)
{
  ui.usbBox->setChecked(usb);
}

void ProjectInfo::setIncludeNetwork(bool network)
{
  ui.networkBox->setChecked(network);
}







