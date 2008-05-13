#ifndef USB_MONITOR_H
#define USB_MONITOR_H

#include <QDialog>
#include "ui_usbmonitor.h"

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
    void onPort(QString port);
};

#endif // USB_MONITOR_H


