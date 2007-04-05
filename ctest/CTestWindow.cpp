/****************************************************************************
**
** Qt SAM7X Uploader.
** MakingThings 2006.
**
****************************************************************************/

#include "CTestWindow.h"
#include "CTestThread.h"

CTestWindow::CTestWindow( QApplication* application ) : QMainWindow( 0 )
{
	this->application = application;
	setupUi(this);
		
	QMainWindow::connect( goButton, SIGNAL( clicked() ), this, SLOT( goButtonClicked() ) );
	
  cTestThread = 0;  
}

// 
void CTestWindow::goButtonClicked()
{  
  if ( cTestThread == 0 )
  {
  	cTestThread = new CTestThread( application, this );
    cTestThread->start();
  }
  
  if ( !cTestThread->isFinished() )
    return;
    
  message( 0 );
  message( "GO\n" );
  cTestThread->start();
}

void CTestWindow::status( char* text )
{
	QString qt( text );
  statusLabel->setText( qt );
}

void CTestWindow::message( char* text )
{
	if ( text != 0 )
    testOutput->insertPlainText( text );
  else
    testOutput->clear();
  testOutput->ensureCursorVisible ();
}


void CTestWindow::customEvent( QEvent* event )
{
	CTEvent *cTEvent = (CTEvent*)event;
	if ( cTEvent->statusBound )
	  status( cTEvent->message );
	else
  	message( cTEvent->message );	
}

