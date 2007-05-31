/*********************************************************************************

 Copyright 2006 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/


/** \file io.c	
	Subsystem for manipulating the general purpose IO lines.
*/

/* Library includes */


/* Scheduler includes */

/* Hardware specific headers */
#include "Board.h"
#include "AT91SAM7X256.h"

#include "io.h"
#include "config.h"

/* Local Structures */

struct IoPin_
{
  uchar users : 7 ;
  uchar lock : 1 ;
} __attribute__((packed));

#define IO_PIN_COUNT 64
#define IO_USERS_MAX 127

typedef struct IoPin_ IoPin ;

struct Io_
{
  int users;
  int init; 
  int portAMask;
  int portBMask;
  IoPin pins[ IO_PIN_COUNT ];
} Io;

// 260?

static void Io_Init( void );
static void Io_Deinit( void );

/* Testing
static int Io_TestDataStructure( void );
static int Io_TestInitialState( void );
static int Io_TestStartStop( void );
static int Io_TestStartStopBits( void );
static int Io_TestPins( short users, bool lock );
*/

/** \defgroup Io
	A mechanism to manage the 32 parallel io lines on the controller.

  Control of the SAM7X's IO lines is more complex than most microcontrollers.  All 
  lines may be inputs or outputs, have pull-ups or not, have a glitch filter enabled 
  or not, and so on.  In addition all IO pins serve at least double and sometimes 
  triple duty, being general IO lines and also being IO lines for one or more of 
  the controller's many on-chip peripherals.  All these functions can be controlled
  from the Io subsystem.

  This API can be used in two ways - either one line at a time or many at a time.  
  Functions supporting the latter are suffixed with "Bits".  So the one-at-a-time 
  mechanism for setting an IO to true is called Io_SetTrue().  It takes an index and
  the ones for setting
  a number of lines is Io_SetTrueBits( ).

  The pattern of use is as follows:  prior to using an IO line, Call Io_Start() on it, the 
  return value indicates if the start operation was successful.  After this, call any 
  of the other Io functions.

  \todo Consider changing the api slightly to permit the immediate use of the io
        without starting it.
  \todo Add glitch filter control
  \todo Implement more bits-style functions
  \ingroup Controller
  @{
*/

int Io_Start( int index, bool lock )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;
  IoPin* p = &Io.pins[ index ];
  // If it's not locked at all and 
  //    either no lock is requested or no a lock is requested but no other are using the io and
  //    there are not a huge number of users
  if ( !p->lock && ( !lock || ( lock && p->users == 0 ) ) && ( p->users < IO_USERS_MAX - 2 ) )
  {
    if ( Io.users == 0 )
      Io_Init();
   
    // Lock it up.
    Io.users++;
    p->users++;
    if ( lock )
      p->lock = true;

    // Output Pair 1
    // Switch to PIO
    // AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA26;
    // AT91C_BASE_PIOB->PIO_PER = AT91C_PIO_PB23;
    
    return CONTROLLER_OK;
  }
  else
    return CONTROLLER_ERROR_TOO_MANY_USERS;
}

int Io_Stop( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;
  IoPin* p = &Io.pins[ index ];
  if ( p->users > 0 )
  {
    Io.users--;
    p->users--;
    p->lock = false;

    if ( Io.users == 0 )
      Io_Deinit();

    return CONTROLLER_OK;
  }
  else
    return CONTROLLER_ERROR_TOO_MANY_STOPS;
}

int Io_StartBits( longlong bits, bool lock )
{
  IoPin* p = &Io.pins[ 0 ];
  int i = 0;
  int failure = -1;
  ullong mask = 1LL;
  for ( i = 0; i < 64; i++ )
  {
    if ( bits & mask )
    {
      if ( !p->lock && ( !lock || ( lock && p->users == 0 ) ) && p->users < IO_USERS_MAX - 2 )
      {
        if ( Io.users == 0 )
          Io_Init();
        if ( lock )
          p->lock = true;
        p->users++;
        Io.users++;
      }
      else
      {
        failure = i;
        break;
      }
    }
    mask <<= 1;
    p++;
  }

  if ( failure != -1 )
  {
    IoPin* p = &Io.pins[ 0 ];
    mask = 1LL;
    for ( i = 0; i < failure; i++ )
    {
      if ( bits & mask )
      {
        if ( lock )
          p->lock = false;
        p->users--;
        Io.users--;
        if ( Io.users == 0 )
          Io_Deinit();
      }
      mask <<= 1;
      p++;
    }
    return CONTROLLER_ERROR_CANT_LOCK;
  }

  return CONTROLLER_OK;
}

