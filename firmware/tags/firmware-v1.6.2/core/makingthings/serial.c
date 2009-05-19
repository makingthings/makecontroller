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

/* Library includes. */
#include <string.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Hardware specific headers. */
#include "Board.h"
#include "AT91SAM7X256.h"

#include "config.h"
#include "io.h"

#include "serial.h"
#include "serial_internal.h"

#define DEFAULT_SERIAL_Q_LEN 100
Serial_ Serial[ SERIAL_PORTS ];

extern void ( SerialIsr_Wrapper )( void );

static int Serial_Init( int index );
static int Serial_Deinit( int index );
static int Serial_SetDefault( int index );
static int Serial_SetDetails( int index );


/** \file serial.c	
	Functions for working with the Serial Interface on the Make Controller Board.
*/

/** \defgroup serial Serial
  Send and receive data via the Make Controller's serial ports.

  There are 2 full serial ports on the Make Controller, and this library provides support for both of them.

  Control all of the common serial characteristics including:
  - \b baud - the speed of the connection (110 - > 2M) in baud or raw bits per second.  9600 baud is the default setting.
  - \b bits - the size of each character (5 - 8).  8 bits is the default setting.
  - \b stopBits - the number of stop bits transmitted (1 or 2)  1 stop bit is the default setting.
  - \b parity - the parity policy (-1 is odd, 0 is none and 1 is even).  Even is the default setting.
  - \b hardwareHandshake - whether hardware handshaking is used or not.  HardwareHandshaking is off by default.

  The subsystem is supplied with small input and output buffers (of 100 characters each) and at present
  the implementation is interrupt per character so it's not particularly fast.

  \todo Convert to DMA interface for higher performance, and add support for debug UART

	\ingroup Core
	@{
*/

/**
	Set the active state of a serial port.
  This is automatically set to 
  true by any call to Serial_Write or Serial_Read.
  @param index Which serial port - SERIAL_0 or SERIAL_1
	@param state An integer specifying the active state - 1 (on) or 0 (off).
	@return CONTROLLER_OK (=0) on success.
*/
int Serial_SetActive( int index, int state )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  if ( state && !sp->active )
    return Serial_Init( index );
  else if( !state && sp->active )
    return Serial_Deinit( index );

  return CONTROLLER_OK;
}

/**
	Read the active state of the Serial subsystem.
  @param index Which serial port - SERIAL_0 or SERIAL_1
	@return State - 1/non-zero (on) or 0 (off).
*/
bool Serial_GetActive( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  return sp->active;
}

/**	
	Write a block of data to the Serial port.  Will block for the time specified (in ms)
  if the queue fills up.
  @param index Which serial port - SERIAL_0 or SERIAL_1
	@param buffer A pointer to the buffer to write from.
	@param count An integer specifying the number of bytes to write.
  @param timeout Time in milliseconds to block waiting for the queue to free up. 0 means don't wait.
  @return status.
*/
int Serial_Write( int index, uchar* buffer, int count, int timeout )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  if ( !sp->active )
  {
    int status = Serial_SetActive( index, 1 );
    if ( status != CONTROLLER_OK )
      return status;
  }

  // Do the business
  while ( count )
  {
    if( xQueueSend( sp->transmitQueue, buffer++, timeout ) == 0 ) 
      return CONTROLLER_ERROR_QUEUE_ERROR; 
    count--;
  }
   
  /* Turn on the Tx interrupt so the ISR will remove the character from the 
  queue and send it. This does not need to be in a critical section as 
  if the interrupt has already removed the character the next interrupt 
  will simply turn off the Tx interrupt again. */ 
  sp->at91UARTRegs->US_IER = AT91C_US_TXRDY; 
 
  return CONTROLLER_OK;
}

