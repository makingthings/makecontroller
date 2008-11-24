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

#include "usb_.h"
#include "string.h"
#include "AT91SAM7X256.h"

/// Size in bytes of the buffer used for reading data from the UsbSerial & USART

void onUsbData(void *pArg, unsigned char status, unsigned int received, unsigned int remaining);

UsbSerial* UsbSerial::_instance = 0;
char UsbSerial::rxBuf[USB_SER_RX_BUF_LEN];

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
  (void)buffer;
  (void)length;
  return 0;
}

int UsbSerial::writeSlip( char *buffer, int length )
{
  (void)buffer;
  (void)length;
  return 0;
}



