/*
	Make.c
*/

#include "stdlib.h"
#include "config.h"
#include "poly.h"
#include "serial.h"

#include "string.h"

void BlinkTask( void* parameters );
void NetworkCheck( void );
void vBasicWEBServer( void *pvParameters );

void Make( )
{
  TaskCreate( BlinkTask, "Blink", 400, 0, 1 );

  // Do this right quick after booting up - otherwise we won't be recognised
  Usb_SetActive( 1 );

  // Fire up the OSC system
  Osc_SetActive( true );
  // Add all the subsystems (make sure OSC_SUBSYSTEM_COUNT is large enough to accomodate them all)
  Osc_RegisterSubsystem( 0, AnalogInOsc_GetName(), AnalogInOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 1, DebugOsc_GetName(), DebugOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 2, SystemOsc_GetName(), SystemOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 3, NetworkOsc_GetName(), NetworkOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 4, SerialOsc_GetName(), SerialOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 5, IoOsc_GetName(), IoOsc_ReceiveMessage, NULL );

  // Starts the network up.  Will not return until a network is found...
  Network_SetActive( true );

	// Start the example webserver
  TaskCreate( vBasicWEBServer, "WebServ", 300, NULL, 4 );
}

void BlinkTask( void* p )
{
 (void)p;
  Led_SetState( 1 );
  Sleep( 1000 );
    
  //Serial_SetBaud( 115200 );

  //int count = 0;
  //uchar *out = "abcdefghijklmnopqrstuvwxyz\n";
  //uchar in[ 100 ];
  while ( true )
  {
    // Serial Test
    //Serial_Write( out, strlen( (char*)out ) + 1, 1000 );

    //int available = Serial_GetReadable( );
/*
    int available = 0;
    if ( available > 0 )
    {
      int len = Serial_Read( in, available, 100 );
      in[ len ] = 0;
      Debug( 0, "Serial:%s", in );
    }
*/
    Led_SetState( 0 );
    Sleep( 90 );
    Led_SetState( 1 );
    Sleep( 10 );
  }
}
