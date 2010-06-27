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


#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>
#include "ui_preferences.h"
#include "MainWindow.h"

class MainWindow;

class Preferences : public QDialog
{
  Q_OBJECT
  public:
    Preferences(MainWindow *mainWindow);
    static QString workspace();
    static QString boardType();
    static QString toolsPath();
    static QString makePath();
    static QString sam7Path();
    QString ctrlBoardVersion() { return ui.mcVersionComboBox->currentText(); }
    QString appBoardVersion() { return ui.appVersionComboBox->currentText(); }

  private:
    MainWindow *mainWindow;
    Ui::Preferences ui;
    QFont currentFont, tempFont;

  public slots:
    void loadAndShow();
  private slots:
    void applyChanges();
    void browseWorkspace();
    void getNewFont();
    void onMakePathButton();
    void onArmElfPathButton();
    void onSam7Button();
};

#endif // PREFERENCES_H

