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

/** \file s2p->c	
	s2p->
	Functions for working with the Serial Interface on the Make Controller Board.
*/

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

#include "Serial2.h"
#include "Serial2_internal.h"

Serial2_ Serial2[ 2 ];

void Serial2_Isr( void ) __attribute__ ((naked));

static int Serial2_Init( int index );
static int Serial2_Deinit( int index );
static int Serial2_SetDefault( int index );
static int Serial2_SetDetails( int index );


/** \defgroup Serial
  Serial provides a way to send and receive data via the serial port.

  The subsystem is supplied with small input and output buffers (of 100 characters each) and at present
  the implementation is interrupt per character so it's not particularly fast.

  Permits all of the common serial characteristics to be set including
  
  baud - the speed of the connection (110 - >2M) in baud or raw bits per second.  9600 baud is the default setting.
  
  bits - the size of each character (5 - 8).  8 bits is the default setting.
  
  stopBits - the number of stop bits transmitted (1 or 2)  1 stop bit is the default setting.
  
  parity - the parity policy (-1 is odd, 0 is none and 1 is even).  Even is the default setting.
  
  hardwareHandshake - whether hardware handshaking is used or not.  HardwareHandshaking is off by default.

  There is also an OSC interface for getting and setting the interface specifics and
  for sending and receiving individual characters.

  \todo Need to complete support for the Hardware Handshaking
  \todo Convert to DMA interface for higher performance.

	\ingroup Controller
	@{
*/

/**
	Set the active state of the Serial subsystem.  This is automatically set to 
  true by any call to Serial2_Write or Serial2_Read.
	@param state An integer specifying the active state - 1 (on) or 0 (off).
	@return CONTROLLER_OK (=0) on success.
*/
int Serial2_SetActive( int index, int state )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  if ( state )
  {
    if ( s2p->users++ == 0 )
    {
      Serial2_Init( index );
    }
  }
  else
  {
    if ( s2p->users > 0 ) 
    {
      if ( --s2p->users == 0 )
      {
        Serial2_Deinit( index );
      }
    }
  }
  return CONTROLLER_OK;
}

/**
	Read the active state of the Serial subsystem.
	@return State - 1/non-zero (on) or 0 (off).
*/
int Serial2_GetActive( int index )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  return s2p->users > 0;
}

/**	
	Write a block of data to the Serial port.  Will block for the time specified (in ms)
  if the queue fills up.
	@param buffer A pointer to the buffer to write from.
	@param count An integer specifying the number of bytes to write.
  @param timeout Time in milliseconds to block waiting for the queue to free up. 0 means don't wait.
  @return status.
*/
int Serial2_Write( int index, uchar* buffer, int count, int timeout )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  if ( s2p->users == 0 )
  {
    int status = Serial2_SetActive( index, 1 );
    if ( status != CONTROLLER_OK )
      return status;
  }

  // Do the business
  while ( count )
  {
    if( xQueueSend( s2p->transmitQueue, buffer++, timeout ) == 0 ) 
      return CONTROLLER_ERROR_QUEUE_ERROR; 
    count--;
  }
   
  /* Turn on the Tx interrupt so the ISR will remove the character from the 
  queue and send it. This does not need to be in a critical section as 
  if the interrupt has already removed the character the next interrupt 
  will simply turn off the Tx interrupt again. */ 
  AT91C_BASE_US0->US_IER = AT91C_US_TXRDY; 
 
  return CONTROLLER_OK;
}

