/*********************************************************************************

 Copyright 2006-2010 MakingThings

 Licensed under the Apache License,
 Version 2.0 (the "License"); you may not use this file except in compliance
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#include "serial.h"
#include "config.h"
#include "ch.h"
#include "hal.h"

#ifndef SERIAL_DEFAULT_PARITY
#define SERIAL_DEFAULT_PARITY 0
#endif

#ifndef SERIAL_DEFAULT_CHARBITS
#define SERIAL_DEFAULT_CHARBITS 8
#endif

#ifndef SERIAL_DEFAULT_STOPBITS
#define SERIAL_DEFAULT_STOPBITS 1
#endif

#ifndef SERIAL_DEFAULT_HANDSHAKE
#define SERIAL_DEFAULT_HANDSHAKE NO
#endif

/**
  \defgroup serial Serial
  Read and write bytes over the serial port.
  The Make Controller has 2 full serial ports.  Control all of the common serial characteristics including:
  - baud - the speed of the connection (110 - > 2M) in baud or raw bits per second.
  - bits - the size of each character (5 - 8).
  - stopBits - the number of stop bits transmitted (1 or 2).
  - parity - the parity policy (-1 is odd, 0 is none and 1 is even).
  - hardwareHandshake - whether hardware handshaking is used or not.

  \section Usage
  First, enable the serial port.  If the default settings are OK for you, serialEnableDefault() can be
  a little easier, or you can customize the settings for the port with serialEnable().  You can check
  how much data is available to be read at any time via serialAvailable().  Read and write with
  serialRead() and serialWrite().

  \code
  void myTask() {
    char serialdata[56];    // a buffer for storing the data we read
    serialEnable(0, 9600);  // enable serial port 0 at 9600 baud
    // now wait for bytes to arrive, and read them if possible
    while (1) {
      int bytes = serialAvailable(0);
      if (bytes > 0) {
        serialRead(0, serialdata, bytes, 0);
      }
    }
  }
  \endcode

  By default, the buffer for each serial port is 64 bytes.  You can customize this by
  defining \b SERIAL_BUFFERS_SIZE in your config.h file.

  \ingroup interfacing
  @{
*/

/**
  Initialize the serial system.
  This is done by default during system startup, but you can disable it by defining \b NO_SERIAL_INIT
  in your config.h file.
*/
void serialInit()
{
  sdInit();
}

/**
  Enable a serial port.
  If you need to customize the settings, use serialEnableAll().

  Otherwise, the default settings can all be redefined in your config.h file.  Their names and default values are:
  - SERIAL_DEFAULT_PARITY 0
  - SERIAL_DEFAULT_CHARBITS 8
  - SERIAL_DEFAULT_STOPBITS 1
  - SERIAL_DEFAULT_HANDSHAKE NO

  @param port Which serial port - valid options are 0 and 1.
  @param baud The rate of this serial port - common options are 9600, 34800, 57600, 115200.
*/
void serialEnable(int port, int baud)
{
  serialEnableAll(port, baud, SERIAL_DEFAULT_PARITY, SERIAL_DEFAULT_CHARBITS,
                SERIAL_DEFAULT_STOPBITS, SERIAL_DEFAULT_HANDSHAKE);
}

/**
  Enable a serial port, specifying all the details.
  @param port Which serial port - valid options are 0 and 1.
  @param baud The rate of this serial port - common options are 9600, 34800, 57600, 115200.
  @param parity -1 is odd, 0 is none, 1 is even. The default is none - 0.
  @param charbits The number of bits in a character - valid options are 5-8, default is 8.
  @param stopbits The stop bits per character - valid options are 1 or 2, 1 is the default.
  @param handshake Whether hardware handshake is enabled.

  \b Example
  \code
  // set up port 0 for 9600 baud
  serialEnable(0, 9600, 0, 8, 1, NO);
  \endcode
*/
void serialEnableAll(int port, int baud, int parity, int charbits, int stopbits, bool handshake)
{
  SerialConfig config = {
    baud,
    AT91C_US_CLKS_CLOCK |
    (handshake ? AT91C_US_USMODE_HWHSH : AT91C_US_USMODE_NORMAL) |
    (((charbits - 5) << 6) & AT91C_US_CHRL) |
    (stopbits == 2 ? AT91C_US_NBSTOP_2_BIT : AT91C_US_NBSTOP_1_BIT) |
    (parity == 0 ? AT91C_US_PAR_NONE : (parity == -1 ? AT91C_US_PAR_ODD : AT91C_US_PAR_EVEN))
  };

#if USE_SAM7_USART0
  if (port == 0) {
    if (handshake == YES) {
      AT91C_BASE_PIOA->PIO_PDR   = AT91C_PA3_RTS0 | AT91C_PA4_CTS0;
      AT91C_BASE_PIOA->PIO_ASR   = AT91C_PA3_RTS0 | AT91C_PA4_CTS0;
      AT91C_BASE_PIOA->PIO_PPUDR = AT91C_PA3_RTS0 | AT91C_PA4_CTS0;
    }
    sdStart(&SD1, &config);
  }
#endif
#if USE_SAM7_USART1
  if (port == 1) {
    // careful - PA8/PA9 are SPI/EEPROM CS lines, but include these if you need them
    if (handshake == YES) {
      AT91C_BASE_PIOA->PIO_PDR   = AT91C_PA8_RTS1 | AT91C_PA9_CTS1;
      AT91C_BASE_PIOA->PIO_ASR   = AT91C_PA8_RTS1 | AT91C_PA9_CTS1;
      AT91C_BASE_PIOA->PIO_PPUDR = AT91C_PA8_RTS1 | AT91C_PA9_CTS1;
    }
    sdStart(&SD2, &config);
  }
#endif
}

