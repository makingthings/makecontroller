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
extern "C" {
  #include "CDCDSerialDriver.h"
  #include "CDCDSerialDriverDescriptors.h"
}

#include "appled_cpp.h"
#include "AT91SAM7X256.h"
#include "Board.h"

/// Size in bytes of the buffer used for reading data from the UsbSerial & USART
#define DATABUFFERSIZE BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CDCDSerialDriverDescriptors_DATAIN)

void onUsbData(void *pArg, unsigned char status, unsigned int received, unsigned int remaining);

UsbSerial* UsbSerial::_instance = 0;

UsbSerial::UsbSerial( )
{
  AT91C_BASE_CKGR->CKGR_PLLR |= AT91C_CKGR_USBDIV_1; // Set the PLL UsbSerial Divider
  CDCDSerialDriver_Initialize();
  USBD_Connect();
  while (USBD_GetState() < USBD_STATE_CONFIGURED); // wait for things to get set up
  readSemaphore.take( );
  justRead = 0;
  remaining = 0;
}

void UsbSerial::init( ) // static
{
  if(!_instance)
    _instance = new UsbSerial( );
}

int UsbSerial::read( char *buffer, int length )
{
  unsigned char result = USBD_Read(CDCDSerialDriverDescriptors_DATAOUT,
                                    buffer, length, onUsbData, this);
  int retval = -1;
  if(result == USBD_STATUS_SUCCESS)
  {
    if( readSemaphore.take( ) )
      retval = justRead;
  }
  return retval;
}

void onUsbData(void *pArg, unsigned char status, unsigned int received, unsigned int remaining)
{
  UsbSerial* usb = (UsbSerial*)pArg;
  if( status == USBD_STATUS_SUCCESS )
  {
    usb->justRead = received;
    usb->remaining = remaining;
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



