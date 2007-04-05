/****************************************************************************
**
** Qt SAM7X Uploader.
** MakingThings 2006.
**
****************************************************************************/

#ifndef CTESTWINDOW_H
#define CTESTWINDOW_H

#include "ui_ctest.h"

//Qt includes
#include <QMainWindow>
#include <QThread>

#include "CTestThread.h"
class CTestThread;

class CTestWindow : public QMainWindow, private Ui::CTest
{
	Q_OBJECT
		
	public:
    CTestWindow( QApplication* application );
	  void message( char* text );
	  void status( char* text );
		void customEvent( QEvent* event );
		
	private:
	  QApplication* application;
	  CTestThread* cTestThread;

	private slots:
	  void goButtonClicked();
};

#endif
