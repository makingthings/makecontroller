/****************************************************************************
**
** CTESTEE
** MakingThings 2006.
**
****************************************************************************/

#include "CTestee.h"
#include "CTestThread.h"

CTestee::CTestee( MessageInterface *messageInterface )
{
	this->messageInterface = messageInterface;
	samba = new Samba( messageInterface );
		
	packetUdp = new PacketUdp( messageInterface );
  osc = new Osc( packetUdp, messageInterface );
}

CTestee::Status CTestee::start()
{
	messageInterface->message( 2, "  Testee Start\n" );
	if ( packetUdp->connect(	"192.168.0.200", 10000, 10000 ) != PacketUdp::OK )
	{
  	messageInterface->message( 2, "  Couldn't set up connection\n" );
	  return ERROR_CANT_OPEN_SOCKET;
	}	

	return OK;
}

CTestee::Status CTestee::stop()
{
	packetUdp->disconnect();
	return OK;
}

CTestee::Status CTestee::checkForTestProgram()
{
	// Check for a reaction
	osc->createMessage( "/ctestee/active" );
	osc->sendPacket();
	
	messageInterface->sleepMs( 100 );
	
 	OscMessage oscMessage;
	Osc::Status s = osc->receive( &oscMessage );
	if ( s != Osc::OK )
	{
    messageInterface->message( 2, "  No response - Not programmed\n" );
	  return ERROR_NO_RESPONSE;
	}
	
  messageInterface->message( 2, "Testee Already Programmed\n" );
	return OK;
}

CTestee::Status CTestee::requestErase()
{
	// Check for a reaction
	osc->createMessage( "/ctestee/active", ",i", 0 );
	osc->sendPacket();
	
	return OK;
}

CTestee::Status CTestee::flash()
{
	messageInterface->message( 2, "  Testee Flashing...\n" );
	messageInterface->message( 3, "    - Connecting\n" );
	
	if ( samba->connect( ) != Samba::OK )
  {
  	messageInterface->message( 3, "  Error Connecting\n" );
	  return ERROR_COULDNT_CONNECT;
  }
  
	messageInterface->message( 2, "    - Sending the file\n" );    

	Samba::Status sambaStatus = samba->flashUpload( "ctestee.bin" );
	if ( sambaStatus != Samba::OK )
  {
  	messageInterface->message( 3, "  Error Uploading\n" );
  	switch ( sambaStatus )
  	{
  		case Samba::ERROR_INCORRECT_CHIP_INFO:
  		  return ERROR_WEIRD_CHIP;
  		case Samba::ERROR_COULDNT_FIND_FILE:
  		case Samba::ERROR_COULDNT_OPEN_FILE:
  		  return ERROR_NO_CTESTEE_BIN;
  		case Samba::ERROR_SENDING_FILE:
  		  return ERROR_COULDNT_DOWNLOAD;
  		default:
  		  return ERROR_SAMBA_ERROR;
  	}
  }

	messageInterface->message( 2, "    - Switching to Boot from Flash\n" );    
 
	if ( samba->bootFromFlash(  ) != Samba::OK )
  {
  	messageInterface->message( 3, "  Error Switching to Boot from Flash\n" );
	  return ERROR_COULDNT_SWITCH;
  }

	if ( samba->disconnect(  ) != Samba::OK )
  {
  	messageInterface->message( 3, "  Error Switching to Boot from Flash\n" );
	  return ERROR_COULDNT_CONNECT;
  }

	return OK;
}

CTestee::Status CTestee::ioPattern( int pattern )
{
	messageInterface->message( 2, "  Testee IO Pattern Set - %d\n", pattern );	
		
	osc->createMessage( "/ctestee/iopattern", ",i", pattern );
	osc->sendPacket();
	
	return OK;
}

CTestee::Status CTestee::eepromTest( )
{	
	osc->createMessage( "/ctestee/eepromtest" );
	osc->sendPacket();
	
	messageInterface->sleepMs( 500 );
	
	OscMessage oscMessage;
	Osc::Status s = osc->receive( &oscMessage );
	if ( s != Osc::OK )
	{
  	messageInterface->message( 2, "  No message back from the testee\n" );
	  return ERROR_NO_REPLY_EEPROM_TEST;
	}
	
   if ( strcmp( oscMessage.address, "/ctestee/eepromtest" ) != 0 )
  {
  	messageInterface->message( 2, "  Wrong message back from the tester - %s\n", oscMessage.address );
    return ERROR_INCORRECT_RESPONSE;
  }
	
	return oscMessage.i ? OK : ERROR_EEPROM_FAILURE;

}

CTestee::Status CTestee::canOut( int mode )
{
	messageInterface->message( 2, "  Setting Can Mode - %d\n", mode );	
		
	osc->createMessage( "/ctestee/canout", ",i", mode );
	osc->sendPacket();
	
	return OK;
}

CTestee::Status CTestee::canIn( int* value )
{
	messageInterface->message( 2, "  Testing IO on Tester\n" );	
		
	osc->createMessage( "/ctestee/canin" );
	osc->sendPacket();
	
	messageInterface->sleepMs( 100 );
	
	OscMessage oscMessage;
	Osc::Status s = osc->receive( &oscMessage );
	if ( s != Osc::OK )
	{
  	messageInterface->message( 2, "  No message back from the tester\n" );
	  return ERROR_NO_RESPONSE;
	}
	
   if ( strcmp( oscMessage.address, "/ctestee/canin" ) != 0 )
  {
  	messageInterface->message( 2, "  Wrong message back from the tester - %s\n", oscMessage.address );
    return ERROR_INCORRECT_RESPONSE;
  }
	
	*value = oscMessage.i;
	
  return OK;
}


