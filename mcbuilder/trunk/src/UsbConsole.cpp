/*********************************************************************************

 Copyright 2008-2009 MakingThings

 Licensed under the Apache License,
 Version 2.0 (the "License"); you may not use this file except in compliance
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/


#include "UsbConsole.h"
#include <QLineEdit>
#include <QTextBlock>
#include <QBuffer>

#define ENUM_FREQUENCY 1000 // check once a second for new USB connections

UsbConsole::UsbConsole( ) : QDialog( )
{
  setupUi(this);
  connect( sendButton, SIGNAL(clicked()), this, SLOT(onCommandLine()));
  connect( commandLine->lineEdit(), SIGNAL(returnPressed()), this, SLOT(onCommandLine()));
  connect( openCloseButton, SIGNAL(clicked()), this, SLOT(onOpenClose()));
  connect( viewList, SIGNAL(activated(QString)), this, SLOT(onView(QString)));
  connect( portList, SIGNAL(activated(QString)), this, SLOT(openDevice(QString)));
  connect( &enumerateTimer, SIGNAL(timeout()), this, SLOT(enumerate()));
  connect(this, SIGNAL(finished(int)), this, SLOT(onFinished()));
  currentView = viewList->currentText();

  port = new QextSerialPort("", QextSerialPort::EventDriven);
  connect(port, SIGNAL(readyRead()), this, SLOT(processNewData()));
}

/*
 Scan the available ports & populate the list.
 If one of the ports is the one that was last open, open it up.
 Otherwise, wait for the user to select one then open that one.
*/
bool UsbConsole::loadAndShow( )
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
void UsbConsole::onCommandLine( )
{
  if(port->isOpen() && !commandLine->currentText().isEmpty())
  {
    if(port->write(commandLine->currentText().toUtf8()) < 0)
      closeDevice();
    else
    {
      QTextBlockFormat format;
      format.setBackground(QColor(229, 237, 247, 255)); // light blue
      if(currentView == "Characters")
        outputConsole->appendPlainText(commandLine->currentText()); // insert the message
      else if(currentView == "Hex")
        outputConsole->appendPlainText(strToHex(commandLine->currentText()));
      outputConsole->moveCursor(QTextCursor::End); // move the cursor to the end
      outputConsole->textCursor().setBlockFormat(format);
      outputConsole->insertPlainText("\n");
      format.setBackground(Qt::white); // reset the format to white for any subsequent messages received
      outputConsole->textCursor().setBlockFormat(format);
    }
    commandLine->clear();
  }
}

/*
 If the view has changed, update the contents of the
 output console accordingly.
*/
void UsbConsole::onView(QString view)
{
  if(view == currentView) // we haven't changed
    return;
  currentView = view;
  if(!outputConsole->blockCount()) // the console is empty
    return;

  QTextCursor c = outputConsole->textCursor();
  c.movePosition(QTextCursor::Start);
  bool keepgoing = true;
  while(keepgoing)
  {
    if(view == "Characters")
      hexToChar(&c);
    else if(view == "Hex")
      charToHex(&c);
    keepgoing = c.movePosition(QTextCursor::NextBlock);
  }
}

/*
  Change the text in a given block from hex to characters.
*/
void UsbConsole::hexToChar(QTextCursor *c)
{
  QStringList hexes = c->block().text().split(" ");
  QString chars;
  foreach(QString hex, hexes)
  {
    if(!hex.isEmpty())
      chars += QChar::fromAscii(hex.toInt(0, 0));
  }
  while(!c->atBlockEnd())
    c->deleteChar();
  c->insertText(chars);
}

/*
  Change the text in a given block to hex.
*/
void UsbConsole::charToHex(QTextCursor *c)
{
  QString str = c->block().text();
  QString hexes;
  int len = str.length();
  for(int i = 0; i < len; i++)
  {
    c->deleteChar();
    hexes += strToHex(str.left(1));
    str.remove(0,1);
  }
  while(!c->atBlockEnd()) // remove
    c->deleteChar();
  c->insertText(hexes);
}

