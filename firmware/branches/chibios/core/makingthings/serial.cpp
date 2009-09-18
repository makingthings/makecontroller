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

#include "serial.h"
#include "io.h"
#include "Board.h"
#include "error.h"

Serial::Internal Serial::internals[SERIAL_PORTS];
extern void (Serial0Isr_Wrapper)(void);
extern void (Serial1Isr_Wrapper)(void);

/**
  Create a new serial port.
  
  \b Example
  \code
  Serial ser(0);
  \endcode
*/
Serial::Serial( int channel, int q_size )
{
  if( channel < 0 || channel >= 2 ) // make sure channel is valid
    return;
  _channel = channel;
  Internal* si = &internals[_channel];
  if( si->rxQueue == NULL )
    si->rxQueue = new Queue( q_size, 1 );
  if( si->txQueue == NULL )
    si->txQueue = new Queue( q_size, 1 );
  
  // default to SERIAL_0 values
  int id = AT91C_ID_US0;
  int rxPin = IO_PA00;
  int txPin = IO_PA01;
  long rxPinBit = IO_PA00_BIT;
  long txPinBit = IO_PA01_BIT;
  switch( _channel )
  {
    case 0:
      // values already set for this above
      si->uart = AT91C_BASE_US0;
      break;
    case 1:
      id = AT91C_ID_US1;
      rxPinBit = IO_PA05_BIT;
      txPinBit = IO_PA06_BIT;
      rxPin = IO_PA05;
      txPin = IO_PA06;
      si->uart = AT91C_BASE_US1;
      break;
  }
  
  AT91C_BASE_PMC->PMC_PCER = 1 << id;   // Enable the peripheral clock
  _baud = SERIAL_DEFAULT_BAUD;
  bits = SERIAL_DEFAULT_BITS;
  _stopBits = SERIAL_DEFAULT_STOPBITS;
  _parity = SERIAL_DEFAULT_PARITY;
  _handshaking = SERIAL_DEFAULT_HANDSHAKING;
  si->uart->US_IDR = (unsigned int) -1; // Disable interrupts
  si->uart->US_TTGR = 0;                // Timeguard disabled
  Io rx( rxPin, Io::A );
  Io tx( txPin, Io::A );
  
  setDetails( );
  
  unsigned int mask = 0x1 << id;                      
  // Disable the interrupt on the interrupt controller
  AT91C_BASE_AIC->AIC_IDCR = mask;
  // Save the interrupt handler routine pointer and the interrupt priority
  if(_channel == 0)
    AT91C_BASE_AIC->AIC_SVR[ id ] = (unsigned int)Serial0Isr_Wrapper;
  else
    AT91C_BASE_AIC->AIC_SVR[ id ] = (unsigned int)Serial1Isr_Wrapper;
  // Store the Source Mode Register
  AT91C_BASE_AIC->AIC_SMR[ id ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 4;
  // Clear the interrupt on the interrupt controller
  AT91C_BASE_AIC->AIC_ICCR = mask;
  AT91C_BASE_AIC->AIC_IECR = mask;
  si->uart->US_IER = AT91C_US_RXRDY;
}

void Serial::setDetails( )
{
  Internal* sp = &internals[ _channel ];
  // Reset receiver and transmitter
  sp->uart->US_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS | AT91C_US_TXDIS; 

  // MCK is 47923200 for the Make Controller Kit
  // Calculate ( * 10 )
  int baudValue = ( MCK * 10 ) / ( _baud * 16 );
  // Round (and / 10)
  if ( ( baudValue % 10 ) >= 5 ) 
    baudValue = ( baudValue / 10 ) + 1; 
  else 
    baudValue /= 10;

  sp->uart->US_BRGR = baudValue;
//  sp->uart->US_BRGR = (MCK / baud) / 16;  ...from Atmel example code...does this work?
  sp->uart->US_MR = 
    ( ( _handshaking ) ? AT91C_US_USMODE_HWHSH : AT91C_US_USMODE_NORMAL ) |
    ( AT91C_US_CLKS_CLOCK ) |
    ( ( ( bits - 5 ) << 6 ) & AT91C_US_CHRL ) |
    ( ( _stopBits == 2 ) ? AT91C_US_NBSTOP_2_BIT : AT91C_US_NBSTOP_1_BIT ) |
    ( ( _parity == 0 ) ? AT91C_US_PAR_NONE : ( ( _parity == -1 ) ? AT91C_US_PAR_ODD : AT91C_US_PAR_EVEN ) );
    // 2 << 14; // this last thing puts it in loopback mode

  sp->uart->US_CR = AT91C_US_RXEN | AT91C_US_TXEN;
}

/**
  Set the baud rate of a serial port.
  
  \b Example
  \code
  Serial ser(0);
  ser.setBaud(115200);
  \endcode
*/
void Serial::setBaud( int rate )
{
  _baud = rate;
  setDetails( );
}

/**
  Returns the current baud rate.
  @return The current baud rate.
  
  \b Example
  \code
  Serial ser(0);
  int baudrate = ser.getBaud();
  \endcode
*/
int Serial::baud( )
{
  return _baud;
}

/**
  Sets the number of bits per character.
  5 through 8 are legal values - 8 is the default.
  @param bits bits per character
  
  \b Example
  \code
  Serial ser(0);
  ser.setDataBits(5);
  \endcode
*/
void Serial::setDataBits( int bits )
{
  if ( this->bits >= 5 && this->bits <= 8 )
    this->bits = bits;
  else
    this->bits = 8;
  setDetails( );
}

/**
  Returns the number of bits for each character.
  @return The current data bits setting.
  
  \b Example
  \code
  Serial ser(0);
  int dbits = ser.getDataBits();
  \endcode
*/
int Serial::dataBits( )
{
  return bits;
}

/**
  Sets the parity.
  -1 is odd, 0 is none, 1 is even.  The default is none - 0.
  @param parity -1, 0 or 1.
  
  \b Example
  \code
  Serial ser(0);
  ser.setParity(-1); // set to odd parity
  \endcode
*/
void Serial::setParity( int parity )
{
  if ( parity >= -1 && parity <= 1 )
    _parity = parity;
  else
    _parity = 1;
  setDetails( );
}

/**
  Returns the current parity.
  -1 is odd, 0 is none, 1 is even.  The default is none - 0.
  @return The current parity setting.
  
  \b Example
  \code
  Serial ser(0);
  int par = getParity();
  \endcode
*/
int Serial::parity( )
{
  return _parity;
}

/**
  Sets the stop bits per character.
  1 or 2 are legal values.  1 is the default.
  @param bits stop bits per character
  
  \b Example
  \code
  Serial ser(0);
  ser.setStopBits(2);
  \endcode
*/
void Serial::setStopBits( int bits )
{
  if ( bits == 1 || bits == 2 )
    _stopBits = bits;
  else
    _stopBits = 1;
  setDetails( );
}

/**
  Returns the number of stop bits.
  @return The number of stop bits.
  
  \b Example
  \code
  Serial ser(0);
  int sbits = ser.getStopBits();
  \endcode
*/
int Serial::stopBits( )
{
  return _stopBits;
}

/**
  Sets whether hardware handshaking is being used.
  @param enable Whether to use handshaking - true or false.
  
  \b Example
  \code
  Serial ser(0);
  ser.setHandshaking(true); // enable hardware handshaking
  \endcode
*/
void Serial::setHandshaking( bool enable )
{
  _handshaking = enable;
  setDetails( );
}

/**
  Returns whether hardware handshaking is enabled or not.
  @return Wheter handshaking is currently enabled - true or false.
  
  \b Example
  \code
  Serial ser(0);
  if( ser.getHandshaking() )
  {
    // then handshaking is enabled
  }
  \endcode
*/
bool Serial::handshaking( )
{
  return _handshaking;
}

/**
  Write a single character
  @param character The character to write.
*/
int Serial::write( char character )
{
  return 0;
}

/**
  Write a block of data
  @param data The data to send.
  @param length How many bytes of data to send.
  @param timeout How long to wait to make sure it goes through.
*/
int Serial::write( char* data, int length, int timeout )
{
  Internal* sp = &internals[ _channel ];
  while ( length ) // Do the business
  {
    if( sp->txQueue->send( data++, timeout ) == 0 ) 
      return CONTROLLER_ERROR_QUEUE_ERROR;
    length--;
  }
   
  /* Turn on the Tx interrupt so the ISR will remove the character from the 
  queue and send it. This does not need to be in a critical section as 
  if the interrupt has already removed the character the next interrupt 
  will simply turn off the Tx interrupt again. */ 
  sp->uart->US_IER = AT91C_US_TXRDY; 
  return CONTROLLER_OK;
}

int Serial::writeDMA(void *data, int length)
{
    Internal* sp = &internals[ _channel ];
    // Check if the first PDC bank is free
    if ((sp->uart->US_TCR == 0) && (sp->uart->US_TNCR == 0))
    {
      sp->uart->US_TPR = (unsigned int) data;
      sp->uart->US_TCR = length;
      sp->uart->US_PTCR = AT91C_PDC_TXTEN;
      return 1;
    }
    // Check if the second PDC bank is free
    else if (sp->uart->US_TNCR == 0)
    {
      sp->uart->US_TNPR = (unsigned int) data;
      sp->uart->US_TNCR = length;
      return 1;
    }
    else
      return 0;
}

int Serial::bytesAvailable( )
{
  Internal* sp = &internals[ _channel ];
  return sp->rxQueue->msgsAvailable( );
}

bool Serial::anyBytesAvailable()
{
  Internal* sp = &internals[ _channel ];
  return ((sp->uart->US_CSR & AT91C_US_RXRDY) != 0);
}

int Serial::read( char* data, int length, int timeout )
{
  Internal* sp = &internals[ _channel ];

  // Do the business
  int count = 0;
  while ( count < length )
  {
    /* Place the character in the queue of characters to be transmitted. */ 
    if( sp->rxQueue->receive( data++, timeout ) == 0 )
      break;
    count++;
  }
  return count;
}

int Serial::readDMA( char* data, int length, int timeout )
{
  Internal* sp = &internals[ _channel ];
  // Check if the first PDC bank is free
  int retval = 0;
  if ((sp->uart->US_RCR == 0) && (sp->uart->US_RNCR == 0))
  {
    sp->uart->US_RPR = (unsigned int) data;
    sp->uart->US_RCR = length;
    sp->uart->US_PTCR = AT91C_PDC_RXTEN;
    retval = 1;
  }
  // Check if the second PDC bank is free
  else if (sp->uart->US_RNCR == 0)
  {
    sp->uart->US_RNPR = (unsigned int) data;
    sp->uart->US_RNCR = length;
    retval = 1;
  }
  
  if(retval)
    sp->uart->US_IER = AT91C_US_RXBUFF;
  
  sp->rxSem->take(); // wait until we get this back from the interrupt
  return retval;
}

char Serial::read( int timeout )
{
  Internal* sp = &internals[ _channel ];
  char c;
  // Do the business
  if( sp->rxQueue->receive( &c, timeout ) == 0 )
    return 0;
  else
    return c;
}

/**
  Clear out the serial port.
  Ensures that there are no bytes in the incoming buffer.

  \b Example
  \code
  Serial ser(1);
  ser.flush( ); // after starting up, make sure there's no junk in there
  \endcode
*/
void Serial::flush( )
{
  while( bytesAvailable( ) )
    read( );
}

/**
  Reset the error flags in the serial system.
  In the normal course of operation, the serial system may experience
  a variety of different error modes, including buffer overruns, framing 
  and parity errors, and more.  You'll usually only want to call this
  after you've determined that errors exist with getErrors().
  If there aren't any errors, this has no effect.
  
  \b Example

  \code 
  Serial ser(1);
  if( ser.getErrors() )
  {
    // handle errors...
    ser.clearErors();
  }
  \endcode
*/
void Serial::clearErrors( )
{
  Internal* sp = &internals[ _channel ];
  if( sp->uart->US_CSR & (AT91C_US_OVRE | AT91C_US_FRAME | AT91C_US_PARE) )
    sp->uart->US_CR = AT91C_US_RSTSTA; // clear all errors
}

/**
  Read whether there are any errors.
  We can check for three kinds of errors in the serial system:
  - buffer overrun
  - framing error
  - parity error
  
  Each parameter will be set with a true or a false, given the current
  error state.  If you don't care to check one of the parameters, just
  pass in 0.
  
  @param overrun (optional) Will be set with the overrun error state.
  @param frame (optional) Will be set with the frame error state.
  @param parity (optional) Will be set with the parity error state.
  @return True if there were any errors, false if there were no errors.

  \b Example
  \code
  Serial ser(1);
  bool over, fr, par;
  if( ser.errors( &over, &fr, &par ) )
  {
    // if we wanted, we could just clear them all right here with clearErrors()
    // but here we'll check to see what kind of errors we got for the sake of the example
    if(over)
    {
      // then we have an overrun error
    }
    if(fr)
    {
      // then we have a framing error
    }
    if(par)
    {
      // then we have a parity error
    }
  }
  else
  {
    // there were no errors
  }
  \endcode
*/
bool Serial::errors( bool* overrun, bool* frame, bool* parity )
{
  bool retval = false;
  Internal* sp = &internals[ _channel ];

  bool ovre = sp->uart->US_CSR & AT91C_US_OVRE;
  if(ovre)
    retval = true;
  if(overrun)
    *overrun = ovre;

  bool fr = sp->uart->US_CSR & AT91C_US_FRAME;
  if(fr)
    retval = true;
  if(frame)
    *frame = fr;

  bool par = sp->uart->US_CSR & AT91C_US_PARE;
  if(par)
    retval = true;
  if(parity)
    *parity = par;
  
  return retval;
}

/**
  Start the transmission of a break.
  This has no effect if a break is already in progress.
  
  \b Example
  \code 
  Serial ser(1);
  ser.startBreak();
  \endcode
*/
void Serial::startBreak( )
{
  internals[ _channel ].uart->US_CR = AT91C_US_STTBRK;
}

/**
  Stop the transmission of a break.
  This has no effect if there's not a break already in progress.
  
  \b Example
  \code
  Serial ser(1);
  ser.stopBreak();
  \endcode
*/
void Serial::stopBreak( )
{
  internals[ _channel ].uart->US_CR = AT91C_US_STPBRK;
}



