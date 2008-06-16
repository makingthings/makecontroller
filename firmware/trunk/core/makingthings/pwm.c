/*********************************************************************************

 Copyright 2006 MakingThings

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
	//ToDo: DW choose which functions are public, and confirm/correct annotations
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

/** \defgroup Pwm
	The Pwm subsystem provides control of the 4 PWM outputs on the SAM7X.
	The Pwm subsystem of the Controller Board can be used independently from the PWM_Out subsystem
	of the Application Board, and in fact the AppBoard PwmOut relies on the Controller Board PWM.
	
	@see PWM_Out
	\ingroup Controller
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

/** @}
*/

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
  Io_PioDisable( io );

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
  Io_PioEnable( io );
  
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
