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

/** \file usb.c	
	USB Subsystem.
	Functions for controlling the USB port on the Make Controller Board.
*/

#include "usb.h"
#include "rtos.h"
#include "stdio.h"
#include "USB-CDC.h"
#include "config.h"
#include "queue.h"
#include "string.h"

int Usb_Write(xBULKBUFFER *q);
int Usb_Read(char *buffer, int length);

extern xQueueHandle xTxCDC; 
extern xQueueHandle xRxCDC; 


static int Usb_Users;
// set by the running task when it gets started
int Usb_Running;

#define mainUSB_PRIORITY			( 4 )
#define mainUSB_TASK_STACK		( 600 )
#define TICKRATE 1000

void vUSBCDCTask( void *pvParameters );

/** \defgroup Usb
	The Usb subsystem provides access to USB CDC functionality.  On Mac OS X, when this
  subsystem is running and plugged in, a new serial device is created.  The filename 
  that appears is similar to /dev/cu.usbmodem.xxxx.  It may be opened for reading and 
  writing like a regular file.  On Windows, the first time the device is seen, it needs 
  to be pointed to a .INF file containing additional information.  The INF file is in 
  the same directory as this file.  When this is set up, the device can be opened as a 
  COM: port.  See also the MC Helper codebase for code snippets.
  \todo expanded usb support to include emulation of other kinds of USB device - mouse, 
        keyboard, etc.
  \todo rename the INF file to make.inf
* \ingroup Controller
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
      /* Create the USB task.*/
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
	Get a character from the USB port.  If the USB subsystem is not already initialized,
  a call to this function will do so.  If this is the case, this first call may take a little
  while (1 second?) to return.  The return value is an integer to accomodate full 8 bit
  communication and still provide a mechanism to return "no character" - the -1.
	@param timeout an integer specifying, in milliseconds, how long to wait for a character before quitting
	@return the latest character from the USB port, or -1 if there was none
*/
#define MAX_BUFFER_LENGTH 400
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
	Send a character out on the USB port.   If the USB subsystem is not already initialized,
  a call to this function will do so.  If this is the case, this first call may take a little
  while (1 second?) to return.
	@param c the character to send out
*/
#if 0
int Usb_Put( int c, int timeout )
{
  if ( !Usb_Users )
    Usb_SetActive( 1 );

  vUSBSendByte( (char)c, timeout / TICKRATE );
  return CONTROLLER_OK;
}
#else

int Usb_Write(xBULKBUFFER *q)
{
  if ( !Usb_Users )
    Usb_SetActive( 1 );

  while(xQueueSend( xTxCDC, q, 0) != pdPASS)
    {
      Sleep( usbSHORTEST_DELAY );
    }
  
  return CONTROLLER_OK;
}
#endif
#if 0
int QueueAndTest(xBULKBUFFER *pq, char newByte, int i)
{
  pq->Data[i++] = newByte;

  if (i == EP_FIFO) 
    { 
      queue.Count = i; 
      Usb_Write(&queue); 
      i = 0; 
    }

  return i;
}
#endif


// SLIP codes
#define END             0300    // indicates end of packet 
#define ESC             0333    // indicates byte stuffing 
#define ESC_END         0334    // ESC ESC_END means END data byte 
#define ESC_ESC         0335    // ESC ESC_ESC means ESC data byte

int Usb_SlipSend( char* buffer, int length )
{
  xBULKBUFFER queue;
  int i = 0;

  queue.Data[i++] = (char) END;

  while( length-- )
    {
      switch(*buffer)
	{
	  // if it's the same code as an END character, we send a special 
	  //two character code so as not to make the receiver think we sent an END
	case END:
	  queue.Data[i++] = (char) ESC;
	  if (i == EP_FIFO) { queue.Count = i; Usb_Write(&queue); i = 0; }
	  queue.Data[i++] = (char) ESC_END;
	  if (i == EP_FIFO) { queue.Count = i; Usb_Write(&queue); i = 0; }
	  break;
	  
	  // if it's the same code as an ESC character, we send a special 
	  //two character code so as not to make the receiver think we sent an ESC
	case ESC:
	  queue.Data[i++] = (char) ESC;
	  if (i == EP_FIFO) { queue.Count = i; Usb_Write(&queue); i = 0; }
	  queue.Data[i++] = (char) ESC_ESC;
	  if (i == EP_FIFO) { queue.Count = i; Usb_Write(&queue); i = 0; }
	  break;
	  //otherwise, just send the character
	default:
	  queue.Data[i++] = *buffer;
	  if (i == EP_FIFO) { queue.Count = i; Usb_Write(&queue); i = 0; }
	}
      buffer++;
    }
  
  // tell the receiver that we're done sending the packet
  queue.Data[i++] = END;
  queue.Count = i; 
  Usb_Write(&queue);
  
  return CONTROLLER_OK;
}

/* 
 */

int Usb_SlipReceive( char* buffer, int length )
{
  int started = 0, count = 0;
  int justGot;
  char parseBuf[EP_FIFO];
  char* bp = buffer, *pbp;

  while ( count < length )
  {
    justGot = Usb_Read( parseBuf, length );
    pbp = parseBuf;

    int i;
    for( i = 0; i < justGot; i++ )
    {
      switch( *pbp )
      {
        case END:
          if( started && count ) // it was the END byte
            return count; // We're done now if we had received any characters
          else // skipping all starting END bytes
            started = true;
          break;					
        case ESC:
          // if it's the same code as an ESC character, we just want to skip it and 
          // stick the next byte in the packet
          pbp++;
          // no break here, just stick it in the packet		
        default:
          *bp++ = *pbp;
          count++;
      }
      pbp++;
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
  int value;
  switch ( property )
  {
    case 0:
      value = Usb_GetActive( );
      break;
  }
  
  return value;
}

#endif  // OSC


