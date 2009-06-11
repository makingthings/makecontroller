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

#include <string.h>
#include <stdio.h>

#include "error.h"
#include "io.h"
#include "analogin.h"
#include "AT91SAM7X256.h"

#define ANALOGIN_0_IO IO_PB27
#define ANALOGIN_1_IO IO_PB28
#define ANALOGIN_2_IO IO_PB29
#define ANALOGIN_3_IO IO_PB30
#define AUTOSENDSAVE 0xDF

// extern
void AnalogInIsr_Wrapper( );

// statics
AnalogIn::Manager AnalogIn::manager;

/**
  Create a new AnalogIn object.
  There are 8 analog ins on the Make Controller - pass in which channel this
  AnalogIn should read from.
  @param channel Which analog in channel - valid options are from 0 to 7.
  
  \par Example
  \code
  // we can create an AnalogIn locally (faster since it doesn't allocate any memory)
  AnalogIn ain0(0);

  // or allocate a new one
  AnalogIn* ain0 = new AnalogIn(0);
  \endcode
*/
AnalogIn::AnalogIn( int channel )
{
  if ( channel < 0 || channel >= ANALOGIN_CHANNELS )
  {
    index = -1;
    return;
  }
  index = channel;
  if(!manager.activeChannels)
  {
    managerInit();
    manager.activeChannels |= (1 << index); // mark it as used
  }

  // The lower four channel pins are shared with other subsystems, so config as GPIO inputs
  if ( index < 4 )
  {
    Io pin( getIo( index ), Io::GPIO, INPUT );
    pin.setPullup( false );
  }
}

AnalogIn::~AnalogIn()
{
  int c = 1 << index;
  manager.activeChannels &= ~c; // mark it as unused
  if(!manager.activeChannels) // if that was our last channel, turn everything off
    managerDeinit();
}

/** 
  Read the value of an analog input.
  @return The value as an integer (0 - 1023).
  
  \par Example
  \code
  AnalogIn ain0(0);
  if( ain0.value() > 500 )
  {
     // then do this
  }
  else
  {
    // then do that
  }
  \endcode
*/
int AnalogIn::value( )
{
  int value;
  if ( !manager.semaphore.take( 1000 ) )
    return -1;

  // disable other channels, and enable the one we want
  int mask = 1 << index; 
  AT91C_BASE_ADC->ADC_CHDR = ~mask;
  AT91C_BASE_ADC->ADC_CHER = mask;
  AT91C_BASE_ADC->ADC_CR = AT91C_ADC_START; // Start the conversion

  if ( !manager.doneSemaphore.take( 1000 ) ) // wait for the ISR
    return -1;

  value = AT91C_BASE_ADC->ADC_LCDR & 0xFFFF; // grab the last converted value

  manager.semaphore.give( );

  return value;
}

/** 
  Read the value of all the analog inputs.
  If you want to read all the anaog ins, this is quicker than reading them all 
  separately.  Make sure to provide an array of 8 ints, as this does not do
  any checking about the size of the array it's writing to.

  @param values An array of ints to be filled with the values.
  @return 0 on success, otherwise non-zero.
  
  \par Example
  \code
  int samples[8];
  AnalogIn::multi( samples );
   // now samples is filled with all the analogin values
  \endcode
*/
bool AnalogIn::multi( int values[] ) // static
{
  if(!manager.activeChannels)
    managerInit();

  if ( !manager.semaphore.take(1000) ) // lock the channel
    return false;

  manager.activeChannels |= 0xFF;

  // enable all the channels
  AT91C_BASE_ADC->ADC_CHER = AT91C_ADC_CH0 |
                              AT91C_ADC_CH1 |
                              AT91C_ADC_CH2 |
                              AT91C_ADC_CH3 |
                              AT91C_ADC_CH4 |
                              AT91C_ADC_CH5 |
                              AT91C_ADC_CH6 |
                              AT91C_ADC_CH7;

  manager.waitingForMulti = true;
  manager.multiConversionsComplete = 0;
  AT91C_BASE_ADC->ADC_CR = AT91C_ADC_START; // start the conversion

  if ( !manager.doneSemaphore.take(1000) )
    return false;

  // read all the data channels into the passed in array
  *values++ = AT91C_BASE_ADC->ADC_CDR0;
  *values++ = AT91C_BASE_ADC->ADC_CDR1;
  *values++ = AT91C_BASE_ADC->ADC_CDR2;
  *values++ = AT91C_BASE_ADC->ADC_CDR3;
  *values++ = AT91C_BASE_ADC->ADC_CDR4;
  *values++ = AT91C_BASE_ADC->ADC_CDR5;
  *values++ = AT91C_BASE_ADC->ADC_CDR6;
  *values++ = AT91C_BASE_ADC->ADC_CDR7;
  
  manager.waitingForMulti = false;
  manager.semaphore.give(); // free up the channel

  return true;
}

