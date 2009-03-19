// MakingThings - Make Controller Kit - 2006

#include "debug.h"
#include <stdio.h>
#include "osc.h"
#include "config.h"
#include "ctester.h"

static int CTester_CalculateCurrent( int index );

void CanPowerDown( void );
void CanPowerUp( void );
void CanSendDominant( void );
void CanSendNothing( void );
int CanReceive( void );

static int CTester_Init( void );

#if (CONTROLLER_VERSION <= 100)
#define CTESTER_IOS 30
#elif (CONTROLLER_VERSION >= 200)
#define CTESTER_IOS 32
#endif

int CTester_Io[ CTESTER_IOS ] =
{
  IO_PA00,
  IO_PA01,
  IO_PA02,
  IO_PA03,
  IO_PA04,
  IO_PA05,
  IO_PA06,
  IO_PA08,
  IO_PA13,
  IO_PA14,
  IO_PA15,
  #if (CONTROLLER_VERSION >= 200)
  IO_PA19,
  IO_PA20,
  #endif
  IO_PA23,
  IO_PA24,
  IO_PA25,
  IO_PA26,
  IO_PA27,
  IO_PA28,
  IO_PA29,
  IO_PA30,
  IO_PB19,
  IO_PB20,
  IO_PB21,
  IO_PB22,
  IO_PB23,
  IO_PB24,
  IO_PB25,
  IO_PB27,
  IO_PB28,
  IO_PB29,
  IO_PB30
};


#define DEBUG_MAX_MESSAGE 200

struct CTester_
{
  char init;
  int  testeePower;
  int  ioPattern;
  int  canOut;
} CTesterData;


int CTester_SetTesteePower( int state )
{
  if ( !CTesterData.init )
    CTester_Init();
  
  CTesterData.testeePower = state;

  switch ( state )
  {
    case 0:
      Io_SetValue( IO_PA21, false );
      Io_SetValue( IO_PA22, false );
      break;
    case 1:
      Io_SetValue( IO_PA21, true );
      Io_SetValue( IO_PA22, false );
      break;
    case 2:
      Io_SetValue( IO_PA21, false );
      Io_SetValue( IO_PA22, true );
      break;
  }

  return 0;
}

int CTester_GetTesteePower( )
{
  if ( !CTesterData.init )
    CTester_Init();
  
  return CTesterData.testeePower;
}


int CTester_GetTesteeCurrent( )
{
  if ( !CTesterData.init )
    CTester_Init();
  
  return CTester_CalculateCurrent( 4 );
}


int CTester_GetTesteeVoltage( )
{
  if ( !CTesterData.init )
    CTester_Init();
  
  int a = AnalogIn_GetValue( 5 );
  // x2 because of the voltage divider
  float c = 2 * 3.3 * a / 1.024; 
  return (int)c;
}


int CTester_SetIoPattern( int ioPattern )
{
  if ( !CTesterData.init )
    CTester_Init();

  CTesterData.ioPattern = ioPattern;
  return 0;
}

int CTester_GetIoTest()
{
  if ( !CTesterData.init )
    CTester_Init();

  int result = 0;
  int fault = 1;
  int i;

  switch ( CTesterData.ioPattern )
  {
    case 0:
      for ( i = 0; i < CTESTER_IOS; i++ )
      {
        int io = CTester_Io[ i ];
        if ( Io_GetValue( io ) )
          result |= fault;
        fault <<= 1;
      }
      break;
    case 1:
      for ( i = 0; i < CTESTER_IOS; i++ )
      {
        int io = CTester_Io[ i ];
        if ( !Io_GetValue( io ) )
          result |= fault;
        fault <<= 1;
      }
      break;
  }       
  
  return result;
}

int CTester_GetIoPattern( )
{
  if ( !CTesterData.init )
    CTester_Init();
  
  return CTesterData.ioPattern;
}


int CTester_GetCanOut( )
{
  if ( !CTesterData.init )
    CTester_Init();
  
  return CTesterData.canOut;
}

int CTester_SetCanOut( int canOut )
{
  if ( !CTesterData.init )
    CTester_Init();

  CTesterData.canOut = canOut;
  switch ( canOut )
  {
    case 0:
      CanPowerDown();
      CanSendNothing();
      break;
    case 1:
      CanPowerUp();
      CanSendNothing();
      break;
    case 2:
      CanPowerUp();
      CanSendDominant();
      break;
  }
  return 0;
}

int CTester_GetCanIn( )
{
  if ( !CTesterData.init )
    CTester_Init();
  
  return CanReceive();
}

void CanPowerDown()
{
  Io_SetValue( IO_PA07, true );
}

