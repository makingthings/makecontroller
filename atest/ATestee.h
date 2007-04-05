/****************************************************************************
**
** CTESTEE
** MakingThings 2006.
**
****************************************************************************/

#ifndef CTESTEE_H
#define CTESTEE_H

#include "MessageInterface.h"
#include "Samba.h"
#include "Osc.h"
#include "PacketUdp.h"

class ATestThread;

class ATestee
{		
	public:
	  enum Status { OK, ERROR_COULDNT_CONNECT, ERROR_COULDNT_DOWNLOAD, ERROR_COULDNT_SWITCH,
	  	            ERROR_CANT_OPEN_SOCKET, ERROR_ALREADY_PROGRAMMED, ERROR_EEPROM_FAILURE,
	  	            ERROR_NO_REPLY_EEPROM_TEST, ERROR_INCORRECT_RESPONSE, ERROR_NO_PROGRAM,
	  	            ERROR_NO_SAMBA, ERROR_NO_TESTER };	

	  ATestee( MessageInterface *messageInterface );

	  Status start();
	  Status stop();
	  
	  Status eepromTest( );
		Status flash();
		
    Status checkForCTestProgram();
    Status checkForATestProgram();
    Status requestErase();
    Status restart();
    
    Status checkForSamba();
    	  
    Status performTest( int i, int* result );
    		
	private:
    MessageInterface *messageInterface;
    PacketUdp* packetUdp;
    Osc* osc;
    
    Samba* samba;
};

#endif
