/*********************************************************************************

 Copyright 2006-2009 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#include "Board.h"
#include "AT91SAM7X256.h" 
#include "io.h"
#include "config.h"

#define IO_PIN_COUNT_ 64

/**
  Create a new Io object.
  
  @param index Which pin to control - see IoIndices
  @param peripheral (optional) Which Peripheral to configure the Io as - defaults to GPIO.
  @param output (optional) If peripheral is GPIO, set whether it's an input or an output - 
  defaults to output.
  
  \b Example
  \code
  Io io(IO_PA07); // control pin PA07, defaults to GPIO configured as an output
  io.on(); // turn the output on
  
  // or specify more config info
  Io io(IO_PA08, Io::GPIO, false); // control pin PA08, as an input
  bool is_pa08_on = io.value(); // is it on?
  \endcode
*/
Io::Io( int index, Peripheral peripheral, bool output  )
{
  io_pin = INVALID_PIN;
  if ( index < 0 || index > IO_PIN_COUNT_ )
    return;
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOA; // maybe only assert these once...
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOB;
  io_pin = index;
  setPeripheral( peripheral );
  if( peripheral == GPIO )
    setDirection( output );
}

/**
  Set which pin an Io is controlling.
  
  \b Example
  \code
  
  \endcode
*/
bool Io::setPin( int pin )
{
  if ( pin < 0 || pin > IO_PIN_COUNT_ )
    return false;
  io_pin = pin;
  return true;
}

/**
  Read which pin an Io is controlling.
  
  \b Example
  \code

  \endcode
*/
int Io::pin( )
{
  return ( io_pin == INVALID_PIN ) ? -1 : io_pin;
}

/**
  Read whether an Io is on or off.
  
  \b Example
  \code

  \endcode
*/
bool Io::value( )
{
  if ( io_pin == INVALID_PIN )
    return 0;
  
  int mask = 1 << ( io_pin & 0x1F );
  if ( io_pin < 32 )
    return ( ( AT91C_BASE_PIOA->PIO_PDSR & mask ) != 0 ) ? 1 : 0;
  else
    return ( ( AT91C_BASE_PIOB->PIO_PDSR & mask ) != 0 ) ? 1 : 0;
}

/**
  Turn an Io on or off.
  
  The Io must be configured as an output to turn it on/off.
  
  \b Example
  \code

  \endcode
*/
bool Io::setValue( bool onoff )
{
  if ( io_pin == INVALID_PIN )
    return 0;

  int mask = 1 << ( io_pin & 0x1F );
  if ( io_pin < 32 ) // port A
  {
    if ( !onoff )
      AT91C_BASE_PIOA->PIO_SODR = mask; // set it
    else
      AT91C_BASE_PIOA->PIO_CODR = mask; // clear
  }
  else // port B
  {
    if ( !onoff )
      AT91C_BASE_PIOB->PIO_SODR = mask; // set it
    else
      AT91C_BASE_PIOB->PIO_CODR = mask; // clear it
  }
  return true;
}

/**
  Turn on Io on.
  
  \b Example
  \code

  \endcode
*/
bool Io::on()
{
  int mask = 1 << ( io_pin & 0x1F );
  if ( io_pin < 32 ) // port A
    AT91C_BASE_PIOA->PIO_CODR = mask;
  else // port B
    AT91C_BASE_PIOB->PIO_CODR = mask;
  return true;
}

/**
  Turn an Io off.
  
  \b Example
  \code

  \endcode
*/
bool Io::off()
{
  int mask = 1 << ( io_pin & 0x1F );
  if ( io_pin < 32 ) // port A
    AT91C_BASE_PIOA->PIO_SODR = mask;
  else // port B
    AT91C_BASE_PIOB->PIO_SODR = mask;
  return true;
}

/**
  Read whether an Io is an input or an output.
  
  \b Example
  \code

  \endcode
*/
bool Io::direction( )
{
  if ( io_pin == INVALID_PIN )
    return 0;

  if ( io_pin < 32 ) // port A
    return ( AT91C_BASE_PIOA->PIO_OSR & ( 1 << io_pin ) ) != 0;
  else // port B
    return ( AT91C_BASE_PIOB->PIO_OSR & ( 1 << ( io_pin & 0x1F ) ) ) != 0;
}

/**
  Set an Io as an input or an output
  
  \b Example
  \code

  \endcode
*/
bool Io::setDirection( bool output )
{
  if ( io_pin == INVALID_PIN )
    return 0;
  
  if( output ) // set as output
  {
    if ( io_pin < 32 ) // port A
      AT91C_BASE_PIOA->PIO_OER = 1 << io_pin;
    else // port B
      AT91C_BASE_PIOB->PIO_OER = 1 << ( io_pin & 0x1F );
  }
  else // set as input
  {
    if ( io_pin < 32 ) // port A
      AT91C_BASE_PIOA->PIO_ODR = 1 << io_pin;
    else // port B
      AT91C_BASE_PIOB->PIO_ODR = 1 << ( io_pin & 0x1F );
  }
  return true;
}

