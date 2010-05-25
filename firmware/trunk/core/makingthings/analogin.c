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

#include "analogin.h"
#include "core.h"

#define ANALOGIN_0 AT91C_PIO_PB27
#define ANALOGIN_1 AT91C_PIO_PB28
#define ANALOGIN_2 AT91C_PIO_PB29
#define ANALOGIN_3 AT91C_PIO_PB30

struct AinManager {
  Mutex adcLock;               // lock for the adc system
  Semaphore conversionLock;    // signal for a conversion
  bool processMultiChannelIsr; // are we waiting for a multi conversion or just a single channel
  int multiChannelConversions; // mask of which conversions have been completed
};

static struct AinManager manager;

/**
  \defgroup analogin Analog Input
  10-bit analog inputs.
  The analog inputs read incoming signals from 0 - 3.3V.  They are rated as 5V tolerant, 
  but will not return meaningful values for anything above 3.3V.
  
  \section Usage
  ainInit() is called during system startup, so you can start reading via ainValue() 
  whenever you like.  If you don't want it to be initialized automatically, define
  \b NO_AIN_INIT in your config.h file.
  
  \section Values
  Analog inputs will return a value between 0 and 1023, corresponding to the range of \b 0 to \b 3.3V on the input.
  
  If you want to convert this to the actual voltage, you can use the following conversion:
  \code float voltage = 3.3 * (ainValue(1) / 1023.0); \endcode
  where \b ainValue is the AnalogIn value.
  
  A quicker version that doesn't use floating point, but will be slightly less precise:
  \code int voltage = (100 * ainValue(1)) / 1023; \endcode
  \ingroup io
  @{
*/

/** 
  Read the value of an analog input.
  @param channel Which analog in to sample - valid options are 0-7.
  @return The value as an integer (0 - 1023).
  
  \b Example
  \code
  if (ainValue(0) > 500) {
     // then do this
  }
  \endcode
*/
int ainValue(int channel)
{
  chMtxLock(&manager.adcLock);
  // disable other channels, and enable the one we want
  AT91C_BASE_ADC->ADC_CHDR = ~(1 << channel);
  AT91C_BASE_ADC->ADC_CHER = (1 << channel);
  AT91C_BASE_ADC->ADC_CR = AT91C_ADC_START; // Start the conversion

  if (chSemWait(&manager.conversionLock) != RDY_OK)
    return false;

  int value = AT91C_BASE_ADC->ADC_LCDR & 0xFFFF; // grab the last converted value
  chMtxUnlock();
  return value;
}

/** 
  Read the value of all the analog inputs.
  If you want to read all the anaog ins, this is quicker than reading them all 
  separately.  Make sure to provide an array of 8 ints, as this does not do
  any checking about the size of the array it's writing to.
  @param values An array of ints to be filled with the values.
  @return non-zero on success, zero on failure.
  
  \b Example
  \code
  int samples[8];
  ainMulti(samples);
   // now samples is filled with all the analogin values
  \endcode
*/
bool ainMulti(int values[])
{
  chMtxLock(&manager.adcLock);
  // enable all the channels
  AT91C_BASE_ADC->ADC_CHER =  AT91C_ADC_CH0 | AT91C_ADC_CH1 | AT91C_ADC_CH2 |
                              AT91C_ADC_CH3 | AT91C_ADC_CH4 | AT91C_ADC_CH5 |
                              AT91C_ADC_CH6 | AT91C_ADC_CH7;

  manager.processMultiChannelIsr = true;    // how to process the ISR
  manager.multiChannelConversions = 0;      // which channels have completed
  AT91C_BASE_ADC->ADC_CR = AT91C_ADC_START; // start the conversion

  if (chSemWait(&manager.conversionLock) != RDY_OK)
    return false;

  // read all the data channels into the passed in array
  *values++ = AT91C_BASE_ADC->ADC_CDR0;
  *values++ = AT91C_BASE_ADC->ADC_CDR1;
  *values++ = AT91C_BASE_ADC->ADC_CDR2;
  *values++ = AT91C_BASE_ADC->ADC_CDR3;
  *values++ = AT91C_BASE_ADC->ADC_CDR4;
  *values++ = AT91C_BASE_ADC->ADC_CDR5;
  *values++ = AT91C_BASE_ADC->ADC_CDR6;
  *values   = AT91C_BASE_ADC->ADC_CDR7;
  
  manager.processMultiChannelIsr = false;
  chMtxUnlock();
  return true;
}

