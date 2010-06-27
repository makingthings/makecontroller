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

#ifndef PROJECT_INFO_H
#define PROJECT_INFO_H

#include <QDialog>
#include <QDomDocument>
#include <QDir>

#include <QTreeWidget>
#include "MainWindow.h"
#include "ProjectManager.h"

#ifdef MCBUILDER_TEST_SUITE
#include "TestBuilder.h"
#include "TestProjectInfo.h"
#endif

// subclassed so we have access to the context menu events
class FileBrowser : public QTreeWidget
{
  Q_OBJECT
public:
  FileBrowser(QWidget *parent = 0) : QTreeWidget(0)
  {
    setParent(parent);
    actionRemoveFromProject = new QAction(tr("Remove from project"), this);
    connect(actionRemoveFromProject, SIGNAL(triggered()), this, SLOT(onRemoveRequest()));
  }
  void contextMenuEvent(QContextMenuEvent *event);

private:
  QAction *actionRemoveFromProject;

private slots:
  void onRemoveRequest();

signals:
  void removeFileRequest(const QString & filename);
};

#include "ui_projectinfo.h"

class MainWindow;

class ProjectInfo : public QDialog
{
  Q_OBJECT
  public:
    ProjectInfo(MainWindow *mainWindow);
    QString version() { return ui.versionEdit->text(); }
    QString optLevel() { return ui.optLevelBox->currentText(); }
    bool debug() { return ui.debugInfoCheckbox->isChecked(); }
    bool includeOsc() { return ui.oscBox->isChecked(); }
    bool includeUsb() { return ui.usbBox->isChecked(); }
    bool includeNetwork() { return ui.networkBox->isChecked(); }
    bool load(const QString & projectPath);
    bool diffProjects( const QString & newProjectPath, bool saveUiToFile = false );

  signals:
    void projectInfoUpdated();

  private:
    MainWindow *mainWindow;
    Ui::ProjectInfoUi ui;
    ProjectManager projectManager;
    QString projectFilePath( const QString & projectPath );
    bool configChanged;
    void loadFileBrowser(QDir *projectDir, QDomDocument *projectFile);
    // mostly for testing...
    void setVersion(QString version) { ui.versionEdit->setText(version); }
    void setIncludeOsc(bool osc);
    void setIncludeUsb(bool usb);
    void setIncludeNetwork(bool network);

  private slots:
    void applyChanges( );
    void restoreDefaults( );
    void onRemoveFileRequest(const QString & filename);

  #ifdef MCBUILDER_TEST_SUITE
  friend class TestBuilder;
  friend class TestProjectInfo;
  #endif
};

#endif // PROJECT_INFO_H

