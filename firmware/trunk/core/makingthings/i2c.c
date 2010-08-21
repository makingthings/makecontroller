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

#include "i2c.h"
#include "hal.h"
#include "board.h"
#include "config.h"

#if SAM7_PLATFORM == SAM7S256
#define I2C_PINS            (AT91C_PA3_TWD | AT91C_PA4_TWCK)
#elif SAM7_PLATFORM == SAM7X256
#define I2C_PINS            (AT91C_PA10_TWD | AT91C_PA11_TWCK)
#endif // SAM7_PLATFORM

#ifndef I2C_DEFAULT_RATE
#define I2C_DEFAULT_RATE 200000
#endif

typedef struct I2CDriver_t {
  uint16_t length;
  uint16_t transferred;
  uint8_t *data;
  unsigned int state;
  Semaphore sem; // for internal ISR sync
  Mutex mutex; // for externally locking access to the I2C bus
} I2CDriver;

static I2CDriver i2cDriver;

/**
  \defgroup I2C I2C
  Interface with I2C (TWI / two-wire) devices.

  \section Usage
  First, initialize the I2C system with i2cInit().  The default I2C_DEFAULT_RATE of 200kHz
  is used, but you can define \b I2C_DEFAULT_RATE in your config.h to change this, or
  use i2cSetBitrate() if you need to change it on the fly.

  Once you're configured, use i2cRead() and i2cWrite() to send and receive data with your
  devices.  The driver is interrupt driven, but not thread safe - if you're going to
  be communicating with devices from different threads, be sure to surround your I2C
  activities with calls to i2cAcquireBus() and i2cReleaseBus().

  \b Example
  \code
  #define MY_DEVICE_ADDRESS 0x12
  i2cInit();

  uint8_t mydata[24];
  i2cAcquireBus(); // first get access to the I2C system
  int status = i2cRead(MY_DEVICE_ADDRESS, mydata, 24, 0, 0);
  i2cReleaseBus();
  if (status != 0) {
    // then there was a problem
  }
  \endcode
  \ingroup interfacing
  @{
*/

/**
  Acquire exclusive access to the I2C system.
  Call this before reading or writing, and be sure to call i2cReleaseBus() once you're done.
*/
void i2cAcquireBus()
{
  chMtxLock(&i2cDriver.mutex);
}

/**
  Release exclusive access to the I2C system.
*/
void i2cReleaseBus()
{
  chMtxUnlock();
}

/*
 * I2C ISR handler.
 * Update the driver object with information about the current transfer.
 * Once a transfer is complete, signal the reader/writer.  Enable/disable
 * interrupts as appropriate to move to the next state.
 */
static void serveI2cISR(void)
{
  uint32_t status = AT91C_BASE_TWI->TWI_SR & AT91C_BASE_TWI->TWI_IMR;
  if (status & AT91C_TWI_NACK) {
    // no ACK from the device - set the error state, and finish the transaction
    AT91C_BASE_TWI->TWI_IDR = (AT91C_TWI_TXCOMP | AT91C_TWI_RXRDY
        | AT91C_TWI_TXRDY | AT91C_TWI_NACK);
    i2cDriver.state = I2C_ERROR_NODEV;
    chSemSignalI(&i2cDriver.sem);
  }
  else if (status & AT91C_TWI_RXRDY) {
    // read is ready - load from the holding register and send a STOP if we're done
    i2cDriver.data[i2cDriver.transferred++] = AT91C_BASE_TWI->TWI_RHR;
    if (i2cDriver.transferred == i2cDriver.length) {
      AT91C_BASE_TWI->TWI_IDR = AT91C_TWI_RXRDY;
      AT91C_BASE_TWI->TWI_IER = AT91C_TWI_TXCOMP;
    }
    else if (i2cDriver.transferred == (i2cDriver.length - 1)) {
      AT91C_BASE_TWI->TWI_CR = AT91C_TWI_STOP;
    }
  }
  else if (status & AT91C_TWI_TXRDY) {
    // byte was written - send the next one if we're not finished yet
    if (i2cDriver.transferred == i2cDriver.length) {
      // STOP event (and thus TXCOMP ISR) automatically generated when TXRDY == 1 and we get an ACK
      AT91C_BASE_TWI->TWI_IDR = AT91C_TWI_TXRDY;
      AT91C_BASE_TWI->TWI_IER = AT91C_TWI_TXCOMP;
    }
    else {
      AT91C_BASE_TWI->TWI_THR = i2cDriver.data[i2cDriver.transferred++];
    }
  }
  else if (status & AT91C_TWI_TXCOMP) {
    // transaction complete - signal to the waiting writer/reader
    AT91C_BASE_TWI->TWI_IDR = (AT91C_TWI_TXCOMP | AT91C_TWI_NACK);
    i2cDriver.state = I2C_OK;
    chSemSignalI(&i2cDriver.sem);
  }
}

static CH_IRQ_HANDLER( i2cISR ) {
  CH_IRQ_PROLOGUE();
  serveI2cISR();
  AT91C_BASE_AIC->AIC_EOICR = 0;
  CH_IRQ_EPILOGUE();
}

