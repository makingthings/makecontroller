/****************************************************************************
**
** CTESTER
** MakingThings 2006.
**
****************************************************************************/

#ifndef CTESTER_H
#define CTESTER_H

//Qt includes

#include <QThread>

#include "MessageInterface.h"
#include "Osc.h"
#include "PacketUdp.h"

class CTester
{		
	public:
	  enum Status { OK, ERROR_NO_TESTER, ERROR_INCORRECT_RESPONSE, ERROR_NO_REPLY_IOTEST,
	  	            ERROR_NO_REPLY_CURRENT,ERROR_NO_REPLY_VOLTAGE, ERROR_CANT_OPEN_SOCKET,
	  	            ERROR_NO_RESPONSE };	
	
	  CTester( MessageInterface *messageInterface );
	  
	  Status start();
	  Status stop();
      Status checkForTesterProgram();

	  Status testeePowerUp3_3V();
	  Status testeePowerUpVPlus();
	  Status testeePowerDown();
	  
	  Status testeeReadCurrent( int* current );
	  Status testeeReadVoltage( int* voltage );
	  Status ioPattern( int pattern );
	  Status ioTest( int* result );
	  
	  Status canOut( int mode );
	  Status canIn( int* value );
		
	private:
    MessageInterface *messageInterface;
    PacketUdp* packetUdp;
    Osc* osc;
};


#endif
