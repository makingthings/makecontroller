/*
	make.c
	
	make.c is the main project file.  The Run( ) task gets called on bootup, so stick any initialization stuff in there.
	In Heavy, by default we set the USB, OSC, and Network systems active, but you don't need to if you aren't using them.
	Furthermore, only register the OSC subsystems you need - by default, we register all of them.
*/

#include "core.h"
#include "led.h"
#include "appled.h"

void blinkLoop( void* parameters );

void Run( ) // this task gets called as soon as we boot up.
{
  new Task( blinkLoop, "Blink", 400, 1 );

  UsbSerial* usb = UsbSerial::get();
  Network* net = Network::get();
  
  // Fire up the OSC system and register the subsystems you want to use
//  Osc_SetActive( true, true, true, true );
//  // make sure OSC_SUBSYSTEM_COUNT (osc.h) is large enough to accomodate them all
//  //Osc_RegisterSubsystem( AppLedOsc_GetName(), AppLedOsc_ReceiveMessage, NULL );
//  Osc_RegisterSubsystem( DipSwitchOsc_GetName(), DipSwitchOsc_ReceiveMessage, DipSwitchOsc_Async );
//  Osc_RegisterSubsystem( ServoOsc_GetName(), ServoOsc_ReceiveMessage, NULL );
//  Osc_RegisterSubsystem( AnalogInOsc_GetName(), AnalogInOsc_ReceiveMessage, AnalogInOsc_Async );
//  Osc_RegisterSubsystem( DigitalOutOsc_GetName(), DigitalOutOsc_ReceiveMessage, NULL );
//  Osc_RegisterSubsystem( DigitalInOsc_GetName(), DigitalInOsc_ReceiveMessage, NULL );
//  Osc_RegisterSubsystem( MotorOsc_GetName(), MotorOsc_ReceiveMessage, NULL );
//  Osc_RegisterSubsystem( PwmOutOsc_GetName(), PwmOutOsc_ReceiveMessage, NULL );
//  //Osc_RegisterSubsystem( LedOsc_GetName(), LedOsc_ReceiveMessage, NULL );
//  Osc_RegisterSubsystem( DebugOsc_GetName(), DebugOsc_ReceiveMessage, NULL );
//  Osc_RegisterSubsystem( SystemOsc_GetName(), SystemOsc_ReceiveMessage, NULL );
//  Osc_RegisterSubsystem( NetworkOsc_GetName(), NetworkOsc_ReceiveMessage, NULL );
//  Osc_RegisterSubsystem( SerialOsc_GetName(), SerialOsc_ReceiveMessage, NULL );
//  Osc_RegisterSubsystem( IoOsc_GetName(), IoOsc_ReceiveMessage, NULL );
//  Osc_RegisterSubsystem( StepperOsc_GetName(), StepperOsc_ReceiveMessage, NULL );
//  Osc_RegisterSubsystem( XBeeOsc_GetName(), XBeeOsc_ReceiveMessage, XBeeOsc_Async );
//  Osc_RegisterSubsystem( XBeeConfigOsc_GetName(), XBeeConfigOsc_ReceiveMessage, NULL );
//  Osc_RegisterSubsystem( WebServerOsc_GetName(), WebServerOsc_ReceiveMessage, NULL );

  // Starts the network up.  Will not return until a network is found...
  // Network_SetActive( true );
}

// A very simple task...a good starting point for programming experiments.
// If you do anything more exciting than blink the LED in this task, however,
// you may need to increase the stack allocated to it above.
void blinkLoop( void* p )
{
  (void)p;
  Led led;
  AppLed al0(0), al1(1), al2(2), al3(3);
  led.setState( 1 );
  Task::sleep( 1000 );

  while ( true )
  {
    led.setState( 0 );
    al0.setState( 0 );
    al1.setState( 0 );
    al2.setState( 0 );
    al3.setState( 0 );
    Task::sleep( 900 );
    led.setState( 1 );
    al0.setState( 1 );
    al1.setState( 1 );
    al2.setState( 1 );
    al3.setState( 1 );
    Task::sleep( 10 ); 
  }
}




