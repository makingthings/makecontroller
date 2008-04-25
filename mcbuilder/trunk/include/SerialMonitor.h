#ifndef SERIAL_MONITOR_H
#define SERIAL_MONITOR_H

#include <QDialog>
#include "ui_serialmonitor.h"

class SerialMonitor : public QDialog, private Ui::SerialMonitorUi
{
	Q_OBJECT
  
	public:
		SerialMonitor( );
	public slots:
		bool loadAndShow();
	private slots:
		void onCommandLine( );
    void onView(QString view);
    void onPort(QString port);
};

#endif // SERIAL_MONITOR_H


