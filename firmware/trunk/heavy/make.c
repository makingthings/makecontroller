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

FastTimerEntry fastTimerEntry;
int ledState;

TimerEntry timerEntry;
TimerEntry timerEntry1;
TimerEntry timerEntry2;
TimerEntry timerEntry3;
int appledState[ 4 ];

void TimerCallback( int id )
{
  AppLed_SetState( id, appledState[ id ] );
  appledState[ id ] = !appledState[ id ];

  if ( id == 3 )
  {
    if ( appledState[  3 ] )
    {
      Timer_Cancel( &timerEntry1 );      
    }
    else
    {
      Timer_Set( &timerEntry1 );
    }
  }

  if ( id == 0 )
  {
    Timer_Cancel( &timerEntry );
    Timer_Set( &timerEntry );
  }

  if ( id == 2 )
  {
    Timer_Cancel( &timerEntry2 );
    Timer_Set( &timerEntry2 );
  }
}

void FastTimerCallback( int id )
{
  Led_SetState( ledState );
  ledState = !ledState;
}

void Make( )
{
  Timer_InitializeEntry( &timerEntry, TimerCallback, 0, 50, 1 );
  Timer_Set( &timerEntry );
  Timer_InitializeEntry( &timerEntry1, TimerCallback, 1, 120, 1 );
  Timer_Set( &timerEntry1 );
  Timer_InitializeEntry( &timerEntry2, TimerCallback, 2, 200, 1 );
  Timer_Set( &timerEntry2 );
  Timer_InitializeEntry( &timerEntry3, TimerCallback, 3, 2000, 1 );
  Timer_Set( &timerEntry3 );

  //FastTimer_InitializeEntry( &fastTimerEntry, FastTimerCallback, 0, 5000, 1 );
  //FastTimer_Set( &fastTimerEntry );

  //Stepper_SetPosition( 0, 1000 );

  TaskCreate( BlinkTask, "Blink", 400, 0, 1 );

  // Do this right quick after booting up - otherwise we won't be recognised
  Usb_SetActive( 1 );

  // Active the Poly Function Task
  Poly_SetActive( true );

  // Fire up the OSC system
  Osc_SetActive( true );
  // Add all the subsystems (make sure OSC_SUBSYSTEM_COUNT is large enough to accomodate them all)
  Osc_RegisterSubsystem( 0, AppLedOsc_GetName(), AppLedOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 1, DipSwitchOsc_GetName(), DipSwitchOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 2, ServoOsc_GetName(), ServoOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 3, AnalogInOsc_GetName(), AnalogInOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 4, DigitalOutOsc_GetName(), DigitalOutOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 5, DigitalInOsc_GetName(), DigitalInOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 6, MotorOsc_GetName(), MotorOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 7, PwmOutOsc_GetName(), PwmOutOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 8, LedOsc_GetName(), LedOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 9, DebugOsc_GetName(), DebugOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 10, SystemOsc_GetName(), SystemOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 11, NetworkOsc_GetName(), NetworkOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 12, SerialOsc_GetName(), SerialOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 13, IoOsc_GetName(), IoOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( 14, StepperOsc_GetName(), StepperOsc_ReceiveMessage, NULL );

  // Permit DIP switches to change the base IP settings
  NetworkCheck();

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

  int pos = 500;
  int speed = 0;

  //Servo_SetPosition( 0, pos );

  while ( true )
  {
    //Servo_SetPosition( 0, pos );
    //pos = ( pos == 0 ) ? 780 : 0;
    Sleep( 10 );

    //int speedNew = AnalogIn_GetValue( 0 );
    //if ( speedNew != speed )
    //{
    //  speed = speedNew;
    //  Stepper_SetSpeed( 0, ( ( speed * speed ) + 1 ) * 50  );
    //}
    //Led_SetState( 0 );
    //Sleep( 90 );
    //Led_SetState( 1 );
    //Sleep( 10 );
  }
}



// Make sure the network settings are OK
void NetworkCheck()
{
  // Check to see if there's an override from the dip switch
  // The DIP switch would need to be 
  //   1 2 3 4 5 6 7 8
  //   x X y Y 1 1 1 0
  // Where Xx & Yy are added to the base address
  //   192.168.0+Yy.200+Xx
  int sw = DipSwitch_GetValue();
  int dipload = ( sw & 0xF0 ) == 0x70;

  // if either the network settings were invalid or the dipswitch is set to load
  if ( !Network_GetValid( ) || dipload )
  {
    // we're shooting for 192.168.0.200
    int a3 = 0;
    int a4 = 200;

    // the dip switch can offset the loaded addresses a little bit
    if ( dipload )
    {
      // 0000Yy00 
      a3 += ( sw & 0x0C ) >> 2;
      // 000000Xx 
      a4 += ( sw & 0x03 );
    }

    Network_SetAddress( 192, 168, a3, a4 );
    Network_SetMask( 255, 255, 255, 0 );
    Network_SetGateway( 192, 168, a3, 1 );
    Network_SetValid( 1 );
  }
}
