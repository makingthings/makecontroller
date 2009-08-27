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

#include "usb_serial.h"

#ifdef MAKE_CTRL_USB

#include "error.h"

#include "string.h"
#include "AT91SAM7X256.h"
#include "Board.h"
#include "CDCDSerialDriver.h"
#include "CDCDSerialDriverDescriptors.h"

#include "rtos.h"

#define MAX_INCOMING_SLIP_PACKET 400
#define MAX_OUTGOING_SLIP_PACKET 600
#define USB_SER_RX_BUF_LEN BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CDCDSerialDriverDescriptors_DATAIN)

// SLIP codes
#define END             0300    // indicates end of packet 
#define ESC             0333    // indicates byte stuffing 
#define ESC_END         0334    // ESC ESC_END means END data byte 
#define ESC_ESC         0335    // ESC ESC_ESC means ESC data byte

static void UsbSerial_onUsbData(void *pArg, unsigned char status, unsigned int received, unsigned int remaining);

typedef struct {
  Semaphore readSemaphore;
  int justGot;
  int rxBufCount;
  char rxBuf[USB_SER_RX_BUF_LEN];
  char slipOutBuf[MAX_OUTGOING_SLIP_PACKET];
  char slipInBuf[MAX_INCOMING_SLIP_PACKET];
} UsbSerial;

static UsbSerial usbSerial;

void UsbSerial_begin( void )
{
  CDCDSerialDriver_Initialize();
  USBD_Connect();
  usbSerial.readSemaphore = SemaphoreCreate();
  SemaphoreTake( usbSerial.readSemaphore, -1 );
  usbSerial.justGot = 0;
  usbSerial.rxBufCount = 0;
}

/**
  Check if the USB system got set up OK.
  When things are starting up, if you want to wait until the USB is ready, 
  you can use this to check.  
  @return Whether the UsbSerial system is currently running.
  
  \b Example
  \code
  while( !UsbSerial_isActive() ) // while usb is not active
    Sleep(10);        // wait around for a little bit
  // now we're ready to go
  \endcode
*/
bool UsbSerial_isActive()
{
  return USBD_GetState() == USBD_STATE_CONFIGURED;
}

/**
  Read data from a USB host.
  This will read up to 64 bytes of data at a time, as this is the maximum USB transfer
  for the Make Controller internally.  If you want to read more than that, 
  keep calling read until you've got what you need.  
  
  If nothing is ready to be read, this will not return until new data arrives.
  @param buffer Where to store the incoming data.
  @param length How many bytes to read. 64 is the max that can be read at one time.
  @param timeout The number of milliseconds to wait if no data is available.  -1 means wait forever.
  @return The number of bytes successfully read.
  
  \b Example
  \code
  char mydata[128];
  // simplest is reading a short chunk
  int read = UsbSerial_read(mydata, 20);
  
  // or, we can wait until we've read more than the maximum of 64 bytes
  int got_so_far = 0;
  while(got_so_far < 128) // wait until we've read 128 bytes
  {
    int read = UsbSerial_read(mydata, (128 - got_so_far)); // read some new data
    got_so_far += read; // add to how much we've gotten so far
  }
  \endcode
*/
int UsbSerial_read( char *buffer, int length, int timeout )
{
  if( USBD_GetState() != USBD_STATE_CONFIGURED )
    return 0;
  int length_to_go = length;
  if( usbSerial.rxBufCount ) { // do we already have some lying around?
    int copylen = MIN(usbSerial.rxBufCount, length_to_go);
    memcpy( buffer, usbSerial.rxBuf, copylen );
    buffer += copylen;
    usbSerial.rxBufCount -= copylen;
    length_to_go -= copylen;
  }
  if(length_to_go) { // if we still would like to get more
    unsigned char result = USBD_Read(CDCDSerialDriverDescriptors_DATAOUT,
                                  usbSerial.rxBuf, USB_SER_RX_BUF_LEN, UsbSerial_onUsbData, 0);
    if(result == USBD_STATUS_SUCCESS) {
      if( SemaphoreTake( usbSerial.readSemaphore, timeout ) ) {
        int copylen = MIN(usbSerial.justGot, length_to_go);
        memcpy( buffer, usbSerial.rxBuf, copylen );
        buffer += copylen;
        usbSerial.rxBufCount -= copylen;
        length_to_go -= copylen;
        usbSerial.justGot = 0;
      }
    }
  }
  return length - length_to_go;
}

