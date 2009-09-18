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

#include "config.h"
#ifdef MAKE_CTRL_USB
#include "rtos.h"

extern "C" {
  #include "Board.h"
  #include "CDCDSerialDriver.h"
  #include "CDCDSerialDriverDescriptors.h"
}

#define MAX_INCOMING_SLIP_PACKET 400
#define MAX_OUTGOING_SLIP_PACKET 600
#define USB_SER_RX_BUF_LEN BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CDCDSerialDriverDescriptors_DATAIN)

// class Semaphore;

/**
  Virutal serial port USB communication.
  This allows the Make Controller look like a serial modem to your desktop, which it can then easily
  open up, read and write to.
  
  \section Usage
  There's only one UsbSerial object in the system, so you never create your own.  You can access
  it through the get() method.
  
  \code
  UsbSerial* usb = UsbSerial::get(); // first get a reference to the central UsbSerial object
  usb->write("hello", 5); // write a little something
  
  char buffer[128];
  int got = usb->read(buffer, 128); // and read
  \endcode
  
  \section Drivers
  On OS X, the system driver is used - no external drivers are needed.
  An entry in \b /dev is created - similar to <b>/dev/cu.usbmodem.xxxx</b>.  It may be opened for reading and 
  writing like a regular file using the standard POSIX open(), close(), read(), write() functions.

  On Windows, the first time the device is seen, it needs 
  to be pointed to a .INF file containing additional information - the \b make_controller_kit.inf in 
  the same directory as this file.  Once Windows sets this up, the device can be opened as a normal
  COM port.  If you've installed mchelper or mcbuilder, this file is already in the right spot
  on your system.
*/

class UsbSerial
{
public:
  bool isActive();
  int read( char *buffer, int length, int timeout = -1 );
  int write( const char *buffer, int length );
  int readSlip( char *buffer, int length );
  int writeSlip( const char *buffer, int length );
  static UsbSerial* get();

protected:
  UsbSerial( );
  static UsbSerial* _instance; // the only instance of UsbSerial anywhere.
  Semaphore readSemaphore;
  int justGot, rxBufCount;
  char rxBuf[USB_SER_RX_BUF_LEN];
  char slipOutBuf[MAX_OUTGOING_SLIP_PACKET];
  char slipInBuf[MAX_INCOMING_SLIP_PACKET];
  
  friend void onUsbData( void *pArg, unsigned char status, unsigned int transferred, unsigned int remaining);
};

#endif // MAKE_CTRL_USB

#endif // USB_SERIAL_H
