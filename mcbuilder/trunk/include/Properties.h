#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <QDialog>
#include <QDomDocument>
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