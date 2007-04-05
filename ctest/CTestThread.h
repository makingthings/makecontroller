/****************************************************************************
**
** Qt SAM7X Uploader.
** MakingThings 2006.
**
****************************************************************************/

#ifndef CTESTTHREAD_H
#define CTESTTHREAD_H

//Qt includes

#include <QThread>

class CTestWindow;
#include "CTestWindow.h"

class CTester;
class CTestee;

#include "MessageInterface.h"

class CTestThread : public QThread, public MessageInterface
{
	Q_OBJECT
		
	public:	
    CTestThread( QApplication* application, CTestWindow* cTestWindow );
	  void run();
		void message( int level, char *text, ... );
	  void status( char* text );
	  
	  void sleepMs( int ms );
	  
	private:
	  void unpackResults( int results );
	  void failed();
	  void reset();
	  QApplication* application;
	  CTestWindow* cTestWindow;
	  CTester* cTester;
	  CTestee* cTestee;
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
