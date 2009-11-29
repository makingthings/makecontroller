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

#include "pwm.h"
#include "ch.h"
#include "hal.h"
#include "error.h"

#ifndef PWM_DEFAULT_FREQ
#define PWM_DEFAULT_FREQ 350
#endif

#define PWM_DUTY_MAX 1024
#define PWM_CHANNEL_0_IO AT91C_PB19_PWM0
#define PWM_CHANNEL_1_IO AT91C_PB20_PWM1
#define PWM_CHANNEL_2_IO AT91C_PB21_PWM2
#define PWM_CHANNEL_3_IO AT91C_PB22_PWM3

static int pwmFindClockConfiguration(int frequency);
static int pwmGetIo( int channel );

/**
  Create a new PWM channel.

  @param channel Which channel to use - valid options are 0, 1, 2, 3.

  \b Example
  \code
  Pwm p(0); // make a new Pwm for channel 0
  p.setDuty(512);
  \endcode
*/
bool pwmEnableChannel( int channel )
{
  if ( channel < 0 || channel >= PWM_COUNT )
    return false;
  // configure to use peripheral A...all channels are on port B
  AT91C_BASE_PIOB->PIO_ASR = pwmGetIo(channel);

  unsigned int mask = 1 << channel;
  AT91S_PWMC_CH *pwm = &AT91C_BASE_PWMC->PWMC_CH[ channel ];
  pwm->PWMC_CMR =
    // AT91C_PWMC_CPRE_MCK |  // Divider Clock ? 
       AT91C_PWMC_CPRE_MCKA |    //Divider Clock A
    // AT91C_PWMC_CPRE_MCKB;  //Divider Clock B
    // AT91C_PWMC_CPD;  // Channel Update Period 
       AT91C_PWMC_CPOL; // Channel Polarity Invert
    // AT91C_PWMC_CALG ; // Channel Alignment Center

  pwm->PWMC_CPRDR = PWM_DUTY_MAX; // Set the Period register (sample size bit fied )
  pwm->PWMC_CDTYR = 0;            // Set the duty cycle register (output value)
  pwm->PWMC_CUPDR = 0 ;           // Initialise the Update register write only

  AT91C_BASE_PWMC->PWMC_ENA = mask; // enable this channel
  return true;
}

void pwmDisableChannel( int channel )
{
  // could reconfig the pio pin as well, possibly...
  AT91C_BASE_PWMC->PWMC_DIS = 1 << channel; // disable this channel
}

/**	
	Set the duty of a PWM device.
  @param duty The duty - (0 - 1023).

  \b Example
  \code
  Pwm myPwm(3);
  myPwm.setDuty( 512 ); // set duty to half
  \endcode
*/
void pwmSetDuty( int channel, int duty )
{
  // If channel is disabled, write to CDTY
  if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) == 0)
      AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CDTYR = duty;
  // Otherwise use update register
  else {
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CMR &= ~AT91C_PWMC_CPD;
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CUPDR = duty;
  }
}

void pwmInit()
{
  // turn on pwm power, disable all channels and configure clock A
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PWMC;
  AT91C_BASE_PWMC->PWMC_DIS = AT91C_PWMC_CHID0 | AT91C_PWMC_CHID1 | AT91C_PWMC_CHID2 | AT91C_PWMC_CHID3;
  pwmSetFrequency(PWM_DEFAULT_FREQ);
}

void pwmDeinit()
{
  AT91C_BASE_PMC->PMC_PCDR = 1 << AT91C_ID_PWMC; // disable the PWM clock
}

int pwmGetIo( int channel )
{  
  switch ( channel )
  {
    case 0: return PWM_CHANNEL_0_IO;
    case 1: return PWM_CHANNEL_1_IO;
    case 2: return PWM_CHANNEL_2_IO;
    case 3: return PWM_CHANNEL_3_IO;
    default: return 0;
  }
}

/**
  Configure the waveform of this Pwm channel.
  You can specify the alignment - options are left and center - and
  the polarity - whether the waveform starts at a low or a high level.
  The default is left aligned, and starting high.

  @param left_aligned Whether the waveform should be left aligned (is otherwise center aligned).
  @param starts_low Whether the waveform should start at a low level (otherwise starts high).

  \b Example
  \code
  Pwm pwm(1); // channel 1
  pwm.setWaveform( false, true ); // set it to right-aligned and starting low
  \endcode
*/
void pwmSetWaveform( int channel, bool left_aligned, bool starts_low )
{
  unsigned int alignment = left_aligned ? 0 : 1;
  unsigned int polarity = starts_low ? 0 : 1;
  // Disable channel (effective at the end of the current period)
  if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) != 0) {
    AT91C_BASE_PWMC->PWMC_DIS = 1 << channel;
    while ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) != 0);
  }

  // Configure channel
  AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CMR = alignment | polarity;
}

/**
  Set the frequency of the PWM system.
  Note that this will change the frequency for all PWM channels

  @param freq The frequency in Hz (cycles per second).
  @return True if the frequency was set successfully, false if not.

  \b Example
  \code
  Pwm::setFrequency(500); // set it to 500 Hz
  \endcode
*/
bool pwmSetFrequency(int freq)
{
  unsigned int mode = 0;
  unsigned int result = pwmFindClockConfiguration( freq * 1000 );
  if(!result)
    return false;
  mode |= result;
  /*
    All channels use clock divider A currently, so we can just set the frequency there
    and be done with it.  If we also wanted to configure channel B, we'd do something like
    result = findClockConfiguration( freq );
    mode |= (result << 16);
  */
  AT91C_BASE_PWMC->PWMC_MR = mode;
  return true;
}

/**
  Set the period of a cycle.
  
  @param period The period in for a cycle.

  \b Example
  \code
  Pwm* p = new Pwm(2);
  p->setPeriod(200);
  \endcode
*/
bool pwmSetPeriod( int channel, int period )
{
  // If channel is disabled, write to CPRD
  if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) == 0)
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CPRDR = period;
  // Otherwise use update register
  else {
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CMR |= AT91C_PWMC_CPD;
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CUPDR = period;
  }
  return true;
}

// from at91lib softpack
int pwmFindClockConfiguration(int frequency)
{
  static const unsigned int divisors[11] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
  unsigned char divisor = 0;
  unsigned int prescaler;

  // Find prescaler and divisor values
  prescaler = (MCK / divisors[divisor]) / frequency;
  while ((prescaler > 255) && (divisor < 11)) {
    divisor++;
    prescaler = (MCK / divisors[divisor]) / frequency;
  }

  // Return result
  if (divisor < 11)
    return prescaler | (divisor << 8);
  else
    return 0;
}



