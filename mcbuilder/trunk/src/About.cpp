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


#include "About.h"
#include <QDate>

/*
 The static dialog that pops up when "about mcbuilder" is selected from the menu.
*/
About::About( ) : QDialog( )
{
	setupUi(this);
  //textEdit->setOpenExternalLinks(true);
  versionLabel->setText(QString("<font size=4>mcbuilder</font><br>Version %1").arg(MCBUILDER_VERSION));
  textEdit->append(QString("mcbuilder by <a href=\"http://www.apache.org/licenses/LICENSE-2.0.html\">MakingThings</a>, %1<br>").arg(QDate::currentDate().toString("yyyy")));
  textEdit->append("Thanks to Erik Gilling for the use of <a href=\"http://www.apache.org/licenses/LICENSE-2.0.html\">sam7utils</a><br>");
  textEdit->append("Thanks to Michael Fischer for the use of <a href=\"http://www.apache.org/licenses/LICENSE-2.0.html\">Yagarto</a><br>");
  textEdit->append("Icons by Matt Ball, etc.\n");
}






