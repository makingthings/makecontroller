/****************************************************************************
**
** CTESTEE
** MakingThings 2006.
**
****************************************************************************/

#include "ATestee.h"
#include "ATestThread.h"

ATestee::ATestee( MessageInterface *messageInterface )
{
	this->messageInterface = messageInterface;
	samba = new Samba( messageInterface );
		
	packetUdp = new PacketUdp( messageInterface );
  osc = new Osc( packetUdp, messageInterface );
}

ATestee::Status ATestee::start()
{
	messageInterface->message( 2, "Testee Start..." );
	if ( packetUdp->connect(	"192.168.0.200", 10000, 10000 ) != PacketUdp::OK )
	{
  	messageInterface->message( 2, "  Couldn't set up connection\n" );
	  return ERROR_CANT_OPEN_SOCKET;
	}	
	
	return OK;
}

ATestee::Status ATestee::stop()
{
	packetUdp->disconnect();
	return OK;
}

ATestee::Status ATestee::checkForCTestProgram()
{
	messageInterface->message( 2, "  Sending Active Test\n" );

  // Make sure the unit is on
	osc->createMessage( "/ctestee/active" );
	osc->sendPacket();
	
	messageInterface->sleepMs( 100 );
	
 	OscMessage oscMessage;
	Osc::Status s = osc->receive( &oscMessage );
	if ( s != Osc::OK )
	{
  	messageInterface->message( 2, "  No message back from the tester\n" );
	  return ERROR_NO_TESTER;
	}
	
  if ( strcmp( oscMessage.address, "/ctestee/active" ) != 0 )
  {
  	messageInterface->message( 2, "  Wrong message back from the tester\n" );
    return ERROR_INCORRECT_RESPONSE;
  }
  
  return OK;
}

ATestee::Status ATestee::checkForATestProgram()
{
	// Check for a reaction
	osc->createMessage( "/atestee/active" );
	osc->sendPacket();
	
	messageInterface->sleepMs( 100 );
	
 	OscMessage oscMessage;
	Osc::Status s = osc->receive( &oscMessage );
	if ( s != Osc::OK )
	{
    messageInterface->message( 2, "  No response - Not programmed\n" );
	  return ERROR_NO_PROGRAM;
	}
	
  messageInterface->message( 2, "Testee Already Programmed\n" );
	return OK;
}

ATestee::Status ATestee::checkForSamba()
{
	Samba::Status status= samba->connect();
	if ( status == Samba::OK )
	{
		samba->disconnect();
		return OK;
	}
	return ERROR_NO_SAMBA;
}

ATestee::Status ATestee::restart()
{
	Samba::Status status= samba->restart();
	if ( status == Samba::OK )
	{
		return OK;
	}
	return ERROR_NO_SAMBA;
}
	  
ATestee::Status ATestee::performTest( int i, int* result )
{
	return OK;
}

ATestee::Status ATestee::requestErase()
{
	// Shut her down
	osc->createMessage( "/ctestee/active", ",i", 0 );
	osc->sendPacket();
	
	return OK;
}

ATestee::Status ATestee::flash()
{
	messageInterface->message( 2, "Testee Flashing\n" );
	messageInterface->message( 3, "  Connecting\n" );
	
	if ( samba->connect( ) != Samba::OK )
  {
  	messageInterface->message( 3, "  Error Connecting\n" );
	  return ERROR_COULDNT_CONNECT;
  }
  
	messageInterface->message( 3, "  Sending the file\n" );    

	if ( samba->flashUpload( "ctestee.bin" ) != Samba::OK )
  {
  	messageInterface->message( 3, "  Error Uploading\n" );
	  return ERROR_COULDNT_CONNECT;
  }

	messageInterface->message( 3, "Switching to Boot from Flash\n" );    
 
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


