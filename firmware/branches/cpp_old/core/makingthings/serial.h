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

#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"
#include "rtos.h"
#include "AT91SAM7X256.h"

#define SERIAL_PORTS 2
#define SERIAL_DEFAULT_BAUD        9600
#define SERIAL_DEFAULT_BITS        8
#define SERIAL_DEFAULT_STOPBITS    1
#define SERIAL_DEFAULT_PARITY      0
#define SERIAL_DEFAULT_HANDSHAKING 0
#define SERIAL_RX_BUF_SIZE         100

/**
  Send and receive data via the Make Controller's serial ports.

  There are 2 full serial ports on the Make Controller, and this library provides support for both of them.

  Control all of the common serial characteristics including:
  - \b baud - the speed of the connection (110 - > 2M) in baud or raw bits per second.  9600 baud is the default setting.
  - \b bits - the size of each character (5 - 8).  8 bits is the default setting.
  - \b stopBits - the number of stop bits transmitted (1 or 2)  1 stop bit is the default setting.
  - \b parity - the parity policy (-1 is odd, 0 is none and 1 is even).  Even is the default setting.
  - \b hardwareHandshake - whether hardware handshaking is used or not.  HardwareHandshaking is off by default.

  The subsystem is supplied with small input and output buffers (of 100 characters each) and at present
  the implementation is interrupt per character so it's not particularly fast.

  \todo Convert to DMA interface for higher performance, and add support for debug UART
*/
class Serial
{
  public:
    Serial( int channel, int q_size = 100 );
    void setBaud( int rate );
    int baud( );

    void setDataBits( int bits );
    int dataBits( );

    void setParity( int parity );
    int parity( );

    void setStopBits( int bits );
    int stopBits( );

    void setHandshaking( bool enable );
    bool handshaking( );

    int write( char character );
    int write( char* data, int length, int timeout = 0);
    int writeDMA(void *data, int length);
    int bytesAvailable( );
    bool anyBytesAvailable();
    int read( char* data, int length, int timeout = 0 );
    char read( int timeout = 0 );
    int readDMA( char* data, int length, int timeout = 0 );

    void flush( );
    void clearErrors( );
    bool errors( bool* overrun = 0, bool* frame = 0, bool* parity = 0 );
    void startBreak( );
    void stopBreak( );

  protected:
    int _channel, _baud, bits, _parity, _stopBits, _handshaking;
    void setDetails( );
    
    // static stuff
    typedef struct{
      AT91S_USART* uart;
      Queue* rxQueue;
      Queue* txQueue;
      Semaphore* rxSem;
      Semaphore* txSem;
      char rxBuf[SERIAL_RX_BUF_SIZE];
    } Internal;
    friend void SerialIsr_Handler( int index );
    static Internal internals[SERIAL_PORTS];
};

#endif // SERIAL_H
