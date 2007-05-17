/*
	Make.c
*/

#include "stdlib.h"
#include "config.h"
#include "ctestee.h"
#include "osc.h"
#include "atestee.h"

void BlinkTask( void* p );

void MakeInit( )
{ 
  TaskCreate( BlinkTask, "Blink", 200, 0, 2 );
 
  Osc_SetReplyPort( 0, 10000 );
  Osc_SetActive( true );

  Osc_RegisterSubsystem( 0, AdcOsc_GetName(), AdcOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 1, LedOsc_GetName(), LedOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 2, DebugOsc_GetName(), DebugOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 3, CTesteeOsc_GetName(), CTesteeOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 4, ATesteeOsc_GetName(), ATesteeOsc_ReceiveMessage, NULL );
}

void BlinkTask( void* p )
{
 (void)p;
  
  while ( true )
  {
    Sleep( 100 );
    Led_SetState( 0 );
    Sleep( 100 );
    Led_SetState( 1 );
  }
}
