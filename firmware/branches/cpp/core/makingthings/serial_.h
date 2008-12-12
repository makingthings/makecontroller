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

#ifndef SERIAL__H
#define SERIAL__H

#include "types.h"
#include "rtos_.h"
#include "AT91SAM7X256.h"

#define SERIAL_PORTS 2
#define SERIAL_DEFAULT_BAUD        9600
#define SERIAL_DEFAULT_BITS        8
#define SERIAL_DEFAULT_STOPBITS    1
#define SERIAL_DEFAULT_PARITY      0
#define SERIAL_DEFAULT_HANDSHAKING 0

class Serial
{
  public:
    Serial( int channel, int q_size = 100 );
    void setBaud( int rate );
    int getBaud( );

    void setDataBits( int bits );
    int getDataBits( );

    void setParity( int parity );
    int getParity( );

    void setStopBits( int bits );
    int getStopBits( );

    void setHandshaking( bool enable );
    bool getHandshaking( );

    int write( char character );
    int write( char* data, int length, int timeout );
    int bytesAvailable( );
    int read( char* data, int length, int timeout );
    char readChar( );

    void flush( );
    void clearErrors( );
    bool getErrors( bool* overrun, bool* frame, bool* parity );
    void startBreak( );
    void stopBreak( );

  protected:
    int _channel, baud, bits, parity;
    int stopBits, handshaking;
    void setDetails( );
    
    // static stuff
    typedef struct{
      AT91S_USART* uart;
      Queue* rxQueue;
      Queue* txQueue;
    } Internal;
    friend void SerialIsr_Handler( );
    static Internal internals[SERIAL_PORTS];
};

#endif // SERIAL_H
