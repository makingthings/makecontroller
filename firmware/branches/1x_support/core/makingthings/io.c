/*********************************************************************************

 Copyright 2006-2008 MakingThings

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

static void Io_Init( void );
static void Io_Deinit( void );
static int Io_SetOutput( int index );
static int Io_SetInput( int index );
static int Io_PullupEnable( int index );
static int Io_PullupDisable( int index );
static int Io_PioEnable( int index );
static int Io_PioDisable( int index );

/** \defgroup Io IO
	A mechanism to manage the 64 IO lines on the controller.

  The 64 IO (Input/Output) lines on the Make Controller are grouped into 2 ports of 32 lines each, 
  port A and port B.  Each line  has many parameters that can be configured:
   - whether it's an input or an output
   - whether it's turned on or off (if it's an output)
   - whether the internal pullup is enabled
   - whether the glitch filter is enabled
   - and more...
  
  In addition, all IO pins serve at least double and sometimes triple duty, being general IO lines 
  and also being IO lines for one or more of the controller's many on-chip peripherals, such as the
  Ethernet system, USB system, SPI, UART, etc.  To help address this, the IO system provides a mechanism to "lock"
  the IO lines, so that you can always be sure that your line has not been altered by some other
  system while you weren't looking.
  
  \section api IO API 
  This API can be used in two ways - either one line at a time or many at a time.  Functions to set
  the values of all the lines at once end in \b Bits.  For example, the one-at-a-time 
  mechanism for setting an IO line configured as an output to true is called Io_SetTrue().  
  It takes a single index and the one for setting a number of lines is Io_SetTrueBits( ) which takes a
  mask with the values of all the lines.

  To find the index to use for a single line, please see \ref IoIndices.  For the constants to help
  create masks for the \b Bits style, please see \ref IoBits.

  \section use Normal Use
  The pattern of use is to call Io_Start() prior to using an IO line.  If it returns successfully, the 
  line was not previously locked and you can use it as you please.  Otherwise, you don't have access to the 
  IO line, and should not make any calls to configure or use it.

  \todo Add glitch filter control
  \ingroup Core
  @{
*/

/**
  Get access to an IO line, possibly locking it.
  @param index The IO line to start - use the appropriate entry from \ref IoIndices
  @param lock Whether to lock this line from being used by another system.
  @return 0 on success, < 0 on failure

  \b Example

  \code
  if( Io_Start(IO_PA18, true) == CONTROLLER_OK)
  {
    // then we have access to PA18 and successfully locked it
  }
  else
    // can't use PA18
  \endcode
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
    
    return CONTROLLER_OK;
  }
  else
    return CONTROLLER_ERROR_TOO_MANY_USERS;
}

/**
  Release your lock on an IO line, possibly deactivating it.
  When you call Io_Stop(), it will remove the lock you placed on the line.  If no other
  systems are using it, the line will be deactivated.
  @param index The IO line to stop - use the appropriate entry from \ref IoIndices
  @return 0 on success, < 0 on failure

  \b Example

  \code
  Io_Stop(IO_PA18);
  \endcode
*/
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

/**
  Read whether an IO pin is in use.
  @param index An int specifying which IO line.  Use the appropriate entry from the \ref IoIndices
  @return non-zero if active, 0 if inactive
  
  \b Example
  \code
  if( Io_GetActive( IO_PA23 ) )
  {
		// it's already active
  }
  else
  {
    // not yet active
  }
  \endcode
*/
bool Io_GetActive( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;
  IoPin* p = &Io.pins[ index ];
  return p->users > 0;
}

/**
  Set whether an IO line is an output or an input.
  Use the IO_INPUT and IO_OUTPUT symbols to avoid confusion.
  @param index An int specifying which IO line.  Use the appropriate entry from the \ref IoIndices
  @param output Specify 1 for an output, or 0 for an input.
  @return CONTROLLER_OK (0) on success, non-zero otherwise.
  
  \b Example
  \code
  // Set io23 to an input
  if( Io_SetDirection( IO_PA23, IO_INPUT ) == CONTROLLER_OK )
  {
		// success
  }
  \endcode
*/
int Io_SetDirection( int index, bool output )
{
  if ( output )
    return Io_SetOutput( index );
  else
    return Io_SetInput( index );
}

