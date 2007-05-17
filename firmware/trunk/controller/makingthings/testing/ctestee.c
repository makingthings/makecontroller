// MakingThings - Make Controller Kit - 2006

/** \file ctestee.c	
	Controller Testee
	Functions for working with the status LED on the Make Controller Board.
*/

#include "ctestee.h"
#include "debug.h"
#include <stdio.h>
#include "osc.h"
#include "config.h"

static int CTestee_Init( void );

#define CTESTEE_IOS 30

int CTestee_Io[ CTESTEE_IOS ] =
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

struct CTestee_
{
  char init;
  int  ioPattern;
  int  canOut;
} CTesteeData;

int CTestee_BootFromFlash( void );
int CTestee_EepromTest( void );
void CanPowerDown( void );
void CanPowerUp( void );
void CanSendDominant( void );
void CanSendNothing( void );
int CanReceive( void );

/** \defgroup CTestee
* CTestee Subsystem of the Make Controller Kit.
* \ingroup ControllerBoard
* @{
*/

/**
	CTestee_SetTesteePower.
	Sets whether the specified LED is active. \n
	@param state 0 = off, 1 = V+ Power, 2 = 3.3V Power
	@return Zero on success.
*/
int CTestee_SetIoPattern( int ioPattern )
{
  if ( !CTesteeData.init )
    CTestee_Init();

  CTesteeData.ioPattern = ioPattern;

  int i;

  switch ( ioPattern )
  {
    case 0:
      for ( i = 0; i < CTESTEE_IOS; i++ )
      {
        int io = CTestee_Io[ i ];
        Io_SetFalse( io ); 
      }
      break;
    case 1:
      for ( i = 0; i < CTESTEE_IOS; i++ )
      {
        int io = CTestee_Io[ i ];
        Io_SetTrue( io ); 
      }
      break;
  }       
  
  return 0;
}

/**
	CTestee_GetIoPattern.
	@return state.
*/
int CTestee_GetIoPattern( )
{
  if ( !CTesteeData.init )
    CTestee_Init();
  
  return CTesteeData.ioPattern;
}


int CTestee_GetCanOut( )
{
  if ( !CTesteeData.init )
    CTestee_Init();
  
  return CTesteeData.canOut;
}

int CTestee_SetCanOut( int canOut )
{
  if ( !CTesteeData.init )
    CTestee_Init();

  CTesteeData.canOut = canOut;
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
  if ( !CTesteeData.init )
    CTestee_Init();
  
  return CanReceive();
}

void CanPowerDown()
{
  Io_SetTrue( IO_PA07 );
}

void CanPowerUp()
{
  Io_SetFalse( IO_PA07 );
}

void CanSendDominant()
{
  Io_SetFalse( IO_PA20 );
}

void CanSendNothing()
{
  Io_SetTrue( IO_PA20 );
}

int CanReceive()
{
  return Io_GetValue( IO_PA19 );
}


/** @}
*/

int CTestee_Init( )
{
  // CAN 
  // RS - Controls speed, etc.
  Io_Start( IO_PA07, true );
  Io_PullupDisable( IO_PA07 );
  Io_PioEnable( IO_PA07 );
  Io_SetOutput( IO_PA07 );
  Io_SetTrue( IO_PA07 );

  // RxD - Receive Data
  Io_Start( IO_PA19, true );
  Io_PullupDisable( IO_PA19 );
  Io_PioEnable( IO_PA19 );
  Io_SetInput( IO_PA19 );

  // TxD - Transmit Data
  Io_Start( IO_PA20, true );
  Io_PullupDisable( IO_PA20 );
  Io_PioEnable( IO_PA20 );
  Io_SetOutput( IO_PA20 );
  Io_SetTrue( IO_PA20 );

  CanPowerDown();
  CanSendNothing();

  CTesteeData.canOut = 0;

  CTesteeData.init = true;

  int i;
  for ( i = 0; i < CTESTEE_IOS; i++ )
  {
    int io = CTestee_Io[ i ];
    Io_Start( io, false );
    Io_SetOutput( io ); 
    Io_PullupDisable( io );
    Io_SetFalse( io ); 
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
  Eeprom_Write( address, (char*)&value, 4 );
  value = 0;
  Eeprom_Read( address, (char*)&value, 4 );

  if ( value != 10 )
    return 0;

  value = -10;
  Eeprom_Write( address, (char*)&value, 4 );
  value = 0;
  Eeprom_Read( address, (char*)&value, 4 );

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
      if ( !CTesteeData.init )
        CTestee_Init();
      if ( value == 0 )
      {
        CTestee_BootFromFlash( );
      }
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
      if ( !CTesteeData.init )
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


