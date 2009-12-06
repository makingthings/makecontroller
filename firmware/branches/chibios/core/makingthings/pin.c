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

#include "pin.h"
#include "at91lib/aic.h"
#include "core.h"

#if (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) || (SAM7_PLATFORM == SAM7X512)
#define IOPORT(p) ((p < 32) ? IOPORT1 : IOPORT2)
#define PIN(p) (p % 32)
#define PIN_MASK(p) (1 << (p % 32))
#define IO_PIN_COUNT 64
#else
#define IOPORT(p) IOPORT1
#define PIN(p) (p)
#define PIN_MASK(p) (1 << p)
#define IO_PIN_COUNT 32
#endif

#ifndef PIN_NO_ISR

#ifndef MAX_INTERRUPT_SOURCES
#define MAX_INTERRUPT_SOURCES 8
#endif

struct InterruptSource {
  void* context;
  PinInterruptHandler callback;
  int pin;
};

static struct InterruptSource isrSources[MAX_INTERRUPT_SOURCES];
static unsigned int isrSourceCount = 0;

static void pinInitInterrupts(Group group, unsigned int priority);
#endif // PIN_NO_ISR

/** \defgroup Pins
  Control any of the 35 pins on the Make Controller.

  Each of the pins on the Make Controller can be used as a digital in/out, and many
  can be used as part of one of the peripherals on the board (serial, usb, etc).

  \section Usage
  For each pin you'll generally want to first set its mode, then control it.  The most
  common modes are INPUT and OUTPUT - here's how they work.

  \subsection Input
  First set a pin as an input, then you can read the value on that pin:
  \code
  pinSetMode(PIN_PA08, INPUT);
  bool isItOn = pinValue(PIN_PA08);
  \endcode

  \subsection Output
  First set a pin as an output, then you can turn it on and off:
  \code
  pinSetMode(PIN_PA08, OUTPUT);
  pinSetValue(ON);  // turn it on
  pinSetValue(OFF); // turn it off
  \endcode

  \section Interrupts
  You can also register to get notified immediately when the value on a pin
  has changed, rather than constantly reading the value to see if it has changed.
  This can be much more efficient if you need to know precisely when a pin's value
  has changed - see pinAddInterruptHandler() for details.

  \section group Group Functions
  For most pin functions, there are corresponding \b group functions that allow you
  to perform the same action simultaneously on many pins.  To specify a group of pins,
  use the symbols from \ref PinBits as follows:
  \code int pingroup = (PIN_PA13_BIT | PIN_PA14_BIT | PIN_PA15_BIT); \endcode
  to turn these all off at the same time, it's as easy as:
  \code pinGroupOff(GROUP_A, pingroup); \endcode
  Note that within a group you can only specify pins from the same port - \b GROUP_A or \b GROUP_B.
  @{
*/

/**
 * Read whether a pin is on or off.
 * The pin can be configured as an \b INPUT or \b OUTPUT.
 * @param pin Which pin to read.
 * @return True if the pin is high, false if it's low.
 * \b Example
 * \code
 * pinSetMode(PIN_PB28, INPUT); // set as an input
 * if (pinValue(PIN_PB28) == ON) {
 *   // then it's on
 * }
 * else {
 *   // then it's off
 * }
 * \endcode
 */
bool pinValue(Pin pin)
{
  return palReadPad(IOPORT(pin), PIN(pin));
}

/**
 * Same as pinValue() but for a group of pins at once.
 * @param group Which group of pins to read - GROUP_A or GROUP_B
 * @return The value for that group of pins.
 */
int pinGroupValue(Group group)
{
  return palReadPort(group);
}

/**
  Turn a pin on or off.
  To be meaningful, the pin must be configured as an \b OUTPUT.
  If you want to turn the pin on or off directly, use pinOn() or pinOff().

  You can use the ON and OFF symbols to specify the value.
  @param pin Which pin to control.
  @param value True to turn it on, false to turn it off.
  @return True on success, false on failure.
  
  \b Example
  \code
  pinSetMode(PIN_PB28, OUTPUT); // set it as an output
  pinSetValue(ON);              // turn it on
  \endcode
*/
void pinSetValue(Pin pin, bool value)
{
  if (value) {
    palSetPad(IOPORT(pin), PIN(pin));
  }
  else {
    palClearPad(IOPORT(pin), PIN(pin));
  }
}

/**
 * Same as pinSetValue() but for a group of pins.
 * @param group Which port the pins are on.
 * @param pins Which pins to control.
 * @param value Which value to set the pins to.
 */
