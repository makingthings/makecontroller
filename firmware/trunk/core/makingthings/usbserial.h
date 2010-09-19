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

#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#include "types.h"
#include "board.h"
#include "usb/device/cdc-serial/CDCDSerialDriver.h"
#include "usb/device/cdc-serial/CDCDSerialDriverDescriptors.h"

#define USBSER_MAX_READ BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CDCDSerialDriverDescriptors_DATAOUT)
#define USBSER_MAX_WRITE BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CDCDSerialDriverDescriptors_DATAIN)

#ifdef __cplusplus
extern "C" {
#endif
void usbserialInit(void);
bool usbserialIsActive(void);
int  usbserialAvailable(void);
int  usbserialRead(char *buffer, int length, int timeout);
char usbserialGet(void);
int  usbserialWrite(const char *buffer, int length, int timeout);
int  usbserialPut(char c);
int  usbserialReadSlip(char *buffer, int length, int timeout);
int  usbserialWriteSlip(const char *buffer, int length, int timeout);
#ifdef __cplusplus
}
#endif

#endif // USB_SERIAL_H
