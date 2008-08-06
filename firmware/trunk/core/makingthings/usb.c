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

/** \file usb.c	
	USB Subsystem.
	Functions for controlling the USB port on the Make Controller Board.
*/

#include "config.h"
#ifdef MAKE_CTRL_USB

#include "usb.h"
#include "rtos.h"
#include "stdio.h"
#include "USB-CDC.h"
#include "queue.h"
#include "string.h"

// SLIP codes
#define END             0300    // indicates end of packet 
#define ESC             0333    // indicates byte stuffing 
#define ESC_END         0334    // ESC ESC_END means END data byte 
#define ESC_ESC         0335    // ESC ESC_ESC means ESC data byte

extern xQueueHandle xTxCDC; 
extern xQueueHandle xRxCDC; 

static int Usb_Users;
// set by the running task when it gets started
int Usb_Running;

#define mainUSB_PRIORITY			( 4 )
#define mainUSB_TASK_STACK		( 600 )
#define TICKRATE 1000

void vUSBCDCTask( void *pvParameters );
static void Usb_WriteInternal(xBULKBUFFER *pq);
Usb_* Usb = NULL;

/** \defgroup USB USB
	The USB subsystem provides access to USB Virtual Serial Port (CDC) functionality.  
  
  On OS X, the system USB CDC driver is used - no external drivers are needed.
  An entry in \b /dev is created - similar to <b>/dev/cu.usbmodem.xxxx</b>.  It may be opened for reading and 
  writing like a regular file using the standard POSIX open(), close(), read(), write() functions.
  
  On Windows, the first time the device is seen, it needs 
  to be pointed to a .INF file containing additional information - the \b make_controller_kit.inf in 
  the same directory as this file.  Once Windows sets this up, the device can be opened as a normal
  COM port.  See also the \b mchelper codebase for code snippets.

  \todo expand usb support to include emulation of other kinds of USB device - mouse, keyboard, etc.
* \ingroup Core
* @{
*/

/**
	Sets whether the Usb subsystem is active.
	@param state An integer specifying the active state of the Usb system - 1 (on) or 0 (off).
	@return Zero on success.
*/
int Usb_SetActive( int state )
{
  if ( state )
  {
    if ( Usb_Users++ == 0 )
    {
      // Create the USB task.
      Usb = MallocWait( sizeof( Usb_ ), 100 );
      TaskCreate(  vUSBCDCTask, "USB", mainUSB_TASK_STACK, NULL, mainUSB_PRIORITY );

      while ( !Usb_Running )
        Sleep( 100 );
    }
  }
  else
  {
    if ( Usb_Users )
    {
      if ( --Usb_Users == 0 )
      {
        /* kill the task, someday */
      }
    }
  }
  return CONTROLLER_OK;
}

/**
	Read the active state of the Usb system.
	@return State - 1 (on) or 0 (off).
*/
int Usb_GetActive( )
{
  return Usb_Users > 0;
}


/**	
	Read from the USB port.  
  Pass in a pointer to a char buffer to read into, and the number of characters requested to read.
  This function will return immediately with the number of characters read.
	@param buffer A pointer to a char buffer to read into
  @param length An integer specifying the number of characters to read
	@return The number of characters successfully read.
*/
int Usb_Read(char *buffer, int length)
{
  static xBULKBUFFER DeQueue;
  static int queueRemaining = 0; // whether or not there's anything left over from last time.
  static unsigned char *queuePtr = DeQueue.Data; // pointer into our block buffer
  char *bufferPtr = buffer; // pointer into the buffer passed in

  while( bufferPtr - buffer < length )
  {
    if( queueRemaining ) // first check if there's anything left from a previous read
    {
      if( queuePtr - DeQueue.Data < DeQueue.Count )
        *bufferPtr++ = *queuePtr++;
      else 
        queueRemaining = 0;
    }
    else // nothing left to read, so grab more from the queue
    {
      if ( xQueueReceive( xRxCDC, &DeQueue, 0 ) == pdTRUE  ) // got a new message from USB waiting on the queue
      {
        queueRemaining = 1;
        queuePtr = DeQueue.Data;
      }
      else // nothing was available so we're done
        break;
    }
  }
  return bufferPtr - buffer;
}

/**	
	Write to the USB port.
  Pass in a pointer to a char buffer to from, and the number of characters requested to write.
	@param buffer A pointer to a char buffer to write from
  @param length An integer specifying the number of characters to write
	@return The number of characters successfully written
*/
int Usb_Write( char* buffer, int length )
{
  if ( !Usb_Users )
    Usb_SetActive( 1 );

  xBULKBUFFER q;
  q.Count = 0;
  int count = length;

  // load bytes into the BULKBUFFER and when it's full, send it and continue loading
  while( length-- )
  {
    q.Data[q.Count++] = *buffer++;
    if(q.Count == EP_FIFO)
      Usb_WriteInternal(&q);
  }

  // if bytes have been loaded into the BULKBUFFER but not sent because it's not full yet, send it now
  if( q.Count ) 
    Usb_WriteInternal(&q);
  
  return count;
}

// the outgoing USB hardware can write a block of 64 (EP_FIFO) bytes.
// test the buffer's size and send it if we're full.
void Usb_WriteInternal(xBULKBUFFER *pq)
{
  while(xQueueSend( xTxCDC, pq, 0) != pdPASS)
    Sleep( usbSHORTEST_DELAY );
  pq->Count = 0;
}