/**	
	Read data from the Serial port.  Will block for the time specified (in ms) if 
  there are insufficient characters.  Blocking can be avoided if Serial2_GetReadable( )
  is used to determine how many characters are available to read prior to calling
  this function.
	@param buffer A pointer to the buffer to read into.
	@param size An integer specifying the maximum number of bytes to read.
  @param timeout Time in milliseconds to block waiting for the specified number of bytes. 0 means don't wait.
  @return number of bytes read (>=0) or error <0 .
*/
int Serial2_Read( int index, uchar* buffer, int size, int timeout )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  if ( s2p->users == 0 )
  {
    int status = Serial2_SetActive( index, 1 );
    if ( status != CONTROLLER_OK )
      return status;
  }

  // Do the business
  int count = 0;
  while ( count < size )
  {
    /* Place the character in the queue of characters to be transmitted. */ 
    if( xQueueReceive( s2p->receiveQueue, buffer++, timeout ) == 0 )
      break;
    count++;
  }

  return count;
}

/**	
	Returns the number of bytes in the queue waiting to be read.
  @return bytes in the receive queue.
*/
int Serial2_GetReadable( int index )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  return uxQueueMessagesWaiting( s2p->receiveQueue );
}

/**	
	Sends a character (in the range of 0 to 255) to the write queue
	@param character The character to be sent.  Must be 0 <= c < 256.
  @return status.
*/
int Serial2_SetChar( int index, int character )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  if ( s2p->users == 0 )
  {
    int status = Serial2_SetActive( index, 1 );
    if ( status != CONTROLLER_OK )
      return status;
  }

  if ( character >= 0 && character < 256 )
  {
    unsigned char c = (unsigned char)character;
    if( xQueueSend( s2p->transmitQueue, &c, 0 ) == 0 ) 
      return CONTROLLER_ERROR_QUEUE_ERROR; 
    AT91C_BASE_US0->US_IER = AT91C_US_TXRDY; 
   }

  return CONTROLLER_OK;
}

/**	
	Sets the serial baud rate.
	@param baud The desired baud rate.
  @return status.
*/
int Serial2_SetBaud( int index, int baud )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  // If there are no default values, get some
  if ( !s2p->detailsInitialized )
    Serial2_SetDefault( index );

  s2p->baud = baud;
  Serial2_SetDetails( index );

  return CONTROLLER_OK;
}

/**	
	Sets the number of bits per character.  5 - 8 are legal values.  8 is the default.
	@param bits bits per character
  @return status.
*/
int Serial2_SetBits( int index, int bits )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  // If there are no default values, get some
  if ( !s2p->detailsInitialized )
    Serial2_SetDefault( index );

  if ( s2p->bits >= 5 && s2p->bits <= 8 )
    s2p->bits = bits;
  else
    s2p->bits = 8;

  Serial2_SetDetails( index );

  return CONTROLLER_OK;
}

/**	
	Sets the parity.  -1 is odd, 0 is none, 1 is even.  The default is none - 0.
	@param parity -1, 0 or 1.
  @return status.
*/
int Serial2_SetParity( int index, int parity )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  // If there are no default values, get some
  if ( !s2p->detailsInitialized )
    Serial2_SetDefault( index );

  if ( parity >= -1 && parity <= 1 )
    s2p->parity = parity;
  else
    s2p->parity = 1;
  Serial2_SetDetails( index );

  return CONTROLLER_OK;
}

/**	
	Sets the stop bits per character.  1 or 2 are legal values.  1 is the default.
	@param stopBits stop bits per character
  @return status.
*/
int Serial2_SetStopBits( int index, int stopBits )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  // If there are no default values, get some
  if ( !s2p->detailsInitialized )
    Serial2_SetDefault( index );

  if ( stopBits == 1 || stopBits == 2 )
    s2p->stopBits = stopBits;
  else
    s2p->stopBits = 1;

  Serial2_SetDetails( index );

  return CONTROLLER_OK;
}

/**	
	Sets whether hardware handshaking is being used.
	@param hardwareHandshake sets hardware handshaking on (1) or off (0)
  @return status.
*/
int Serial2_SetHardwareHandshake( int index, int hardwareHandshake )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  // If there are no default values, get some
  if ( !s2p->detailsInitialized )
    Serial2_SetDefault( index );

  s2p->hardwareHandshake = hardwareHandshake;
  Serial2_SetDetails( index );
  
  return CONTROLLER_OK;
}

