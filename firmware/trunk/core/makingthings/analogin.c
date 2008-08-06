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

// MakingThings - Make Controller Board - 2006

/** \file analogin.c	
	Functions for reading the analog inputs on the MAKE Application Board.
*/

/* Library includes. */
#include <string.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Hardware specific headers. */
#include "Board.h"
#include "AT91SAM7X256.h"

#include "config.h"

#include "io.h"

#include "analogin.h"
#include "analogin_internal.h"

#define ANALOGIN_0_IO IO_PB27
#define ANALOGIN_1_IO IO_PB28
#define ANALOGIN_2_IO IO_PB29
#define ANALOGIN_3_IO IO_PB30

#define AUTOSENDSAVE 0xDF

static int AnalogIn_Start( int index );
static int AnalogIn_Stop( int index );

static int AnalogIn_Init( void );
static int AnalogIn_Deinit( void );

static int AnalogIn_GetIo( int index );

extern void ( AnalogInIsr_Wrapper )( void );

struct AnalogIn_* AnalogIn;

/** \defgroup AnalogIn Analog Inputs
	The AnalogIn subsystem converts 0-3.3V signals to 10-bit digital values.
	The analog to digital converters read incoming signals from 0 - 3.3V.  They are rated as 5V tolerant, 
  and indeed can momentarily withstand higher voltages, but will not return meaningful values for anything 
  above 3.3V.\n
	
  Converting the 0 - 1023 reading of the AnalogIn channel into a voltage is performed as follows:
   \verbatim v = 3.3 * ( a / 1023.0 ) \endverbatim
    where a is the AnalogIn value

  This is of course a floating point operation (slowish) using a division (slowish).  Fixed point 
  versions may be more suitable for some applications.  Where reduced accuracy is acceptable, the following 
  can be used to get the input as a percentage of 3.3V:
  \verbatim  p = ( 100 * a ) / 1023 \endverbatim

  Initializing the controller's AnalogIn system is pretty involved (see AnalogIn_Init() in AnalogIn->c).  There are a lot of 
  different options - different converter speeds, DMA access, etc.  We've chosen something pretty simple here.
  More ambitious users may wish to alter the implementation.

  \todo Provide multi-channel conversion routines

	\ingroup Core
	@{
*/

