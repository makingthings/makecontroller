/****************************************************************************
**
** Qt SAM7X Uploader.
** MakingThings 2006.
**
****************************************************************************/

#include "ATestWindow.h"
#include "ATestThread.h"

ATestWindow::ATestWindow( QApplication* application ) : QMainWindow( 0 )
{
	this->application = application;
	setupUi(this);
		
	QMainWindow::connect( goButton, SIGNAL( clicked() ), this, SLOT( goButtonClicked() ) );
	
  aTestThread = 0;  
}

// 
void ATestWindow::goButtonClicked()
{  
  if ( aTestThread == 0 )
  {
  	aTestThread = new ATestThread( application, this );
    aTestThread->start();
  }
  
  if ( !aTestThread->isFinished() )
    return;
    
  message( 0 );
  message( "GO\n" );
  aTestThread->start();
}

void ATestWindow::status( char* text )
{
	QString qt( text );
  statusLabel->setText( qt );
}

void ATestWindow::message( char* text )
{
	if ( text != 0 )
    testOutput->insertPlainText( text );
  else
    testOutput->clear();
  testOutput->ensureCursorVisible ();
}


void ATestWindow::customEvent( QEvent* event )
{
	CTEvent *cTEvent = (CTEvent*)event;
	if ( cTEvent->statusBound )
	  status( cTEvent->message );
	else
  	message( cTEvent->message );	
}

