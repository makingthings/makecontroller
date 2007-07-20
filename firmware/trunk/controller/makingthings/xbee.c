/*
	Make.c
*/

#include "stdlib.h"
#include <stdio.h>

#include "xbee.h"

XBee_* XBee;

int XBee_SetActive( int state )
{
  if ( state != 0 && XBee == NULL ) 
  {
    XBee = Malloc( sizeof( XBee__ ) );
  }
  if ( state == 0 && XBee != NULL ) 
  {
    Free( XBee );
    XBee = 0;
  }
}

int XBee_GetActive( )
{
  return ( XBee != NULL );
}

int XBee_GetPacket()
{
  
}

