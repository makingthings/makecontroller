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
#include "core.h"

#ifndef PWM_DEFAULT_FREQ
#define PWM_DEFAULT_FREQ 5000
#endif

#define PWM_DUTY_MAX 1023
#define PWM_CHANNEL_0_PIN PIN_PB19
#define PWM_CHANNEL_1_PIN PIN_PB20
#define PWM_CHANNEL_2_PIN PIN_PB21
#define PWM_CHANNEL_3_PIN PIN_PB22

static int pwmFindClockConfiguration(int frequency);
static int pwmGetPin(int channel);

/**
  \defgroup PWM
  Control the 4 Pulse Width Modulation outputs.

  The Make Controller has 4 PWM signals.  Each be configured separately, and can control
  up to 2 output lines.  These lines can drive the same signal or they can
  be inverted from one another.

  \section Usage
  First call pwmInit() to initialize the pwm system, then enable any channels you want to use
  via pwmEnableChannel().  pwmSetDuty() will control the duty cycle for a given channel,
  and pwmSetWaveform() gives some more control over the output.

  \section Hardware
  The PWM lines on the Make Controller are located on the following signal lines:
  - channel 0 is PB19
  - channel 1 is PB20
  - channel 2 is PB21
  - channel 3 is PB22

  The \ref pwmout system relies on the Pwm system, and provides control of the output lines associated with
  a given Pwm signal.

  \ingroup io
  @{
*/

/**
  Enable a PWM channel.
  Specify which channel, and also the configuration you'd like.  The alignment and polarity are optional - if you
  don't need these, simply set them to 0 to use the default settings.
  @param channel Which channel to use - valid options are 0, 1, 2, 3.
  @param center_aligned (optional) Whether the waveform should be center aligned (is otherwise left aligned).
  @param starts_low (optional) Whether the waveform should start at a low level (otherwise starts high).

  \b Example
  \code
  pwmEnableChannel(0, 0, 0); // enable channel 0
  \endcode
*/
bool pwmEnable(int channel, bool center_aligned, bool starts_low)
{
  // configure to use peripheral A...all channels are on port B
  pinSetMode(pwmGetPin(channel), PERIPHERAL_A);

  // Disable channel (effective at the end of the current period)
  AT91C_BASE_PWMC->PWMC_DIS = 1 << channel;
  while ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) != 0)
    ;

  AT91S_PWMC_CH *pwm = &AT91C_BASE_PWMC->PWMC_CH[channel];
  pwm->PWMC_CMR = AT91C_PWMC_CPRE_MCKA |                    // Divider Clock A
                  (starts_low     ? 0 : AT91C_PWMC_CPOL) |  // Channel Polarity Invert
                  (center_aligned ? AT91C_PWMC_CALG : 0);   // Channel Alignment Center

  pwm->PWMC_CPRDR = PWM_DUTY_MAX; // Set the Period register (sample size bit fied )
  pwm->PWMC_CDTYR = 0;            // Set the duty cycle register (output value)
  pwm->PWMC_CUPDR = 0;            // Initialise the Update register write only

  AT91C_BASE_PWMC->PWMC_ENA = (1 << channel); // enable this channel
  return true;
}

/**
  Disable a PWM channel.
  @param channel Which channel to use - valid options are 0, 1, 2, 3.

  \b Example
  \code
  pwmDisableChannel(0); // disable channel 0
  \endcode
*/
void pwmDisable(int channel)
{
  // could reconfig the pio pin as well, possibly...
  AT91C_BASE_PWMC->PWMC_DIS = (1 << channel); // disable this channel
}

/**	
	Set the duty of a PWM device.
	@param channel Which channel to use - valid options are 0, 1, 2, 3.
  @param duty The duty - (0 - 1023).

  \b Example
  \code
  pwmSetDuty(3, 512); // set duty to half on channel 3
  \endcode
*/
void pwmSetDuty(int channel, int duty)
{
  // If channel is disabled, write to CDTY
  if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) == 0) {
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CDTYR = duty;
  }
  else { // Otherwise use update register
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CMR &= ~AT91C_PWMC_CPD;
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CUPDR = duty;
  }
}

/**
  Initialize the PWM system.
*/
void pwmInit()
{
  // turn on pwm power, disable all channels and configure clock A
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PWMC;
  AT91C_BASE_PWMC->PWMC_DIS = AT91C_PWMC_CHID0 | AT91C_PWMC_CHID1 | AT91C_PWMC_CHID2 | AT91C_PWMC_CHID3;
  pwmSetFrequency(PWM_DEFAULT_FREQ);
}

/**
  Deinitialize the PWM system.
*/
void pwmDeinit()
{
  AT91C_BASE_PMC->PMC_PCDR = 1 << AT91C_ID_PWMC; // disable the PWM clock
  AT91C_BASE_PWMC->PWMC_DIS = AT91C_PWMC_CHID0 | AT91C_PWMC_CHID1 | AT91C_PWMC_CHID2 | AT91C_PWMC_CHID3;
}

int pwmGetPin(int channel)
{  
  switch (channel) {
    case 0: return PWM_CHANNEL_0_PIN;
    case 1: return PWM_CHANNEL_1_PIN;
    case 2: return PWM_CHANNEL_2_PIN;
    case 3: return PWM_CHANNEL_3_PIN;
    default: return 0;
  }
}

/**
  Set the frequency of the PWM system.
  Frequencies from 1000 - 45000 should work well.

  This changes the frequency for all PWM channels.
  @param freq The frequency in Hz (cycles per second).
  @return True if the frequency was set successfully, false if not.

  \b Example
  \code
  pwmSetFrequency(1000); // set it to 1 KHz
  \endcode
*/
bool pwmSetFrequency(int freq)
{
  unsigned int mode = 0;
  unsigned int result = pwmFindClockConfiguration(freq * 1000);
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

/** @}
*/

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
  return (divisor < 11) ? prescaler | (divisor << 8) : 0;
}



