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



#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include "MainWindow.h"

int main( int argc, char *argv[] )
{
  QApplication app(argc, argv);
  
  QString locale = QLocale::system().name();

  QTranslator qtTranslator;
  QTranslator mcbuilderTranslator;

  qtTranslator.load(QString("qt_") + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
  app.installTranslator(&qtTranslator);

  mcbuilderTranslator.load(QString("mcbuilder_") + locale);
  app.installTranslator(&mcbuilderTranslator);

  MainWindow window;
  window.show();
  return app.exec();
}

