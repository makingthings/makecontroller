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


#include "UsbMonitor.h"
#include "qextserialenumerator.h"
#include <QLineEdit>

#define ENUM_FREQUENCY 1000 // check once a second for new USB connections

UsbMonitor::UsbMonitor( ) : QDialog( )
{
	setupUi(this);
  connect( sendButton, SIGNAL(clicked()), this, SLOT(onCommandLine()));
  connect( commandLine->lineEdit(), SIGNAL(returnPressed()), this, SLOT(onCommandLine()));
  connect( openCloseButton, SIGNAL(clicked()), this, SLOT(onOpenClose()));
  connect( viewList, SIGNAL(activated(QString)), this, SLOT(onView(QString)));
  connect( portList, SIGNAL(activated(QString)), this, SLOT(openDevice(QString)));
  connect( &enumerateTimer, SIGNAL(timeout()), this, SLOT(enumerate()));
  connect(this, SIGNAL(finished(int)), this, SLOT(onFinished()));
  
  port = new QextSerialPort("", QextSerialPort::EventDriven);
  connect(port, SIGNAL(readyRead()), this, SLOT(processNewData()));
}

/* 
 Scan the available ports & populate the list.
 If one of the ports is the one that was last open, open it up.
 Otherwise, wait for the user to select one then open that one.
*/
bool UsbMonitor::loadAndShow( )
{
  if(!portList->currentText().isEmpty())
    openDevice(portList->currentText());
  enumerate();
  enumerateTimer.start(ENUM_FREQUENCY);
  this->show();
	return true;
}

/*
 Send the contents of the commandLine to the serial port,
 and add them to the output console
*/
void UsbMonitor::onCommandLine( )
{
  if(port->isOpen() && !commandLine->currentText().isEmpty())
  {
    port->write(commandLine->currentText().toUtf8());
    commandLine->clear();
  }
}

/*
 If the view has changed, update the contents of the 
 output console accordingly.
*/
void UsbMonitor::onView(QString view)
{
  (void)view;
}

/*
  Called periodically while the dialog is open to check for new Make Controller USB devices.
  If we find a new one, pop it into the UI and save its name.
  If one has gone away, remove it from the UI.
*/
void UsbMonitor::enumerate()
{
  QextSerialEnumerator enumerator;
  QList<QextPortInfo> portInfos = enumerator.getPorts();
  QStringList foundPorts;
  // check for new ports...
  foreach(QextPortInfo portInfo, portInfos)
  {
    if(portInfo.friendName.startsWith("Make Controller Ki")
        && !ports.contains(portInfo.portName) 
        && !closedPorts.contains(portInfo.portName)) // found a new port
    {
      openDevice(portInfo.portName);
    }
    foundPorts << portInfo.portName;
  }
  
  // now check for ports that have gone away
  foreach(QString portname, ports)
  {
    if(!foundPorts.contains(portname))
    {
      closedPorts.removeAll(port->portName());
      ports.removeAll(port->portName());
      portList->removeItem(portList->findText(port->portName()));
      update();
      closeDevice();
    }
  }
}

/*
  Open the USB port with the given name
  Update the UI accordingly.
*/
void UsbMonitor::openDevice(QString name)
{
  if(port->isOpen())
    port->close();
  port->setPortName(name);
  if(port->open(QIODevice::ReadWrite))
  {
    if(!ports.contains(name))
      ports.append(name);
    if(portList->findText(name) < 0)
      portList->addItem(name);
    // the port might already be in the list, but we always want to set its icon appropriately
    portList->setItemIcon( portList->findText(name), QIcon(":/icons/green_dot.png"));
    openCloseButton->setText("Close");
  }
}

/*
  Close the USB port.
  Update the UI accordingly.
*/
void UsbMonitor::closeDevice()
{
  if(port->isOpen())
  {
    port->close();
    openCloseButton->setText("Open");
  }
}

/*
  The open/close button has been clicked.
  If the port is currently closed, try to open it and set our state
  to close it the next time it's clicked, and vice versa.
*/
void UsbMonitor::onOpenClose()
{
  if(port->isOpen())
  {
    // put this port name on the list of ports manually closed by the user
    // so it doesn't get added into the UI multiple times when it's subsequently opened
    closedPorts.append(port->portName());
    portList->setItemIcon( portList->currentIndex(), QIcon(":/icons/red_dot.png"));
    closeDevice();
  }
  else
  {
    openDevice(portList->currentText());
    portList->setItemIcon( portList->currentIndex(), QIcon(":/icons/green_dot.png"));
  }
}

/*
  The dialog has been closed.
  Close the USB connection if it's open, and stop the enumerator.
*/
void UsbMonitor::onFinished()
{
  enumerateTimer.stop();
  closeDevice();
  outputConsole->clear();
}

/*
  New data is available at the USB port.
  Read it and stuff it into the UI.
*/
void UsbMonitor::processNewData()
{
  QByteArray newData;
  if(!port->isOpen())
    return;
  int avail = port->bytesAvailable();
  if(avail > 0 )
  {
    newData.resize(avail);
    if(port->read(newData.data(), newData.size()) < 0)
      return;
    else
    {
      outputConsole->moveCursor(QTextCursor::End);
      outputConsole->insertPlainText(newData);
    }
  }
}





