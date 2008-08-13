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

/** \file pwm.c	
	PWM - Pulse Width Modulation.
	Library of functions for the Make Application Board's PwmOut Subsystem.
*/

/* Library includes. */
#include <string.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Hardware specific headers. */
#include "Board.h"
#include "AT91SAM7X256.h"

#include "io.h"
#include "config.h"
#include "pwm.h"

int Pwm_GetChannelIo( int channel );

#define PWM_DUTY_MAX 1024

#define PWM_COUNT 4

#define PWM_CHANNEL_0_IO IO_PB19
#define PWM_CHANNEL_1_IO IO_PB20
#define PWM_CHANNEL_2_IO IO_PB21
#define PWM_CHANNEL_3_IO IO_PB22

static int Pwm_Init( void );
static int Pwm_Deinit( void );

struct Pwm_
{
  int users;
  int channels;
  int duty[ PWM_COUNT ];
} Pwm;

/** \defgroup Pwm PWM (Pulse Width Modulation)
	The PWM subsystem provides control of the 4 PWM outputs on the SAM7X.

  The Make Controller has 4 PWM lines.  These can each be configured separately and can control
  up to 2 output lines directly, the 2 lines running either parallel or inverted.  For a very simple
  start, just see Pwm_Set( ) and Pwm_Get( ) as these will start driving your PWMs immediately with
  very little hassle.  

  \section padjust Period adjustment of the PWM unit
  Configuring and setting the clock for the PWM system can be quite a complicated matter.  Here are 
  some of the relevant issues.
  
  Each of the 4 PWM channels is fed in a clock, as determined by its clock source:
   - Clock source 0-10 represent the Master clock divided by 2 to the power of the Clock Source Value.  eg. a clock 
   source value of 5 = a clock rate of MasterClock/(2^5)
   - A value of 11 sets the Clock source to be generated by clock Divider A (Default)
   - A value of 12 sets the Clock source to be generated by clock Divider B
  
  If either Clock Divider A or Clock Divider B is used, you can adjust their values individually to
  allow the clock period to precisely match your needs.  Each clock divider has two values, a \b Mux value 
  and a \b Divider value.  
  
  The mux works just like the clock source values, and chooses Master clock divided by 2 to 
  the power of the Clock Source Value, eg. a DividerXMux value of 5 == a clock rate of MasterClock/(2^5).
  The Divider value sets a linear divider, which is fed the clock value as selected by the Mux, and returns that 
  clock value divided by the divider value.  This output value is what is fed out of the divider unit.  A output 
  formula: 
  \code output = MCLK / ( (2^DividerMux) * DividerValue ) \endcode
	
  The PWM subsystem of the Controller Board can be used independently from the \ref PwmOut 
  library, since the \ref PwmOut library relies on the core PWM.

	\ingroup Core
	@{
*/

