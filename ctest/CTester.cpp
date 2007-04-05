/****************************************************************************
**
** QTESTER
** MakingThings 2006.
**
****************************************************************************/

#include "CTester.h"

CTester::CTester( MessageInterface *messageInterface )
{
	this->messageInterface = messageInterface;
	
	packetUdp = new PacketUdp( messageInterface );
  osc = new Osc( packetUdp, messageInterface );
}

CTester::Status CTester::start()
{
	//messageInterface->message( 1, "Tester Start\n" );

	if ( packetUdp->connect(	"192.168.0.204", 12000, 12000 ) != PacketUdp::OK )
	  return ERROR_CANT_OPEN_SOCKET;

  return OK;
}

CTester::Status CTester::checkForTesterProgram()
{	
	messageInterface->message( 2, "  Sending Active Test\n" );

  // Make sure the unit is on
	osc->createMessage( "/ctester/active" );
	osc->sendPacket();
	
	messageInterface->sleepMs( 100 );
	
 	OscMessage oscMessage;
	Osc::Status s = osc->receive( &oscMessage );
	if ( s != Osc::OK )
	{
  	messageInterface->message( 2, "  No message back from the tester\n" );
	  return ERROR_NO_TESTER;
	}
	
  if ( strcmp( oscMessage.address, "/ctester/active" ) != 0 )
  {
  	messageInterface->message( 2, "  Wrong message back from the tester\n" );
    return ERROR_INCORRECT_RESPONSE;
  }
	
	return OK;
}


CTester::Status CTester::stop()
{
	packetUdp->disconnect();
	return OK;
}

CTester::Status CTester::testeePowerUpVPlus()
{
	messageInterface->message( 2, "  Powering up V+ on the Testee\n" );	
		
	osc->createMessage( "/ctester/testeepower", ",i", 1 );
	osc->sendPacket();
	
	return OK;
}

CTester::Status CTester::testeePowerUp3_3V()
{
	messageInterface->message( 2, "  Powering up 3.3V on the Testee\n" );	
		
	osc->createMessage( "/ctester/testeepower", ",i", 2 );
	osc->sendPacket();
	
	return OK;
}

CTester::Status CTester::testeePowerDown()
{
	messageInterface->message( 2, "  Powering down the Testee\n" );	
		
	osc->createMessage( "/ctester/testeepower", ",i", 0 );
	osc->sendPacket();
	
	return OK;
}

CTester::Status CTester::testeeReadCurrent( int* current )
{
	messageInterface->message( 2, "  Reading the current on the Testee... " );	
		
	osc->createMessage( "/ctester/testeecurrent" );
	osc->sendPacket();
	
	messageInterface->sleepMs( 500 );
	
	OscMessage oscMessage;
	Osc::Status s = osc->receive( &oscMessage );
	if ( s != Osc::OK )
	{
  	  messageInterface->message( 2, "  No message back from the tester\n" );
	  return ERROR_NO_REPLY_CURRENT;
	}
	
    if ( strcmp( oscMessage.address, "/ctester/testeecurrent" ) != 0 )
    {
  	  messageInterface->message( 2, "  Wrong message back from the tester - %s\n", oscMessage.address );
      return ERROR_INCORRECT_RESPONSE;
    }
	
	*current = oscMessage.i;
	messageInterface->message( 2, " current: %dmA\n", *current );
	
  return OK;	
}

CTester::Status CTester::testeeReadVoltage( int* voltage )
{
	messageInterface->message( 2, "  Reading the voltage on the Testee... " );	
		
	osc->createMessage( "/ctester/testvoltage" );
	osc->sendPacket();
	
	messageInterface->sleepMs( 100 );
	
	OscMessage oscMessage;
	Osc::Status s = osc->receive( &oscMessage );
	if ( s != Osc::OK )
	{
  	  messageInterface->message( 2, "  No message back from the tester\n" );
	  return ERROR_NO_REPLY_VOLTAGE;
	}
	
  if ( strcmp( oscMessage.address, "/ctester/testvoltage" ) != 0 )
  {
	  messageInterface->message( 2, "  Wrong message back from the tester - %s\n", oscMessage.address );
    return ERROR_INCORRECT_RESPONSE;
  }
	
	*voltage = oscMessage.i;
	messageInterface->message( 2, " voltage: %dmV\n", *voltage);
	
  return OK;	
}

CTester::Status CTester::ioPattern( int pattern )
{
	messageInterface->message( 2, "  Setting IO pattern on Tester - %d\n", pattern );	
		
	osc->createMessage( "/ctester/iopattern", ",i", pattern );
	osc->sendPacket();
	
	return OK;
}

CTester::Status CTester::ioTest( int* result )
{
	messageInterface->message( 2, "  Testing IO on Tester\n" );	
		
	osc->createMessage( "/ctester/iotest" );
	osc->sendPacket();
	
	messageInterface->sleepMs( 100 );
	
	OscMessage oscMessage;
	Osc::Status s = osc->receive( &oscMessage );
	if ( s != Osc::OK )
	{
  	messageInterface->message( 2, "  No message back from the tester\n" );
	  return ERROR_NO_REPLY_IOTEST;
	}
	
   if ( strcmp( oscMessage.address, "/ctester/iotest" ) != 0 )
  {
  	messageInterface->message( 2, "  Wrong message back from the tester - %s\n", oscMessage.address );
    return ERROR_INCORRECT_RESPONSE;
  }
	
	*result = oscMessage.i;
	
  return OK;	
}

CTester::Status CTester::canOut( int mode )
{
	messageInterface->message( 2, "  Setting Can Mode - %d\n", mode );	
		
	osc->createMessage( "/ctester/canout", ",i", mode );
	osc->sendPacket();
	
	return OK;
}

CTester::Status CTester::canIn( int* value )
{
	messageInterface->message( 2, "  Testing IO on Tester\n" );	
		
	osc->createMessage( "/ctester/canin" );
	osc->sendPacket();
	
	messageInterface->sleepMs( 100 );
	
	OscMessage oscMessage;
	Osc::Status s = osc->receive( &oscMessage );
	if ( s != Osc::OK )
	{
  	messageInterface->message( 2, "  No message back from the tester\n" );
	  return ERROR_NO_RESPONSE;
	}
	
   if ( strcmp( oscMessage.address, "/ctester/canin" ) != 0 )
  {
  	messageInterface->message( 2, "  Wrong message back from the tester - %s\n", oscMessage.address );
    return ERROR_INCORRECT_RESPONSE;
  }
	
	*value = oscMessage.i;
	
  return OK;
}
