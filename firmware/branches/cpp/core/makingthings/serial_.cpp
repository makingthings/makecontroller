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

#include "serial_.h"
#include "io_cpp.h"
#include "Board.h"

Serial::Internal Serial::internals[SERIAL_PORTS];
extern void (SerialIsr_Wrapper)(void);

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
  int rxPin = IO_PA00_BIT;
  int txPin = IO_PA01_BIT;
  long rxPinBit = IO_PA00;
  long txPinBit = IO_PA01;
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
  baud = SERIAL_DEFAULT_BAUD;
  bits = SERIAL_DEFAULT_BITS;
  stopBits = SERIAL_DEFAULT_STOPBITS;
  parity = SERIAL_DEFAULT_PARITY;
  handshaking = SERIAL_DEFAULT_HANDSHAKING;
  si->uart->US_IDR = (unsigned int) -1; // Disable interrupts
  si->uart->US_TTGR = 0;                // Timeguard disabled
  Io rx( rxPin, IO_A );
  Io tx( txPin, IO_A );

  setDetails( );

  unsigned int mask = 0x1 << id;                      
  /* Disable the interrupt on the interrupt controller */					
  AT91C_BASE_AIC->AIC_IDCR = mask ;										
  /* Save the interrupt handler routine pointer and the interrupt priority */	
  AT91C_BASE_AIC->AIC_SVR[ id ] = (unsigned int)SerialIsr_Wrapper;
  /* Store the Source Mode Register */									
  AT91C_BASE_AIC->AIC_SMR[ id ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 4  ;				
  /* Clear the interrupt on the interrupt controller */					
  AT91C_BASE_AIC->AIC_ICCR = mask ;					
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
  int baudValue = ( MCK * 10 ) / ( baud * 16 );
  // Round (and / 10)
  if ( ( baudValue % 10 ) >= 5 ) 
    baudValue = ( baudValue / 10 ) + 1; 
  else 
    baudValue /= 10;

  sp->uart->US_BRGR = baudValue;
  sp->uart->US_MR = 
    ( ( handshaking ) ? AT91C_US_USMODE_HWHSH : AT91C_US_USMODE_NORMAL ) |
    ( AT91C_US_CLKS_CLOCK ) |
    ( ( ( bits - 5 ) << 6 ) & AT91C_US_CHRL ) |
    ( ( stopBits == 2 ) ? AT91C_US_NBSTOP_2_BIT : AT91C_US_NBSTOP_1_BIT ) |
    ( ( parity == 0 ) ? AT91C_US_PAR_NONE : ( ( parity == -1 ) ? AT91C_US_PAR_ODD : AT91C_US_PAR_EVEN ) );
    // 2 << 14; // this last thing puts it in loopback mode

  sp->uart->US_CR = AT91C_US_RXEN | AT91C_US_TXEN; 
}

void Serial::setBaud( int rate )
{
  baud = rate;
  setDetails( );
}

int Serial::getBaud( )
{
  return baud;
}

void Serial::setDataBits( int bits )
{
  if ( this->bits >= 5 && this->bits <= 8 )
    this->bits = bits;
  else
    this->bits = 8;
  setDetails( );
}

int Serial::getDataBits( )
{
  return bits;
}

void Serial::setParity( int parity )
{
  if ( parity >= -1 && parity <= 1 )
    this->parity = parity;
  else
    this->parity = 1;
  setDetails( );
}

int Serial::getParity( )
{
  return parity;
}

void Serial::setStopBits( int bits )
{
  if ( bits == 1 || bits == 2 )
    stopBits = bits;
  else
    stopBits = 1;
  setDetails( );
}

int Serial::getStopBits( )
{
  return stopBits;
}

void Serial::setHandshaking( bool enable )
{
  handshaking = enable;
  setDetails( );
}

bool Serial::getHandshaking( )
{
  return handshaking;
}

int Serial::write( char character )
{

}

int Serial::write( char* data, int length, int timeout )
{
  Internal* sp = &internals[ _channel ];
  while ( length ) // Do the business
  {
    if( sp->txQueue->sendToBack( data++, timeout ) == 0 ) 
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

int Serial::bytesAvailable( )
{
  Internal* sp = &internals[ _channel ];
  return sp->rxQueue->msgsAvailable( );
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

char Serial::readChar( )
{

}

void Serial::flush( )
{
  while( bytesAvailable( ) )
    readChar( );
}

void Serial::clearErrors( )
{
  Internal* sp = &internals[ _channel ];
  if( sp->uart->US_CSR & (AT91C_US_OVRE | AT91C_US_FRAME | AT91C_US_PARE) )
    sp->uart->US_CR = AT91C_US_RSTSTA; // clear all errors
}

bool Serial::getErrors( bool* overrun, bool* frame, bool* parity )
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

void Serial::startBreak( )
{
  internals[ _channel ].uart->US_CR = AT91C_US_STTBRK;
}

void Serial::stopBreak( )
{
  internals[ _channel ].uart->US_CR = AT91C_US_STPBRK;
}



