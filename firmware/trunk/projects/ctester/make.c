/*
	Make.c
*/

#include "stdlib.h"
#include "config.h"
#include "ctester.h"
#include "osc.h"

void BlinkTask( void* p );

void Run( )
{ 
  CTester_SetTesteePower( 0 );

  TaskCreate( BlinkTask, "Blink", 400, 0, 2 );
 
  Osc_SetReplyPort( 0, 12000 );
  Osc_SetActive( true, true, true, false );

  Osc_RegisterSubsystem( AnalogInOsc_GetName(), AnalogInOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( LedOsc_GetName(), LedOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( DebugOsc_GetName(), DebugOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( CTesterOsc_GetName(), CTesterOsc_ReceiveMessage, NULL );
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