/**	
	Read data from the Serial port.  Will block for the time specified (in ms) if 
  there are insufficient characters.  Blocking can be avoided if Serial_GetReadable( )
  is used to determine how many characters are available to read prior to calling
  this function.
  @param index Which serial port - SERIAL_0 or SERIAL_1
	@param buffer A pointer to the buffer to read into.
	@param size An integer specifying the maximum number of bytes to read.
  @param timeout Time in milliseconds to block waiting for the specified number of bytes. 0 means don't wait.
  @return number of bytes read (>=0) or error <0 .
*/
int Serial_Read( int index, uchar* buffer, int size, int timeout )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  if ( !sp->active )
  {
    int status = Serial_SetActive( index, 1 );
    if ( status != CONTROLLER_OK )
      return status;
  }

  // Do the business
  int count = 0;
  while ( count < size )
  {
    /* Place the character in the queue of characters to be transmitted. */ 
    if( xQueueReceive( sp->receiveQueue, buffer++, timeout ) == 0 )
      break;
    count++;
  }

  return count;
}

/**	
	Returns the number of bytes in the queue waiting to be read.
  @param index Which serial port - SERIAL_0 or SERIAL_1
  @return bytes in the receive queue.
*/
int Serial_GetReadable( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  return uxQueueMessagesWaiting( sp->receiveQueue );
}

/**	
	Sends a character (in the range of 0 to 255) to the write queue
  @param index Which serial port - SERIAL_0 or SERIAL_1
	@param character The character to be sent.  Must be 0 <= c < 256.
  @return status.
*/
int Serial_SetChar( int index, int character )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  if ( !sp->active )
  {
    int status = Serial_SetActive( index, 1 );
    if ( status != CONTROLLER_OK )
      return status;
  }

  if ( character >= 0 && character < 256 )
  {
    unsigned char c = (unsigned char)character;
    if( xQueueSend( sp->transmitQueue, &c, 0 ) == 0 ) 
      return CONTROLLER_ERROR_QUEUE_ERROR; 
    sp->at91UARTRegs->US_IER = AT91C_US_TXRDY; 
   }

  return CONTROLLER_OK;
}

/**	
	Sets the serial baud rate.
  @param index Which serial port - SERIAL_0 or SERIAL_1
	@param baud The desired baud rate.
  @return status.
*/
int Serial_SetBaud( int index, int baud )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  // If there are no default values, get some
  if ( !sp->detailsInitialized )
    Serial_SetDefault( index );

  sp->baud = baud;
  Serial_SetDetails( index );

  return CONTROLLER_OK;
}

/**	
	Sets the number of bits per character.  5 - 8 are legal values.  8 is the default.
  @param index Which serial port - SERIAL_0 or SERIAL_1
	@param bits bits per character
  @return status.
*/
int Serial_SetBits( int index, int bits )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  // If there are no default values, get some
  if ( !sp->detailsInitialized )
    Serial_SetDefault( index );

  if ( sp->bits >= 5 && sp->bits <= 8 )
    sp->bits = bits;
  else
    sp->bits = 8;

  Serial_SetDetails( index );

  return CONTROLLER_OK;
}

/**	
	Sets the parity.  -1 is odd, 0 is none, 1 is even.  The default is none - 0.
  @param index Which serial port - SERIAL_0 or SERIAL_1
	@param parity -1, 0 or 1.
  @return status.
*/
int Serial_SetParity( int index, int parity )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  // If there are no default values, get some
  if ( !sp->detailsInitialized )
    Serial_SetDefault( index );

  if ( parity >= -1 && parity <= 1 )
    sp->parity = parity;
  else
    sp->parity = 1;
  Serial_SetDetails( index );

  return CONTROLLER_OK;
}

/**	
	Sets the stop bits per character.  1 or 2 are legal values.  1 is the default.
  @param index Which serial port - SERIAL_0 or SERIAL_1
	@param stopBits stop bits per character
  @return status.
*/
int Serial_SetStopBits( int index, int stopBits )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  // If there are no default values, get some
  if ( !sp->detailsInitialized )
    Serial_SetDefault( index );

  if ( stopBits == 1 || stopBits == 2 )
    sp->stopBits = stopBits;
  else
    sp->stopBits = 1;

  Serial_SetDetails( index );

  return CONTROLLER_OK;
}

/**	
	Sets whether hardware handshaking is being used.
  @param index Which serial port - SERIAL_0 or SERIAL_1
	@param hardwareHandshake sets hardware handshaking on (1) or off (0)
  @return status.
*/
int Serial_SetHardwareHandshake( int index, int hardwareHandshake )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  // If there are no default values, get some
  if ( !sp->detailsInitialized )
    Serial_SetDefault( index );

  sp->hardwareHandshake = hardwareHandshake;
  Serial_SetDetails( index );
  
  return CONTROLLER_OK;
}