/**	
	Returns a single character from the receive queue if available.  This character is returned
  unsigned - i.e. having a value of 0 - 255.  The return value is -1 if there is no character waiting.
  @return character from the queue or -1 if there is no character.
*/
int Serial2_GetChar( int index )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  if ( s2p->users == 0 )
  {
    int status = Serial2_SetActive( index, 1 );
    if ( status != CONTROLLER_OK )
      return -1;
  }

  if ( uxQueueMessagesWaiting( s2p->receiveQueue ) )
  {
    unsigned char c;
    if( xQueueReceive( s2p->receiveQueue, &c, 0 ) == 0 )
      return -1;
    else
      return (int)c;
  }
  else
    return -1; 
}

/**	
	Returns the current baud rate
  @return baud
*/
int Serial2_GetBaud( int index )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  // If there are no default values, get some
  if ( !s2p->detailsInitialized )
    Serial2_SetDefault( index );

  return s2p->baud;
}

/**	
	Returns the number of bits for each character
  @return bits
*/
int Serial2_GetBits( int index )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  // If there are no default values, get some
  if ( !s2p->detailsInitialized )
    Serial2_SetDefault( index );

  return s2p->bits;
}

/**	
	Returns the current parity.  -1 means odd, 0 means none, 1 means even
  @return parity
*/
int Serial2_GetParity( int index )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  // If there are no default values, get some
  if ( !s2p->detailsInitialized )
    Serial2_SetDefault( index );

  return s2p->parity;
}

/**	
	Returns the number of stop bits.
  @return stopBits
*/
int Serial2_GetStopBits( int index )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  // If there are no default values, get some
  if ( !s2p->detailsInitialized )
    Serial2_SetDefault( index );

  return s2p->stopBits;
}

/**	
	Returns whether hardware handshaking is being employed or not.
  @return hardwareHandshake
*/
int Serial2_GetHardwareHandshake( int index )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  // If there are no default values, get some
  if ( !s2p->detailsInitialized )
    Serial2_SetDefault( index );

  return s2p->hardwareHandshake;
}

/** @}
*/

int Serial2_Init( int index )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];
 
  // If there are no default values, get some
  if ( !s2p->detailsInitialized )
    Serial2_SetDefault( index );

  // Enable the peripheral clock
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_US0;

  int status;
   
  status = Io_StartBits( IO_PA00_BIT | IO_PA01_BIT, false  );
  if ( status != CONTROLLER_OK )
    return status;

  Io_SetPeripheralA( IO_PA00 );
  Io_SetPeripheralA( IO_PA01 );
  Io_PioDisable( IO_PA00 );
  Io_PioDisable( IO_PA01 );
  
  // Create the queues
  s2p->receiveQueue = xQueueCreate( 100, 1 ); 
  s2p->transmitQueue = xQueueCreate( 100, 1 ); 

  // Disable interrupts
  AT91C_BASE_US0->US_IDR = (unsigned int) -1;

  // Timeguard disabled
  AT91C_BASE_US0->US_TTGR = 0;

  // Most of the detail setting is done in here
  // Also Resets TXRX and re-enables RX
  Serial2_SetDetails( index );

  unsigned int mask = 0x1 << AT91C_ID_US0;		
                        
  /* Disable the interrupt on the interrupt controller */					
  AT91C_BASE_AIC->AIC_IDCR = mask ;										
  /* Save the interrupt handler routine pointer and the interrupt priority */	
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_US0 ] = (unsigned int)Serial2_Isr;			
  /* Store the Source Mode Register */									
  AT91C_BASE_AIC->AIC_SMR[ AT91C_ID_US0 ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 4  ;				
  /* Clear the interrupt on the interrupt controller */					
  AT91C_BASE_AIC->AIC_ICCR = mask ;					

  AT91C_BASE_AIC->AIC_IECR = mask;

  AT91C_BASE_US0->US_IER = AT91C_US_RXRDY; 


  return CONTROLLER_OK;
}

