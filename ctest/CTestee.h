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

class CTestThread;

class CTestee
{		
	public:
	  enum Status { OK, ERROR_COULDNT_CONNECT, ERROR_COULDNT_DOWNLOAD, ERROR_COULDNT_SWITCH,
	  	            ERROR_WEIRD_CHIP, ERROR_NO_CTESTEE_BIN, ERROR_SAMBA_ERROR,
	  	            ERROR_CANT_OPEN_SOCKET, ERROR_ALREADY_PROGRAMMED, ERROR_EEPROM_FAILURE,
	  	            ERROR_NO_REPLY_EEPROM_TEST, ERROR_INCORRECT_RESPONSE, ERROR_NO_RESPONSE };	

	  CTestee( MessageInterface *messageInterface );

	  Status start();
	  Status stop();
	  
	  Status eepromTest( );
		Status flash();

    Status checkForTestProgram();
    Status requestErase();
    	  
    Status ioPattern( int pattern );
    
	  Status canOut( int mode );
	  Status canIn( int* value );
    		
	private:
    MessageInterface *messageInterface;
    PacketUdp* packetUdp;
    Osc* osc;
    
    Samba* samba;
};

#endif