/**	
	Returns a single character from the receive queue if available.
  This character is returned unsigned - i.e. having a value of 0 - 255.  
  The return value is -1 if there is no character waiting.
  @param index Which serial port - SERIAL_0 or SERIAL_1
  @return character from the queue or -1 if there is no character.
*/
int Serial_GetChar( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  if ( !sp->active )
  {
    int status = Serial_SetActive( index, 1 );
    if ( status != CONTROLLER_OK )
      return -1;
  }

  if ( uxQueueMessagesWaiting( sp->receiveQueue ) )
  {
    unsigned char c;
    if( xQueueReceive( sp->receiveQueue, &c, 0 ) == 0 )
      return -1;
    else
      return (int)c;
  }
  else
    return -1; 
}

/**	
	Returns the current baud rate
  @param index Which serial port - SERIAL_0 or SERIAL_1
  @return baud
*/
int Serial_GetBaud( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  // If there are no default values, get some
  if ( !sp->detailsInitialized )
    Serial_SetDefault( index );

  return sp->baud;
}

/**	
	Returns the number of bits for each character
  @param index Which serial port - SERIAL_0 or SERIAL_1
  @return bits
*/
int Serial_GetBits( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  // If there are no default values, get some
  if ( !sp->detailsInitialized )
    Serial_SetDefault( index );

  return sp->bits;
}

/**	
	Returns the current parity.  -1 means odd, 0 means none, 1 means even
  @param index Which serial port - SERIAL_0 or SERIAL_1
  @return parity
*/
int Serial_GetParity( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  // If there are no default values, get some
  if ( !sp->detailsInitialized )
    Serial_SetDefault( index );

  return sp->parity;
}

/**	
	Returns the number of stop bits.
  @param index Which serial port - SERIAL_0 or SERIAL_1
  @return stopBits
*/
int Serial_GetStopBits( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  // If there are no default values, get some
  if ( !sp->detailsInitialized )
    Serial_SetDefault( index );

  return sp->stopBits;
}

/**	
	Returns whether hardware handshaking is being employed or not.
  @param index Which serial port - SERIAL_0 or SERIAL_1
  @return hardwareHandshake
*/
int Serial_GetHardwareHandshake( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  // If there are no default values, get some
  if ( !sp->detailsInitialized )
    Serial_SetDefault( index );

  return sp->hardwareHandshake;
}

/**
  Clear out the serial port.
  Ensures that there are no bytes in the incoming buffer.
  @param index Which serial port - SERIAL_0 or SERIAL_1

  \b Example
  \code
  Serial_SetActive(SERIAL_0, 1);
  Serial_Flush(SERIAL_0); // after starting up, make sure there's no junk in there
  \endcode
*/
void Serial_Flush( int index )
{
  char c;
  if ( index < 0 || index >= SERIAL_PORTS )
    return;

  while( Serial_GetReadable( index ) )
    c = Serial_GetChar( index );
}

/**
  Reset the error flags in the serial system.
  In the normal course of operation, the serial system may experience
  a variety of different error modes, including buffer overruns, framing 
  and parity errors, and more.  When in an error state, the serial system
  may behave differently than normal.

  Serial_ClearErrors() resets the appropriate status bits to a state of
  normal operation.  It will only reset the error states if there are 
  currently any errors.
  @param index Which serial port - SERIAL_0 or SERIAL_1
  @see Serial_GetErrors
  
  \b Example

  \code 
  Serial_ClearErrors(SERIAL_0);
  // that's all there is to it.
  \endcode
*/
void Serial_ClearErrors( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return;
  
  Serial_* sp = &Serial[ index ];
  if( sp->at91UARTRegs->US_CSR & (AT91C_US_OVRE | AT91C_US_FRAME | AT91C_US_PARE) )
    sp->at91UARTRegs->US_CR = AT91C_US_RSTSTA; // clear all errors
}

