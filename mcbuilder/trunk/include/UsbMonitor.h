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


