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


#ifndef FIND_REPLACE_H
#define FIND_REPLACE_H

#include <QDialog>
#include "ui_findreplace.h"
#include "MainWindow.h"

class FindReplace : public QDialog
{
  Q_OBJECT

  public:
    FindReplace(MainWindow *mw);
  private:
    MainWindow *mainWindow;
    Ui::FindReplaceUi ui;
    QTextDocument::FindFlags getFlags(bool forward = true);

  private slots:
    void onNext();
    void onPrevious();
    void onReplace();
    void onReplaceAll();
};

#endif // FIND_REPLACE_H


