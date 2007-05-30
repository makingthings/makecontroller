/****************************************************************************
**
** Qt SAM7X Uploader.
** MakingThings 2006.
**
****************************************************************************/

#include "ATestThread.h"
#include "ATestWindow.h"
#include "ATestee.h"

ATestThread::ATestThread( QApplication* application, ATestWindow* aTestWindow ) : QThread()
{
  this->application = application;
  this->aTestWindow = aTestWindow;
  
	aTestee = new ATestee( this );
}

// 
void ATestThread::run()
{
	ATestee::Status aTesteeStatus;
	
	status( "Initializing" );
	
	message( 1, "Starting..." );
	aTesteeStatus = aTestee->start();
	if ( aTesteeStatus != ATestee::OK )
	{
		message( 1, "Failed Startup\n" );
		message( 1, "  Are you running dumpOsc?\n" );
		aTestee->stop();
		return;
	}
	else
	  message( 1, "OK\n" );

	status( "Testing" );
		
	message( 1, "Checking for Ethernet Connection..." );
	aTesteeStatus = ATestee::ERROR_INCORRECT_RESPONSE;
	int timeout = 20; // wait up to 10 seconds in half-second increments
	while( timeout-- )
	{
		aTesteeStatus = aTestee->checkForCTestProgram();
		if ( aTesteeStatus == ATestee::OK )
		{	
			timeout = 20;
			while( timeout-- )
			{
				aTesteeStatus = aTestee->checkForATestProgram();
				if( aTesteeStatus == ATestee::OK )
				{
					message( 1, "OK\n" );
					break;
				}
				sleepMs( 500 );
			}
			if ( aTesteeStatus != ATestee::OK )
		    {
		    	message( 1, "Need to reprogram - running old program\n" );
		    	aTestee->requestErase();
		    	message( 1, "Please unplug & replug the Application Tester, then run this test again.\n" );
				failed();
				return;
		    }
			message( 1, "OK\n" );
		  	break;
		}
		sleepMs( 500 );
	}
	
	if ( aTesteeStatus != ATestee::OK )
	{
		message( 1, "  None\n" );

		message( 1, "Checking for Samba... " );	
		aTesteeStatus = aTestee->checkForSamba();
		if ( aTesteeStatus == ATestee::OK )
		{
		  message( 1, "Found \n" );	
		  message( 1, "THIS CONTROLLER MAY BE UNTESTED\n" );
		  message( 1, "Loading Program" );	
		  
			if ( aTestee->flash() != ATestee::OK )
			{
				message( 1, "Load failed\n" );
				failed();
				return;
			}				
			message( 1, "OK\n" );

		  message( 1, "Please unplug & replug the Application Tester, then run this test again.\n" );
	    failed();
	    return;		
		}
		else
		{
			message( 1, "Not Found\n" );
		}

		message( 1, "Checking for Ethernet Connection..." );
		aTesteeStatus = aTestee->checkForATestProgram();
		if ( aTesteeStatus != ATestee::OK )
		{
			message( 1, "  No Connection to Testee\n" );
			message( 1, "  Check Ethernet Hardware, location J27\n" );
	    failed();
	    return;
		}
 
	  failed();
    return;
	}			
	    
	message( 1, "Testing Enable Off Lines Off..." );
  int result;
	aTesteeStatus = aTestee->performTest( 0, &result );
	if ( aTesteeStatus != ATestee::OK )
	{
		message( 1, "Failed %d\n", result  );
		failed();
		return;
	}
	else
	  message( 1, "OK\n" );

	message( 1, "Testing Enable Off Lines On..." );
	aTesteeStatus = aTestee->performTest( 1, &result );
	if ( aTesteeStatus != ATestee::OK )
	{
		message( 1, "Failed %d\n", result  );
		failed();
		return;
	}
	else
	  message( 1, "OK\n" );

	message( 1, "Testing Enable On Lines Off..." );
	aTesteeStatus = aTestee->performTest( 2, &result );
	if ( aTesteeStatus != ATestee::OK )
	{
		message( 1, "Failed %d\n", result  );
		failed();
		return;
	}
	else
	  message( 1, "OK\n" ); 

	message( 1, "Testing Enable On Lines On..." );
	aTesteeStatus = aTestee->performTest( 3, &result );
	if ( aTesteeStatus != ATestee::OK )
	{
		message( 1, "Failed %d\n", result  );
		failed();
		return;
	}
	else
	  message( 1, "OK\n" );

  message( 1, "ALL OK" );

	reset();
	
	status( "OK" );
}

void ATestThread::message( int level, char *format, ... )
{
	if ( level == 1 )
	{
		va_list args;
		char buffer[ 1000 ];
		
		va_start( args, format );
		vsnprintf( buffer, 1000, format, args );
		va_end( args );
		
		CTEvent* ctEvent = new CTEvent( false, level, buffer );
		
		application->postEvent( aTestWindow, ctEvent );
	}
}

void ATestThread::sleepMs( int ms )
{
  usleep( ms * 1000 );	
}

void ATestThread::failed( )
{
  status( "FAILED" );
	aTestee->stop();
}

void ATestThread::reset( )
{
	aTestee->stop();
}

void ATestThread::status( char *text )
{
	CTEvent* ctEvent = new CTEvent( true, 0, text );
	
	application->postEvent( aTestWindow, ctEvent );
}

CTEvent::CTEvent( bool statusBound, int level, char* message ) : QEvent( (Type)10000 )
{
	this->statusBound = statusBound;
	this->level = level;
	
  if ( message != 0 )
	  this->message = strdup( message ); 
	else
	  this->message = 0;
}

CTEvent::~CTEvent( )
{
  if ( message != 0 )
    free( message );
}