/** 
  Read the value of an analog input without the use of any OS services.
  This will busy wait until the read has completed.  Note that this is not 
  thread safe and shouldn't be used if another part of the code might be 
  using it or the thread safe versions.
  @param channel Which analog in to sample - valid options are 0-7.
  @return The value as an integer (0 - 1023).
  
  \par Example
  \code
  int value = ainValueWait(0);
  \endcode
*/
int ainValueWait(int channel)
{
  AT91C_BASE_ADC->ADC_CHDR = ~(1 << channel); // disable all other channels
  AT91C_BASE_ADC->ADC_CHER = (1 << channel);  // enable our channel
  AT91C_BASE_ADC->ADC_IDR = AT91C_ADC_DRDY;   // turn off data ready interrupt
  AT91C_BASE_ADC->ADC_CR = AT91C_ADC_START;   // start the conversion
  while (!(AT91C_BASE_ADC->ADC_SR & AT91C_ADC_DRDY))
    ; // Busy wait
  AT91C_BASE_ADC->ADC_IER = AT91C_ADC_DRDY; // turn interrupt back on
  return AT91C_BASE_ADC->ADC_LCDR & 0xFFFF; // last converted value
}

static void ainServeInterrupt(void) {
  int status = AT91C_BASE_ADC->ADC_SR;
  if (manager.processMultiChannelIsr) {
    unsigned int i, mask;
    // check if we got an End Of Conversion in any of our channels
    for (i = 0; i < ANALOGIN_CHANNELS; i++) {
      mask = 1 << i;
      if (status & mask)
        manager.multiChannelConversions |= mask;
    }
    // if we got End Of Conversion in all our channels, indicate we're done
    if (manager.multiChannelConversions == 0xFF) {
      status = AT91C_BASE_ADC->ADC_LCDR; // dummy read to clear
      chSemSignalI(&manager.conversionLock);
    }
  }
  else if (status & AT91C_ADC_DRDY) {
    chSemSignalI(&manager.conversionLock);
    status = AT91C_BASE_ADC->ADC_LCDR; // dummy read to clear
  }
}

static CH_IRQ_HANDLER(AnalogInIsr) {
  CH_IRQ_PROLOGUE();
  ainServeInterrupt();
  AT91C_BASE_AIC->AIC_EOICR = 0;
  CH_IRQ_EPILOGUE();
}

/**
  Initialize the analog in system.
*/
void ainInit(void)
{
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_ADC; // enable the peripheral clock
  AT91C_BASE_ADC->ADC_CR = AT91C_ADC_SWRST;     // reset to clear out previous settings
  
  // prescal = (mckClock / (2*adcClock)) - 1;
  // startup = ((adcClock/1000000) * startupTime / 8) - 1;
  // shtim = (((adcClock/1000000) * sampleAndHoldTime)/1000) - 1;
  
  // Set up
  AT91C_BASE_ADC->ADC_MR =
       AT91C_ADC_TRGEN_DIS | // Hardware Trigger Disabled
    // AT91C_ADC_TRGEN_EN  | // Hardware Trigger Disabled
    //   AT91C_ADC_TRGSEL_ | // Hardware Trigger Disabled
    // AT91C_ADC_TRGSEL_TIOA0  | // Trigger Selection Don't Care
       AT91C_ADC_LOWRES_10_BIT | // 10 bit conversion
    // AT91C_ADC_LOWRES_8_BIT | // 8 bit conversion
       AT91C_ADC_SLEEP_NORMAL_MODE | // SLEEP
    // AT91C_ADC_SLEEP_MODE | // SLEEP
       ((9 << 8) & AT91C_ADC_PRESCAL) | // Prescale rate (8 bits)
       ((127 << 16) & AT91C_ADC_STARTUP) | // Startup rate
       ((127 << 24) & AT91C_ADC_SHTIM ); // Sample and Hold Time
   
  // initialize non-adc pins
  palSetGroupMode(IOPORT2,
                  ANALOGIN_0 | ANALOGIN_1 | ANALOGIN_2 | ANALOGIN_3,
                  PAL_MODE_INPUT);
  
  // init locks
  chMtxInit(&manager.adcLock);
  chSemInit(&manager.conversionLock, 0);
  manager.multiChannelConversions = 0;
  manager.processMultiChannelIsr = false;
  
  // initialize interrupts
  AIC_ConfigureIT(AT91C_ID_ADC, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 4, AnalogInIsr);
  AIC_EnableIT(AT91C_ID_ADC);
}

