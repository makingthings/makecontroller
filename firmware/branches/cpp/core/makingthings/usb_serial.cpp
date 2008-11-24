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

#include "usb_serial.h"
#include "string.h"
#include "AT91SAM7X256.h"

// SLIP codes
#define END             0300    // indicates end of packet 
#define ESC             0333    // indicates byte stuffing 
#define ESC_END         0334    // ESC ESC_END means END data byte 
#define ESC_ESC         0335    // ESC ESC_ESC means ESC data byte

void onUsbData(void *pArg, unsigned char status, unsigned int received, unsigned int remaining);

UsbSerial* UsbSerial::_instance = 0;

UsbSerial::UsbSerial( )
{
  AT91C_BASE_CKGR->CKGR_PLLR |= AT91C_CKGR_USBDIV_1; // Set the PLL UsbSerial Divider
  CDCDSerialDriver_Initialize();
  USBD_Connect();
  while (USBD_GetState() < USBD_STATE_CONFIGURED); // wait for things to get set up
  readSemaphore.take( );
  justGot = 0;
  rxBufCount = 0;
}

void UsbSerial::init( ) // static
{
  if(!_instance)
    _instance = new UsbSerial( );
}

bool UsbSerial::isActive()
{
  return USBD_GetState() == USBD_STATE_CONFIGURED;
}

int UsbSerial::read( char *buffer, int length )
{
//  int readcount = 0;
  int length_to_go = length;
  while( length_to_go )
  {
    if( rxBufCount ) // do we already have some lying around?
    {
      int copylen = (rxBufCount > length_to_go) ? length_to_go : rxBufCount;
      memcpy( buffer, rxBuf, copylen );
      buffer += copylen;
      rxBufCount -= copylen;
      length_to_go -= copylen;
      continue; // re-evaluate how far along we've gotten
    }
    unsigned char result = USBD_Read(CDCDSerialDriverDescriptors_DATAOUT,
                                    rxBuf, USB_SER_RX_BUF_LEN, onUsbData, this);
    if(result == USBD_STATUS_SUCCESS)
    {
      if( readSemaphore.take( ) )
      {
        int copylen = (justGot > length_to_go) ? length_to_go : justGot;
        memcpy( buffer, rxBuf, copylen );
        buffer += copylen;
        rxBufCount -= copylen;
        length_to_go -= copylen;
        justGot = 0;
      }
    }
  }
  return length;
}

void onUsbData(void *pArg, unsigned char status, unsigned int received, unsigned int remaining)
{
  (void)remaining; // not much that can be done with this, I think - they're just dropped
  UsbSerial* usb = (UsbSerial*)pArg;
  if( status == USBD_STATUS_SUCCESS )
  {
    usb->rxBufCount += received;
    usb->justGot = received;
  }
  usb->readSemaphore.giveFromISR(0);
}

int UsbSerial::write( char *buffer, int length )
{
  unsigned char result = USBD_Write(CDCDSerialDriverDescriptors_DATAIN, buffer, length, 0, 0);
  return (result == USBD_STATUS_SUCCESS) ? 0 : -1;
}

int UsbSerial::readSlip( char *buffer, int length )
{
  if( length > MAX_INCOMING_SLIP_PACKET )
    return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;

  int started = 0, finished = 0, count = 0, i;
  char *pbp = slipInBuf;
  static int bufRemaining = 0;
  char *bp = buffer;

  while ( count < length )
  {
    if( !bufRemaining ) // if there's nothing left over from last time, get more
    {
      bufRemaining = read( slipInBuf, MAX_INCOMING_SLIP_PACKET );
      pbp = slipInBuf;
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
    Task::sleep(1);
  }
  return CONTROLLER_ERROR_BAD_FORMAT; // should never get here
  return 0;
}

int UsbSerial::writeSlip( char *buffer, int length )
{
  if( length > MAX_OUTGOING_SLIP_PACKET )
     return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;

   char *obp = slipOutBuf, *bp = buffer;
   *obp++ = (char)END;

   while( length-- )
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
   int sendLength = obp - slipOutBuf;
   write( slipOutBuf, sendLength );
   return CONTROLLER_OK;
}



