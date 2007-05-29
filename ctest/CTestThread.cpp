/****************************************************************************
**
** Qt SAM7X Uploader.
** MakingThings 2006.
**
****************************************************************************/

#include "CTestThread.h"
#include "CTestWindow.h"
#include "CTestee.h"
#include "CTester.h"

CTestThread::CTestThread( QApplication* application, CTestWindow* cTestWindow ) : QThread()
{
  this->application = application;
  this->cTestWindow = cTestWindow;
  
  cTester = new CTester( this );
	cTestee = new CTestee( this );
}


void CTestThread::run()
{
	CTester::Status cTesterStatus;
	CTestee::Status cTesteeStatus;
	
	status( "Initializing" );

	message( 1, "Initializing Tester Comms..." );
	cTesterStatus = cTester->start();
	if ( cTesterStatus != CTester::OK )
	{
		message( 1, "FAILED\n" );
		message( 1, "  ** Are you running dumpOsc or similar on port 12000?\n" );
		cTester->stop();
		return;
	}
	else
	  message( 1, "OK\n" );
	
	message( 1, "Checking Tester Program..." );
	cTesterStatus = cTester->checkForTesterProgram();
	if ( cTesterStatus != CTester::OK )
	{
		message( 1, "FAILED - No program\n" );
		message( 1, "  ** Make sure the tester has power\n" );
		message( 1, "  ** Make sure the tester's LED is blinking\n" );
		message( 1, "  ** Make sure the ethernet cable is plugged into the tester\n" );
		cTester->stop();
		return;
	}
	else
	  message( 1, "OK\n" );
	
	message( 1, "Initializing Testee..." );
	cTesteeStatus = cTestee->start();
	if ( cTesteeStatus != CTestee::OK )
	{
		message( 1, "FAILED - Couldn't start\n" );
		message( 1, "  ** Are you running dumpOsc or similar on port 10000?\n" );
		cTester->stop();
		cTestee->stop();
		return;
	}
	else
	  message( 1, "OK\n" );
	  
	status( "Testing" );
		
	message( 1, "Testing Current @ 3.3V... " );
	cTesterStatus = cTester->testeePowerUp3_3V();
	
	sleepMs( 250 );
	
	int current;
	cTesterStatus = cTester->testeeReadCurrent( &current );
	if ( cTesterStatus != CTester::OK )
	{
		message( 1, "FAILED - Testee failed to read currrent\n" );
		failed();
		return;
	}

	message( 1, "%dmA ", current );

  if ( current < 30 )
	{
		message( 1, "FAILED - Current Too Low\n" );
		failed();
		return;
	}
  if ( current > 200 )
	{
		message( 1, "FAILED - Current Too High\n" );
		message( 1, "  ** Check for bridges from 3.3V lines to Ground\n" );
		message( 1, "  ** Check for bridges from V+ lines to Ground\n" );
		failed();
		return;
	}
	
	message( 1, "OK\n" );
	
	if ( current > 70 )
	{
		message( 1, "Checking for pre-programmed Testee..." );
		cTesteeStatus = CTestee::ERROR_NO_RESPONSE;

		// Suspect that it's already programmed... check...
		int timeout = 20; // wait up to 10 seconds in half-second increments
		while( timeout-- )
		{
			cTesteeStatus = cTestee->checkForTestProgram();
			if ( cTesteeStatus == CTestee::OK )
			{
				message( 1, "Already Programmed\n" );
				message( 1, "Erasing..." );
			  	cTesteeStatus = cTestee->requestErase();	
				message( 1, "OK\n" );
			  	break;
			}
			sleepMs( 500 );
		}
		if ( cTesteeStatus != CTestee::OK )
		{
			message( 1, "No Program\n" );
  			message( 1, "Current of %d is too high", current );
      		failed();				
      		return;	
		}
	}	

	message( 1, "Testing Current @ V+... " );
	
	cTesterStatus = cTester->testeePowerDown();

  sleepMs( 250 );
  
	cTesterStatus = cTester->testeePowerUpVPlus();

  sleep( 1 );

	cTesterStatus = cTester->testeeReadCurrent( &current );
	if ( cTesterStatus != CTester::OK )
	{
		message( 1, "FAILED - could not read current\n" );
		failed();
		return;
	}

	message( 1, "%dmA ", current );
	
  if ( current < 30 )
	{
		message( 1, "FAILED - Current Too Low\n" );
		message( 1, "  ** Check board is inserted properly\n" );
		failed();
		return;
	}
  if ( current > 70 )
	{
		message( 1, "FAILED - Current Too High\n" );
		message( 1, "  ** Check for bridges from V+ lines to Gnd\n" );
		message( 1, "  ** Check on-board regulator\n" );
		failed();
		return;
	}
	
	message( 1, "OK\n" );
	
	message( 1, "Testing 3.3V Voltage... " );
	
	int voltage;
	cTesterStatus = cTester->testeeReadVoltage( &voltage );
	if ( cTesterStatus != CTester::OK )
	{
		message( 1, "FAILED - Testee failed to read voltage\n" );
		failed();
		return;
	}

	message( 1, "%2.2fV ", (float)voltage/1000.0 );

  if ( voltage < 3200 )
	{
		message( 1, "FAILED - Voltage Too Low\n" );
		message( 1, "  ** Check regulator\n" );
		failed();
		return;
	}
	
  if ( voltage > 3400 )
	{
		message( 1, "FAILED - Voltage Too High\n" );
		message( 1, "  ** Check regulator\n" );
		failed();
		return;
	}
	message( 1, "OK\n" );
		
	message( 1, "Programming..." );
	cTesteeStatus = cTestee->flash();
	if ( cTesteeStatus != CTestee::OK )
	{
		switch ( cTesteeStatus )
		{
			case CTestee::ERROR_COULDNT_CONNECT:
    		message( 1, "FAILED - Couldn't Connect" );
    		message( 1, "  ** Check USB cable is plugged into the testee\n" );
    		message( 1, "  ** Check USB lines on board\n" );
			  break;
			case CTestee::ERROR_WEIRD_CHIP:
    		message( 1, "FAILED - Strange Chip ID" );
    		message( 1, "  ** Check Atmel chip on board\n" );
			  break;
			case CTestee::ERROR_NO_CTESTEE_BIN:
    		message( 1, "FAILED - Couldn't find ctestee.bin" );
    		message( 1, "  ** Make sure the ctestee.bin is in the current directory\n" );
			  break;
			case CTestee::ERROR_COULDNT_DOWNLOAD:
    		message( 1, "FAILED - Couldn't complete download" );
    		message( 1, "  ** Noisey power supply?\n");
    		message( 1, "  ** Flakey USB connection?\n" );
    		message( 1, "  ** Processor not running right?\n" );
			  break;
			default:
    		message( 1, "FAILED - Unknown Error - %d", cTesteeStatus );
    		message( 1, "  ** Please note error number\n");
			  break;			  
		}
		failed();
		return;
	}		
	message( 1, "OK\n" );

  message( 1, "Checking for Successful Download..." );
	cTesterStatus = cTester->testeePowerDown();

  sleepMs( 250 );
  
	cTesterStatus = cTester->testeePowerUpVPlus();
  
	int timeout = 20; // wait up to 10 seconds in half-second increments
	while( timeout-- )
	{
		cTesteeStatus = cTestee->checkForTestProgram();
		if ( cTesteeStatus == CTestee::OK )
		{	
			message( 1, "OK\n" );
		  	break;
		}
		sleepMs( 500 );
	}
	if ( cTesteeStatus != CTestee::OK )
	{
		message( 1, "FAILED - No Test Program after download\n" );
    	message( 1, "  ** Check ethernet cable is plugged into the testee\n" );
    	message( 1, "  ** Check ethernet circuit on board\n" );
  		failed();				
  		return;	
	}
  
	message( 1, "Testing IO - All Lines OFF..." );
	cTesterStatus = cTester->ioPattern( 0 );
	cTesteeStatus = cTestee->ioPattern( 0 );

	sleep( 1 );
	
  int ioResults;
  
  cTesterStatus = cTester->ioTest( &ioResults);
	if ( cTesterStatus != CTester::OK )
	{
		message( 1, "FAILED - no results\n" );
    message( 1, "  ** Check ethernet circuit on board\n" );
    message( 1, "  ** Processor not running right?\n" );
   	failed();
		return;
	}

  if ( ioResults != 0 )
	{
		message( 1, "FAILED - %d\n", ioResults );
		unpackResults( ioResults );
		failed();
		return;
	}
  
  message( 1, "OK\n" );
  
	message( 1, "Testing IO - All Lines ON..." );

	cTesterStatus = cTester->ioPattern( 1 );
	cTesteeStatus = cTestee->ioPattern( 1 );

	sleepMs( 100 );

	cTesterStatus = cTester->ioTest( &ioResults );
	if ( cTesterStatus != CTester::OK )
	{
		message( 1, "FAILED - no results\n" );
    message( 1, "  ** Check ethernet circuit on board\n" );
    message( 1, "  ** Processor not running right?\n" );
		failed();
		return;
	}

  if ( ioResults != 0 )
	{
		message( 1, "FAILED %d\n", ioResults );
		unpackResults( ioResults );
		failed();
		return;
	}	
  message( 1, "OK\n" );
	
  message( 1, "Testing EEPROM..." );	

	cTesteeStatus = cTestee->eepromTest( );
	if ( cTesteeStatus != CTestee::OK )
	{
		message( 1, "FAILED - Testee failed EEPROM test\n" );
		failed();
		return;
	}
	else
    message( 1, "OK\n" );
    
  message( 1, "Testing CAN Chip Off..." );	

	cTesterStatus = cTester->canOut( 0 );
	cTesteeStatus = cTestee->canOut( 0 );

  sleepMs( 100 );
  
  int value;
	cTesteeStatus = cTestee->canIn( &value );
	if ( cTesteeStatus != CTestee::OK )
	{
		message( 1, "FAILED - Got no value\n" );
		message( 1, "  ** Check Correct ctestee.bin version\n" );
		failed();
		return;
	}

  if ( value != 1 )
  {
		message( 1, "FAILED - Got dominant\n" );
		message( 1, "  ** Check output on CAN chip\n" );
		failed();
		return;
  }  	
  
  message( 1, "OK\n" );
  
  message( 1, "Testing CAN Chip On, No Signal..." );	

	cTesteeStatus = cTestee->canOut( 1 );

  sleepMs( 100 );
  
	cTesteeStatus = cTestee->canIn( &value );
	if ( cTesteeStatus != CTestee::OK )
	{
		message( 1, "FAILED - Got no value\n" );
		message( 1, "  ** Check Correct ctestee.bin version\n" );
		failed();
		return;
	}

  if ( value != 1 )
  {
		message( 1, "FAILED - Got dominant with chip on but no signal\n" );
		message( 1, "  ** Check CAN IO lines on board to connector\n" );
		failed();
		return;
  }  	

  message( 1, "OK\n" );
  
  message( 1, "Testing CAN Chip On, Signal..." );	

	cTesteeStatus = cTestee->canOut( 2 );

  sleepMs( 100 );
  
	cTesteeStatus = cTestee->canIn( &value );
	if ( cTesteeStatus != CTestee::OK )
	{
		message( 1, "FAILED - Got no value\n" );
		message( 1, "  ** Check Correct ctestee.bin version\n" );
		failed();
		return;
	}

  if ( value != 0 )
  {
		message( 1, "FAILED - Signalling, but no signal\n" );
		message( 1, "  ** Check CAN chip\n" );
		message( 1, "  ** Check CAN Rx lines\n" );
		failed();
		return;
  }  	
  
  message( 1, "OK\n" );

  message( 1, "Testing CAN IO..." );	

	cTesterStatus = cTester->canOut( 2 );
	cTesteeStatus = cTestee->canOut( 1 );

  sleepMs( 100 );
  
	cTesteeStatus = cTestee->canIn( &value );
	if ( cTesteeStatus != CTestee::OK )
	{
		message( 1, "FAILED - Got no value\n" );
		message( 1, "  ** Check Correct ctestee.bin version\n" );
		failed();
		return;
	}

  if ( value != 0 )
  {
		message( 1, "FAILED - No dominant signal\n" );
		message( 1, "  ** Check CAN IO lines on board\n" );
		failed();
		return;
  }  	
  
  cTesterStatus = cTester->canOut( 0 );

  message( 1, "OK\n" );
  
  cTestee->setNetworkConfig( );
  cTestee->setSerialNumber( );
  
  
	reset();
	
	status( "OK" );
	message( 1, "TESTING COMPLETE - ALL OK\n" );
}

void CTestThread::unpackResults( int results )
{
	int i;
	int mask = 1;
	
  for ( i = 0; i < 31; i++ )
  {
    if ( results & mask )
      message( 1, "  Line %d Failed\n", i );
    mask <<= 1;	
  }
}

void CTestThread::message( int level, char *format, ... )
{
	va_list args;
	char buffer[ 1000 ];
	
	va_start( args, format );
	vsnprintf( buffer, 1000, format, args );
	va_end( args );
	
	CTEvent* ctEvent = new CTEvent( false, level, buffer );
	
	if( level == 1 )
	  application->postEvent( cTestWindow, ctEvent );
}

void CTestThread::sleepMs( int ms )
{
  usleep( ms * 1000 );	
}

void CTestThread::failed( )
{
  status( "FAILED" );
	cTester->testeePowerDown();
	cTester->stop();
	cTestee->stop();
}

void CTestThread::reset( )
{
	cTestee->stop();
	cTester->testeePowerDown();
	cTester->stop();
}

void CTestThread::status( char *text )
{
	CTEvent* ctEvent = new CTEvent( true, 0, text );
	
	application->postEvent( cTestWindow, ctEvent );
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