/**
  Deinitialize the analog in system.
*/
void ainDeinit(void)
{
  AT91C_BASE_PMC->PMC_PCDR = 1 << AT91C_ID_ADC; // disable peripheral clock
  AIC_DisableIT(AT91C_ID_ADC);                  // disable interrupts
}

/** @}
*/

#ifdef OSC

/** \defgroup AnalogInOSC Analog In - OSC
  Read the Application Board's Analog Inputs via OSC.
  \ingroup OSC
  
  \section devices Devices
  There are 8 Analog Inputs on the Make Application Board, numbered 0 - 7.
  
  \section properties Properties
  The Analog Ins have three properties:
  - value
  - active
  - autosend

  \par Value
  The \b value property corresponds to the incoming signal of an Analog In.
  The range of values you can expect to get back are from 0 - 1023.
  Because you can only ever \em read the value of an input, you'll never
  want to include an argument at the end of your OSC message to read the value.\n
  To read the sixth Analog In, send the message
  \verbatim /analogin/5/value \endverbatim
  The board will then respond by sending back an OSC message with the Analog In value.
  
  \par Autosend
  The \b autosend property corresponds to whether an analogin will automatically send a message
  when its incoming value changes.
  To tell the Controller to automatically send messages from analogin 4, send the message
  \verbatim /analogin/5/autosend 1 \endverbatim
  To have the Controller stop sending messages from analogin 4, send the message
  \verbatim /analogin/5/autosend 0 \endverbatim
  All autosend messages send at the same interval.  You can set this interval, in 
  milliseconds, by sending the message
  \verbatim /system/autosend-interval 10 \endverbatim
  so that messages will be sent every 10 milliseconds.  This can be anywhere from 1 to 5000 milliseconds.
  \par
  You also need to select whether the board should send to you over USB or Ethernet.  Send
  \verbatim /system/autosend-usb 1 \endverbatim
  to send via USB, and 
  \verbatim /system/autosend-udp 1 \endverbatim
  to send via Ethernet.  Via Ethernet, the board will send messages to the last address it received a message from.
  
  \par Active
  The \b active property corresponds to the active state of an Analog In.
  If an Analog In is set to be active, no other tasks will be able to
  read from it as an Analog In.  If you're not seeing appropriate
  responses to your messages to the Analog In, check the whether it's 
  locked by sending a message like
  \verbatim /analogin/0/active \endverbatim
  \par
  You can set the active flag by sending
  \verbatim /analogin/0/active 1 \endverbatim
*/

static bool ainOscHandler(OscChannel ch, char* address, int idx, OscData d[], int datalen)
{
  UNUSED(d);
  if (datalen == 0) {
    OscData d = {
      .type = INT,
      .value.i = ainValue(idx)
    };
    oscCreateMessage(ch, address, &d, 1);
    return true;
  }
  return false;
}

static const OscNode ainAutosendNode = {
  .name = "autosend",
  .handler = 0 // ainAutosendHandler TODO!!
};
static const OscNode ainValueNode = {
  .name = "value",
  .handler = ainOscHandler
};
static const OscNode ainRange = {
  .range = 8,
  .children = { &ainValueNode, &ainAutosendNode, 0 }
};
const OscNode ainOsc = {
  .name = "ain",
  .children = { &ainRange, 0 }
};
#endif // OSC
