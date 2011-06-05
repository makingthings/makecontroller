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

#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <QDialog>
#include "ui_inspector.h"
#include "MainWindow.h"
#include "Board.h"

class MainWindow;
class Board;

class Inspector : public QDialog, private Ui::InspectorUi
{
  Q_OBJECT
public:
  Inspector(MainWindow *mainWindow);
  void setData(Board* board);
  void clear( );

public slots:
  void loadAndShow();

private slots:
  void onFinished();
  void getBoardInfo();
  void onApply();
  void onRevert();
  void onAnyValueEdited();

private:
  MainWindow *mainWindow;
  QTimer infoTimer;
  void setLabelsRole(QPalette::ColorRole role);
  void closeEvent(QCloseEvent *e);
};

#endif // INSPECTOR_H