QString UsbConsole::strToHex(QString str)
{
  QString hex;
  int len = str.size();
  for(int i = 0; i < len; i++)
  {
    hex += QString("0x%1 ").arg(QString::number(*(str.toUtf8().data()), 16));
    str.remove(0,1);
  }
  return hex;
}

/*
  Called periodically while the dialog is open to check for new Make Controller USB devices.
  If we find a new one, pop it into the UI and save its name.
  If one has gone away, remove it from the UI.
*/
void UsbConsole::enumerate()
{
  QList<QextPortInfo> portInfos = enumerator.getPorts();
  QStringList foundPorts;
  // check for new ports...
  foreach(QextPortInfo portInfo, portInfos)
  {
    QString asciiname = port->portName().toAscii();
    //qDebug("enumerated: %s", qPrintable(portInfo.friendName));
  if(portInfo.friendName.startsWith("Make Controller Ki")
        && !ports.contains(asciiname)
        && !closedPorts.contains(asciiname)) // found a new port
    {
      openDevice(portInfo.portName);
    }
    foundPorts << portInfo.portName.toAscii();
  }

  // now check for ports that have gone away
  foreach(QString portname, ports)
  {
    if(!foundPorts.contains(portname))
    {
      QString asciiname = port->portName().toAscii();
      closedPorts.removeAll(asciiname);
      ports.removeAll(asciiname);
      int idx = portList->findText(asciiname);
      if(idx > -1)
        portList->removeItem(idx);
      update();
      closeDevice();
    }
  }
}

/*
  Open the USB port with the given name
  Update the UI accordingly.
*/
void UsbConsole::openDevice(QString name)
{
  if(port->isOpen())
    port->close();
  port->setPortName(name);
  if(port->open(QIODevice::ReadWrite))
  {
  qDebug("UsbConsole: opened %s", qPrintable(name));
  QString asciiname = name.toAscii();
    ports.append(asciiname);
    if(portList->findText(asciiname) < 0)
      portList->addItem(asciiname);
    // the port might already be in the list, but we always want to set its icon appropriately
    if(portList->count())
      portList->setItemIcon( portList->currentIndex(), QIcon(":/icons/green_dot.png"));
    openCloseButton->setText("Close");
  }
}

/*
  Close the USB port.
  Update the UI accordingly.
*/
void UsbConsole::closeDevice()
{
  if(port->isOpen())
  {
    port->close();
    if(portList->count())
      portList->setItemIcon( portList->currentIndex(), QIcon(":/icons/red_dot.png"));
    openCloseButton->setText("Open");
  }
}

/*
  The open/close button has been clicked.
  If the port is currently closed, try to open it and set our state
  to close it the next time it's clicked, and vice versa.
*/
void UsbConsole::onOpenClose()
{
  if(port->isOpen())
  {
    // put this port name on the list of ports manually closed by the user
    // so it doesn't get added into the UI multiple times when it's subsequently opened
    closedPorts.append(port->portName().toAscii());
    portList->setItemIcon( portList->currentIndex(), QIcon(":/icons/red_dot.png"));
    closeDevice();
  }
  else
  {
    if(!portList->currentText().isEmpty())
    {
      openDevice(portList->currentText());
      portList->setItemIcon( portList->currentIndex(), QIcon(":/icons/green_dot.png"));
    }
  }
}

/*
  The dialog has been closed.
  Close the USB connection if it's open, and stop the enumerator.
*/
void UsbConsole::onFinished()
{
  enumerateTimer.stop();
  closeDevice();
  outputConsole->clear();
}

/*
  New data is available at the USB port.
  Read it and stuff it into the UI.
*/
void UsbConsole::processNewData()
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
      if(currentView == "Characters") // just pop it in there
        outputConsole->insertPlainText(newData);
      else if(currentView == "Hex")
        outputConsole->insertPlainText(strToHex(newData));
    }
  }
}