int Serial2_Deinit( int index )
{
  return CONTROLLER_OK;
}

int Serial2_SetDetails( int index )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  if ( s2p->users )
  {
     // Reset receiver and transmitter
    AT91C_BASE_US0->US_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS | AT91C_US_TXDIS; 

    int baudValue; 

    // MCK is 47923200 for the Make Controller Kit

    // Calculate ( * 10 )
    baudValue = ( MCK * 10 ) / ( s2p->baud * 16 );
    // Round (and / 10)
    if ( ( baudValue % 10 ) >= 5 ) 
      baudValue = ( baudValue / 10 ) + 1; 
    else 
      baudValue /= 10;

    AT91C_BASE_US0->US_BRGR = baudValue; 

    AT91C_BASE_US0->US_MR = 
      ( ( s2p->hardwareHandshake ) ? AT91C_US_USMODE_HWHSH : AT91C_US_USMODE_NORMAL ) |
      ( AT91C_US_CLKS_CLOCK ) |
      ( ( ( s2p->bits - 5 ) << 6 ) & AT91C_US_CHRL ) |
      ( ( s2p->stopBits == 1 ) ? AT91C_US_NBSTOP_1_BIT : AT91C_US_NBSTOP_1_BIT ) |
      ( ( s2p->parity == 0 ) ? AT91C_US_PAR_NONE : ( ( s2p->parity == -1 ) ? AT91C_US_PAR_ODD : AT91C_US_PAR_EVEN ) );
      // 2 << 14; // this last thing puts it in loopback mode


    AT91C_BASE_US0->US_CR = AT91C_US_RXEN | AT91C_US_TXEN; 
  }
  return CONTROLLER_OK;
}

