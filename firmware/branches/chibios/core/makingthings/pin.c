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

#include "board.h"
#include "at91lib/AT91SAM7X256.h"
#include "at91lib/aic.h"
#include "pin.h"
#include "core.h"

#define IO_PIN_COUNT 64

#define IOPORT(p) ((p < 32) ? IOPORT1 : IOPORT2)
#define PIN(p) (p % 32)
#define PIN_MASK(p) (1 << (p % 32))

#ifndef PIN_NO_ISR

#ifndef MAX_INTERRUPT_SOURCES
#define MAX_INTERRUPT_SOURCES 8
#endif

struct InterruptSource
{
  void* context;
  PinInterruptHandler callback;
  int pin;
};

static struct InterruptSource isrSources[MAX_INTERRUPT_SOURCES];
static unsigned int isrSourceCount = 0;

static void pinInitInterrupts(Port port, unsigned int priority);
#endif // PIN_NO_ISR


/**
  Read whether an Io is on or off.
  For this to be meaningful, the Io must be configured as a \b GPIO and set as an \b input.
  @return True if the pin is high, false if it's low.
  
  \b Example
  \code
  Io io(IO_PB28, Io::GPIO, false); // new io, configured as a GPIO and an input
  bool is_pb28_on = io.value(); // is it on?
  \endcode
*/

bool pinValue(int pin)
{
  return palReadPad(IOPORT(pin), PIN(pin));
}

int pinGroupValue(Port port)
{
  return palReadPort(port);
}

/**
  Turn an Io on or off.
  For this to be meaningful, the Io must be configured as a \b GPIO and set as an \b output.
  If you want to turn the Io on or off directly, use on() or off().
  @param onoff True to turn it on, false to turn it off.
  @return True on success, false on failure.
  
  \b Example
  \code
  Io io(IO_PB28); // new io, defaults to GPIO output
  io.setValue(true); // turn it on
  \endcode
*/
void pinSetValue(int pin, bool value)
{
  if (value) {
    palSetPad(IOPORT(pin), PIN(pin));
  }
  else {
    palClearPad(IOPORT(pin), PIN(pin));
  }
}

void pinGroupSetValue(Port port, int pins, bool value)
{
  if(value) {
    palSetPort(port, pins);
  }
  else {
    palClearPort(port, pins);
  }
}

/**
  Turn on Io on.
  For this to be meaningful, the Io must be configured as a \b GPIO and set as an \b output.
  
  This is slightly faster than setValue() since it doesn't have to check
  whether to turn it on or off.
  @return True on success, false on failure.
  
  \b Example
  \code
  Io io(IO_PB18); // defaults to GPIO output
  io.on(); // turn it on
  \endcode
*/
void pinOn(int pin)
{
  palSetPad(IOPORT(pin), PIN(pin));
}

void pinGroupOn(Port port, int pins)
{
  palSetPort(port, pins);
}

/**
  Turn an Io off.
  For this to be meaningful, the Io must be configured as a \b GPIO and set as an \b output.
  
  This is slightly faster than setValue() since it doesn't have to check
  whether to turn it on or off.
  @return True on success, false on failure.
  
  \b Example
  \code
  Io io(IO_PB18); // defaults to GPIO output
  io.off(); // turn it off
  \endcode
*/
void pinOff(int pin)
{
  palClearPad(IOPORT(pin), PIN(pin));
}

void pinGroupOff(Port port, int pins)
{
  palClearPort(port, pins);
}

/**
  Set an Io as an input or an output
  For this to be meaningful, the Io must be configured as a \b GPIO.
  
  You can also use the \b OUTPUT and \b INPUT symbols instead of true/false, 
  for convenience.
  @param output True to set it as an output, false to set it as an input.
  
  \b Example
  \code
  Io io(IO_PA01); // is a GPIO output by default
  io.setDirection(INPUT); // now change it to an input
  \endcode
*/
void pinSetMode(int pin, PinMode mode)
{
  pinGroupSetMode(IOPORT(pin), PIN_MASK(pin), mode);
}

void pinGroupSetMode(Port port, int pins, PinMode mode)
{
  switch(mode) {
    case PERIPHERAL_A:
      port->PIO_PDR = pins;
      port->PIO_ASR = pins;
      break;
    case PERIPHERAL_B:
      port->PIO_PDR = pins;
      port->PIO_BSR = pins;
      break;
    case PULLUP_ON:
      port->PIO_PPUER = pins;
      break;
    case PULLUP_OFF:
      port->PIO_PPUDR = pins;
      break;
    case GLITCH_FILTER_ON:
      port->PIO_IFER = pins;
      break;
    case GLITCH_FILTER_OFF:
      port->PIO_IFDR = pins;
      break;
    default:
      palSetGroupMode(port, pins, mode);
      break;
  }
}

