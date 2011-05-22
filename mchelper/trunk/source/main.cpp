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

#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include "MainWindow.h"

int main( int argc, char *argv[] )
{
  QApplication app(argc, argv);

  QCoreApplication::setOrganizationName("MakingThings");
  QCoreApplication::setOrganizationDomain("makingthings.com");
  QCoreApplication::setApplicationName("mchelper");

  QString locale = QLocale::system().name();

  QTranslator qtTranslator;
  QTranslator mchelperTranslator;

  qtTranslator.load(QString("qt_") + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
  app.installTranslator(&qtTranslator);

  mchelperTranslator.load(QString("mchelper_") + locale);
  app.installTranslator(&mchelperTranslator);

  bool no_ui = app.arguments().contains("-no_ui");
  MainWindow window(no_ui);
  if (!no_ui)
    window.show();
  return app.exec();
}


