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


