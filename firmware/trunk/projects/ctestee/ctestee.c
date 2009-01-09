// MakingThings - Make Controller Kit - 2006

/** \file ctestee.c	
	Controller Testee
	Functions for working with the status LED on the Make Controller Board.
*/

#include "debug.h"
#include <stdio.h>
#include "osc.h"
#include "config.h"
#include "AT91SAM7X256.h"
#include "ctestee.h"
#include "io.h"

#if (CONTROLLER_VERSION <= 100)
#define CTESTEE_IOS 30
#elif (CONTROLLER_VERSION >= 200)
#define CTESTEE_IOS 32
#endif

typedef struct CTestee_
{
  int  ioPattern;
  int  canOut;
  int Io[ CTESTEE_IOS ];
} CTestee;
CTestee* CTesteeData;

static int CTestee_Init( void );
int CTestee_BootFromFlash( void );
int CTestee_EepromTest( void );
void CanPowerDown( void );
void CanPowerUp( void );
void CanSendDominant( void );
void CanSendNothing( void );
int CanReceive( void );

int CTestee_SetIoPattern( int ioPattern )
{
  if ( CTesteeData == NULL )
    CTestee_Init();

  CTesteeData->ioPattern = ioPattern;

  int i;

  switch ( ioPattern )
  {
    case 0:
      for ( i = 0; i < CTESTEE_IOS; i++ )
      {
        int io = CTesteeData->Io[ i ];
        Io_SetValue( io, false );
      }
      break;
    case 1:
      for ( i = 0; i < CTESTEE_IOS; i++ )
      {
        int io = CTesteeData->Io[ i ];
        Io_SetValue( io, true ); 
      }
      break;
  }       
  
  return 0;
}

int CTestee_GetIoPattern( )
{
  if ( CTesteeData == NULL )
    CTestee_Init();
  
  return CTesteeData->ioPattern;
}


int CTestee_GetCanOut( )
{
  if ( CTesteeData == NULL )
    CTestee_Init();
  
  return CTesteeData->canOut;
}