void UsbSerial_onUsbData(void *pArg, unsigned char status, unsigned int received, unsigned int remaining)
{
  UNUSED(pArg);
  UNUSED(remaining); // not much that can be done with this, I think - they're just dropped
  // in fact, we're pretty unlikely to ever see it since we're always trying to read as much as can be transferred at once
  if( status == USBD_STATUS_SUCCESS ) {
    usbSerial.rxBufCount += received;
    usbSerial.justGot = received;
  }
  SemaphoreGiveFromISR( usbSerial.readSemaphore, 0 );
}

/**
  Write data to a USB host.
  @param buffer The data to send.
  @param length How many bytes to send.
  @return The number of bytes successfully written.
  
  \b Example
  \code
  int written = UsbSerial_write( "hi hi", 5 );
  \endcode
*/
int UsbSerial_write( const char *buffer, int length )
{
  int rv = 0;
  if( USBD_GetState() == USBD_STATE_CONFIGURED ) {
    if( USBD_Write(CDCDSerialDriverDescriptors_DATAIN, buffer, length, 0, 0) == USBD_STATUS_SUCCESS )
      rv = length;
  }
  return rv;
}

/**
  Read from the USB port using SLIP codes to de-packetize messages.
  SLIP (Serial Line Internet Protocol) is a way to separate one "packet" from another 
  on an open serial connection.  This is the way OSC messages are sent over USB, for example.  
  
  SLIP uses a simple start/end byte and an escape byte in case your data actually 
  contains the start/end byte.  This function will not return until it has received a complete 
  SLIP encoded message, and will pass back the original message with the SLIP codes removed.

  Check the Wikipedia description of SLIP at http://en.wikipedia.org/wiki/Serial_Line_Internet_Protocol
  @param buffer Where to store the incoming data.
  @param length The number of bytes to read.
  @return The number of characters successfully read.
  @see read() for a similar example
*/
int UsbSerial_readSlip( char *buffer, int length )
{
  if( length > MAX_INCOMING_SLIP_PACKET )
    return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;

  int started = 0, finished = 0, count = 0, i;
  char *pbp = usbSerial.slipInBuf;
  static int bufRemaining = 0;
  char *bp = buffer;

  while ( count < length )
  {
    if( !bufRemaining ) { // if there's nothing left over from last time, get more
      bufRemaining = UsbSerial_read( usbSerial.slipInBuf, MAX_INCOMING_SLIP_PACKET, -1 );
      pbp = usbSerial.slipInBuf;
    }

    for( i = 0; i < bufRemaining; i++ ) {
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
          if( started ) {
            if( *pbp == ESC_END ) {
              *bp++ = END;
              count++;
              break;
            }
            else if( *pbp == ESC_ESC ) {
              *bp++ = ESC;
              count++;
              break;
            }
          }
          // no break here
        default:
          if( started ) {
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

/**
  Write to the USB port using SLIP codes to packetize messages.
  SLIP (Serial Line Internet Protocol) is a way to separate one "packet" from 
  another on an open serial connection.  This is the way OSC messages are sent over USB, 
  for example.  SLIP uses a simple start/end byte and an escape byte in case your data
  actually contains the start/end byte.  Pass your normal buffer to this function to
  have the SLIP codes inserted and then write it out over USB.

  Check the Wikipedia description of SLIP at http://en.wikipedia.org/wiki/Serial_Line_Internet_Protocol
  @param buffer The data to write.
  @param length The number of bytes to write.
  @return The number of characters successfully written.
  @see write() for a similar example.
*/
int UsbSerial_writeSlip( const char *buffer, int length )
{
  if( length > MAX_OUTGOING_SLIP_PACKET )
     return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;

   char* obp = usbSerial.slipOutBuf;
   const char* bp = buffer;
   *obp++ = (char)END;

   while( length-- ) {
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
   int sendLength = obp - usbSerial.slipOutBuf;
   UsbSerial_write( usbSerial.slipOutBuf, sendLength );
   return CONTROLLER_OK;
}

#endif // MAKE_CTRL_USB

