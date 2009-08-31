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

// SLIP codes
#define END             0300    // indicates end of packet 
#define ESC             0333    // indicates byte stuffing 
#define ESC_END         0334    // ESC ESC_END means END data byte 
#define ESC_ESC         0335    // ESC ESC_ESC means ESC data byte

// transfer callbacks
static void UsbSerial_onUsbData(void *pArg, unsigned char status, unsigned int received, unsigned int remaining);
static void UsbSerial_onUsbWritten(void *pArg, unsigned char status, unsigned int received, unsigned int remaining);
// helper
static int UsbSerial_writeSlipIfFull( char** bufptr, char* buf, int timeout );

typedef struct {
  Semaphore readSemaphore;
  Semaphore writeSemaphore;
  int justGot;
  int justWrote;
  int rxBufCount;
  char rxBuf[USBSER_MAX_READ];
  char slipOutBuf[USBSER_MAX_WRITE];
  char slipInBuf[USBSER_MAX_READ];
  int slipInCount;
} UsbSerial;

static UsbSerial usbSerial;

void UsbSerial_begin( void )
{
  USBD_Disconnect(); // tell the host we're not attached
  CDCDSerialDriver_Initialize();
  USBD_Connect();
  usbSerial.readSemaphore = SemaphoreCreate();
  SemaphoreTake( usbSerial.readSemaphore, -1 );
  usbSerial.writeSemaphore = SemaphoreCreate();
  SemaphoreTake( usbSerial.writeSemaphore, -1 );
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
                                  usbSerial.rxBuf, USBSER_MAX_READ, UsbSerial_onUsbData, 0);
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
  UNUSED(remaining);
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
int UsbSerial_write( const char *buffer, int length, int timeout )
{
  int rv = 0;
  if( USBD_GetState() == USBD_STATE_CONFIGURED ) {
    if( USBD_Write(CDCDSerialDriverDescriptors_DATAIN, 
          buffer, length, UsbSerial_onUsbWritten, 0) == USBD_STATUS_SUCCESS ) {
      if( SemaphoreTake( usbSerial.writeSemaphore, timeout ) ) {
        rv = usbSerial.justWrote;
        usbSerial.justWrote = 0;
      }
    }
  }
  return rv;
}

void UsbSerial_onUsbWritten(void *pArg, unsigned char status, unsigned int received, unsigned int remaining)
{
  UNUSED(pArg);
  if( status == USBD_STATUS_SUCCESS ) {
    usbSerial.justWrote += received;
  }
  if( remaining <= 0 )
    SemaphoreGiveFromISR( usbSerial.writeSemaphore, 0 );
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
int UsbSerial_readSlip( char *buffer, int length, int timeout )
{
  int received = 0;
  static int idx = 0;
  char c;

  while ( received < length )
  {
    if( idx >= usbSerial.slipInCount ) { // if there's nothing left over from last time, get more
      usbSerial.slipInCount = UsbSerial_read( usbSerial.slipInBuf, USBSER_MAX_READ, timeout );
      idx = 0;
    }
    
    c = usbSerial.slipInBuf[idx++];
    switch( c )
    {
      case END:
        if( received ) // only return if we actually got anything
          return received;
        else
          break;
      case ESC:
        // get the next byte.  if it's not an ESC_END or ESC_ESC, it's a
        // malformed packet.  http://tools.ietf.org/html/rfc1055 says just
        // drop it in the packet in this case
        if( idx >= usbSerial.slipInCount ) break;
        c = usbSerial.slipInBuf[idx++];
        if( c == ESC_END )
          c = END;
        else if( c == ESC_ESC )
          c = ESC;
        // no break here
      default:
        buffer[received++] = c;
        break;
    }
  }
  return CONTROLLER_ERROR_BAD_FORMAT; // error if we get here
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
int UsbSerial_writeSlip( const char *buffer, int length, int timeout )
{
  char* obp = usbSerial.slipOutBuf;
  int count = 0;
  char c;
  *obp++ = END; // clear out any line noise
   while( length-- ) {
     c = *buffer++;
     switch(c)
     {
       // if it's the same code as an END character, we send a special 
       //two character code so as not to make the receiver think we sent an END
       case END:
         *obp++ = (char) ESC;
         count += UsbSerial_writeSlipIfFull(&obp, usbSerial.slipOutBuf, timeout );
         *obp++ = (char) ESC_END;
         count += UsbSerial_writeSlipIfFull(&obp, usbSerial.slipOutBuf, timeout );
         break;
         // if it's the same code as an ESC character, we send a special 
         //two character code so as not to make the receiver think we sent an ESC
       case ESC:
         *obp++ = (char) ESC;
         count += UsbSerial_writeSlipIfFull(&obp, usbSerial.slipOutBuf, timeout );
         *obp++ = (char) ESC_ESC;
         count += UsbSerial_writeSlipIfFull(&obp, usbSerial.slipOutBuf, timeout );
         break;
         //otherwise, just send the character
       default:
         *obp++ = c;
         count += UsbSerial_writeSlipIfFull(&obp, usbSerial.slipOutBuf, timeout );
     }
   }

   *obp++ = END; // end byte
   count += UsbSerial_write( usbSerial.slipOutBuf, (obp - usbSerial.slipOutBuf), timeout );
   return count;
}

int UsbSerial_writeSlipIfFull( char** bufptr, char* buf, int timeout )
{
  int bufSize = *bufptr - buf;
  if( bufSize >= USBSER_MAX_WRITE ) {
    *bufptr = buf;
    return UsbSerial_write( buf, bufSize, timeout );
  }
  else
    return 0;
}

#endif // MAKE_CTRL_USB