/**
  Read whether an IO line is an output or an input.
  @param index An int specifying which IO line.  Use the appropriate entry from the \ref IoIndices
  @return Non-zero if the pin is an output, 0 if it's an input.
  
  \b Example
  \code
  // Check whether IO 23 is an output or input
  if( Io_GetDirection( IO_PA23 ) )
  {
		// then we're an output
  }
  else
  {
		// we're an input
  }
  \endcode
*/
bool Io_GetDirection( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( index < 32 ) 
    return ( AT91C_BASE_PIOA->PIO_OSR & ( 1 << index ) ) != 0;
  else
    return ( AT91C_BASE_PIOB->PIO_OSR & ( 1 << ( index & 0x1F ) ) ) != 0;
}

// static
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

// static
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

 /**
  Turn an IO line on or off.
	This IO should have already been set to be an output via Io_SetDirection( )
  @param index An int specifying which IO line.  Use the appropriate entry from the \ref IoIndices
	@param value Non-zero for on, 0 for off.
  @return CONTROLLER_OK (0) on success, otherwise non-zero.
  
  \b Example
  \code
  // Turn on IO 17
  Io_SetValue( IO_PA17, 1 );
  \endcode
*/
int Io_SetValue( int index, bool value )
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

 /**
  Read whether an IO line, presumably set as an output, is on or off.
  @param index An int specifying which IO line.  Use the appropriate entry from the \ref IoIndices
  @return CONTROLLER_OK (0) on success, otherwise non-zero.
  
  \b Example
  \code
  // Turn on IO 17
  Io_SetValue( IO_PA17, 1 );
  \endcode
*/
bool Io_GetValue( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return 0;
  int mask = 1 << ( index & 0x1F );
  if ( index < 32 )
    return ( ( AT91C_BASE_PIOA->PIO_PDSR & mask ) != 0 ) ? 1 : 0;
  else
    return ( ( AT91C_BASE_PIOB->PIO_PDSR & mask ) != 0 ) ? 1 : 0;
}

/**
  Configure an IO line to be part of its peripheral A.
  @param index An int specifying which IO line.  Use the appropriate entry from the \ref IoIndices
  @return 0 on success, non-zero on failure

  \b Example
  \code
  Io_SetPeripheralA( IO_PA18);
  \endcode
*/
int Io_SetPeripheralA( int index )
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

/**
  Configure an IO line to be part of its peripheral B.
  @param index An int specifying which IO line.  Use the appropriate entry from the \ref IoIndices
  @return 0 on success, non-zero on failure

  \b Example
  \code
  Io_SetPeripheralB( IO_PA18);
  \endcode
*/
int Io_SetPeripheralB( int index )
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

/**
  Configure an IO line to be a general purpose IO.
  @param index An int specifying which IO line.  Use the appropriate entry from the \ref IoIndices
  @param enable Whether to enable a pin as a PIO or disable it, reverting to an unconfigured state.
  @return 0 on success, non-zero on failure

  \b Example
  \code
  Io_SetPio( IO_PA18);
  \endcode
*/
int Io_SetPio( int index, bool enable )
{
  if ( enable )
    return Io_PioEnable( index );
  else
    return Io_PioDisable( index );
}

/**
  Read whether an IO line is configured as a general purpose IO.
  @param index An int specifying which IO line.  Use the appropriate entry from the \ref IoIndices
  @return true if it is a PIO, false if it's not.

  \b Example
  \code
  if( Io_GetPio( IO_PA18) )
  {
    // then we know PA18 is a PIO
  }
  \endcode
*/
bool Io_GetPio( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( index < 32 ) 
    return ( AT91C_BASE_PIOA->PIO_PSR & ( 1 << index ) ) != 0;
  else
    return ( AT91C_BASE_PIOB->PIO_PSR & ( 1 << ( index & 0x1F ) ) ) != 0;
}

// static
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

