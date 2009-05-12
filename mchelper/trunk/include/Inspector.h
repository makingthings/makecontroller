#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <QDialog>
#include "ui_inspector.h"
#include "MainWindow.h"
#include "Board.h"

class MainWindow;
class Board;

class Inspector : public QDialog, private Ui::InspectorUi
{
  Q_OBJECT
public:
  Inspector(MainWindow *mainWindow);
  void setData(Board* board);
  void clear( );

public slots:
  void loadAndShow();

private slots:
  void onFinished();
  void getBoardInfo();
  void onApply();
  void onRevert();
  void onAnyValueEdited();

private:
  MainWindow *mainWindow;
  QTimer infoTimer;
  void setLabelsRole(QPalette::ColorRole role);
};

#endif // INSPECTOR_H