/**
	Sets whether the specified channel is active.
	This initializes the AnalogIn system and gets a lock on its IO lines.  It only needs to be called once.
	@param index An integer specifying the channel (0 - 7).
	@param state An integer specifying the active state - 1 (active) or 0 (inactive).
	@return Zero on success.
	
	
  \par Example
  \code
  if( AnalogIn_SetActive( 0, 1 )  == CONTROLLER_OK ) // set analogin 0 to active
		// then continue processing
	else
		// some error occurred
  \endcode
*/
int AnalogIn_SetActive( int index, int state )
{
  if ( index < 0 || index >= ANALOGIN_CHANNELS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( state )
  {
    if( AnalogIn == NULL )
    {
      AnalogIn = MallocWait( sizeof( struct AnalogIn_ ), 100 );
      AnalogIn->users = 0;
      int i;
      for( i = 0; i < ANALOGIN_CHANNELS; i++ )
        AnalogIn->channelUsers[ i ] = 0;
      #ifdef OSC
      AnalogIn_AutoSendInit( );
      #endif
    }

    return AnalogIn_Start( index );
  }
  else
  {
    // TODO: check if any channels are still active and if not, Free( ) the AnalogIn struct
    return AnalogIn_Stop( index );
  }
}

/**
	Returns the active state of a channel.
	@param index An integer specifying the ANALOGIN channel (0 - 7).
	@return State - 1/non-zero (active) or 0 (inactive).
	
	\par Example
  \code
  int active = AnalogIn_GetActive( 0 ) // check whether analogin 0 is active
  \endcode
*/
int AnalogIn_GetActive( int index )
{
  if( AnalogIn == NULL )
    return 0;

  if ( index < 0 || index >= ANALOGIN_CHANNELS )
    return false;
  return AnalogIn->channelUsers[ index ] > 0;
}

/**	
	Read the value of an analog input.
	@param index An integer specifying which input (0-7).
	@return The value as an integer (0 - 1023).
	
	\par Example
  \code
  int analogin1 = AnalogIn_GetValue( 1 );
  \endcode
*/
int AnalogIn_GetValue( int index )
{
  AnalogIn_SetActive( index, 1 );
  if ( index < 0 || index >= ANALOGIN_CHANNELS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( AnalogIn->channelUsers[ index ] < 1 )
  {
    int status = AnalogIn_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  int value;

  if ( !xSemaphoreTake( AnalogIn->semaphore, 1000 ) )
    return -1;

  /* Third Step: Select the active channel */
  int mask = 1 << index; 
  AT91C_BASE_ADC->ADC_CHDR = ~mask;
  AT91C_BASE_ADC->ADC_CHER = mask;
  
  /* Fourth Step: Start the conversion */
  AT91C_BASE_ADC->ADC_CR = AT91C_ADC_START;

  /* Busy wait */
  // while ( !( AT91C_BASE_ADC->ADC_CHSR  & ( 1 << index ) ) );

  if ( !xSemaphoreTake( AnalogIn->doneSemaphore, 1000 ) )
    return -1;

  value = AT91C_BASE_ADC->ADC_LCDR & 0xFFFF;

  xSemaphoreGive( AnalogIn->semaphore );

  return value;
}

/**	
  Read the value of several of the analog inputs.
  Due to the current ISR handling, this isn't too much quicker than making
	calls to each of the channels individually.
  @param mask A bit mask specifying which channels to read.
  @param values A pointer to an int array to be filled with the values.
  @return 0 on success, otherwise non-zero.
	
	\par Example
  \code
  int mask = 0xFF;
	int samples[8];
	AnalogIn_GetValueMulti( mask, samples ); // now samples is filled with all the analogin values
  \endcode
*/
int AnalogIn_GetValueMulti( int mask, int values[] )
{
  //AnalogIn_SetActive( 1 );
  if ( mask < 0 || mask > 255 ) // check the value is a valid 8-bit mask
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  int i; // Is this the best way to make sure everything is started up properly?
  for( i = 0; i < 8; i++ )
  {
    if( mask >> i & 1 )
    {
      if ( AnalogIn->channelUsers[ i ] < 1 )
      {
        int status = AnalogIn_Start( i );
        if ( status != CONTROLLER_OK )
        return status;
      }
    }
  }

  if ( !xSemaphoreTake( AnalogIn->semaphore, 1000 ) )
    return -1;

  /* Third Step: Select the active channels */
  AT91C_BASE_ADC->ADC_CHDR = ~mask;
  AT91C_BASE_ADC->ADC_CHER = mask;
  
  /* Fourth Step: Start the conversion */
  AT91C_BASE_ADC->ADC_CR = AT91C_ADC_START;

  if ( !xSemaphoreTake( AnalogIn->doneSemaphore, 1000 ) )
    return -1;

  //Figure out which of the channels we want to read
  volatile uint* reg = &AT91C_BASE_ADC->ADC_CDR0; // the address of the first ADC result register
  for( i = 0; i < 8; i++ )
  {
    if( mask >> i & 1 )
      values[ i ] = *reg++ & 0xFFFF;
  }

  xSemaphoreGive( AnalogIn->semaphore );

  return CONTROLLER_OK;
}

/**	
	Read the value of an analog input without the use of any OS services.
  Note that this is not thread safe and shouldn't be used if another 
  part of the code might be using it or the thread safe versions.
	@param index An integer specifying which input (0-7).
	@return The value as an integer (0 - 1023).
	
	\par Example
  \code
  int analogin1 = AnalogIn_GetValueWait( 1 );
  \endcode
*/
int AnalogIn_GetValueWait( int index )
{
  AnalogIn_SetActive( index, 1 );
  if ( index < 0 || index >= ANALOGIN_CHANNELS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( AnalogIn->channelUsers[ index ] < 1 )
  {
    int status = AnalogIn_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  int value = 0;

  // Third Step: Select the active channel
  int mask = 1 << index; 
  AT91C_BASE_ADC->ADC_CHDR = ~mask;
  AT91C_BASE_ADC->ADC_CHER = mask;
  
  AT91C_BASE_ADC->ADC_IDR = AT91C_ADC_DRDY; 

  // Fourth Step: Start the conversion
  AT91C_BASE_ADC->ADC_CR = AT91C_ADC_START;

  value++;
  value++;

  // Busy wait
  while ( !( AT91C_BASE_ADC->ADC_SR & AT91C_ADC_DRDY ) )
    value++;

  AT91C_BASE_ADC->ADC_IDR = AT91C_ADC_DRDY; 

  value = AT91C_BASE_ADC->ADC_LCDR & 0xFFFF;

  return value;
}

/** @}
*/

#ifdef OSC
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

int AnalogIn_Start( int index )
{
  if ( index < 0 || index >= ANALOGIN_CHANNELS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;
  if ( AnalogIn->channelUsers[ index ]++ == 0 )
  {
    int status;

    // The lower four channel pins are shared with other subsystems, so no locking
    if ( index < 4 )
    {
      int io = AnalogIn_GetIo( index );
      status = Io_Start( io, false );
      if ( status != CONTROLLER_OK )
      {
        AnalogIn->channelUsers[ index ]--;
        return status;
      }

      Io_SetPullup( io, false );
    }

    if ( AnalogIn->users++ == 0 )
    {
      AnalogIn_Init();  
    }
  }
  
  return CONTROLLER_OK;
}

int AnalogIn_Stop( int index )
{
  if ( index < 0 || index >= ANALOGIN_CHANNELS )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;
  if ( AnalogIn->channelUsers[ index ] <= 0 )
    return CONTROLLER_ERROR_TOO_MANY_STOPS;
  if ( --AnalogIn->channelUsers[ index ] == 0 )
  {
    if ( index < 4 )
    {
      int io = AnalogIn_GetIo( index );
      Io_Stop( io );
    }
    if ( --AnalogIn->users == 0 )
    {
      AnalogIn_Deinit();
    }
  }
  return CONTROLLER_OK;
}

int AnalogIn_Init()
{
  // Enable the peripheral clock
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_ADC;

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

  // Do the OS stuff

  vSemaphoreCreateBinary( AnalogIn->semaphore );

  // Create the sempahore that will be used to wake the calling process up 
  vSemaphoreCreateBinary( AnalogIn->doneSemaphore );
  xSemaphoreTake( AnalogIn->doneSemaphore, 0 );

  // Initialize the interrupts
  // WAS AT91F_AIC_ConfigureIt( AT91C_ID_ADC, 3, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, ( void (*)( void ) ) AnalogInIsr_Wrapper );
  // Which is defined at the bottom of the AT91SAM7X256.h file
  unsigned int mask ;													
																			
  mask = 0x1 << AT91C_ID_ADC;		
                        
  /* Disable the interrupt on the interrupt controller */					
  AT91C_BASE_AIC->AIC_IDCR = mask ;										
  /* Save the interrupt handler routine pointer and the interrupt priority */	
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_ADC ] = (unsigned int)AnalogInIsr_Wrapper;			
  /* Store the Source Mode Register */									
  AT91C_BASE_AIC->AIC_SMR[ AT91C_ID_ADC ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 4  ;				
  /* Clear the interrupt on the interrupt controller */					
  AT91C_BASE_AIC->AIC_ICCR = mask ;					

  AT91C_BASE_ADC->ADC_IER = AT91C_ADC_DRDY; 

	AT91C_BASE_AIC->AIC_IECR = mask;

  return CONTROLLER_OK;
}

int AnalogIn_Deinit()
{
  return CONTROLLER_OK;
}

int AnalogIn_GetIo( int index )
{
  int io = -1;
  switch ( index )
  {
    case 0:
      io = ANALOGIN_0_IO;
      break;
    case 1:
      io = ANALOGIN_1_IO;
      break;
    case 2:
      io = ANALOGIN_2_IO;
      break;
    case 3:
      io = ANALOGIN_3_IO;
      break;
  }
  return io;
}

#ifdef OSC

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
