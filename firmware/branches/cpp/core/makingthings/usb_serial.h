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

#ifndef USB__H
#define USB__H

#include "config.h"
#include "rtos_.h"
#include "Board.h"
extern "C" {
  #include "CDCDSerialDriver.h"
  #include "CDCDSerialDriverDescriptors.h"
}

// #define MAX_INCOMING_SLIP_PACKET 400
// #define MAX_OUTGOING_SLIP_PACKET 600

#define USB UsbSerial::instance()
#define USB_SER_RX_BUF_LEN BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CDCDSerialDriverDescriptors_DATAIN)

class UsbSerial
{
public:
  void init( );
  bool isActive();
  int read( char *buffer, int length );
  int write( char *buffer, int length );
  int readSlip( char *buffer, int length );
  int writeSlip( char *buffer, int length );
  static UsbSerial* instance( )
  {
    if( !_instance )
      _instance = new UsbSerial();
    return _instance;
  }

protected:
  UsbSerial( );
  static UsbSerial* _instance; // the only instance of UsbSerial anywhere.
  friend void onUsbData( void *pArg, unsigned char status, unsigned int transferred, unsigned int remaining);
  Semaphore readSemaphore;
  Queue* rxQueue;
  int justGot;
  int rxBufCount;
  static char rxBuf[];
};

#endif // USB__H
