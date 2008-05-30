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


#include "ProjectInfo.h"
#include <QDir>

#define DEFAULT_VERSION "0.1.0"
#define DEFAULT_HEAPSIZE 18000
#define DEFAULT_OPTLEVEL "Optimize For Size (-Os)"
#define DEFAULT_INCLUDE_DEBUG Qt::Unchecked
#define DEFAULT_INCLUDE_OSC Qt::Unchecked
#define DEFAULT_INCLUDE_USB Qt::Unchecked
#define DEFAULT_INCLUDE_NETWORK Qt::Unchecked
#define DEFAULT_NETWORK_MEMPOOL 2000
#define DEFAULT_UDP_SOCKETS 4
#define DEFAULT_TCP_SOCKETS 4
#define DEFAULT_TCP_SERVERS 2

ProjectInfo::ProjectInfo(MainWindow *mainWindow) : QDialog( 0 )
{
	this->mainWindow = mainWindow;
	setupUi(this);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(applyChanges()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(accept()));
  connect(defaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
  connect(networkBox, SIGNAL(stateChanged(int)), this, SLOT(onNetworkChanged(int)));
  load( ); // initialize
}

/*
	Read the project ProjectInfo from the project profile, load them into
	the ProjectInfo dialog and show it.
*/
bool ProjectInfo::loadAndShow( )
{
	load();
	show();
	return true;
}

/*
  Read the project's ProjectInfo from the project file
  and load them into the UI.
*/  
bool ProjectInfo::load()
{
  QDir projectDir(mainWindow->currentProjectPath());
  if(!projectDir.exists())
    return false;
	QString projectName = projectDir.dirName();
	setWindowTitle(projectName + " - Project Info");
	
	// read the ProjectInfo file
	QFile file(projectFilePath());
	if(file.open(QIODevice::ReadOnly|QFile::Text))
	{
		QDomDocument projectFile;
    if(projectFile.setContent(&file))
		{
			versionEdit->setText(projectFile.elementsByTagName("version").at(0).toElement().text());
			heapSizeEdit->setText(projectFile.elementsByTagName("heapsize").at(0).toElement().text());
			QString optlevel = projectFile.elementsByTagName("optlevel").at(0).toElement().text();
			optLevelBox->setCurrentIndex(optLevelBox->findText(optlevel));
			Qt::CheckState state = (projectFile.elementsByTagName("debuginfo").at(0).toElement().text() == "true") ? Qt::Checked : Qt::Unchecked;
			debugInfoCheckbox->setCheckState(state);
      
      state = (projectFile.elementsByTagName("include_osc").at(0).toElement().text() == "true") ? Qt::Checked : Qt::Unchecked;
			oscBox->setCheckState(state);
      
      state = (projectFile.elementsByTagName("include_usb").at(0).toElement().text() == "true") ? Qt::Checked : Qt::Unchecked;
			usbBox->setCheckState(state);
      
      state = (projectFile.elementsByTagName("include_network").at(0).toElement().text() == "true") ? Qt::Checked : Qt::Unchecked;
			networkBox->setCheckState(state);
      setNetworkSectionEnabled(state == Qt::Checked);
      
      networkMempoolEdit->setText(projectFile.elementsByTagName("network_mempool").at(0).toElement().text());
      udpSocketEdit->setText(projectFile.elementsByTagName("network_udp_sockets").at(0).toElement().text());
      tcpSocketEdit->setText(projectFile.elementsByTagName("network_tcp_sockets").at(0).toElement().text());
      tcpServerEdit->setText(projectFile.elementsByTagName("network_tcp_servers").at(0).toElement().text());
      
      loadFileBrowser( &projectDir, &projectFile);
		}
		file.close();
	}
	else
		return false;
  return true;
}

void ProjectInfo::loadFileBrowser(QDir *projectDir, QDomDocument *projectFile)
{
  // setup the file browser
  fileBrowser->clear(); // get rid of anything previously in there
  QDomNodeList allFiles = projectFile->elementsByTagName("files").at(0).childNodes();
  QTreeWidgetItem *top = new QTreeWidgetItem(QStringList() << projectDir->dirName());
  top->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirIcon));
  fileBrowser->addTopLevelItem(top);
  
  // only deals with files in the top level directory at the moment
  for(int i = 0; i < allFiles.count(); i++)
  {
    QFileInfo fi(allFiles.at(i).toElement().text());
    if(!fi.fileName().isEmpty())
    {
      if(projectDir->exists(fi.fileName()))
      {
        QString buildtype = allFiles.at(i).toElement().attribute("type");
        QTreeWidgetItem *child = new QTreeWidgetItem(QStringList() << fi.fileName() << buildtype);
        child->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_FileIcon));
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
	QFile file(projectFilePath());
	if(file.open(QIODevice::ReadWrite|QFile::Text))
	{
		QDomDocument projectFile;
    if(projectFile.setContent(&file))
    {
      // to get at the actual text of an element, you need to grab its child,
      // which will be a QDomText node
      if(versionEdit->text() != projectFile.elementsByTagName("version").at(0).toElement().text())
        projectFile.elementsByTagName("version").at(0).firstChild().setNodeValue(versionEdit->text());
        
      if(heapSizeEdit->text() != projectFile.elementsByTagName("heapsize").at(0).toElement().text())
        projectFile.elementsByTagName("heapsize").at(0).firstChild().setNodeValue(heapSizeEdit->text());
        
      if(optLevelBox->currentText() != projectFile.elementsByTagName("optlevel").at(0).toElement().text())
        projectFile.elementsByTagName("optlevel").at(0).firstChild().setNodeValue(optLevelBox->currentText());
      
      Qt::CheckState state = (projectFile.elementsByTagName("debuginfo").at(0).toElement().text() == "true") ? Qt::Checked : Qt::Unchecked;
      if(debugInfoCheckbox->checkState() != state)
      {
        QString debugstr = (debugInfoCheckbox->checkState() == Qt::Checked) ? "true" : "false";
        projectFile.elementsByTagName("debuginfo").at(0).firstChild().setNodeValue(debugstr);
      }
      
      state = (projectFile.elementsByTagName("include_osc").at(0).toElement().text() == "true") ? Qt::Checked : Qt::Unchecked;
      if(oscBox->checkState() != state)
      {
        QString str = (oscBox->checkState() == Qt::Checked) ? "true" : "false";
        projectFile.elementsByTagName("include_osc").at(0).firstChild().setNodeValue(str);
      }
      
      state = (projectFile.elementsByTagName("include_usb").at(0).toElement().text() == "true") ? Qt::Checked : Qt::Unchecked;
      if(usbBox->checkState() != state)
      {
        QString str = (usbBox->checkState() == Qt::Checked) ? "true" : "false";
        projectFile.elementsByTagName("include_usb").at(0).firstChild().setNodeValue(str);
      }
      
      state = (projectFile.elementsByTagName("include_network").at(0).toElement().text() == "true") ? Qt::Checked : Qt::Unchecked;
      if(networkBox->checkState() != state)
      {
        QString str = (networkBox->checkState() == Qt::Checked) ? "true" : "false";
        projectFile.elementsByTagName("include_network").at(0).firstChild().setNodeValue(str);
      }
      
      if(networkMempoolEdit->text() != projectFile.elementsByTagName("network_mempool").at(0).toElement().text())
        projectFile.elementsByTagName("network_mempool").at(0).firstChild().setNodeValue(networkMempoolEdit->text());
        
      if(udpSocketEdit->text() != projectFile.elementsByTagName("network_udp_sockets").at(0).toElement().text())
        projectFile.elementsByTagName("network_udp_sockets").at(0).firstChild().setNodeValue(udpSocketEdit->text());
      
      if(tcpSocketEdit->text() != projectFile.elementsByTagName("network_tcp_sockets").at(0).toElement().text())
        projectFile.elementsByTagName("network_tcp_sockets").at(0).firstChild().setNodeValue(tcpSocketEdit->text());
      
      if(tcpServerEdit->text() != projectFile.elementsByTagName("network_tcp_servers").at(0).toElement().text())
        projectFile.elementsByTagName("network_tcp_servers").at(0).firstChild().setNodeValue(tcpServerEdit->text());
      
      file.resize(0); // clear out the current contents so we can update them, since we opened as read/write
      file.write(projectFile.toByteArray(2));
    }
		file.close();
	}
  accept();
}

