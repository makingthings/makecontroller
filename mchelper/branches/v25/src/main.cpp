#include <QApplication>
#include "MainWindow.h"

int main( int argc, char *argv[] )
{
  QApplication app(argc, argv);
  QStringList args;
  for(int i = 0; i < argc; i++)
    args << argv[i];
  bool no_ui = args.contains("-no_ui");
  MainWindow window(no_ui);
  if(!no_ui)
    window.show();
  return app.exec();
}