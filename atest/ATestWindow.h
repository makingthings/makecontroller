/****************************************************************************
**
** Qt SAM7X Uploader.
** MakingThings 2006.
**
****************************************************************************/

#ifndef ATESTWINDOW_H
#define ATESTWINDOW_H

#include "ui_atest.h"

//Qt includes
#include <QMainWindow>
#include <QThread>

#include "ATestThread.h"
class ATestThread;

class ATestWindow : public QMainWindow, private Ui::ATest
{
	Q_OBJECT
		
	public:
    ATestWindow( QApplication* application );
	  void message( char* text );
	  void status( char* text );
		void customEvent( QEvent* event );
		
	private:
	  QApplication* application;
	  ATestThread* aTestThread;

	private slots:
	  void goButtonClicked();
};

#endif
