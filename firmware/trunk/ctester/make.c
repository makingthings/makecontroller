/*
	Make.c
*/

#include "stdlib.h"
#include "config.h"
#include "ctester.h"
#include "osc.h"

void BlinkTask( void* p );

void MakeInit( )
{ 
  CTester_SetTesteePower( 0 );

  TaskCreate( BlinkTask, "Blink", 200, 0, 2 );
 
  Osc_SetReplyPort( 0, 12000 );
  Osc_SetActive( true );

  Osc_RegisterSubsystem( 0, AdcOsc_GetName(), AdcOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 1, LedOsc_GetName(), LedOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 2, DebugOsc_GetName(), DebugOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 3, CTesterOsc_GetName(), CTesterOsc_ReceiveMessage, NULL );
}

void BlinkTask( void* p )
{
 (void)p;
  
  while ( true )
  {
    Sleep( 100 );
    Led_SetState( 0 );
    Sleep( 900 );
    Led_SetState( 1 );
  }
}