int  Io_StopBits( longlong bits )
{
  IoPin* p = &Io.pins[ 0 ];
  int i = 0;
  ullong mask = 1LL;
  for ( i = 0; i < 64; i++ )
  {
    if ( bits & mask )
    {
        p->lock = false;
        p->users--;
        Io.users--;

        if ( Io.users == 0 )
          Io_Deinit();
    }
    mask <<= 1;
    p++;
  }
  return CONTROLLER_OK;
}

int Io_SetActive( int index, int value )
{
  if ( value )
    return Io_Start( index, false );
  else
    return Io_Stop( index );
  return CONTROLLER_OK;
}

int Io_GetActive( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;
  IoPin* p = &Io.pins[ index ];
  return p->users > 0;
}

int Io_SetDirection( int index, int output )
{
  if ( output )
    return Io_SetOutput( index );
  else
    return Io_SetInput( index );
}

int Io_GetDirection( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( index < 32 ) 
    return ( AT91C_BASE_PIOA->PIO_OSR & ( 1 << index ) ) != 0;
  else
    return ( AT91C_BASE_PIOB->PIO_OSR & ( 1 << ( index & 0x1F ) ) ) != 0;
}

int Io_SetOutput( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  // Switch to outputs
  if ( index < 32 ) 
    AT91C_BASE_PIOA->PIO_OER = 1 << index;
  else
    AT91C_BASE_PIOB->PIO_OER = 1 << ( index & 0x1F );

  return CONTROLLER_OK;
}

int Io_SetInput( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  // Switch to inputs
  if ( index < 32 ) 
    AT91C_BASE_PIOA->PIO_ODR = 1 << index;
  else
    AT91C_BASE_PIOB->PIO_ODR = 1 << ( index & 0x1F );

  return CONTROLLER_OK;
}


int Io_SetTrue( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
      AT91C_BASE_PIOA->PIO_SODR = mask;
  else
      AT91C_BASE_PIOB->PIO_SODR = mask;
  
  return CONTROLLER_OK;
}


int Io_SetFalse( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
      AT91C_BASE_PIOA->PIO_CODR = mask;
  else
      AT91C_BASE_PIOB->PIO_CODR = mask;
  
  return CONTROLLER_OK;
}

int Io_SetValue( int index, char value )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  // Switch to outputs
  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
  {
    if ( value )
      AT91C_BASE_PIOA->PIO_SODR = mask;
    else
      AT91C_BASE_PIOA->PIO_CODR = mask;
  }
  else
  {
    if ( value )
      AT91C_BASE_PIOB->PIO_SODR = mask;
    else
      AT91C_BASE_PIOB->PIO_CODR = mask;
  }
  
  return CONTROLLER_OK;
}

char Io_GetValue( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return 0;
  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
    return ( ( AT91C_BASE_PIOA->PIO_PDSR & mask ) != 0 ) ? 1 : 0;
  else
    return ( ( AT91C_BASE_PIOB->PIO_PDSR & mask ) != 0 ) ? 1 : 0;
}

int  Io_SetPeripheralA( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
      AT91C_BASE_PIOA->PIO_ASR = mask;
  else
      AT91C_BASE_PIOB->PIO_ASR = mask;
  
  return CONTROLLER_OK;
}

int  Io_SetPeripheralB( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
      AT91C_BASE_PIOA->PIO_BSR = mask;
  else
      AT91C_BASE_PIOB->PIO_BSR = mask;
  
  return CONTROLLER_OK;
}


int Io_SetPio( int index, int enable )
{
  if ( enable )
    return Io_PioEnable( index );
  else
    return Io_PioDisable( index );
}

int Io_GetPio( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( index < 32 ) 
    return ( AT91C_BASE_PIOA->PIO_PSR & ( 1 << index ) ) != 0;
  else
    return ( AT91C_BASE_PIOB->PIO_PSR & ( 1 << ( index & 0x1F ) ) ) != 0;
}