void CanPowerUp()
{
  Io_SetValue( IO_PA07, false );
}

void CanSendDominant()
{
  Io_SetValue( IO_PA20, false );
}

void CanSendNothing()
{
  Io_SetValue( IO_PA20, true );
}

int CanReceive()
{
  return Io_GetValue( IO_PA19 );
}


int CTester_Init( )
{
  // Power control
  Io_Start( IO_PA21, true );
  Io_Start( IO_PA22, true );
  Io_SetDirection( IO_PA21, true );
  Io_SetDirection( IO_PA22, true );
  Io_SetPullup( IO_PA21, false );
  Io_SetPullup( IO_PA22, false );
  Io_SetPio( IO_PA21, true );
  Io_SetPio( IO_PA22, true );
  Io_SetValue( IO_PA21, false );
  Io_SetValue( IO_PA22, false );

  // Current
  AnalogIn_GetValue( 4 );

  // Voltage
  AnalogIn_GetValue( 5 );

  // CAN 
  // RS - Controls speed, etc.
  #if (CONTROLLER_VERSION <= 100)
  Io_Start( IO_PA07, true );
  Io_SetPullup( IO_PA07, false );
  Io_SetPio( IO_PA07, true );
  Io_SetDirection( IO_PA07, true );
  Io_SetValue( IO_PA07, true );

  // RxD - Receive Data
  Io_Start( IO_PA19, true );
  Io_SetPullup( IO_PA19, false );
  Io_SetPio( IO_PA19, true );
  Io_SetDirection( IO_PA19, false );

  // TxD - Transmit Data
  Io_Start( IO_PA20, true );
  Io_SetPullup( IO_PA20, false );
  Io_SetPio( IO_PA20, true );
  Io_SetDirection( IO_PA20, true );
  Io_SetValue( IO_PA20, true );

  CanPowerDown();
  CanSendNothing();
  #endif // CONTROLLER_VERSION <= 100

  CTesterData.init = true;

  CTesterData.canOut = 0;

  int i;
  for ( i = 0; i < CTESTER_IOS; i++ )
  {
    int io = CTester_Io[ i ];
    Io_Start( io, true );
    Io_SetPio( io, true );
    Io_SetDirection( io, false ); 
    Io_SetPullup( io, false );
  }

  return 0;
}

int CTester_CalculateCurrent( int index )
{
  /*
    A/D Converter Reference voltage is 3.3V
    Current Sense Reference voltage (that from which the actual voltage output is subtracted) is 2.5V
    Gain is 50V/V

    So current is:
    Vad4 = 3.3 * ad4 / 1024
    I = ( 2.5 - Vad4 ) / 50 / 0.1
    = ( 2.5 - Vad4 ) / 5
  */
  float voltage = ( 3.3 * AnalogIn_GetValue( index ) / 1024.0 );
  float c = ((2.5 - voltage) / 5) * 1000; // * 1000 for milliamps
  return (int)c;
}



#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* CTesterOsc_Name = "ctester";
static char* CTesterOsc_PropertyNames[] = { "active", "testeepower", "testeecurrent", "testvoltage", 
                                            "iopattern", "iotest", "canout", "canin", 0 }; // must have a trailing 0

int CTesterOsc_PropertySet( int property, int value );
int CTesterOsc_PropertyGet( int property );

// Returns the name of the subsystem
const char* CTesterOsc_GetName( )
{
  return CTesterOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int CTesterOsc_ReceiveMessage( int channel, char* message, int length )
{
  return Osc_IntReceiverHelper( channel, message, length, 
                                CTesterOsc_Name,
                                CTesterOsc_PropertySet, CTesterOsc_PropertyGet, 
                                CTesterOsc_PropertyNames );
}

// Set the indexed property
int CTesterOsc_PropertySet( int property, int value )
{
  switch ( property )
  {
    case 1: 
      CTester_SetTesteePower( value );
      break;      
    case 4:
      CTester_SetIoPattern( value );
      break;
    case 6:
      CTester_SetCanOut( value );
      break;
  }
  return CONTROLLER_OK;
}

// Get the property
int CTesterOsc_PropertyGet( int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      value = 1;
      break;
    case 1:
      value = CTester_GetTesteePower( );
      break;
    case 2:
      value = CTester_GetTesteeCurrent( );
      break;
    case 3:
      value = CTester_GetTesteeVoltage( );
      break;
    case 4:
      value = CTester_GetIoPattern( );
      break;
    case 5:
      value = CTester_GetIoTest( );
      break;
    case 6:
      value = CTester_GetCanOut( );
      break;
    case 7:
      value = CTester_GetCanIn( );
      break;
  }
  
  return value;
}



