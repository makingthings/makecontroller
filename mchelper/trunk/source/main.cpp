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

  QStringList args;
  for(int i = 0; i < argc; i++)
    args << argv[i];
  bool no_ui = args.contains("-no_ui");
  MainWindow window(no_ui);
  if(!no_ui)
    window.show();
  return app.exec();
}


