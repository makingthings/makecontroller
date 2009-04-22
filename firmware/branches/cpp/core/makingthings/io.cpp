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
  
  If you're configuring it as a GPIO, you can use the \b INPUT and \b OUTPUT 
  symbols instead of true/false, for convenience.
  
  @param index Which pin to control - see IoIndices
  @param peripheral (optional) Which Peripheral to configure the Io as - defaults to GPIO.
  @param output (optional) If peripheral is GPIO, set whether it's an input or an output - 
  defaults to OUTPUT.
  
  \b Example
  \code
  Io io(IO_PA07); // control pin PA07, defaults to GPIO configured as an output
  io.on(); // turn the output on
  
  // or specify more config info
  Io io(IO_PA08, Io::GPIO, INPUT); // control pin PA08, as an input
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
  
  See \ref IoIndices for the list of possible values.
  
  \b Example
  \code
  Io io; // nothing specified - invalid until configured
  io.setPin(IO_PA27); // set it to control PA27
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
  
  @return The pin being controlled - see \ref IoIndices for a list of possible values.
  
  \b Example
  \code
  Io io(IO_PA09);
  int p = io.pin();
  p == IO_PA09; // true
  \endcode
*/
int Io::pin( )
{
  return ( io_pin == INVALID_PIN ) ? -1 : io_pin;
}

/**
  Read whether an Io is on or off.
  For this to be meaningful, the Io must be configured as a \b GPIO and set as an \b input.
  @return True if the pin is high, false if it's low.
  
  \b Example
  \code
  Io io(IO_PB28, Io::GPIO, false); // new io, configured as a GPIO and an input
  bool is_pb28_on = io.value(); // is it on?
  \endcode
*/
bool Io::value( )
{
  if ( io_pin == INVALID_PIN )
    return 0;
  
  int mask = 1 << ( io_pin & 0x1F );
  if ( io_pin < 32 )
    return ( ( AT91C_BASE_PIOA->PIO_PDSR & mask ) != 0 );
  else
    return ( ( AT91C_BASE_PIOB->PIO_PDSR & mask ) != 0 );
}

/**
  Turn an Io on or off.
  For this to be meaningful, the Io must be configured as a \b GPIO and set as an \b output.
  If you want to turn the Io on or off directly, use on() or off().
  @param onoff True to turn it on, false to turn it off.
  @return True on success, false on failure.
  
  \b Example
  \code
  Io io(IO_PB28); // new io, defaults to GPIO output
  io.setValue(true); // turn it on
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
  For this to be meaningful, the Io must be configured as a \b GPIO and set as an \b output.
  
  This is slightly faster than setValue() since it doesn't have to check
  whether to turn it on or off.
  @return True on success, false on failure.
  
  \b Example
  \code
  Io io(IO_PB18); // defaults to GPIO output
  io.on(); // turn it on
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
  For this to be meaningful, the Io must be configured as a \b GPIO and set as an \b output.
  
  This is slightly faster than setValue() since it doesn't have to check
  whether to turn it on or off.
  @return True on success, false on failure.
  
  \b Example
  \code
  Io io(IO_PB18); // defaults to GPIO output
  io.off(); // turn it off
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
  For this to be meaningful, the Io must be configured as a \b GPIO.
  @return True if it's an output, false if it's an input.
  
  \b Example
  \code
  Io io(IO_PA13);
  if(io.direction())
  {
    // then we're an output
  }
  else
  {
    // we're an input
  }
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
  For this to be meaningful, the Io must be configured as a \b GPIO.
  
  You can also use the \b OUTPUT and \b INPUT symbols instead of true/false, 
  for convenience.
  @param output True to set it as an output, false to set it as an input.
  
  \b Example
  \code
  Io io(IO_PA01); // is a GPIO output by default
  io.setDirection(INPUT); // now change it to an input
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
  @return True if pullup is enabled, false if not.
  
  \b Example
  \code
  Io io(IO_PA14);
  bool is_pa14_pulledup = io.pullup(); // is it pulled up?
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
  @param enabled True to enabled the pullup, false to disable it.
  
  \b Example
  \code
  Io pa14(IO_PA14);
  pa14.setPullup(true); // turn the pullup on
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
  @return True if the filter is enabled, false if not.
  
  \b Example
  \code
  Io pa8(IO_PA08);
  bool is_pa8_filtered = pa8.filter(); // is the filter on?
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
  @param enabled True to enable the filter, false to disable it.
  
  \b Example
  \code
  Io io(IO_PB12);
  io.setFilter(true); // turn the filter on
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

/*
  Read which perihperal an Io has been configured as.
  
  \b Example
  \code

  \endcode
*/
// int Io::peripheral( )
// {
//   if ( io_pin == INVALID_PIN )
//     return 0;
//     
//   if ( io_pin < 32 ) 
//     return ( AT91C_BASE_PIOA->PIO_PSR & ( 1 << io_pin ) ) != 0;
//   else
//     return ( AT91C_BASE_PIOB->PIO_PSR & ( 1 << ( io_pin & 0x1F ) ) ) != 0;
// }

/**
  Set the peripheral an Io should be configured as.
  
  @param periph The Peripheral to set this pin to.
  @param disableGpio (optional) If you're specifying peripheral A or B, this can
  optionally make sure the GPIO configuration is turned off.
  
  \b Example
  \code
  Io io(IO_PB09); // defaults to GPIO output
  io.setPeripheral(Io::A); // set to A & disable GPIO
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

// bool Io::releasePeripherals( )
// {
//   if ( io_pin == INVALID_PIN )
//     return 0;
//   
//   // check each possible peripheral, and make sure it's cleared
//   return false;
// }




