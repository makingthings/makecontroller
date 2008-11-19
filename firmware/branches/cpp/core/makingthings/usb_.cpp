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

/// Size in bytes of the buffer used for reading data from the USB & USART
#define DATABUFFERSIZE BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CDCDSerialDriverDescriptors_DATAIN)

void USB::init( ) // static
{
  AppLed led(3);
  led.setState(1);
  
  AT91C_BASE_CKGR->CKGR_PLLR |= AT91C_CKGR_USBDIV_1; // Set the PLL USB Divider
  
  CDCDSerialDriver_Initialize();
  USBD_Connect();
  while (USBD_GetState() < USBD_STATE_CONFIGURED); // wait for things to get set up
  led.setState(0);
}

int USB::read( char *buffer, int length ) // static
{
  return 0;
}

int USB::write( char *buffer, int length ) // static
{
  return 0;
}

int USB::readSlip( char *buffer, int length ) // static
{
  return 0;
}

int USB::writeSlip( char *buffer, int length ) // static
{
  return 0;
}



