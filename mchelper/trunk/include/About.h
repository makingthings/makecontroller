#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>

class About : public QDialog
{
  Q_OBJECT
public:
  About();
private:
  QPushButton *okButton;
  QLabel title, version, icon;
  QLabel *description;
  QPixmap *mchelperIcon;
  QVBoxLayout *topLevelLayout;
  QHBoxLayout *buttonLayout;
};

#endif // ABOUT_H


