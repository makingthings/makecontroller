/*
  OSC-Heavy, MakingThings 2008
  
  Heavy is a general purpose program that exposes many of the features on the 
  Make Controller through OSC so that other devices can control its operation
  remotely.
  
  The OSC system internally starts up tasks that listen on the USB and Ethernet
  systems and route messages to systems that have been registered.
*/

// include all the libraries we'll be using
#include "config.h"
#include "xbee.h"
#include "webserver.h"
#include "stepper.h"
#include "servo.h"
#include "pwmout.h"
#include "motor.h"
#include "digitalout.h"
#include "digitalin.h"
#include "dipswitch.h"
#include "appled.h"

void BlinkTask( void* p );

void Run( ) // this task gets called as soon as we boot up.
{
  Usb_SetActive(1); // turn on USB
  TaskCreate( BlinkTask, "Blink", 1000, 0, 3 );
  
  // fire up the OSC system, enabling USB, UDP and autosend tasks
  Osc_SetActive( true, true, true, true );
  
  // now register all the subsystems we want to use with OSC
  Osc_RegisterSubsystem( AppLedOsc_GetName(), AppLedOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( DipSwitchOsc_GetName(), DipSwitchOsc_ReceiveMessage, DipSwitchOsc_Async );
  Osc_RegisterSubsystem( ServoOsc_GetName(), ServoOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( AnalogInOsc_GetName(), AnalogInOsc_ReceiveMessage, AnalogInOsc_Async );
  Osc_RegisterSubsystem( DigitalOutOsc_GetName(), DigitalOutOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( DigitalInOsc_GetName(), DigitalInOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( MotorOsc_GetName(), MotorOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( PwmOutOsc_GetName(), PwmOutOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( LedOsc_GetName(), LedOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( DebugOsc_GetName(), DebugOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( SystemOsc_GetName(), SystemOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( NetworkOsc_GetName(), NetworkOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( SerialOsc_GetName(), SerialOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( IoOsc_GetName(), IoOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( StepperOsc_GetName(), StepperOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( XBeeOsc_GetName(), XBeeOsc_ReceiveMessage, XBeeOsc_Async );
  Osc_RegisterSubsystem( XBeeConfigOsc_GetName(), XBeeConfigOsc_ReceiveMessage, NULL );
  Osc_RegisterSubsystem( WebServerOsc_GetName(), WebServerOsc_ReceiveMessage, NULL );
  
  // finally, start up the network system
  Network_SetActive(1);
  WebServer_SetActive(1);
}

// sit in a loop blinking the Controller Board LED on and off
// to provide a "heartbeat" for the board
void BlinkTask( void* p )
{
  (void)p; // unused variable
  Led_SetState(1);
  Sleep(1000);

  while( true )
  {
    Led_SetState(0);
    Sleep(990);
    Led_SetState(1);
    Sleep(10);
  }
}