// static
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

 /**
  Set the pullup resistor for an IO line on or off.
  @param index An int specifying which IO line.  Use the appropriate entry from the \ref IoIndices
	@param enable Non-zero for on, 0 for off.
  @return CONTROLLER_OK (0) on success, otherwise non-zero.
  
  \b Example
  \code
  // Turn on the pullup for IO 17
  Io_SetPullup( IO_PA17, 1 );
  \endcode
*/
int Io_SetPullup( int index, bool enable )
{
  if ( enable )
    return Io_PullupEnable( index );
  else
    return Io_PullupDisable( index );
}

 /**
  Read whether the pullup resistor for an IO line on or off.
  @param index An int specifying which IO line.  Use the appropriate entry from the \ref IoIndices
  @return CONTROLLER_OK (0) on success, otherwise non-zero.
  
  \b Example
  \code
  // Turn on the pullup for IO 17
  Io_GetPullup( IO_PA17 );
  \endcode
*/
bool Io_GetPullup( int index )
{
  if ( index < 0 || index > IO_PIN_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  // The PullUp status register is inverted.
  if ( index < 32 ) 
    return ( AT91C_BASE_PIOA->PIO_PPUSR & ( 1 << index ) ) == 0;
  else
    return ( AT91C_BASE_PIOB->PIO_PPUSR & ( 1 << ( index & 0x1F ) ) ) == 0;
}

// static
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

// static
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

/*
  Enable/disable IO pullups by the batch.
  @param bits The bitmask containing the IO lines you'd like to configure - see \ref IoBits.  Must be
  a longlong to accommodate all 64 bits.
  @param enable true to enable, false to disable the pullup

  \b Example
  \code
  longlong mymask = 0;
  mymask |= (IO_PA18_BIT | IO_PA00_BIT | IO_PB12_BIT );
  Io_SetPullupBits( mymask, enable ); // enable those pullups
  \endcode
*/
void Io_SetPullupBits( longlong bits, bool enable )
{
  if(enable)
  {
    AT91C_BASE_PIOB->PIO_PPUER = bits >> 32;
    AT91C_BASE_PIOA->PIO_PPUER = bits & 0xFFFFFFFF;
  }
  else
  {
    AT91C_BASE_PIOB->PIO_PPUDR = bits >> 32;
    AT91C_BASE_PIOA->PIO_PPUDR = bits & 0xFFFFFFFF;
  }
}

/**
  Set the values of a batch of IO lines at once.
  @param bits The bitmask containing the IO lines you'd like to configure - see \ref IoBits.  Must be
  a longlong to accommodate all 64 bits.
  @param values The mask of 0 or non-0 values that you'd like to write into those lines.
*/
void Io_SetValueBits( longlong bits, longlong values )
{
  int aBits = bits & 0xFFFFFFFF;
  int aValues = values & 0xFFFFFFFF;
  int bBits = bits >> 32;
  int bValues = values >> 32;

  AT91C_BASE_PIOA->PIO_SODR = (int)( aBits & aValues );
  AT91C_BASE_PIOA->PIO_CODR = (int)( aBits & ~aValues  );
  AT91C_BASE_PIOB->PIO_SODR = (int)( bBits & bValues );
  AT91C_BASE_PIOB->PIO_CODR = (int)( bBits & ~bValues  );
}

/**
  Set a batch of IO lines to a particular direction (in or out) at once.
  @param bits The bitmask containing the IO lines you'd like to configure - see \ref IoBits.  Must be
  a longlong to accommodate all 64 bits.
  @param output true to set the lines as outputs, false to set them as inputs
*/
void Io_SetDirectionBits( longlong bits, bool output )
{
  if(output)
  {
    AT91C_BASE_PIOA->PIO_OER = bits & 0xFFFFFFFF;
    AT91C_BASE_PIOB->PIO_OER = bits >> 32;
  }
  else
  {
    AT91C_BASE_PIOA->PIO_ODR = bits & 0xFFFFFFFF;
    AT91C_BASE_PIOB->PIO_ODR = bits >> 32;
  }
}

/**
  Set a batch of IO lines to being general IOs at once.
  @param bits The bitmask containing the IO lines you'd like to configure - see \ref IoBits.  Must be
  a longlong to accommodate all 64 bits.
  @param enable true to configure the lines as PIOs, false un-configure them
*/
void Io_SetPioBits( longlong bits, bool enable )
{
  if(enable)
  {
    AT91C_BASE_PIOA->PIO_PER = bits & 0xFFFFFFFF;
    AT91C_BASE_PIOB->PIO_PER = bits >> 32;
  }
  else
  {
    AT91C_BASE_PIOA->PIO_PER = bits & 0xFFFFFFFF;
    AT91C_BASE_PIOB->PIO_PER = bits >> 32;
  }
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

// static
void Io_Init()
{
  Io.init++;

  	/* Enable the peripheral clock. */
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOA;
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOB;

  Io.portAMask = 0;
  Io.portBMask = 0;
}

// static
void Io_Deinit()
{
  Io.init--;
}

/**
  Get a bitmask with the output values of all the IO lines.
  @return A longlong (64 bit) value with the values of the output lines.

  \b Example
  \code
  longlong values = Io_GetValueBits( );
  if( values & IO_PA18_BIT )
  {
    // then we know PA18 is configured as an output
  }
  \endcode
*/
longlong Io_GetValueBits( )
{
  return ( ((longlong)AT91C_BASE_PIOB->PIO_PDSR) << 32 ) | AT91C_BASE_PIOA->PIO_PDSR;
}

/**
  Get a bitmask with the state of the internal pullup for all the IO lines.
  @return A longlong (64 bit) value with the status of the pullups.

  \b Example
  \code
  longlong pullups = Io_GetPullupBits( );
  if( pullups & IO_PA18_BIT )
  {
    // then we know PA18 has its pullup turned on
  }
  \endcode
*/
longlong Io_GetPullupBits( )
{
  return ( ((longlong)AT91C_BASE_PIOB->PIO_PPUSR) << 32 ) | AT91C_BASE_PIOA->PIO_PPUSR;
}

/**
  Get a bitmask indicating PIO configuration for all the IO lines.
  @return A longlong (64 bit) value specifying which lines are configured as PIOs.

  \b Example
  \code
  longlong pios = Io_GetPioBits( );
  if( pios & IO_PA18_BIT )
  {
    // then we know PA18 is configured as a PIO
  }
  \endcode
*/
longlong Io_GetPioBits( )
{
  return ( ((longlong)AT91C_BASE_PIOB->PIO_PSR) << 32 ) | AT91C_BASE_PIOA->PIO_PSR;
}

/**
  Get a bitmask indicating the in-or-out configuration for all the IO lines.
  @return A longlong (64 bit) value specifying which lines are inputs and which are outputs.  Non-zero
  values indicate that a line is configured as an output.

  \b Example
  \code
  longlong directions = Io_GetDirectionBits( );
  if( directions & IO_PA18_BIT )
  {
    // then we know PA18 is configured as an output
  }
  \endcode
*/
longlong Io_GetDirectionBits( )
{
  return ( ((longlong)AT91C_BASE_PIOB->PIO_OSR) << 32 ) | AT91C_BASE_PIOA->PIO_OSR;
}

/** @}
*/

#ifdef OSC

/** \defgroup IoOSC IO - OSC
  Control any pin's direction and output from OSC.

  The IO system allows for the manual manipulation of individual signal lines on the Make Controller.
  Proceed with care if you're using these at the same time as any of the other subsystems, as most 
  other subsystems rely on the IO system internally to maintain control of the appropriate signal lines.
  \ingroup OSC
	
	\section devices Devices
	There are 64 IO's on the Make Controller, numbered 0 - 63.
  - IOs <b>0 - 31</b> correspond to \b PortA
  - IOs <b>32 - 63 </b> correspond to \b PortB.
	
	\section properties Properties
	Each IO has four properties:
  - output
  - value
  - pullup
  - active

	\par Output
	The \b output property sets whether the IO is an output.
  A value of \b 1 makes it an output, and a value of \b 0 makes it an input.
	For example, to make pin 4 an output, send a message like
	\verbatim /io/4/output 1\endverbatim
	Send the message
  \verbatim /io/4/output 0\endverbatim
  to make it an input.

  \par Pullup
  The \b pullup property determines whether a given IOs pullup resistor is enabled.
  To enable it on IO 2, send the message
  \verbatim /io/2/pullup 1\endverbatim
  and to disable it, send the message
  \verbatim /io/2/pullup 0\endverbatim

	\par Value
	Writing the \b value property sets the on/off value of a given IO.
  The IO system only permits digital IO so, we only want to send ones and zeros as values.
	For example, to activate pin 4, send a message like
	\verbatim /io/4/value 1\endverbatim
	Change the argument 1 to a 0 to turn it off.
	Reading the \b value property returns the value of a given IO.
	For example, to read pin 4, send a message like
	\verbatim /io/4/value\endverbatim
    
	\par Active
	The \b active property corresponds to the active state of an IO.
  Read whether IO 0 is active by sending the message
	\verbatim /io/0/active \endverbatim
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
      if(value)
        Io_Start( index, false );
      else
        Io_Stop( index );
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
  int value = 0;
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
  int value = 0;
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