int  Io_PioEnable( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
      AT91C_BASE_PIOA->PIO_PER = mask;
  else
      AT91C_BASE_PIOB->PIO_PER = mask;
  
  return CONTROLLER_OK;
}

int  Io_PioDisable( int index )
{  
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
      AT91C_BASE_PIOA->PIO_PDR = mask;
  else
      AT91C_BASE_PIOB->PIO_PDR = mask;
  
  return CONTROLLER_OK;
}

int Io_SetPullup( int index, int enable )
{
  if ( enable )
    return Io_PullupEnable( index );
  else
    return Io_PullupDisable( index );
}

int Io_GetPullup( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  // The PullUp status register is inverted.
  if ( index < 32 ) 
    return ( AT91C_BASE_PIOA->PIO_PPUSR & ( 1 << index ) ) == 0;
  else
    return ( AT91C_BASE_PIOB->PIO_PPUSR & ( 1 << ( index & 0x1F ) ) ) == 0;
}

int  Io_PullupEnable( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
      AT91C_BASE_PIOA->PIO_PPUER = mask;
  else
      AT91C_BASE_PIOB->PIO_PPUER = mask;
  
  return CONTROLLER_OK;
}

int  Io_PullupDisable( int index )
{  
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
      AT91C_BASE_PIOA->PIO_PPUDR = mask;
  else
      AT91C_BASE_PIOB->PIO_PPUDR = mask;
  
  return CONTROLLER_OK;
}

void Io_SetTrueBits( longlong bits )
{
  AT91C_BASE_PIOA->PIO_SODR = bits >> 32; 
  AT91C_BASE_PIOB->PIO_SODR = bits & 0xFFFFFFFF;
}

void Io_SetFalseBits( longlong bits )
{
  AT91C_BASE_PIOA->PIO_CODR = bits >> 32;
  AT91C_BASE_PIOB->PIO_CODR = bits & 0xFFFFFFFF;
}

void Io_SetValueBits( longlong bits, longlong values )
{
  int aBits = bits >> 32;
  int aValues = values >> 32;
  int bBits = bits & 0xFFFFFFFF;
  int bValues = values & 0xFFFFFFFF;
  AT91C_BASE_PIOA->PIO_SODR = (int)( aBits & aValues );
  AT91C_BASE_PIOA->PIO_CODR = (int)( aBits & ~aValues  );
  AT91C_BASE_PIOB->PIO_SODR = (int)( bBits & bValues );
  AT91C_BASE_PIOB->PIO_CODR = (int)( bBits & ~bValues  );
}

void Io_SetOutputBits( longlong bits )
{
  AT91C_BASE_PIOA->PIO_OER = bits >> 32;
  AT91C_BASE_PIOB->PIO_OER = bits & 0xFFFFFFFF;
}

void Io_SetInputBits( longlong bits )
{
  AT91C_BASE_PIOA->PIO_ODR = bits >> 32;
  AT91C_BASE_PIOB->PIO_ODR = bits & 0xFFFFFFFF;
}

int Io_SetPortA( int value )
{
  AT91C_BASE_PIOA->PIO_SODR = (int)( Io.portAMask & value );
  AT91C_BASE_PIOA->PIO_CODR = (int)( Io.portAMask & ~value  );
  return CONTROLLER_OK;
}

int Io_GetPortA( )
{
  return Io.portAMask & AT91C_BASE_PIOA->PIO_PDSR;
}

int Io_SetPortB( int value )
{
  AT91C_BASE_PIOB->PIO_SODR = (int)( Io.portBMask & value );
  AT91C_BASE_PIOB->PIO_CODR = (int)( Io.portBMask & ~value  );
  return CONTROLLER_OK;
}

int Io_GetPortB( )
{
  return Io.portBMask & AT91C_BASE_PIOB->PIO_PDSR;
}

int Io_SetPortAMask( int value )
{
  Io.portAMask = value;
  return CONTROLLER_OK;
}

int Io_GetPortAMask( )
{
  return Io.portAMask;
}

int Io_SetPortBMask( int value )
{
  Io.portBMask = value;
  return CONTROLLER_OK;
}

int Io_GetPortBMask( )
{
  return Io.portBMask;
}