/**
  Disable a previously enabled serial port.
  @param port Which serial port - valid options are 0 and 1.
*/
void serialDisable(int port)
{
#if USE_SAM7_USART0
  if (port == 0) sdStop(&SD1);
#endif
#if USE_SAM7_USART1
  if (port == 1) sdStop(&SD2);
#endif
}

/**
  Read the number of bytes available to be read from a serial port.
  This returns the number of bytes that have already been received,
  such that you can be sure a serialRead() won't wait for data to arrive.
  @param port Which serial port - valid options are 0 and 1.

  \b Example
  \code
  char buffer[20];
  int avail = serialAvailable(0); // how many on serial port 0
  if (avail > 0) {
    serialRead(0, buffer, avail, 0); // read the available bytes with no timeout
  }
  \endcode
*/
int serialAvailable(int port)
{
#if USE_SAM7_USART0
  if (port == 0) return chQSpace(&SD1.sd.iqueue);
#endif
#if USE_SAM7_USART1
  if (port == 1) return chQSpace(&SD2.sd.iqueue);
#endif
  return 0;
}

/**
  Read data from a serial port.
  @param port Which serial port - valid options are 0 and 1.
  @param buf The buffer to read the data into.
  @param len The number of bytes to read
  @param timeout How long to wait (in milliseconds) for data to arrive.
  @return The number of bytes read - remember, this may be fewer than you asked for.

  \b Example
  \code
  // try to read 28 bytes, and wait 50 milliseconds if they're not available yet.
  char serialbuf[28];
  int got = serialRead(0, serialbuf, 28, 50);
  \endcode
*/
int serialRead(int port, char* buf, int len, int timeout)
{
#if USE_SAM7_USART0
  if (port == 0)
    return sdReadTimeout(&SD1, (uint8_t*)buf, (size_t)len, MS2ST(timeout));
#endif
#if USE_SAM7_USART1
  if (port == 1)
    return sdReadTimeout(&SD2, (uint8_t*)buf, (size_t)len, MS2ST(timeout));
#endif
  return 0;
}

/**
  Get a single character from the serial port.
  @param port Which serial port - valid options are 0 and 1.
  @param timeout How long to wait (in milliseconds) for data to arrive.
  @return The character received.
*/
char serialGet(int port, int timeout)
{
#if USE_SAM7_USART0
  if (port == 0)
    return chIQGetTimeout(&SD1.sd.iqueue, MS2ST(timeout));
#endif
#if USE_SAM7_USART1
  if (port == 1)
    return chIQGetTimeout(&SD2.sd.iqueue, MS2ST(timeout));
#endif
  return 0;
}

/**
  Write data to a serial port.
  @param port Which serial port - valid options are 0 and 1.
  @param buf The buffer containing the data to write.
  @param len The number of bytes to write.
  @param timeout How long to wait (in milliseconds) for data to be written.
  @return The number of bytes successfully written.

  \b Example
  \code
  char serialbuf[3];
  serialbuf[0] = 1;
  serialbuf[1] = 2;
  serialbuf[2] = 3;
  int wrote = serialWrite(0, serialbuf, 3, 0);
  \endcode
*/
int serialWrite(int port, char* buf, int len, int timeout)
{
#if USE_SAM7_USART0
  if (port == 0)
    return sdWriteTimeout(&SD1, (uint8_t*)buf, (size_t)len, MS2ST(timeout));
#endif
#if USE_SAM7_USART1
  if (port == 1)
    return sdWriteTimeout(&SD2, (uint8_t*)buf, (size_t)len, MS2ST(timeout));
#endif
  return 0;
}

/**
  Write a single character to the serial port.
  @param port Which serial port - valid options are 0 and 1.
  @param c The character to write.
  @param timeout How long to wait (in milliseconds) for data to be written.
*/
int serialPut(int port, char c, int timeout)
{
#if USE_SAM7_USART0
  if (port == 0)
    return chOQPutTimeout(&SD1.sd.oqueue, (uint8_t)c, MS2ST(timeout));
#endif
#if USE_SAM7_USART1
  if (port == 1)
    return chOQPutTimeout(&SD2.sd.oqueue, (uint8_t)c, MS2ST(timeout));
#endif
  return 0;
}

/** @} */

