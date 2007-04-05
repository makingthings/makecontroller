/****************************************************************************
**
** MESSAGEINTERFACE
** MakingThings 2006.
**
****************************************************************************/

#ifndef MESSAGEINTERFACE_H
#define MESSAGEINTERFACE_H

class MessageInterface
{		
	public:
	  virtual void message( int level, char* format, ... ) = 0;
	  virtual void sleepMs( int sm ) = 0;
};

#endif