#ifndef PIN_NO_ISR

/**
  Add an interrupt handler for this signal.
  If you want to get notified when the signal on this pin changes,
  you can register an interrupt handler, instead of constantly reading the pin.
  To do this, define a function you want to be called when the pin changes, and
  then register it with the Io you want to monitor.  The function has to have a
  specific signature - it must be of the form:
  \code void myHandler(void* context) \endcode
  
  This function will be called any time the signal on the Io pin changes.  When you 
  register the handler, you can provide a pointer to some context that will be passed
  into your handler.  This can be an instance of a class, if you want to 
  
  \b Example
  \code
  void myHandler(void* context); // declare our handler
  
  void myTask(void* p)
  {
    Io io(IO_PB27, Io::GPIO, INPUT);   // Io on AnalogIn 0 - as digital input
    io.addInterruptHandler(myHandler); // register our handler
    
    while(true)
    {
      // do the rest of my task...
    }
  }
  
  int count = 0; // how many times has our interrupt triggered?
  // now this will get called every time the value on PB27 changes
  void myHandler(void* context)
  {
    count++;
    if(count > 100)
      count = 0;
  }
  \endcode
  
  @param h The function to be called when there's an interrupt
  @param arg (optional) Argument that will be passed into your handler, if desired.
  @return True if the handler was registered successfully, false if not.
*/
bool pinAddInterruptHandler(int pin, PinInterruptHandler h, void* arg)
{
  if(isrSourceCount >= MAX_INTERRUPT_SOURCES)
    return false;
  
  isrSources[isrSourceCount].pin = pin;
  Port port = IOPORT(pin);

  // if this is the first time for either channel, set it up
  if(port->PIO_IMR == 0)
    pinInitInterrupts(port, (AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 3) );
  
  port->PIO_ISR;                  // clear the status register
  pinEnableInterruptHandler(pin); // enable our channel

  isrSources[isrSourceCount].callback = h;
  isrSources[isrSourceCount++].context = arg; // make sure to increment our handler count

  return true;
}

/**
  Disable the interrupt handler for this io line.
  If you've already registered an interrupt with addInterruptHandler()
  this will disable it.
  
  \b Example
  \code
  Io io(IO_PB27, Io::GPIO, INPUT);
  io.addInterruptHandler(myHandler); // start notifications
  
  // ... some time later, we want to stop getting notified
  
  io.removeInterruptHandler( ); // stop notifications
  \endcode
  
  @return True on success, false on failure.
*/
void pinDisableInterruptHandler(int pin)
{
  IOPORT(pin)->PIO_IDR = PIN_MASK(pin);
}

void pinEnableInterruptHandler(int pin)
{
  IOPORT(pin)->PIO_IER = PIN_MASK(pin);
}

static void pinServeInterrupt( Port port )
{
  unsigned int status = port->PIO_ISR;
  status &= port->PIO_IMR;

  // Check pending events
  if(status) {
    unsigned short i;
    unsigned short pinMask;
    struct InterruptSource* is;
    for( i = 0; status != 0  && i < isrSourceCount; i++ ) {
      is = &(isrSources[i]);
      pinMask = PIN_MASK(is->pin);
      if( (IOPORT(is->pin) == port) && ((status & pinMask) != 0) ) {
        is->callback(is->context);     // callback the handler
        status &= ~(pinMask);          // mark this channel as serviced
      }
    }
  }
}

CH_IRQ_HANDLER( pinIsrA ) {
  CH_IRQ_PROLOGUE();
  pinServeInterrupt(AT91C_BASE_PIOA);
  CH_IRQ_EPILOGUE();
}

CH_IRQ_HANDLER( pinIsrB ) {
  CH_IRQ_PROLOGUE();
  pinServeInterrupt(AT91C_BASE_PIOB);
  CH_IRQ_EPILOGUE();
}

/*
  Turn on interrupts for a pio channel - a or b
  at a given priority.
*/
void pinInitInterrupts(Port port, unsigned int priority)
{
  unsigned int chan;
  void (*isr_handler)(void);
  
  if( port == AT91C_BASE_PIOA ) {
    chan = AT91C_ID_PIOA;
    isr_handler = pinIsrA;
  }
  else if( port == AT91C_BASE_PIOB ) {
    chan = AT91C_ID_PIOB;
    isr_handler = pinIsrB;
  }
  else
    return;
  
  port->PIO_ISR;                                // clear with a read
  port->PIO_IDR = 0xFFFFFFFF;                   // disable all by default
  AIC_ConfigureIT(chan, priority, isr_handler); // set it up
  AIC_EnableIT(chan);
}

#endif // PIN_NO_ISR




