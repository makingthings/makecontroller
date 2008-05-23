#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <QDialog>
#include <QDomDocument>
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


#include "MainWindow.h"
#include "ui_properties.h"

class MainWindow;

class Properties : public QDialog, private Ui::Properties
{
	Q_OBJECT
	public:
		Properties(MainWindow *mainWindow);
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
    
	public slots:
		bool loadAndShow();
    
	private:
		MainWindow *mainWindow;
		QString propFilePath( );
    bool load();
    bool configChanged;
    void setNetworkSectionEnabled(bool state);
    
	private slots:
		void applyChanges( );
    void restoreDefaults( );
    void onNetworkChanged(int state);
};

#endif // PROPERTIES_H