void pinGroupSetValue(Group group, int pins, bool value)
{
  if(value) {
    palSetPort(group, pins);
  }
  else {
    palClearPort(group, pins);
  }
}

/**
  Turn a pin on.
  To be meaningful, the pin must be configured as an \b OUTPUT.
  
  This is slightly faster than pinSetValue() since it doesn't have to check
  whether to turn it on or off.
  @param pin Which pin to turn on.
  @return True on success, false on failure.
  
  \b Example
  \code
  pinSetMode(PIN_PB18, OUTPUT); // turn it into an output
  pinOn(PIN_PB18);              // turn it on
  \endcode
*/
void pinOn(Pin pin)
{
  palSetPad(IOPORT(pin), PIN(pin));
}

/**
 * Same as pinOn() but for a group of pins.
 * @param group Which port are the pins on.
 * @param pins The pins to turn on
 */
void pinGroupOn(Group group, int pins)
{
  palSetPort(group, pins);
}

/**
  Turn a pin off.
  To be meaningful, this pin must be configured as an \b OUTPUT.
  
  This is slightly faster than pinSetValue() since it doesn't have to check
  whether to turn it on or off.
  @param pin Which pin to turn off.
  @return True on success, false on failure.
  
  \b Example
  \code
  pinSetMode(PIN_PB18, OUTPUT); // turn it into an output
  pinOff(PIN_PB18);             // turn it off
  \endcode
*/
void pinOff(Pin pin)
{
  palClearPad(IOPORT(pin), PIN(pin));
}

/**
 * Same as pinOff() but for a group of pins.
 * @param group Which port are the pins on.
 * @param pins The pins to turn off
 */
void pinGroupOff(Group group, int pins)
{
  palClearPort(group, pins);
}

/**
  Set the mode for a pin.
  Pins can operate in a variety of modes - see PinMode for options.
  @param pin Which pin to set the mode for.
  @param mode Which PinMode to use for this pin.
  
  \b Example
  \code
  pinSetMode(PIN_PA1, INPUT);
  \endcode
*/
void pinSetMode(Pin pin, PinMode mode)
{
  pinGroupSetMode(IOPORT(pin), PIN_MASK(pin), mode);
}

/**
 * Same as pinSetMode(), but for a group of pins.
 * \b Example
 * \code
 * // set pins PA0, PA1, PA2 as outputs, all at once
 * int pingroup = (PIN_PA0_BIT | PIN_PA1_BIT | PIN_PA2_BIT); // the group of pins to control
 * pinGroupSetMode(GROUP_A, pingroup, OUTPUT);
 * \endcode
 */
void pinGroupSetMode(Group group, int pins, PinMode mode)
{
  switch(mode) {
    case PERIPHERAL_A:
      group->PIO_PDR = pins;
      group->PIO_ASR = pins;
      break;
    case PERIPHERAL_B:
      group->PIO_PDR = pins;
      group->PIO_BSR = pins;
      break;
    case PULLUP_ON:
      group->PIO_PPUER = pins;
      break;
    case PULLUP_OFF:
      group->PIO_PPUDR = pins;
      break;
    case GLITCH_FILTER_ON:
      group->PIO_IFER = pins;
      break;
    case GLITCH_FILTER_OFF:
      group->PIO_IFDR = pins;
      break;
    default:
      palSetGroupMode(group, pins, mode);
      break;
  }
}

#ifndef PIN_NO_ISR

/**
  Add an interrupt handler for this pin.
  Your handler will get called any time the value on this pin changes,
  meaning you don't have to constantly check its value yourself.
  Your handler must have this specific signature:
  \code void myHandler(void* context) \endcode
  
  Note that in your handler, you shouldn't do anything that takes too long,
  and definitely can't sleep().

  You must first set the mode the of the pin to \b INPUT.  Since the handler
  is called whenever there's a change, you don't necessarily know if it turned
  on or off, but you can check from within the handler:
  
  \b Example
  \code
  int count = 0; // we'll keep track of how many changes we've seen
  void myHandler(void* context)
  {
    if(pinValue(PIN_PB27) == ON) {
      // then the change was from low to high
    }
    else {
      // it was from high to low
    }

    count++; // increment our count
  }

  void myTask(void* p)
  {
    pinSetMode(PIN_PB27, INPUT);       // set the pin as an input
    pinAddInterruptHandler(PIN_PB27, myHandler, 0); // register our handler
    
    while(true) {
      // do the rest of my task...
    }
  }
  \endcode
  
  @param pin The pin to monitor for changes.
  @param h Your handler that will be called when the input value changes.
  @param arg (optional) Argument that will be passed into your handler, if desired.
  @return True if the handler was registered successfully, false if not.
*/
bool pinAddInterruptHandler(Pin pin, PinInterruptHandler h, void* arg)
{
  if(isrSourceCount >= MAX_INTERRUPT_SOURCES)
    return false;
  
  isrSources[isrSourceCount].pin = pin;

  // if this is the first time for either channel, set it up
  if(IOPORT(pin)->PIO_IMR == 0)
    pinInitInterrupts(IOPORT(pin), (AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 3) );
  
  IOPORT(pin)->PIO_ISR;           // clear the status register
  pinEnableHandler(pin); // enable our channel

  isrSources[isrSourceCount].callback = h;
  isrSources[isrSourceCount++].context = arg; // make sure to increment our handler count

  return true;
}