/** 
  Read the value of an analog input without the use of any OS services.
  This will busy wait until the read has completed.  Note that this is not 
  thread safe and shouldn't be used if another part of the code might be 
  using it or the thread safe versions.
  @return The value as an integer (0 - 1023).
  
  \par Example
  \code
  AnalogIn ain0(0);
  int value = ain0.valueWait( );
  \endcode
*/
int AnalogIn::valueWait( )
{
  // select the active channel
  int mask = 1 << index; 
  AT91C_BASE_ADC->ADC_CHDR = ~mask; // disable all other channels
  AT91C_BASE_ADC->ADC_CHER = mask;  // enable our channel
  
  AT91C_BASE_ADC->ADC_IDR = AT91C_ADC_DRDY; // turn off data ready interrupt
  AT91C_BASE_ADC->ADC_CR = AT91C_ADC_START; // start the conversion
  while ( !( AT91C_BASE_ADC->ADC_SR & AT91C_ADC_DRDY ) ); // Busy wait till it's done
  AT91C_BASE_ADC->ADC_IER = AT91C_ADC_DRDY;

  return AT91C_BASE_ADC->ADC_LCDR & 0xFFFF; // last converted value
}


#ifdef OSC___
void AnalogIn_AutoSendInit( )
{
  int autosend;
  Eeprom_Read( EEPROM_ANALOGIN_AUTOSEND, (uchar*)&autosend, 4 );
  if( !((autosend >> 16) & 0xFF) == AUTOSENDSAVE )
    AnalogIn->autosend = AUTOSENDSAVE << 16;
  else
    AnalogIn->autosend = autosend;
}
#endif

int AnalogIn::managerInit()
{
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_ADC; // enable the peripheral clock
  AT91C_BASE_ADC->ADC_CR = AT91C_ADC_SWRST;     // reset to clear out previous settings

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
       ( ( 9 << 8 ) & AT91C_ADC_PRESCAL ) | // Prescale rate (8 bits)
       ( ( 127 << 16 ) & AT91C_ADC_STARTUP ) | // Startup rate
       ( ( 127 << 24 ) & AT91C_ADC_SHTIM ); // Sample and Hold Time

  //TODO: Will need to fine-tune these timings.
  manager.doneSemaphore.take();

  // Initialize the interrupts
  unsigned int mask = 0x1 << AT91C_ID_ADC;                          
                        
  // Disable the interrupt controller & register our interrupt handler
  AT91C_BASE_AIC->AIC_IDCR = mask ;                   
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_ADC ] = (unsigned int)AnalogInIsr_Wrapper;      
  AT91C_BASE_AIC->AIC_SMR[ AT91C_ID_ADC ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 4  ;       
  AT91C_BASE_AIC->AIC_ICCR = mask ;         
  AT91C_BASE_ADC->ADC_IER = AT91C_ADC_DRDY; 
  AT91C_BASE_AIC->AIC_IECR = mask;

  return CONTROLLER_OK;
}

