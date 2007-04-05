/****************************************************************************
**
** Qt SAM7X Uploader.
** MakingThings 2006.
**
****************************************************************************/

#ifndef ATESTTHREAD_H
#define ATESTTHREAD_H

//Qt includes

#include <QThread>

class ATestWindow;
#include "ATestWindow.h"

class ATestee;

#include "MessageInterface.h"

class ATestThread : public QThread, public MessageInterface
{
	Q_OBJECT
		
	public:	
    ATestThread( QApplication* application, ATestWindow* cTestWindow );
	  void run();
		void message( int level, char *text, ... );
	  void status( char* text );
	  
	  void sleepMs( int ms );
	  
	private:
	  void failed();
	  void reset();
	  QApplication* application;
	  ATestWindow* aTestWindow;
	  ATestee* aTestee;
};

class CTEvent : public QEvent
{
	public:
	  CTEvent( bool statusBound, int level, char* message );
	  ~CTEvent();
	  
	bool statusBound;  
	int level;
	char* message;
};

#endif