/**
  Read whether there are any errors.
  We can check for three kinds of errors in the serial system:
  - buffer overrun
  - framing error
  - parity error
  
  Each parameter will be set with a true or a false given the current
  error state.  If you don't care to check one of the parameters, just
  pass in 0.
  
  @param index Which serial port - SERIAL_0 or SERIAL_1
  @param overrun A bool that will be set with the overrun error state.
  @param frame A bool that will be set with the frame error state.
  @param parity A bool that will be set with the parity error state.
  @return True if there were any errors, false if there were no errors.
  @see Serial_ClearErrors( )

  \b Example
  \code
  bool over, fr, par;
  if( Serial_GetErrors( SERIAL_0, &over, &fr, &par ) )
  {
    // if we wanted, we could just clear them all right here with Serial_ClearErrors()
    // but here we'll check to see what kind of errors we got
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
bool Serial_GetErrors( int index, bool* overrun, bool* frame, bool* parity )
{
  bool retval = false;
  if ( index < 0 || index >= SERIAL_PORTS )
    return false;
  
  Serial_* sp = &Serial[ index ];

  bool ovre = sp->at91UARTRegs->US_CSR & AT91C_US_OVRE;
  if(ovre)
    retval = true;
  if(overrun)
    *overrun = ovre;

  bool fr = sp->at91UARTRegs->US_CSR & AT91C_US_FRAME;
  if(fr)
    retval = true;
  if(frame)
    *frame = fr;

  bool par = sp->at91UARTRegs->US_CSR & AT91C_US_PARE;
  if(par)
    retval = true;
  if(parity)
    *parity = par;
  
  return retval;
}

/**
  Start the transimission of a break.
  This has no effect if a break is already in progress.
  @param index Which serial port - SERIAL_0 or SERIAL_1

  \b Example
  
  \code 
  Serial_StartBreak(SERIAL_0);
  \endcode
*/
void Serial_StartBreak( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return;
  Serial[ index ].at91UARTRegs->US_CR = AT91C_US_STTBRK;
}

/**
  Stop the transimission of a break.
  This has no effect if there's not a break already in progress.
  @param index Which serial port - SERIAL_0 or SERIAL_1

  \b Example
  
  \code 
  Serial_StopBreak(SERIAL_0);
  \endcode
*/
void Serial_StopBreak( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return;
  Serial[ index ].at91UARTRegs->US_CR = AT91C_US_STPBRK;
}

/** @}
*/

int Serial_Init( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  // default to SERIAL_0 values
  int id = AT91C_ID_US0;
  int rxPin = IO_PA00;
  int txPin = IO_PA01;
  long rxPinBit = IO_PA00_BIT;
  long txPinBit = IO_PA01_BIT;
  switch( index )
  {
    case SERIAL_0:
      // values already set for this above
      sp->at91UARTRegs = AT91C_BASE_US0;
      break;
    case SERIAL_1:
      id = AT91C_ID_US1;
      rxPinBit = IO_PA05_BIT;
      txPinBit = IO_PA06_BIT;
      rxPin = IO_PA05;
      txPin = IO_PA06;
      sp->at91UARTRegs = AT91C_BASE_US1;
      break;
  }

  // Enable the peripheral clock
  AT91C_BASE_PMC->PMC_PCER = 1 << id;
   
  int status = Io_StartBits( rxPinBit | txPinBit, false  );
  if ( status != CONTROLLER_OK )
    return status;

  Io_SetPeripheralA( rxPin );
  Io_SetPeripheralA( txPin );
  Io_SetPio( rxPin, false );
  Io_SetPio( txPin, false );
  
  // Create the queues
  if( sp->rxQSize == 0 )
   sp->rxQSize = DEFAULT_SERIAL_Q_LEN;
  sp->receiveQueue = xQueueCreate( sp->rxQSize, 1 );

  if( sp->txQSize == 0 )
     sp->txQSize = DEFAULT_SERIAL_Q_LEN;
  sp->transmitQueue = xQueueCreate( sp->txQSize, 1 );

  // Disable interrupts
  sp->at91UARTRegs->US_IDR = (unsigned int) -1;

  // Timeguard disabled
  sp->at91UARTRegs->US_TTGR = 0;

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

  sp->at91UARTRegs->US_IER = AT91C_US_RXRDY; 

  sp->active = true;

  // If there are no default values, get some
  if ( !sp->detailsInitialized )
    Serial_SetDefault( index );
  // Most of the detail setting is done in here
  // Also Resets TXRX and re-enables RX
  Serial_SetDetails( index );

  return CONTROLLER_OK;
}

