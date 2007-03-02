/*
	Make.c
*/

#include "stdlib.h"
#include "config.h"
#include "serial.h"
#include "string.h"
#include "lwip/dhcp.h" // <-- move all this stuff into Network, eventually

#define IP_ADDRESS( a, b, c, d ) ( ( (int)a << 24 ) + ( (int)b << 16 ) + ( (int)c << 8 ) + (int)d )

void BlinkTask( void* parameters );
void NetworkCheck( void );
void vBasicWEBServer( void *pvParameters );
void TcpSenderTask( void* p );
void DhcpFineTask( void* p );
void DhcpCoarseTask( void* p );

void Make( )
{
  TaskCreate( BlinkTask, "Blink", 50, 0, 1 );
  //TaskCreate( TcpSenderTask, "TcpSend", 400, NULL, 4 );

  // Do this right quick after booting up - otherwise we won't be recognised
  Usb_SetActive( 1 );

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
  //Osc_RegisterSubsystem( 8, LedOsc_GetName(), LedOsc_ReceiveMessage, NULL );
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

  while ( true )
  {
    Led_SetState( 0 );
    Sleep( 900 );
    Led_SetState( 1 );
    Sleep( 10 );
  }
}

void TcpSenderTask( void* p )
{
  (void)p;
  int count = 0;
  int result = -1;
  bool ledstate;

  Sleep( 1000 );
  // Chill until the Network is up
  while ( !Network_GetActive() )
    Sleep( 100 );

  unsigned int addr = IP_ADDRESS( 192, 168, 0, 7 );
  //unsigned int addr = IP_ADDRESS( 53, 0, 168, 192 );
  struct netconn* sock = NULL;

  while( true )
  {
    // check to see if we have an open socket
    if( sock == NULL )
      sock = Socket( addr, 10101 );

    if( sock != NULL )
    {
      int dummy = 128;
      char msg[128];
      char readbuf[128];

      Osc_CreateMessageToBuf( msg + 4, &dummy, "/foo", ",i", count++ );
      int length = 128 - dummy;
      if( length != 16 )
        count--;
      *((int*)msg) = length;
    
      if( !SocketWrite( sock, msg, length + 4 ) )
      {
        SocketClose( sock );
        sock = NULL;
      }

      result = SocketRead( sock, readbuf, 6 );
      if( strcmp( readbuf, "Hello" ) == 0 )
      {
        ledstate = !ledstate;
        AppLed_SetState( 0, ledstate );
      }
        
      if( !result )
      {
        SocketClose( sock );
        sock = NULL;
      }
    }
    Sleep( 100 );
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