/*
  Return the path of the project file
  for the current project.
*/
QString ProjectInfo::projectFilePath( )
{
	QDir projectDir(mainWindow->currentProjectPath());
	QString projectName = projectDir.dirName();
	// filename should not have spaces
	return projectDir.filePath(projectName.remove(" ") + ".xml"); 
}

void ProjectInfo::restoreDefaults( )
{
  versionEdit->setText(DEFAULT_VERSION);
  heapSizeEdit->setText(QString::number(DEFAULT_HEAPSIZE));
  optLevelBox->setCurrentIndex(optLevelBox->findText(DEFAULT_OPTLEVEL));
  debugInfoCheckbox->setCheckState(DEFAULT_INCLUDE_DEBUG);
  oscBox->setCheckState(DEFAULT_INCLUDE_OSC);
  usbBox->setCheckState(DEFAULT_INCLUDE_USB);
  networkBox->setCheckState(DEFAULT_INCLUDE_NETWORK);
  networkMempoolEdit->setText(QString::number(DEFAULT_NETWORK_MEMPOOL));
  udpSocketEdit->setText(QString::number(DEFAULT_UDP_SOCKETS));
  tcpSocketEdit->setText(QString::number(DEFAULT_TCP_SOCKETS));
  tcpServerEdit->setText(QString::number(DEFAULT_TCP_SERVERS));
}

void ProjectInfo::onNetworkChanged(int state)
{
  setNetworkSectionEnabled(state == Qt::Checked);
}

void ProjectInfo::setNetworkSectionEnabled(bool state)
{
  networkMempoolEdit->setEnabled(state);
  networkMempoolLabel->setEnabled(state);
  udpSocketEdit->setEnabled(state);
  udpSocketLabel->setEnabled(state);
  tcpSocketEdit->setEnabled(state);
  tcpSocketLabel->setEnabled(state);
  tcpServerEdit->setEnabled(state);
  tcpServerLabel->setEnabled(state);
}

void FileBrowser::contextMenuEvent(QContextMenuEvent *event)
{
  qDebug("got context event");
  (void)event;
}