int Serial_Deinit( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];
  vQueueDelete( sp->receiveQueue );
  vQueueDelete( sp->transmitQueue );
  sp->active = false;
  return CONTROLLER_OK;
}

int Serial_SetDetails( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  if ( sp->active )
  {
     // Reset receiver and transmitter
    sp->at91UARTRegs->US_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS | AT91C_US_TXDIS; 

    // MCK is 47923200 for the Make Controller Kit

    // Calculate ( * 10 )
    int baudValue = ( MCK * 10 ) / ( sp->baud * 16 );
    // Round (and / 10)
    if ( ( baudValue % 10 ) >= 5 ) 
      baudValue = ( baudValue / 10 ) + 1; 
    else 
      baudValue /= 10;

    sp->at91UARTRegs->US_BRGR = baudValue; 

    sp->at91UARTRegs->US_MR = 
      ( ( sp->hardwareHandshake ) ? AT91C_US_USMODE_HWHSH : AT91C_US_USMODE_NORMAL ) |
      ( AT91C_US_CLKS_CLOCK ) |
      ( ( ( sp->bits - 5 ) << 6 ) & AT91C_US_CHRL ) |
      ( ( sp->stopBits == 2 ) ? AT91C_US_NBSTOP_2_BIT : AT91C_US_NBSTOP_1_BIT ) |
      ( ( sp->parity == 0 ) ? AT91C_US_PAR_NONE : ( ( sp->parity == -1 ) ? AT91C_US_PAR_ODD : AT91C_US_PAR_EVEN ) );
      // 2 << 14; // this last thing puts it in loopback mode


    sp->at91UARTRegs->US_CR = AT91C_US_RXEN | AT91C_US_TXEN; 
  }
  return CONTROLLER_OK;
}