int CTestee_SetCanOut( int canOut )
{
  if ( CTesteeData == NULL )
    CTestee_Init();

  CTesteeData->canOut = canOut;
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

int CTestee_GetCanIn( )
{
  if ( CTesteeData == NULL )
    CTestee_Init();
  
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


/** @}
*/

int CTestee_Init( )
{
  if( CTesteeData != NULL )
    return 0;

  CTesteeData = Malloc( sizeof( CTestee ) );
  int i = 0;
  CTesteeData->Io[ i++ ] = IO_PA00;
  CTesteeData->Io[ i++ ] = IO_PA01;
  CTesteeData->Io[ i++ ] = IO_PA02;
  CTesteeData->Io[ i++ ] = IO_PA03;
  CTesteeData->Io[ i++ ] = IO_PA04;
  CTesteeData->Io[ i++ ] = IO_PA05;
  CTesteeData->Io[ i++ ] = IO_PA06;
  CTesteeData->Io[ i++ ] = IO_PA08;
  CTesteeData->Io[ i++ ] = IO_PA13;
  CTesteeData->Io[ i++ ] = IO_PA14;
  CTesteeData->Io[ i++ ] = IO_PA15;
  #if (CONTROLLER_VERSION >= 200)
  CTesteeData->Io[ i++ ] = IO_PA19;
  CTesteeData->Io[ i++ ] = IO_PA20;
  #endif
  CTesteeData->Io[ i++ ] = IO_PA23;
  CTesteeData->Io[ i++ ] = IO_PA24;
  CTesteeData->Io[ i++ ] = IO_PA25;
  CTesteeData->Io[ i++ ] = IO_PA26;
  CTesteeData->Io[ i++ ] = IO_PA27;
  CTesteeData->Io[ i++ ] = IO_PA28;
  #if (CONTROLLER_VERSION <= 100)
  CTesteeData->Io[ i++ ] = IO_PA29;
  CTesteeData->Io[ i++ ] = IO_PA30;
  #elif (CONTROLLER_VERSION <= 200)
  CTesteeData->Io[ i++ ] = IO_PA10;
  CTesteeData->Io[ i++ ] = IO_PA11;
  #endif
  CTesteeData->Io[ i++ ] = IO_PB19;
  CTesteeData->Io[ i++ ] = IO_PB20;
  CTesteeData->Io[ i++ ] = IO_PB21;
  CTesteeData->Io[ i++ ] = IO_PB22;
  CTesteeData->Io[ i++ ] = IO_PB23;
  CTesteeData->Io[ i++ ] = IO_PB24;
  CTesteeData->Io[ i++ ] = IO_PB25;
  CTesteeData->Io[ i++ ] = IO_PB27;
  CTesteeData->Io[ i++ ] = IO_PB28;
  CTesteeData->Io[ i++ ] = IO_PB29;
  CTesteeData->Io[ i++ ] = IO_PB30;
  
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
  #endif // #if (CONTROLLER_VERSION <= 100)

  CTesteeData->canOut = 0;

  for ( i = 0; i < CTESTEE_IOS; i++ )
  {
    int io = CTesteeData->Io[ i ];
    Io_Start( io, false );
    Io_SetPio( io, true );
    Io_SetDirection( io, true ); 
    Io_SetPullup( io, false );
    Io_SetValue( io, false );
  }
  return 0;
}

int CTestee_BootFromFlash()
{
	// Wait for End Of Programming
	while( !(AT91C_BASE_MC->MC_FSR & AT91C_MC_FRDY) );
	
	// Send Boot From Flash Command
	AT91C_BASE_MC->MC_FCR = (AT91C_MC_FCMD_CLR_GP_NVM |  (( (2) << 8) & AT91C_MC_PAGEN) | (0x5A << 24));

  // Wait for End Of Programming
	while( !(AT91C_BASE_MC->MC_FSR & AT91C_MC_FRDY) );

  if ( AT91C_BASE_MC->MC_FSR & AT91C_MC_PROGE )
    return 0;

  if ( AT91C_BASE_MC->MC_FSR & AT91C_MC_GPNVM2 )
    return 0;

  return 1;
}

int CTestee_EepromTest()
{
  int address = 10;
  int value;

  value = 10;
  Eeprom_Write( address, (uchar*)&value, 4 );
  value = 0;
  Eeprom_Read( address, (uchar*)&value, 4 );

  if ( value != 10 )
    return 0;

  value = -10;
  Eeprom_Write( address, (uchar*)&value, 4 );
  value = 0;
  Eeprom_Read( address, (uchar*)&value, 4 );

  if ( value != -10 )
    return 0;

  return 1;

}

#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* CTesteeOsc_Name = "ctestee";
static char* CTesteeOsc_PropertyNames[] = { "active", "iopattern", "eepromtest", "canout", "canin", 0 }; // must have a trailing 0

int CTesteeOsc_PropertySet( int property, int value );
int CTesteeOsc_PropertyGet( int property );

// Returns the name of the subsystem
const char* CTesteeOsc_GetName( )
{
  return CTesteeOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int CTesteeOsc_ReceiveMessage( int channel, char* message, int length )
{
  return Osc_IntReceiverHelper( channel, message, length, 
                                CTesteeOsc_Name,
                                CTesteeOsc_PropertySet, CTesteeOsc_PropertyGet, 
                                CTesteeOsc_PropertyNames );
}

// Set the indexed property
int CTesteeOsc_PropertySet( int property, int value )
{
  switch ( property )
  {
    case 0:
      if ( CTesteeData == NULL )
        CTestee_Init();
      if ( value == 0 )
        CTestee_BootFromFlash( );
      break;
    case 1: 
      CTestee_SetIoPattern( value );
      break;      
    case 3: 
      CTestee_SetCanOut( value );
      break;      
  }
  return CONTROLLER_OK;
}

// Get the property
int CTesteeOsc_PropertyGet( int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      if ( CTesteeData == NULL )
        CTestee_Init();
      value = 1;
      break;
    case 1:
      value = CTestee_GetIoPattern( );
      break;
    case 2: 
      value = CTestee_EepromTest( );
      break; 
    case 3: 
      value = CTestee_GetCanOut( );
      break;              
    case 4: 
      value = CTestee_GetCanIn( );
      break;              
  }
  
  return value;
}



