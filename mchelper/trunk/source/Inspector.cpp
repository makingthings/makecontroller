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

#include "Inspector.h"
#include <QSettings>

#define INSPECTOR_GEOM  "inspector/geometry"

/*
 A dialog for getting/setting general info about a Make Controller
*/
Inspector::Inspector(MainWindow *mainWindow) : QDialog(0)
{
  this->mainWindow = mainWindow;
  setupUi(this);
  connect(this, SIGNAL(finished(int)), this, SLOT(onFinished()));
  connect(&infoTimer, SIGNAL(timeout()), this, SLOT(getBoardInfo()));
  connect(applyButton, SIGNAL(clicked()), this, SLOT(onApply()));
  connect(revertButton, SIGNAL(clicked()), this, SLOT(onRevert()));

  connect(nameEdit, SIGNAL(textEdited(QString)), this, SLOT(onAnyValueEdited()));
  connect(serialEdit, SIGNAL(textEdited(QString)), this, SLOT(onAnyValueEdited()));
  connect(versionEdit, SIGNAL(textEdited(QString)), this, SLOT(onAnyValueEdited()));
  connect(freememEdit, SIGNAL(textEdited(QString)), this, SLOT(onAnyValueEdited()));
  connect(ipEdit, SIGNAL(textEdited(QString)), this, SLOT(onAnyValueEdited()));
  connect(netmaskEdit, SIGNAL(textEdited(QString)), this, SLOT(onAnyValueEdited()));
  connect(gatewayEdit, SIGNAL(textEdited(QString)), this, SLOT(onAnyValueEdited()));
  connect(listenPortEdit, SIGNAL(textEdited(QString)), this, SLOT(onAnyValueEdited()));
  connect(sendPortEdit, SIGNAL(textEdited(QString)), this, SLOT(onAnyValueEdited()));
  connect(dhcpBox, SIGNAL(clicked(bool)), this, SLOT(onAnyValueEdited()));

  QSettings settings;
  restoreGeometry(settings.value(INSPECTOR_GEOM).toByteArray());
  resize(gridLayout->sizeHint());
}

void Inspector::loadAndShow()
{
  getBoardInfo();
  infoTimer.start(1000);
  show();
}

void Inspector::closeEvent(QCloseEvent *e)
{
  QSettings settings;
  settings.setValue(INSPECTOR_GEOM, saveGeometry());
  e->accept();
}

/*
  Populate the line edits with the board's info.
  This should only be called when there's new info for the board,
  or when the revert button has been pressed.
*/
void Inspector::setData(Board* board)
{
  nameEdit->setText(board->name);
  serialEdit->setText(board->serialNumber);
  versionEdit->setText(board->firmwareVersion);
  freememEdit->setText(board->freeMemory);
  ipEdit->setText(board->ip_address);
  netmaskEdit->setText(board->netMask);
  gatewayEdit->setText(board->gateway);
  listenPortEdit->setText(board->udp_listen_port);
  sendPortEdit->setText(board->udp_send_port);
  dhcpBox->setChecked(board->dhcp);
}

/*
  Clear out the inspector.
*/
void Inspector::clear()
{
  nameEdit->text().clear();
  serialEdit->text().clear();
  versionEdit->text().clear();
  freememEdit->text().clear();
  ipEdit->text().clear();
  netmaskEdit->text().clear();
  gatewayEdit->text().clear();
  listenPortEdit->text().clear();
  sendPortEdit->text().clear();
  dhcpBox->setChecked(false);
}

/*
 The dialog has been closed.
 Stop the timer asking for board info
*/
void Inspector::onFinished()
{
  infoTimer.stop();
}

/*
 The dialog is visible and the timer has timed out.
 Send a new request to the board for its info.
*/
void Inspector::getBoardInfo()
{
  Board *board = mainWindow->getCurrentBoard();
  if (board)
    board->sendMessage("/system/info-internal");
}

/*
  Called when the apply button has been pressed.
  zip through the list of fields in the Summary tab and if any have changed, send
  the appropriate message to the board and notify the user of what has been changed in the output window
*/
void Inspector::onApply()
{
  Board* board = mainWindow->getCurrentBoard();
  if (board == NULL)
    return;

  QStringList msgs;

  QString newName = nameEdit->text();
  if (!newName.isEmpty() && board->name != newName) {
    msgs << QString("/system/name \"%1\"").arg(newName);
    mainWindow->setBoardName(board->key(), QString("%1 : %2").arg(newName).arg(board->key()));
  }

  // serial number
  QString newNumber = serialEdit->text();
  if (!newNumber.isEmpty() && board->serialNumber != newNumber)
    msgs << QString("/system/serialnumber %1").arg(newNumber);

  // IP address
  QString newAddress = ipEdit->text();
  if (!newAddress.isEmpty() && board->ip_address != newAddress)
    msgs << QString("/network/address %1").arg(newAddress);

  // dhcp
  if (dhcpBox->isChecked() && !board->dhcp)
    msgs << "/network/dhcp 1";
  else if (dhcpBox->isChecked() && board->dhcp)
    msgs << "/network/dhcp 0";

  // udp listen port
  QString newPort = listenPortEdit->text();
  if (!newPort.isEmpty() && board->udp_listen_port != newPort)
    msgs << QString("/network/osc_udp_listen_port %1").arg(newPort);

  // udp send port
  newPort = sendPortEdit->text();
  if (!newPort.isEmpty() && board->udp_send_port != newPort)
    msgs << QString("/network/osc_udp_send_port %1").arg(newPort);

  setLabelsRole(QPalette::WindowText);
  if (!msgs.isEmpty()) {
    board->sendMessage(msgs);
    mainWindow->updateBoardInfo(board);
    mainWindow->message(msgs, MsgType::Command, board->location());
  }
}

/*
  Called when the revert button has been pressed.
  Set the board's info back to its actual values.
*/
void Inspector::onRevert()
{
  setLabelsRole(QPalette::WindowText);
  Board *brd = mainWindow->getCurrentBoard();
  if (brd)
    setData(brd);
}

void Inspector::onAnyValueEdited()
{
  setLabelsRole(QPalette::Mid);
}

void Inspector::setLabelsRole(QPalette::ColorRole role)
{
  nameLabel->setForegroundRole(role);
  sernumLabel->setForegroundRole(role);
  versionLabel->setForegroundRole(role);
  freememLabel->setForegroundRole(role);
  ipLabel->setForegroundRole(role);
  netmaskLabel->setForegroundRole(role);
  gatewayLabel->setForegroundRole(role);
  listenPortLabel->setForegroundRole(role);
  sendPortLabel->setForegroundRole(role);
  dhcpBox->setForegroundRole(role); // how to actually get at the text?
}