void Io_Init()
{
  Io.init++;

  	/* Enable the peripheral clock. */
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOA;
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOB;

  Io.portAMask = 0;
  Io.portBMask = 0;
}

void Io_Deinit()
{
  Io.init--;
}

longlong Io_GetValueBits( )
{
  return ( ((longlong)AT91C_BASE_PIOA->PIO_PDSR) << 32 ) || AT91C_BASE_PIOB->PIO_PDSR;
}

/** @}
*/

#ifdef OSC

/** \defgroup IoOSC IO - OSC
  Control any pin's direction and output from OSC.  Use with care!
  \ingroup OSC
	
	\section devices Devices
	There are 64 IO's on the Make Controller, numbered 0 - 63.  IO's 0 - 31 
  correspond to PortA, IO's 32 - 63 correspond to PortB.
	
	\section properties Properties
	The IO's have three properties - 'output', 'value' and 'active'.

	\par Output
	The 'output' property sets whether the IO is an output.\n
	For example, to make pin 4 an output, send a message like
	\verbatim /io/4/output 1\endverbatim
	Change the argument 1 to a 0 to make it an input.

	\par Value
	Writing the 'value' property sets the on/off value of a given IO.\n
	For example, to activate pin 4, send a message like
	\verbatim /io/4/value 1\endverbatim
	Change the argument 1 to a 0 to turn it off.
	Reading the 'value' property returns the value of a given IO.\n
	For example, to read pin 4, send a message like
	\verbatim /io/4/value\endverbatim
    
	\par Active
	The 'active' property corresponds to the active state of an IO.
	\verbatim /io/0/active \endverbatim
	\par
	You can set the active flag by sending
	\verbatim /io/0/active 1 \endverbatim
*/

#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* IoOsc_Name = "io";
static char* IoOsc_IndexIntPropertyNames[] = { "active", "value", "output", "pio", "pullup", 0 }; // must have a trailing 0

int IoOsc_IndexIntPropertySet( int index, int property, int value );
int IoOsc_IndexIntPropertyGet( int index, int property );

static char* IoOsc_IntPropertyNames[] = { "porta", "portamask", "portb", "portbmask", 0 }; // must have a trailing 0
int IoOsc_IntPropertySet( int property, int value );
int IoOsc_IntPropertyGet( int property );

// Returns the name of the subsystem
const char* IoOsc_GetName( )
{
  return IoOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int IoOsc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
                                           IO_PIN_COUNT, IoOsc_Name,
                                           IoOsc_IndexIntPropertySet, IoOsc_IndexIntPropertyGet, 
                                           IoOsc_IndexIntPropertyNames );
                                     
  if ( status != CONTROLLER_OK )
  {
    status = Osc_IntReceiverHelper( channel, message, length, 
                                             IoOsc_Name,
                                             IoOsc_IntPropertySet, IoOsc_IntPropertyGet, 
                                             IoOsc_IntPropertyNames );
  }
                                     
  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, IoOsc_Name, status );
  return CONTROLLER_OK;
}

// Set the index, property with the value
int IoOsc_IndexIntPropertySet( int index, int property, int value )
{
  switch ( property )
  {
    case 0: 
      Io_SetActive( index, value );
      break;      
    case 1: 
      Io_SetValue( index, value );
      break;
    case 2:
      Io_SetDirection( index, value );
      break;
    case 3:
      Io_SetPio( index, value );
      break;
    case 4:
      Io_SetPullup( index, value );
      break;  }
  return CONTROLLER_OK;
}

// Get the indexed property
int IoOsc_IndexIntPropertyGet( int index, int property )
{
  int value;
  switch ( property )
  {
    case 0:
      value = Io_GetActive( index );
      break;
    case 1:
      value = Io_GetValue( index );
      break;
    case 2:
      value = Io_GetDirection( index );
      break;
    case 3:
      value = Io_GetPio( index );
      break;
    case 4:
      value = Io_GetPullup( index );
      break;
  }
  
  return value;
}


// Set the property with the value
int IoOsc_IntPropertySet( int property, int value )
{
  switch ( property )
  {
    case 0: 
      Io_SetPortA( value );
      break;      
    case 1: 
      Io_SetPortAMask( value );
      break;
    case 2: 
      Io_SetPortB( value );
      break;      
    case 3: 
      Io_SetPortBMask( value );
      break;      
  }
  return CONTROLLER_OK;
}

