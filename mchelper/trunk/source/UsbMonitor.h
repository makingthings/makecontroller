/*********************************************************************************

 Copyright 2006-2009 MakingThings

 Licensed under the Apache License,
 Version 2.0 (the "License"); you may not use this file except in compliance
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

 *********************************************************************************/

#ifndef USB_MONITOR_H_
#define USB_MONITOR_H_

#include <QThread>

#include "BoardType.h"
#include "MainWindow.h"
#include "qextserialenumerator.h"

class MainWindow;

class UsbMonitor : public QThread
{
  Q_OBJECT
public:
  UsbMonitor(MainWindow* mw);
  void run();

signals:
  void newBoards(QStringList ports, BoardType::Type type);
  void boardsRemoved(QString key);

private slots:
  void onDeviceDiscovered(const QextPortInfo & info);
  void onDeviceTerminated(const QextPortInfo & info);

private:
  QStringList usbSerialList;
  QStringList usbSambaList;
  MainWindow* mainWindow;
  QextSerialEnumerator enumerator;
  bool isMakeController(const QextPortInfo & info);
  bool isSamBa(const QextPortInfo & info);
};

#endif // USB_MONITOR_H_