void AnalogIn::managerDeinit() // static
{
  unsigned int mask = 0x1 << AT91C_ID_ADC;    
  AT91C_BASE_PMC->PMC_PCDR = mask; // Disable the peripheral clock
  AT91C_BASE_AIC->AIC_IDCR = mask; // disable interrupts for the ADC
}

int AnalogIn::getIo( int index )
{
  switch ( index )
  {
    case 0:
      return ANALOGIN_0_IO;
    case 1:
      return ANALOGIN_1_IO;
    case 2:
      return ANALOGIN_2_IO;
    case 3:
      return ANALOGIN_3_IO;
    default:
      return -1;
  }
}

#ifdef OSC___

/** 
  Read whether a particular channel is enabled to check for and send new values automatically.
  @param index An integer specifying which input (0-7).
  @return True if enabled, false if disabled.
*/
bool AnalogIn_GetAutoSend( int index )
{
  AnalogIn_SetActive( index, 1 );
  return (AnalogIn->autosend >> index) & 0x01;
}

/** 
  Set whether a particular channel is enabled to check for and send new values automatically.
  @param index An integer specifying which input (0-7).
  @param onoff A boolean specifying whether to turn atuosending on or off.
*/
void AnalogIn_SetAutoSend( int index, bool onoff )
{
  AnalogIn_SetActive( index, 1 );
  if( ((AnalogIn->autosend >> index) & 0x01) != onoff )
  {
    int mask = (1 << index);
    if( onoff )
      AnalogIn->autosend |= mask;
    else
      AnalogIn->autosend &= ~mask;
    
    Eeprom_Write( EEPROM_ANALOGIN_AUTOSEND, (uchar*)&AnalogIn->autosend, 4 );
  }
}

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

#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* AnalogInOsc_Name = "analogin";
static char* AnalogInOsc_PropertyNames[] = { "active", "value", "autosend", 0 }; // must have a trailing 0

int AnalogInOsc_PropertySet( int index, int property, int value );
int AnalogInOsc_PropertyGet( int index, int property );

// Returns the name of the subsystem
const char* AnalogInOsc_GetName( )
{
  return AnalogInOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int AnalogInOsc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
                                     ANALOGIN_CHANNELS, AnalogInOsc_Name,
                                     AnalogInOsc_PropertySet, AnalogInOsc_PropertyGet, 
                                     AnalogInOsc_PropertyNames );
  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, AnalogInOsc_Name, status );
  return CONTROLLER_OK;

}

// Set the index LED, property with the value
int AnalogInOsc_PropertySet( int index, int property, int value )
{
  switch ( property )
  {
    case 0: 
      AnalogIn_SetActive( index, value );
      break;
    case 2: // autosend 
      AnalogIn_SetAutoSend( index, value );
      break;    
  }
  return CONTROLLER_OK;
}

// Get the index LED, property
int AnalogInOsc_PropertyGet( int index, int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      value = AnalogIn_GetActive( index );
      break;
    case 1:
      value = AnalogIn_GetValue( index );
      break;
    case 2: // autosend
      value = AnalogIn_GetAutoSend( index );
      break;
  }
  
  return value;
}

int AnalogInOsc_Async( int channel )
{
  int newMsgs = 0;
  char address[ OSC_SCRATCH_SIZE ];
  int i;
  int value;
  for( i = 0; i < ANALOGIN_CHANNELS; i ++ )
  {
    if( !AnalogIn_GetAutoSend( i ) )
      continue;
    value = AnalogIn_GetValue( i );
    if( value != AnalogIn->lastValues[i] )
    {
      AnalogIn->lastValues[i] = value;
      snprintf( address, OSC_SCRATCH_SIZE, "/%s/%d/value", AnalogInOsc_Name, i );
      Osc_CreateMessage( channel, address, ",i", value );
      newMsgs++;
    }
  }

  return newMsgs;
}

#endif // OSC