// Get the indexed property
int IoOsc_IntPropertyGet( int property )
{
  int value;
  switch ( property )
  {
    case 0: 
      value = Io_GetPortA(  );
      break;      
    case 1: 
      value = Io_GetPortAMask(  );
      break;
    case 2: 
      value = Io_GetPortB(  );
      break;      
    case 3: 
      value = Io_GetPortBMask( );
      break;
  }
  return value;
}

#endif

/**	
	Io_Test.
	Tests the IO routines.  Also tests the initial state, so should be called before
  any Io routines are called.
	@return status - CONTROLLER_OK (0) for all OK, or an error code
*/
/*
int Io_Test()
{
  int status;

  // Check the data structure
  status = Io_TestDataStructure();
  if ( status != CONTROLLER_OK )
    return status;

  // Check the initial state of the sub-system
  status = Io_TestInitialState();
  if ( status != CONTROLLER_OK )
    return status;

  // Check to make sure Io pins can be started and stopped correctly
  status = Io_TestStartStop();
  if ( status != CONTROLLER_OK )
    return status;

  // Check to make sure Io pins can be started and stopped correctly using BITS
  status = Io_TestStartStopBits();
  if ( status != CONTROLLER_OK )
    return status;

  return CONTROLLER_OK;
}

int Io_TestDataStructure()
{
  // Check for correct packing
  int s = sizeof( struct Io_ ); 
  if ( s != IO_PIN_COUNT + 8 )
    return CONTROLLER_ERROR_DATA_STRUCTURE_SIZE_WRONG;
  return CONTROLLER_OK;
}

int Io_TestInitialState()
{  
  int status;

  // Total init count is zero
  if ( Io.init != 0 )
    return CONTROLLER_ERROR_INITIALIZATION;

  // Total user count is zero
  if ( Io.users != 0 )
    return CONTROLLER_ERROR_INITIALIZATION;

  // Each pin's user count and lock status is where it should be
  status = Io_TestPins( 0, false );
  if ( status != CONTROLLER_OK )
    return status;

  return CONTROLLER_OK;
}

int Io_TestStartStop()
{
  int status;
  int i;

  //
  // Start/Stop Test - first no lock
  //

  // Start with no lock
  for ( i = 0; i < IO_PIN_COUNT; i++ )
  {
    if ( Io_Start( i, false ) != CONTROLLER_OK )
      return CONTROLLER_ERROR_START_FAILED;
  }

  // Attempt to start the new pins again
  for ( i = 0; i < IO_PIN_COUNT; i++ )
  {
    if ( Io_Start( i, false ) != CONTROLLER_OK )
      return CONTROLLER_ERROR_START_FAILED;
  }

  // Make sure the init code has only been run once
  if ( Io.init != 1 )
    return CONTROLLER_ERROR_INCORRECT_INIT;

  // Make sure the global user count is equal to the number of starts
  if ( Io.users != IO_PIN_COUNT * 2 )
    return CONTROLLER_ERROR_COUNT_MISMATCH;

  // Check each pin record to make sure there' are two users there  
  status = Io_TestPins( 2, false );
  if ( status != CONTROLLER_OK )
    return status;


  // Now try to stop all the pins
  for ( i = 0; i < IO_PIN_COUNT; i++ )
  {
    if ( Io_Stop( i ) != CONTROLLER_OK )
      return CONTROLLER_ERROR_STOP_FAILED;
  }

  // Now try to stop all the pins again
  for ( i = 0; i < IO_PIN_COUNT; i++ )
  {
    if ( Io_Stop( i ) != CONTROLLER_OK )
      return CONTROLLER_ERROR_STOP_FAILED;
  }

  // Make sure the module de-initialized
  if ( Io.init != 0 )
    return CONTROLLER_ERROR_INCORRECT_DEINIT;

  // Make sure the users are back to zero
  if ( Io.users != 0 )
    return CONTROLLER_ERROR_COUNT_MISMATCH;

  // Make sure each pin is back to zero
  status = Io_TestPins( 0, false );
  if ( status != CONTROLLER_OK )
    return status;

  //
  // Start/Stop Test - with lock
  //

  // Start with starting each pin
  for ( i = 0; i < IO_PIN_COUNT; i++ )
  {
    if ( Io_Start( i, true ) != CONTROLLER_OK )
      return CONTROLLER_ERROR_START_FAILED;
  }

  // Make sure the init code has been run again
  if ( Io.init != 1 )
    return CONTROLLER_ERROR_INCORRECT_INIT;

  // Make sure the global user count is equal to the number of starts
  if ( Io.users != IO_PIN_COUNT )
    return CONTROLLER_ERROR_COUNT_MISMATCH;

  // Check each pin record to make sure there's one locked user
  status = Io_TestPins( 1, true );
  if ( status != CONTROLLER_OK )
    return status;

  // Now try to use them - no lock, shouldn't work
  for ( i = 0; i < IO_PIN_COUNT; i++ )
  {
    if ( Io_Start( i, false ) == CONTROLLER_OK )
      return CONTROLLER_ERROR_USE_GRANTED_ERROR;
  }

  // Now try to use them - with lock, shouldn't work
  for ( i = 0; i < IO_PIN_COUNT; i++ )
  {
    if ( Io_Start( i, true ) == CONTROLLER_OK )
      return CONTROLLER_ERROR_LOCK_GRANTED_ERROR;
  }

  // Now try to stop all the pins again
  for ( i = 0; i < IO_PIN_COUNT; i++ )
  {
    if ( Io_Stop( i ) != CONTROLLER_OK )
      return CONTROLLER_ERROR_STOP_FAILED;
  }

  // Make sure the module de-initialized
  if ( Io.init != 0 )
    return CONTROLLER_ERROR_INCORRECT_DEINIT;

  // Make sure the users are back to zero
  if ( Io.users != 0 )
    return CONTROLLER_ERROR_COUNT_MISMATCH;

  // Make sure each pin is back to zero and not locked
  status = Io_TestPins( 0, false );
  if ( status != CONTROLLER_OK )
    return status;

  return CONTROLLER_OK;
}

int Io_TestStartStopBits()
{
  int status;

  //
  // Start/Stop Bits Test - first no lock
  //
  llong all = 0xFFFFFFFFFFFFFFFFLL;
  //llong half1 = 0x00000000FFFFFFFFLL;
  //llong half2 = 0xFFFFFFFF00000000LL;

  // Start with no lock
  if ( Io_StartBits( all, false ) != CONTROLLER_OK )
      return CONTROLLER_ERROR_START_FAILED;

  // Start again
  if ( Io_StartBits( all, false ) != CONTROLLER_OK )
      return CONTROLLER_ERROR_START_FAILED;

  // Make sure the init code has only been run once
  if ( Io.init != 1 )
    return CONTROLLER_ERROR_INCORRECT_INIT;

  // Make sure the global user count is equal to the number of starts
  if ( Io.users != IO_PIN_COUNT * 2 )
    return CONTROLLER_ERROR_COUNT_MISMATCH;

  // Check each pin record to make sure there' are two users there  
  status = Io_TestPins( 2, false );
  if ( status != CONTROLLER_OK )
    return status;

  // Now try to stop all the pins
  if ( Io_StopBits( all ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_STOP_FAILED;

  // Now try to stop all the pins again
  if ( Io_StopBits( all ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_STOP_FAILED;

  // Make sure the module de-initialized
  if ( Io.init != 0 )
    return CONTROLLER_ERROR_INCORRECT_DEINIT;

  // Make sure the users are back to zero
  if ( Io.users != 0 )
    return CONTROLLER_ERROR_COUNT_MISMATCH;

  // Make sure each pin is back to zero
  status = Io_TestPins( 0, false );
  if ( status != CONTROLLER_OK )
    return status;

  //
  // Start/Stop Bits Test - with lock
  //

  // Start with starting each pin
  if ( Io_StartBits( all, true ) != CONTROLLER_OK )
      return CONTROLLER_ERROR_START_FAILED;

  // Make sure the init code has been run again
  if ( Io.init != 1 )
    return CONTROLLER_ERROR_INCORRECT_INIT;

  // Make sure the global user count is equal to the number of starts
  if ( Io.users != IO_PIN_COUNT )
    return CONTROLLER_ERROR_COUNT_MISMATCH;

  // Check each pin record to make sure there's one locked user
  status = Io_TestPins( 1, true );
  if ( status != CONTROLLER_OK )
    return status;

  // Now try to use them - no lock, shouldn't work
  if ( Io_StartBits( all, false ) == CONTROLLER_OK )
    return CONTROLLER_ERROR_LOCK_GRANTED_ERROR;

  // Now try to use them - with lock, shouldn't work
  if ( Io_StartBits( all, true ) == CONTROLLER_OK )
    return CONTROLLER_ERROR_LOCK_GRANTED_ERROR;

  // Now try to stop all the pins again
  if ( Io_StopBits( all ) != CONTROLLER_OK )
    return CONTROLLER_ERROR_STOP_FAILED;

  // Make sure the module de-initialized
  if ( Io.init != 0 )
    return CONTROLLER_ERROR_INCORRECT_DEINIT;

  // Make sure the users are back to zero
  if ( Io.users != 0 )
    return CONTROLLER_ERROR_COUNT_MISMATCH;

  // Make sure each pin is back to zero and not locked
  status = Io_TestPins( 0, false );
  if ( status != CONTROLLER_OK )
    return status;

  // 
  // Check individual bits and unwind on error
  //

  status = Io_StartBits( 3LL<<19, false );
  if ( status != CONTROLLER_OK )
    return status;

  // Make sure the init code has been run again
  if ( Io.init != 1 )
    return CONTROLLER_ERROR_INCORRECT_INIT;

  // Make sure the global user count is equal to the number of starts
  if ( Io.users != 2 )
    return CONTROLLER_ERROR_COUNT_MISMATCH;

  // try to lock them all up - lock blocking, shouldn't work
  if ( Io_StartBits( all, true ) == CONTROLLER_OK )
    return CONTROLLER_ERROR_LOCK_GRANTED_ERROR;

  // Make sure the code is still inited
  if ( Io.init != 1 )
    return CONTROLLER_ERROR_INCORRECT_INIT;

  // Make sure the global user count is equal to the number of starts
  if ( Io.users != 2 )
    return CONTROLLER_ERROR_COUNT_MISMATCH;

  // Turn the one bit off
  status = Io_StopBits( 3LL<<19 );
  if ( status != CONTROLLER_OK )
    return status;

  // Make sure the code is un-inited
  if ( Io.init != 0 )
    return CONTROLLER_ERROR_INCORRECT_INIT;

  // Make sure the global user count is zero
  if ( Io.users != 0 )
    return CONTROLLER_ERROR_COUNT_MISMATCH;

  status = Io_StartBits( 3LL<<19, true );
  if ( status != CONTROLLER_OK )
    return status;
  
  // try to lock them all up - lock blocking, shouldn't work
  if ( Io_StartBits( all, true ) == CONTROLLER_OK )
    return CONTROLLER_ERROR_LOCK_GRANTED_ERROR;

  // Make sure the code is still inited
  if ( Io.init != 1 )
    return CONTROLLER_ERROR_INCORRECT_INIT;

  // Make sure the global user count is equal to the number of correct starts
  if ( Io.users != 2 )
    return CONTROLLER_ERROR_COUNT_MISMATCH;

  // Turn the bits off
  status = Io_StopBits( 3LL<<19 );
  if ( status != CONTROLLER_OK )
    return status;

  // Make sure the code is un-inited
  if ( Io.init != 0 )
    return CONTROLLER_ERROR_INCORRECT_INIT;

  // Make sure the global user count is zero
  if ( Io.users != 0 )
    return CONTROLLER_ERROR_COUNT_MISMATCH;


  return CONTROLLER_OK;
}

int Io_TestPins( short users, bool lock )
{
  // Make sure each pin is correct
  IoPin* p = &Io.pins[ 0 ];
  int i;
  for ( i = 0; i < IO_PIN_COUNT; i++ )
  {
    if ( p->users != users )
      return CONTROLLER_ERROR_WRONG_USER_COUNT;
    if ( p->lock != lock )
      return CONTROLLER_ERROR_LOCK_ERROR;
  }

  return CONTROLLER_OK;
}
*/