int Serial2_SetDefault( int index )
{
  if ( index < 0 || index > 1 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  Serial2_* s2p = &Serial2[ index ];

  s2p->baud = 9600;
  s2p->bits = 8;
  s2p->stopBits = 1;
  s2p->parity = 0;
  s2p->hardwareHandshake = 0;

  s2p->detailsInitialized = true;
  return CONTROLLER_OK;
}

#ifdef OSC

/** \defgroup SerialOSC Serial - OSC
  Configure the Serial Port and Read Characters via OSC.
  \ingroup OSC
	
	\section properties Properties
	The Serial Subsystem has several configurational properties and a character getter
  and a character setter.

	\par Baud
	The Baud rate of the device. Valid from 110 baud to >2M baud\n
	To set baud rate to 115200, for example, send the message
	\verbatim /serial/baud 112500\endverbatim
	
	\par Bits
	The number of bits per character.  Can range from 5 to 8\n
	To set the number of bits to 7, for example, send the message
	\verbatim /serial/bits 7\endverbatim
	
	\par StopBits
	The number of stop bits per character.  Can be 1 or 2\n
	To set the number of stop bits to 2, for example, send the message
	\verbatim /serial/stopbits 2\endverbatim

	\par Parity
	The parity of the character.  Can be -1 for odd, 0 for none or 1 for even\n
	To set the parity to even, for example, send the message
	\verbatim /serial/parity 1\endverbatim

	\par HardwareHandshake
	Whether hardware handshaking (i.e. CTS RTS) is being employed.\n
	To set hardware handshaking on, for example, send the message
	\verbatim /serial/hardwarehandshaking 1\endverbatim

	\par Readable
	How many characters are presently available to read.\n
	To check, for example, send the message
	\verbatim /serial/readable\endverbatim

	\par Char
	This property is the mechanism by which individual characters can be sent and received\n
	To send a character 32 (a space), for example, send the message
	\verbatim /serial/char 32\endverbatim
	To get a character send the message
	\verbatim /serial/char\endverbatim
  In this case the reply will be an unsigned value between 0 and 255 or -1 if there is 
  no character available.

*/

#include "osc.h"
#include "string.h"
#include "stdio.h"

#include "types.h"

// Need a list of property names
// MUST end in zero
static char* Serial2Osc_Name = "serial2";
static char* Serial2Osc_IntPropertyNames[] = { "active", "char", "baud", "bits", "stopbits", "parity", "hardwarehandshake", "readable", 0  }; // must have a trailing 0
static char* Serial2Osc_BlobPropertyNames[] = { "block", 0  }; // must have a trailing 0

int Serial2Osc_IntPropertySet( int index, int property, int value );
int Serial2Osc_IntPropertyGet( int index, int property );

int Serial2Osc_BlobPropertySet( int index, int property, uchar* blob, int length );
int Serial2Osc_BlobPropertyGet( int index, int property, uchar* blob, int size );

// Returns the name of the subsystem
const char* Serial2Osc_GetName( )
{
  return Serial2Osc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int Serial2Osc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_IndexIntReceiverHelper( channel, message, length, 2,
                                      Serial2Osc_Name,
                                      Serial2Osc_IntPropertySet, Serial2Osc_IntPropertyGet, 
                                      Serial2Osc_IntPropertyNames );

  if ( status != CONTROLLER_OK )
    status = Osc_IndexBlobReceiverHelper( channel, message, length, 2,
                                      Serial2Osc_Name,
                                      Serial2Osc_BlobPropertySet, Serial2Osc_BlobPropertyGet, 
                                      Serial2Osc_BlobPropertyNames );                        

  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, Serial2Osc_Name, status );
  return CONTROLLER_OK;
}
// Set the index LED, property with the value
int Serial2Osc_IntPropertySet( int index, int property, int value )
{
  switch ( property )
  {
    case 0: 
      Serial2_SetActive( index, value );
      break;      
    case 1: 
      Serial2_SetChar( index, value );
      break;      
    case 2: 
      Serial2_SetBaud( index, value );
      break;      
    case 3: 
      Serial2_SetBits( index, value );
      break;    
    case 4: 
      Serial2_SetStopBits( index, value );
      break;
    case 5: 
      Serial2_SetParity( index, value );
      break;    
    case 6: 
      Serial2_SetHardwareHandshake( index, value );
      break;    
  }
  return CONTROLLER_OK;
}

// Get the index, property
int Serial2Osc_IntPropertyGet( int index, int property )
{
  int value;
  switch ( property )
  {
    case 0:
      value = Serial2_GetActive( index );
      break;
    case 1:
      value = Serial2_GetChar( index );
      break;
    case 2:
      value = Serial2_GetBaud( index );
      break;
    case 3:
      value = Serial2_GetBits( index );
      break;
    case 4:
      value = Serial2_GetStopBits( index );
      break;
    case 5:
      value = Serial2_GetParity( index );
      break;
    case 6:
      value = Serial2_GetHardwareHandshake( index );
      break;
    case 7:
      value = Serial2_GetReadable( index );
      break;
  }
  
  return value;
}

// Get the index LED, property
int Serial2Osc_BlobPropertyGet( int index, int property, uchar* blob, int maxSize )
{
  int xfer = 0;
  switch ( property )
  {
    case 0:
    {
      int length = Serial2_GetReadable( index );
      xfer = ( length < maxSize ) ? length : maxSize;
      if ( xfer > 0 )
        Serial2_Read( index, blob, xfer, 100 );
      break;
    }
  }
  
  return xfer;
}


// Set the index LED, property with the value
int Serial2Osc_BlobPropertySet( int index, int property, uchar* blob, int length )
{
  switch ( property )
  {
    case 0: 
      Serial2_Write( index, blob, length, 100 );
      break;      
  }
  return CONTROLLER_OK;
}

#endif