/**
  Initialize the I2C system.
  Be sure to call this before trying to communicate with any I2C devices.
*/
void i2cInit(void)
{
  chMtxInit(&i2cDriver.mutex);
  chSemInit(&i2cDriver.sem, 0);
  // set for open-drain with pull-up resistor, as periph A
  palSetGroupMode(IOPORT1, I2C_PINS, PAL_MODE_OUTPUT_OPENDRAIN);
  AT91C_BASE_PIOA->PIO_PDR = I2C_PINS;
  AT91C_BASE_PIOA->PIO_ASR = I2C_PINS;

  // turn on clock into I2C system
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TWI);

  i2cSetBitrate(I2C_DEFAULT_RATE);

  AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSEN; // set as master
  AT91C_BASE_TWI->TWI_IDR = 0xFFFFFFFF; // all off
  AIC_ConfigureIT(AT91C_ID_TWI, AT91C_AIC_PRIOR_HIGHEST - 3, i2cISR);
  AIC_EnableIT(AT91C_ID_TWI);
}

/**
  Write data to an I2C device.
  If the device that you're communicating with has an internal register map,
  you can specify the device's internal address to write to in the
  \b internalAddr parameter.  Otherwise, this can be left as 0.

  This is not threadsafe - if you need exclusive access
  to the I2C system, see i2cAcquireBus() and i2cReleaseBus()

  @param deviceAddr The address of the device to communicate with.
  @param data The data to send.
  @param length The amount of data to send.
  @param internalAddr (optional) The device's internal address to write to.
  @param intAddrLen (optional) The size of the internal address (0-3 bytes)
  @return 0 on success, non-zero on error.
*/
int i2cWrite(uint8_t deviceAddr, const uint8_t *data, uint8_t length,
              uint16_t internalAddr, uint16_t intAddrLen)
{
  // Configure device & internal address
  AT91C_BASE_TWI->TWI_MMR = ((deviceAddr << 16) & AT91C_TWI_DADR)
      | ((intAddrLen << 8) & AT91C_TWI_IADRSZ);
  AT91C_BASE_TWI->TWI_IADR = internalAddr;

  // set up the transfer info
  i2cDriver.state = I2C_IN_PROGRESS;
  i2cDriver.data = (uint8_t*) data;
  i2cDriver.length = length;
  i2cDriver.transferred = 1;

  // Write first byte to send - this generates a START event when in write mode
  AT91C_BASE_TWI->TWI_THR = data[0];
  AT91C_BASE_TWI->TWI_IER = (AT91C_TWI_TXRDY | AT91C_TWI_NACK); // turn on interrupts
  return (chSemWait(&i2cDriver.sem) == RDY_OK) ? i2cDriver.state : I2C_TIMED_OUT;
}

/**
  Read data from an I2C device.
  This will wait until all the bytes requested have been received.

  If the device that you're communicating with has an internal register map,
  you can specify the device's internal address to read from in the
  \b internalAddr parameter.  Otherwise, this can be left as 0.

  This is not threadsafe - if you need exclusive access
  to the i2c bus, see i2cAcquireBus() and i2cReleaseBus()
  @param deviceAddr The address of the device to communicate with.
  @param data The data to send.
  @param length The amount of data to send.
  @param internalAddr (optional) The device's internal address to read from.
  @param intAddrLen (optional) The size of the internal address (0-3 bytes)
  @return 0 on success, non-zero on error.
*/
int i2cRead(uint8_t deviceAddr, uint8_t *data, uint8_t length,
            uint16_t internalAddr, uint16_t intAddrLen)
{
  // set up the transfer info
  i2cDriver.state = I2C_IN_PROGRESS;
  i2cDriver.data = data;
  i2cDriver.length = length;
  i2cDriver.transferred = 0;

  // Enable read interrupt and start the transfer
  AT91C_BASE_TWI->TWI_IER = (AT91C_TWI_RXRDY | AT91C_TWI_NACK);
  AT91C_BASE_TWI->TWI_MMR = ((deviceAddr << 16) & AT91C_TWI_DADR)
      | ((intAddrLen << 8) & AT91C_TWI_IADRSZ) | AT91C_TWI_MREAD;
  AT91C_BASE_TWI->TWI_IADR = internalAddr;
  // send start condition.  If the length is 1 or less, set the STOP bit simultaneously
  AT91C_BASE_TWI->TWI_CR = (length > 1) ? AT91C_TWI_START : AT91C_TWI_START
      | AT91C_TWI_STOP;
  // wait for the isr handler to take care of business
  return (chSemWait(&i2cDriver.sem) == RDY_OK) ? i2cDriver.state : I2C_TIMED_OUT;
}

static uint32_t power(unsigned int x, unsigned int y)
{
  uint32_t result = 1;
  while (y > 0) {
    result *= x;
    y--;
  }
  return result;
}

/**
  Set the I2C bit rate in Hz.
  @param rate The rate in Hz.
*/
void i2cSetBitrate(int rate)
{
  uint32_t cldiv, ckdiv = 0;
  uint8_t ok = 0;

  while (!ok) {
    cldiv = ((MCK / (2 * rate)) - 3) / power(2, ckdiv);
    if (cldiv <= 255)
      ok = 1;
    else
      ckdiv++;
  }
  chDbgCheck(ckdiv < 8, "i2cSetBitrate - can't find good clock");
  AT91C_BASE_TWI->TWI_CWGR = (ckdiv << 16) | (cldiv << 8) | cldiv;
}

/** @}
*/