/**
  Read whether the pullup is enabled for an Io.
  
  \b Example
  \code

  \endcode
*/
bool Io::pullup( )
{
  if ( io_pin == INVALID_PIN )
    return 0;

  // The PullUp status register is inverted.
  if ( io_pin < 32 ) 
    return ( AT91C_BASE_PIOA->PIO_PPUSR & ( 1 << io_pin ) ) == 0;
  else
    return ( AT91C_BASE_PIOB->PIO_PPUSR & ( 1 << ( io_pin & 0x1F ) ) ) == 0;
}

/**
  Set whether the pullup is enabled for an Io.
  
  \b Example
  \code

  \endcode
*/
bool Io::setPullup( bool enabled )
{
  if ( io_pin == INVALID_PIN )
    return 0;
    
  int mask = 1 << ( io_pin & 0x1F );
  if( enabled ) // turn it on
  {
    if ( io_pin < 32 )
        AT91C_BASE_PIOA->PIO_PPUER = mask;
    else
        AT91C_BASE_PIOB->PIO_PPUER = mask;
  }
  else // turn it off
  {
    if ( io_pin < 32 )
        AT91C_BASE_PIOA->PIO_PPUDR = mask;
    else
        AT91C_BASE_PIOB->PIO_PPUDR = mask;
  }
  return true;
}

/**
  Read whether the glitch filter is enabled for an Io.
  
  \b Example
  \code

  \endcode
*/
bool Io::filter( )
{
  if ( io_pin == INVALID_PIN )
    return 0;
    
  if ( io_pin < 32 ) 
    return AT91C_BASE_PIOA->PIO_IFSR & ( 1 << io_pin );
  else
    return AT91C_BASE_PIOB->PIO_IFSR & ( 1 << ( io_pin & 0x1F ) );
}

/**
  Set whether the glitch filter is enabled for an Io.
  
  \b Example
  \code

  \endcode
*/
bool Io::setFilter( bool enabled )
{
  if ( io_pin == INVALID_PIN )
    return 0;
  
  int mask = 1 << ( io_pin & 0x1F );
  if( enabled ) // turn it on
  {
    if ( io_pin < 32 )
        AT91C_BASE_PIOA->PIO_IFER = mask;
    else
        AT91C_BASE_PIOB->PIO_IFER = mask;
  }
  else // turn it off
  {
    if ( io_pin < 32 )
        AT91C_BASE_PIOA->PIO_IFDR = mask;
    else
        AT91C_BASE_PIOB->PIO_IFDR = mask;
  }
  return true;
}

/**
  Read which perihperal an Io has been configured as.
  
  \b Example
  \code

  \endcode
*/
int Io::peripheral( )
{
  if ( io_pin == INVALID_PIN )
    return 0;
    
  if ( io_pin < 32 ) 
    return ( AT91C_BASE_PIOA->PIO_PSR & ( 1 << io_pin ) ) != 0;
  else
    return ( AT91C_BASE_PIOB->PIO_PSR & ( 1 << ( io_pin & 0x1F ) ) ) != 0;
}

/**
  Set the peripheral an Io should be configured as.
  
  \b Example
  \code

  \endcode
*/
bool Io::setPeripheral( Peripheral periph, bool disableGpio )
{
  if ( io_pin == INVALID_PIN )
    return 0;
  
  int mask = 1 << ( io_pin & 0x1F );
  switch( periph )
  {
    case A: // disable pio for each
      if ( io_pin < 32 )
      {
        if( disableGpio )
          AT91C_BASE_PIOA->PIO_PDR = mask;
        AT91C_BASE_PIOA->PIO_ASR = mask;
      }
      else
      {
        if( disableGpio )
          AT91C_BASE_PIOB->PIO_PDR = mask;
        AT91C_BASE_PIOB->PIO_ASR = mask;
      }
      break;
    case B:
      if ( io_pin < 32 )
      {
        if( disableGpio )
          AT91C_BASE_PIOA->PIO_PDR = mask;
        AT91C_BASE_PIOA->PIO_BSR = mask;
      }
      else
      {
        if( disableGpio )
          AT91C_BASE_PIOB->PIO_PDR = mask;
        AT91C_BASE_PIOB->PIO_BSR = mask;
      }
      break;
    case GPIO:
      if ( io_pin < 32 )
          AT91C_BASE_PIOA->PIO_PER = mask;
      else
          AT91C_BASE_PIOB->PIO_PER = mask;
      break;
  }
  return true;
}

bool Io::releasePeripherals( )
{
  if ( io_pin == INVALID_PIN )
    return 0;
  
  // check each possible peripheral, and make sure it's cleared
  return false;
}