/**
  Disable the interrupt handler for this pin.
  This is only meaningful if you have registered a handler with
  pinAddInterruptHandler().
  @param pin The pin to disable the handler for.
  
  \b Example
  \code
  pinSetMode(PIN_PB27, INPUT);
  pinAddInterruptHandler(PIN_PB27, myHandler); // start notifications
  
  // ... some time later, we want to stop getting notified
  
  pinDisableHandler(PIN_PB27); // stop notifications
  \endcode
*/
void pinDisableHandler(Pin pin)
{
  IOPORT(pin)->PIO_IDR = PIN_MASK(pin);
}

/**
 * Re-enable the interrupt handler for a pin.
 * This is to re-enable a handler that has previously been
 * disabled via pinDisableHandler().
 * @param pin The pin to re-enable the handler for.
 *
 * \b Example
 * \code
 * pinSetMode(PIN_PB27, INPUT);
 * pinAddInterruptHandler(PIN_PB27, myHandler); // start notifications
 * // ... some time later, we want to stop getting notified
 * pinDisableHandler(PIN_PB27); // stop notifications
 * // ... some time later yet again, we want to turn them back on
 * pinEnableHandler(PIN_PB27);
 * \endcode
 */
void pinEnableHandler(Pin pin)
{
  IOPORT(pin)->PIO_IER = PIN_MASK(pin);
}

/** @}
*/

static void pinServeInterrupt( Group group )
{
  unsigned int status = group->PIO_ISR & group->PIO_IMR;

  // Check pending events
  if(status) {
    unsigned short i;
    unsigned int pinMask;
    struct InterruptSource* is;
    for( i = 0; status != 0  && i < isrSourceCount; i++ ) {
      is = &(isrSources[i]);
      pinMask = PIN_MASK(is->pin);
      if( (IOPORT(is->pin) == group) && ((status & pinMask) != 0) ) {
        is->callback(is->context);     // callback the handler
        status &= ~(pinMask);          // mark this channel as serviced
      }
    }
  }
}

static CH_IRQ_HANDLER( pinIsrA ) {
  CH_IRQ_PROLOGUE();
  pinServeInterrupt(AT91C_BASE_PIOA);
  AT91C_BASE_AIC->AIC_EOICR = 0;
  CH_IRQ_EPILOGUE();
}

#if SAM7_PLATFORM == SAM7X128 || SAM7_PLATFORM == SAM7X256 || SAM7_PLATFORM == SAM7X512
static CH_IRQ_HANDLER( pinIsrB ) {
  CH_IRQ_PROLOGUE();
  pinServeInterrupt(AT91C_BASE_PIOB);
  AT91C_BASE_AIC->AIC_EOICR = 0;
  CH_IRQ_EPILOGUE();
}
#endif

/*
  Turn on interrupts for a pio channel - a or b
  at a given priority.
*/
void pinInitInterrupts(Group group, unsigned int priority)
{
  unsigned int chan;
  void (*isr_handler)(void);
  
  if( group == AT91C_BASE_PIOA ) {
    chan = AT91C_ID_PIOA;
    isr_handler = pinIsrA;
  }
#if SAM7_PLATFORM == SAM7X128 || SAM7_PLATFORM == SAM7X256 || SAM7_PLATFORM == SAM7X512
  else if( group == AT91C_BASE_PIOB ) {
    chan = AT91C_ID_PIOB;
    isr_handler = pinIsrB;
  }
#endif
  else
    return;
  
  group->PIO_ISR;                                // clear with a read
  group->PIO_IDR = 0xFFFFFFFF;                   // disable all by default
  AIC_ConfigureIT(chan, priority, isr_handler); // set it up
  AIC_EnableIT(chan);
}

#endif // PIN_NO_ISR