/**	
	Write to the USB port using SLIP codes to packetize messages.
  SLIP (Serial Line Internet Protocol) is a way to separate one "packet" from another on an open serial connection.
  This is the way OSC messages are sent over USB, for example.  SLIP uses a simple start/end byte and an escape
  byte in case your data actually contains the start/end byte.  Pass your normal buffer to this function to
  have the SLIP codes inserted and then write it out over USB.

  Check the Wikipedia description of SLIP at http://en.wikipedia.org/wiki/Serial_Line_Internet_Protocol
	@param buffer A pointer to a char buffer to write from
  @param length An integer specifying the number of characters to write
	@return The number of characters successfully written
*/
int Usb_SlipSend( char* buffer, int length )
{
  if( length > MAX_OUTGOING_SLIP_PACKET )
    return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;

  char *obp = Usb->slipSendBuffer, *bp = buffer;
  int count = length;

  *obp++ = (char)END;

  while( count-- )
  {
    switch(*bp)
    {
      // if it's the same code as an END character, we send a special 
      //two character code so as not to make the receiver think we sent an END
      case END:
        *obp++ = (char) ESC;
        *obp++ = (char) ESC_END;
        break;
        // if it's the same code as an ESC character, we send a special 
        //two character code so as not to make the receiver think we sent an ESC
      case ESC:
        *obp++ = (char) ESC;
        *obp++ = (char) ESC_ESC;
        break;
        //otherwise, just send the character
      default:
        *obp++ = *bp;
    }
    bp++;
  }
  
  *obp++ = END; // tell the receiver that we're done sending the packet
  int sendLength = obp - Usb->slipSendBuffer;
  Usb_Write( Usb->slipSendBuffer, sendLength );
  
  return CONTROLLER_OK;
}

/**	
	Read from the USB port using SLIP codes to de-packetize messages.
  SLIP (Serial Line Internet Protocol) is a way to separate one "packet" from another on an open serial connection.
  This is the way OSC messages are sent over USB, for example.  SLIP uses a simple start/end byte and an escape
  byte in case your data actually contains the start/end byte.  This function will block until it has received a 
  complete SLIP encoded message, and will pass back the original message with the SLIP codes removed.

  Check the Wikipedia description of SLIP at http://en.wikipedia.org/wiki/Serial_Line_Internet_Protocol
	@param buffer A pointer to a char buffer to read into
  @param length An integer specifying the number of characters to read
	@return The number of characters successfully written
*/
int Usb_SlipReceive( char* buffer, int length )
{
  if( length > MAX_INCOMING_SLIP_PACKET )
    return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;

  int started = 0, finished = 0, count = 0, i;
  char *pbp = Usb->slipReadBuffer;
  static int bufRemaining = 0;
  char *bp = buffer;

  while ( count < length )
  {
    if( !bufRemaining ) // if there's nothing left over from last time, get more
    {
      bufRemaining = Usb_Read( Usb->slipReadBuffer, MAX_INCOMING_SLIP_PACKET );
      pbp = Usb->slipReadBuffer;
    }

    for( i = 0; i < bufRemaining; i++ )
    {
      switch( *pbp )
      {
        case END:
          if( started && count ) // it was the END byte
            finished = true;
          else // skipping all starting END bytes
            started = true;
          break;
        case ESC:
          // get the next byte.  if it's not an ESC_END or ESC_ESC, it's a
          // malformed packet.  http://tools.ietf.org/html/rfc1055 says just
          // drop it in the packet in this case
          pbp++; 
          if( started )
          {
            if( *pbp == ESC_END )
            {
              *bp++ = END;
              count++;
              break;
            }
            else if( *pbp == ESC_ESC )
            {
              *bp++ = ESC;
              count++;
              break;
            }
          }
          // no break here
        default:
          if( started )
          {
            *bp++ = *pbp;
            count++;
          }
          break;
      }
      pbp++;
      bufRemaining--;
      if( finished )
        return count;
    }
    Sleep(1);
  }
  return CONTROLLER_ERROR_BAD_FORMAT; // should never get here
}

/** @}
*/

#ifdef OSC

#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* UsbOsc_Name = "usb";
static char* UsbOsc_PropertyNames[] = { "active", 0 }; // must have a trailing 0

int UsbOsc_PropertySet( int property, int value );
int UsbOsc_PropertyGet( int property );

// Returns the name of the subsystem
const char* UsbOsc_GetName( )
{
  return UsbOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int UsbOsc_ReceiveMessage( int channel, char* message, int length )
{
  return Osc_IntReceiverHelper( channel, message, length, 
                                UsbOsc_Name,
                                UsbOsc_PropertySet, UsbOsc_PropertyGet, 
                                UsbOsc_PropertyNames );
}

// Set the index Usb, property with the value
int UsbOsc_PropertySet( int property, int value )
{
  switch ( property )
  {
    case 0: 
      Usb_SetActive( value );
      break;      
  }
  return CONTROLLER_OK;
}

// Get the index Usb, property
int UsbOsc_PropertyGet( int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      value = Usb_GetActive( );
      break;
  }
  
  return value;
}

#endif  // OSC

#endif // MAKE_CTRL_USB


