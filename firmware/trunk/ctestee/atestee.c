// MakingThings - Make Controller Kit - 2006

/** \file atestee.c	
	Controller Testee
	Functions for working with the status LED on the Make Controller Board.
*/

#include "atestee.h"
#include "debug.h"
#include <stdio.h>
#include "osc.h"
#include "config.h"
#include "adc.h"

static int ATestee_Init( void );
static int ATestee_Test_All( int enable, int outputs, int lower, int upper );

struct ATestee_
{
  char init;
  int  test;
} ATesteeData;

void ATestee_PrepareOutput( int output );

#define ATESTEE_INS 8
#define ATESTEE_OUTS 8
#define ATESTEE_OUT_ENABLES 4

#define ATESTEE_0_OUT IO_PA24
#define ATESTEE_1_OUT IO_PA05
#define ATESTEE_2_OUT IO_PA06
#define ATESTEE_3_OUT IO_PA02
#define ATESTEE_4_OUT IO_PB25
#define ATESTEE_5_OUT IO_PA25
#define ATESTEE_6_OUT IO_PA26
#define ATESTEE_7_OUT IO_PB23
#define ATESTEE_01_ENABLE IO_PB19
#define ATESTEE_23_ENABLE IO_PB20
#define ATESTEE_45_ENABLE IO_PB21
#define ATESTEE_67_ENABLE IO_PB22

int ATestee_Out[ ATESTEE_OUTS ] =
{
  ATESTEE_0_OUT,  
  ATESTEE_1_OUT,  
  ATESTEE_2_OUT,  
  ATESTEE_3_OUT,  
  ATESTEE_4_OUT,  
  ATESTEE_5_OUT,  
  ATESTEE_6_OUT,  
  ATESTEE_7_OUT  
};

int ATestee_OutEnable[ ATESTEE_OUT_ENABLES ] =
{
  ATESTEE_01_ENABLE,  
  ATESTEE_23_ENABLE,  
  ATESTEE_45_ENABLE,  
  ATESTEE_67_ENABLE,  
};

int ATestee_BootFromFlash( void );
void __reset_handler( void );

/** \defgroup ATestee
* ATestee Subsystem of the Make Controller Kit.
* \ingroup ControllerBoard
* @{
*/

/**
	ATestee_SetTest.
	@param test
	@return Zero on success.
*/
int ATestee_SetTest( int test)
{
  if ( !ATesteeData.init )
    ATestee_Init();

  ATesteeData.test = test;

  return CONTROLLER_OK;
}

/**
	ATestee_GetTest.
	@return test.
*/
int ATestee_GetTest( )
{
  if ( !ATesteeData.init )
    ATestee_Init();
  
  return ATesteeData.test;
}

/**
	ATestee_GetTestResult.
	@return state.
*/
int ATestee_GetTestResult( )
{
  int result;

  if ( !ATesteeData.init )
    ATestee_Init();

  switch ( ATesteeData.test )
  {
    case 0:
      // Enable off and outputs off should give near zero
      result = ATestee_Test_All( 0, 0, 0, 2 );
      break;
    case 1:
      // Enable off and outputs on should give near zero
      result = ATestee_Test_All( 0, 1, 0, 2 );
      break;
    case 2:
      // Enable on and outputs off should give near zero
      result = ATestee_Test_All( 1, 0, 20, 40 );
      break;
    case 3:
      // Enable on and outputs on should give 275 - being ( 4.6V * 0.19 ) / 3.3V * 1024
      result = ATestee_Test_All( 1, 1, 250, 300 );
      break;
    default:
      result = -1;
  }       

  return result;
}

int ATestee_Test_All( int enables, int outputs, int lower, int upper )
{
  int result = 0;
  int fault = 1;
  int i;

  // Setup
  // ... all outputs off
  for ( i = 0; i < ATESTEE_OUTS; i++ )
  {
    int io = ATestee_Out[ i ];
    Io_SetValue( io, outputs );
  }
  // ... all enables off
  for ( i = 0; i < ATESTEE_OUT_ENABLES; i++ )
  {
    int io = ATestee_OutEnable[ i ];
    Io_SetValue( io, enables );
  }
  
  // Test
  for ( i = 0; i < 8; i++ )
  {
    int v = Adc_GetValue( i );
    if ( v < lower || v > upper )
      result |= fault;
    fault <<= 1;
  }

  return result;
}

/** @}
*/

int ATestee_Init( )
{
  ATesteeData.init = true;

  int i;

  // Setup
  // ... all outputs are PIO's, Not pulled up, etc. 
  for ( i = 0; i < ATESTEE_OUTS; i++ )
  {
    int io = ATestee_Out[ i ];
    ATestee_PrepareOutput( io );
  }
  // ... all enables are PIO's, Not pulled up, etc. 
  for ( i = 0; i < ATESTEE_OUT_ENABLES; i++ )
  {
    int io = ATestee_OutEnable[ i ];
    ATestee_PrepareOutput( io );
  }
  // ... all ADC's on
  for ( i = 0; i < ATESTEE_INS; i++ )
  {
    Adc_SetActive( i, true );
  }

  return 0;
}

int ATestee_BootFromFlash()
{
  // This doesn't work.  It needs to run from RAM.
  // Maybe we need an IO line for this...

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

void ATestee_PrepareOutput( int output )
{
  Io_Start( output, true );
  Io_SetOutput( output );
  Io_PullupDisable( output );
  Io_PioEnable( output );
  Io_SetValue( output, false );
}

#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* ATesteeOsc_Name = "atestee";
static char* ATesteeOsc_PropertyNames[] = { "active", "test", "testresult", 0 }; // must have a trailing 0

int ATesteeOsc_PropertySet( int property, int value );
int ATesteeOsc_PropertyGet( int property );

// Returns the name of the subsystem
const char* ATesteeOsc_GetName( )
{
  return ATesteeOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int ATesteeOsc_ReceiveMessage( int channel, char* message, int length )
{
  return Osc_IntReceiverHelper( channel, message, length, 
                                ATesteeOsc_Name,
                                ATesteeOsc_PropertySet, ATesteeOsc_PropertyGet, 
                                ATesteeOsc_PropertyNames );
}

// Set the indexed property
int ATesteeOsc_PropertySet( int property, int value )
{
  switch ( property )
  {
    case 0:
      if ( !ATesteeData.init )
        ATestee_Init();
      if ( value == 0 )
      {
        ATestee_BootFromFlash( );
      }
      if ( value == 2 )
      {
        __reset_handler();
      }
      break;
    case 1: 
      ATestee_SetTest( value );
      break;      
    case 2: 
      break;        
  }
  return CONTROLLER_OK;
}

// Get the property
int ATesteeOsc_PropertyGet( int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      if ( !ATesteeData.init )
        ATestee_Init();
      value = 1;
      break;
    case 1:
      value = ATestee_GetTest( );
      break;  
    case 2:
      value = ATestee_GetTestResult();
      break;
  }
  
  return value;
}

