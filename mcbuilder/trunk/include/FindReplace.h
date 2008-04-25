#ifndef FIND_REPLACE_H
#define FIND_REPLACE_H

#include <QDialog>
#include "ui_findreplace.h"
#include "MainWindow.h"

class FindReplace : public QDialog, private Ui::FindReplaceUi
{
	Q_OBJECT
  
	public:
		FindReplace( MainWindow *mw );
  private:
    MainWindow *mainWindow;
    void doFind(bool forward);

	private slots:
		void onNext( );
    void onPrevious( );
    void onReplace( );
    void onReplaceAll( );
};

#endif // FIND_REPLACE_H


