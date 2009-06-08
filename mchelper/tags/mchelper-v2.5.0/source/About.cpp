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

#include <QDate>
#include "About.h"

/*
 The dialog that pops up when "about mchelper" is selected from the menu.
*/
About::About( ) : QDialog( )
{
  ui.setupUi(this);
  ui.verticalLayout->addWidget(&body);
  body.setOpenExternalLinks(true);
  ui.versionLabel->setText(tr("<font size=6>mchelper</font><br>Version %1").arg(MCHELPER_VERSION));
  body.append(tr("By <a href=\"http://www.makingthings.com\">MakingThings</a>, %1.<br>").arg(QDate::currentDate().toString("yyyy")));
  body.append(tr("Thanks to Erik Gilling for <a href=\"http://oss.tekno.us/sam7utils\">sam7utils</a>.<br>"));
  body.append(trUtf8("French translation by Raphaël Doursenaud."));
  resize(ui.verticalLayout->sizeHint());
}

