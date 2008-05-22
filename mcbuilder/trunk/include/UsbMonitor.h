

#ifndef USB_MONITOR_H
#define USB_MONITOR_H

#include <QDialog>
#include <QTimer>
#include "ui_usbmonitor.h"
#include "qextserialport.h"

class UsbMonitor : public QDialog, private Ui::UsbMonitorUi
{
	Q_OBJECT
  
	public:
		UsbMonitor( );
    
	public slots:
		bool loadAndShow();
    
	private slots:
		void onCommandLine( );
    void onView(QString view);
    void enumerate();
    void onOpenClose();
    void onFinished();
    void processNewData();
    void openDevice(QString name);
    
  private:
    QextSerialPort *port;
    QTimer enumerateTimer;
    QStringList ports;
    QStringList closedPorts;
    void closeDevice();
};

#endif // USB_MONITOR_H