/**	
	Set the duty of a PWM device.
	@param index An integer specifying which PWM device (0-3).
  @param duty The duty - (0 - 1023).
	@return 0 on success.
*/
int Pwm_Set( int index, int duty )
{
  if ( index < 0 || index > PWM_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  // Set the duty
  Pwm.duty[ index ] = duty;
  AT91C_BASE_PWMC->PWMC_CH[ index ].PWMC_CUPDR = duty;

  return CONTROLLER_OK;
}

/**	
	Read the current duty of a PWM device.
	@param index An integer specifying which PWM device (0-3).
  @return The duty - (0 - 1023).
*/
int Pwm_Get( int index )
{
  if ( index < 0 || index > PWM_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  return Pwm.duty[ index ];
}

int Pwm_Start( int channel )
{
  int status;

  if ( channel < 0 || channel > PWM_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  // Make sure the channel isn't already being used
  int c = 1 << channel;
  if ( c & Pwm.channels )
    return CONTROLLER_ERROR_CANT_LOCK;

  // lock the correct select line 
  int io = Pwm_GetChannelIo( channel );
  // Try to lock the pin
  status = Io_Start( io, true );
  if ( status != CONTROLLER_OK )
    return status;

  // mark the channel as being used
  Pwm.channels |= c;

  // Disable the PIO for the IO Line
  Io_SetPio( io, false );

  Io_SetPeripheralA( io );

  if ( Pwm.users++ == 0 )
  {   
    status = Pwm_Init();
  }

  // Enable the current channel
  AT91C_BASE_PWMC->PWMC_ENA = 1 << channel;

  return CONTROLLER_OK;
}

int Pwm_Stop( int channel )
{
  if ( Pwm.users <= 0 )
    return CONTROLLER_ERROR_TOO_MANY_STOPS;

  if ( channel < 0 || channel > PWM_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  int c = 1 << channel;
  if ( !( c & Pwm.channels ) )
    return CONTROLLER_ERROR_NOT_LOCKED;

  // Get the IO
  int io = Pwm_GetChannelIo( channel );

  // Set the pin to OFF
  Io_SetValue( io, false );
  Io_SetPio( io, true );
  
  // release the pin
  Io_Stop( io );

  Pwm.channels &= ~c;

  // Disable the PWM's
  AT91C_BASE_PWMC->PWMC_DIS = 1 << channel;

  if ( --Pwm.users == 0 )
    Pwm_Deinit();

  return CONTROLLER_OK;
}

int Pwm_Init()
{
  // Configure PMC by enabling PWM clock
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PWMC;

  // Initially they're stopped
  AT91C_BASE_PWMC->PWMC_DIS = AT91C_PWMC_CHID0 | AT91C_PWMC_CHID1 | AT91C_PWMC_CHID2 | AT91C_PWMC_CHID3;
  
  // Set the Clock A divider
  AT91C_BASE_PWMC->PWMC_MR = (( 4 << 8 ) | 0x08 );  // MCK selection or'ed with Divider

  // Set the Clock
  int i;
  for ( i = 0; i < 4; i++ )
  {
    AT91S_PWMC_CH *pwm = &AT91C_BASE_PWMC->PWMC_CH[ i ];

    pwm->PWMC_CMR =
      // AT91C_PWMC_CPRE_MCK |  // Divider Clock ? 
         AT91C_PWMC_CPRE_MCKA |    //Divider Clock A
      // AT91C_PWMC_CPRE_MCKB;  //Divider Clock B
      // AT91C_PWMC_CPD;  // Channel Update Period 
         AT91C_PWMC_CPOL; // Channel Polarity Invert
      // AT91C_PWMC_CALG ; // Channel Alignment Center

    // Set the Period register (sample size bit fied )
    pwm->PWMC_CPRDR = PWM_DUTY_MAX;
  
    // Set the duty cycle register (output value)
    pwm->PWMC_CDTYR = 0;

    // Initialise the Update register write only
    pwm->PWMC_CUPDR = 0 ;
  }

  return CONTROLLER_OK;
}

int Pwm_Deinit()
{
  // Deconfigure PMC by disabling the PWM clock
  AT91C_BASE_PMC->PMC_PCDR = 1 << AT91C_ID_PWMC;

  return CONTROLLER_OK;
}

int Pwm_GetChannelIo( int channel )
{  
  int io;
  switch ( channel )
  {
    case 0:
      io = PWM_CHANNEL_0_IO;
      break;
    case 1:
      io = PWM_CHANNEL_1_IO;
      break;
    case 2:
      io = PWM_CHANNEL_2_IO;
      break;
    case 3:
      io = PWM_CHANNEL_3_IO;
      break;
    default:
      io = 0;
      break;
  }
  return io;
}

/**
  Adjust the clock period on Divider A.
  See AT91SAM7X manual for more information p.421

  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @param val An int between 0 and 4096.
  @return 0 on success.
*/
int Pwm_SetDividerA(int val)
{ 
  if( val < 0 || val > ( 1 << 12 ) )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  // First disable PWM controller for all 4 channels.
  AT91C_BASE_PWMC->PWMC_DIS = 0x0000000f;

  // Now Set the new Clock values
  AT91C_BASE_PWMC->PWMC_MR = (AT91C_BASE_PWMC->PWMC_MR & 0xffff0000) | val;

  // Re-enable the active channels
  AT91C_BASE_PWMC->PWMC_ENA = Pwm.channels;
  
  return CONTROLLER_OK;
}

/**
  Read the clock period on Divider A.

  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @return The clock period on Divider A.
*/
int Pwm_GetDividerA()
{
  return (AT91C_BASE_PWMC->PWMC_MR & 0x0000ffff);
}

/**
  Adjust the clock period on Divider B.
  See AT91SAM7X manual for more information p.421

  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @param val An int between 0 and 4096.
  @return 0 on success.
*/
int Pwm_SetDividerB(int val)
{ 
  if( val < 0 || val > ( 1 << 12 ) )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  //New function which adjusts the clock period on Divider A.
  //See AT91SAM7X manual for more information p.421

  // First disable PWM controller for all 4 channels.
  AT91C_BASE_PWMC->PWMC_DIS = 0x0000000f;

  // Now Set the new Clock values
  AT91C_BASE_PWMC->PWMC_MR = (AT91C_BASE_PWMC->PWMC_MR & 0x0000ffff) | ( val << 16 );

  // Re-enable the active channels
  AT91C_BASE_PWMC->PWMC_ENA = Pwm.channels;
  
  return CONTROLLER_OK;
}

/**
  Read the clock period on Divider B.

  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @return The clock period on Divider B.
*/
int Pwm_GetDividerB()
{
  return (AT91C_BASE_PWMC->PWMC_MR >> 16);
}

/**
  Set the clock divider for a particular channel.
  For values 0-10, the Master_Clock is divided by (2^val).  For example,
  for 4 the resulting period will be Master Clock / 16, as 2 ^ 4 == 16.

  When val is 11, Clock Divider A is used.  When val is 12, Clock Divider B
  is used.  Values other than 0 - 12 are not valid.

  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @param channel The PWM channel (0-3) you'd like to configure.
  @param val The new clock divider.
  @return 0 on success.
*/
int Pwm_SetClockSource(int channel, int val)
{
  if ( channel < 0 || channel > PWM_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( val < 0 || val > 12 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  AT91S_PWMC_CH *pwm = &AT91C_BASE_PWMC->PWMC_CH[ channel ];
  int c = 1 << channel;

  // Disable the Channel
  AT91C_BASE_PWMC->PWMC_DIS = c;

  // Set the Channel Divider Value
  pwm->PWMC_CMR = (pwm->PWMC_CMR & 0x00000700) | val;
  
  // Re-enable the Channel
  AT91C_BASE_PWMC->PWMC_ENA = c;

  return CONTROLLER_OK;
}

/**
  Read the clock source for a particular channel.
  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @param channel The PWM channel (0-3) whose channel you'd like to read.
  @return The clock source.
  @see Pwm_SetClockSource( )
*/
int Pwm_GetClockSource(int channel)
{
  if ( channel < 0 || channel > PWM_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  return (AT91C_BASE_PWMC->PWMC_CH[ channel ].PWMC_CMR & 0x0000000f);
}

/**
  Set the wave form properties of the PWM contoller for a given channel.
  The waveform properties are controlled by bits 0, 1, and 2.
   - bit 0 sets whether the PWModule is Left aligned (0) or Center aligned (1)
   - bit 1 sets whether the PWMoudle's polarity starts out low (0) or high (1)
   - bit 2 is not supported at this time.

  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @param channel the PWM channel (0-3) that you want to configure
  @param val The mask of values as described above.
  @return 0 on success.
*/
int Pwm_SetWaveformProperties(int channel, int val)
{
  if ( channel < 0 || channel > PWM_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( val < 0 || val > 3 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  AT91S_PWMC_CH *pwm = &AT91C_BASE_PWMC->PWMC_CH[ channel ];
  int c = 1 << channel;

  // Disable the Channel
  AT91C_BASE_PWMC->PWMC_DIS = c;

  // Set the Channel Divider Value
  pwm->PWMC_CMR = (pwm->PWMC_CMR & 0x0000040f) | ( val << 8 );
  
  // Re-enable the Channel
  AT91C_BASE_PWMC->PWMC_ENA = c;

  return CONTROLLER_OK;
}

/**
  Read the waveform configuration of a specified PWM channel

  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @param channel The PWM channel (0-3) to read from.
  @return A bitmask describing the waveform configuration.
  @see Pwm_SetWaveformProperties( )
*/
int Pwm_GetWaveformProperties(int channel)
{
  if ( channel < 0 || channel > PWM_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;
  
  return ( (AT91C_BASE_PWMC->PWMC_CH[ channel ].PWMC_CMR >> 8) & 0x00000003 );
}

/** @}
*/
