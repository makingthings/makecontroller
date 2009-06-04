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

#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>
#include <QTextBrowser>
#include "ui_about.h"

class About : public QDialog
{
  Q_OBJECT
public:
  About();
private:
  QTextBrowser body;
  Ui::AboutUi ui;
};

#endif // ABOUT_H


