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
#endif

// subclassed so we have access to the context menu events
class FileBrowser : public QTreeWidget
{
  Q_OBJECT
public: 
  FileBrowser(QWidget *parent = 0) : QTreeWidget(0)
  {
    setParent(parent);
    actionRemoveFromProject = new QAction("Remove from project...", this);
    actionSetBuildType = new QAction("Change build type to thumb", this);
    connect(actionRemoveFromProject, SIGNAL(triggered()), this, SLOT(onRemoveRequest()));
    connect(actionSetBuildType, SIGNAL(triggered()), this, SLOT(onSetBuildType()));
  }
  void contextMenuEvent(QContextMenuEvent *event);

private:
  QAction *actionRemoveFromProject;
  QAction *actionSetBuildType;
  
private slots:
  void onRemoveRequest();
  void onSetBuildType();
  
signals:
  void removeFileRequest(QString filename);
  void changeBuildType(QString filename, QString newtype);
};

#include "ui_projectinfo.h"

class MainWindow;

class ProjectInfo : public QDialog, private Ui::ProjectInfoUi
{
	Q_OBJECT
	public:
		ProjectInfo(MainWindow *mainWindow);
    QString version() { return versionEdit->text(); }
    QString optLevel() { return optLevelBox->currentText(); }
    bool debug() { return (debugInfoCheckbox->checkState() == Qt::Checked); }
    int heapsize() { return heapSizeEdit->text().toInt(); }
    bool includeOsc() { return (oscBox->checkState() == Qt::Checked); }
    bool includeUsb() { return (usbBox->checkState() == Qt::Checked); }
    bool includeNetwork() { return (networkBox->checkState() == Qt::Checked); }
    int networkMempool() { return networkMempoolEdit->text().toInt(); }
    int udpSockets() { return udpSocketEdit->text().toInt(); }
    int tcpSockets() { return tcpSocketEdit->text().toInt(); }
    int tcpServers() { return tcpServerEdit->text().toInt(); }
    bool load();
    
	public slots:
		bool loadAndShow();
  
  signals:
    void projectInfoUpdated();
    
	private:
		MainWindow *mainWindow;
    ProjectManager projectManager;
		QString projectFilePath( );
    bool configChanged;
    void setNetworkSectionEnabled(bool state);
    void loadFileBrowser(QDir *projectDir, QDomDocument *projectFile);
    // mostly for testing...
    void setVersion(QString version) { versionEdit->setText(version); }
    void setHeapSize(int heap) { heapSizeEdit->setText(QString::number(heap)); }
    void setMempool(int mempool) { networkMempoolEdit->setText(QString::number(mempool)); }
    void setUdp(int udp) { udpSocketEdit->setText(QString::number(udp)); }
    void setTcp(int tcp) { tcpSocketEdit->setText(QString::number(tcp)); }
    void setTcpListen(int tcplisten) { tcpServerEdit->setText(QString::number(tcplisten)); }
    void setIncludeOsc(bool osc);
    void setIncludeUsb(bool usb);
    void setIncludeNetwork(bool network);
    
	private slots:
		void applyChanges( );
    void restoreDefaults( );
    void onNetworkChanged(int state);
    void onRemoveFileRequest(QString filename);
    void onChangeBuildType(QString filename, QString newtype);
  
  #ifdef MCBUILDER_TEST_SUITE
  friend class TestBuilder;
  #endif
};

#endif // PROJECT_INFO_H