int Serial_SetDefault( int index )
{
  if ( index < 0 || index >= SERIAL_PORTS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial_* sp = &Serial[ index ];

  sp->baud = 9600;
  sp->bits = 8;
  sp->stopBits = 1;
  sp->parity = 0;
  sp->hardwareHandshake = 0;

  sp->detailsInitialized = true;
  return CONTROLLER_OK;
}

#ifdef OSC
/** \defgroup SerialOSC Serial - OSC
  Configure the Serial Port and Read Characters via OSC.
  \ingroup OSC

  \section devices Devices
  There are 2 serial ports, so use 0 or 1 as an index.
	
	\section properties Properties
  The Serial Subsystem has the following properties:
  - baud
  - bits
  - stopbits
  - parity
  - hardwarehandshake
  - readable
  - char
  - block

	\subsection Baud
	The Baud rate of the device. Valid from 110 baud to >2M baud.
	To set baud rate to 115200 on the first serial port, for example, send the message
	\verbatim /serial/0/baud 112500\endverbatim
	
	\subsection Bits
	The number of bits per character.  Can range from 5 to 8.
	To set the number of bits to 7 on the first serial port, for example, send the message
	\verbatim /serial/0/bits 7\endverbatim
	
	\subsection StopBits
	The number of stop bits per character.  Can be 1 or 2\n
	To set the number of stop bits to 2 on the second serial port, for example, send the message
	\verbatim /serial/1/stopbits 2\endverbatim

	\subsection Parity
	The parity of the character.  Can be -1 for odd, 0 for none or 1 for even.
	To set the parity to even, for example, send the message
	\verbatim /serial/0/parity 1\endverbatim

	\subsection HardwareHandshake
	Whether hardware handshaking (i.e. CTS RTS) is being employed.
	To set hardware handshaking on, for example, send the message
	\verbatim /serial/0/hardwarehandshake 1\endverbatim

	\subsection Readable
	How many characters are presently available to read.
	To check, for example, send the message
	\verbatim /serial/0/readable\endverbatim

	\subsection Char
	Send and receive individual characters with the char property.
	To send a character 32 (a space), for example, send the message
	\verbatim /serial/0/char 32\endverbatim
	To get a character send the message
	\verbatim /serial/0/char\endverbatim
  In this case the reply will be an unsigned value between 0 and 255 or -1 if there is 
  no character available.

  \subsection Block
  This property allows for reading or writing a block of characters to/from the serial port.  If you're
  writing a block, you must send the block you want to write as an OSC blob.
  
  For example, to send a block:
	\verbatim /serial/1/block blockofchars\endverbatim
	To get a block, send the message
	\verbatim /serial/1/block\endverbatim
  In this case the reply will be a block of up to 100 unsigned chars, or the chars currently available to read
  from the serial port.

*/

#include "osc.h"
#include "string.h"
#include "stdio.h"

#include "types.h"

// Need a list of property names
// MUST end in zero
static char* SerialOsc_Name = "serial";
static char* SerialOsc_IntPropertyNames[] = { "active", "char", "baud", "bits", "stopbits", "parity", "hardwarehandshake", "readable", 0  }; // must have a trailing 0
static char* SerialOsc_BlobPropertyNames[] = { "block", 0  }; // must have a trailing 0

int SerialOsc_IntPropertySet( int index, int property, int value );
int SerialOsc_IntPropertyGet( int index, int property );

int SerialOsc_BlobPropertySet( int index, int property, uchar* blob, int length );
int SerialOsc_BlobPropertyGet( int index, int property, uchar* blob, int size );

// Returns the name of the subsystem
const char* SerialOsc_GetName( )
{
  return SerialOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int SerialOsc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_IndexIntReceiverHelper( channel, message, length, SERIAL_PORTS,
                                      SerialOsc_Name,
                                      SerialOsc_IntPropertySet, SerialOsc_IntPropertyGet, 
                                      SerialOsc_IntPropertyNames );

  if ( status != CONTROLLER_OK )
    status = Osc_IndexBlobReceiverHelper( channel, message, length, SERIAL_PORTS,
                                      SerialOsc_Name,
                                      SerialOsc_BlobPropertySet, SerialOsc_BlobPropertyGet, 
                                      SerialOsc_BlobPropertyNames );                        

  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, SerialOsc_Name, status );
  return CONTROLLER_OK;
}
// Set the index LED, property with the value
int SerialOsc_IntPropertySet( int index, int property, int value )
{
  switch ( property )
  {
    case 0: 
      Serial_SetActive( index, value );
      break;      
    case 1: 
      Serial_SetChar( index, value );
      break;      
    case 2: 
      Serial_SetBaud( index, value );
      break;      
    case 3: 
      Serial_SetBits( index, value );
      break;    
    case 4: 
      Serial_SetStopBits( index, value );
      break;
    case 5: 
      Serial_SetParity( index, value );
      break;    
    case 6: 
      Serial_SetHardwareHandshake( index, value );
      break;    
  }
  return CONTROLLER_OK;
}

// Get the index, property
int SerialOsc_IntPropertyGet( int index, int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      value = Serial_GetActive( index );
      break;
    case 1:
      value = Serial_GetChar( index );
      break;
    case 2:
      value = Serial_GetBaud( index );
      break;
    case 3:
      value = Serial_GetBits( index );
      break;
    case 4:
      value = Serial_GetStopBits( index );
      break;
    case 5:
      value = Serial_GetParity( index );
      break;
    case 6:
      value = Serial_GetHardwareHandshake( index );
      break;
    case 7:
      value = Serial_GetReadable( index );
      break;
  }
  
  return value;
}

// Get the index, property
int SerialOsc_BlobPropertyGet( int index, int property, uchar* blob, int maxSize )
{
  int xfer = 0;
  switch ( property )
  {
    case 0:
    {
      int length = Serial_GetReadable( index );
      xfer = ( length < maxSize ) ? length : maxSize;
      if ( xfer > 0 )
        Serial_Read( index, blob, xfer, 100 );
      break;
    }
  }
  
  return xfer;
}


// Set the index LED, property with the value
int SerialOsc_BlobPropertySet( int index, int property, uchar* blob, int length )
{
  switch ( property )
  {
    case 0: 
      Serial_Write( index, blob, length, 100 );
      break;      
  }
  return CONTROLLER_OK;
}

#endif
