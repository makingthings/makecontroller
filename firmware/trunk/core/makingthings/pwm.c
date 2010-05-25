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
#include "error.h"

#define PWM_DUTY_MAX 1024
#define PWM_CHANNEL_0_IO PIN_PB19
#define PWM_CHANNEL_1_IO PIN_PB20
#define PWM_CHANNEL_2_IO PIN_PB21
#define PWM_CHANNEL_3_IO PIN_PB22

static unsigned int pwmFindClockConfiguration(int frequency, bool forChannel);
static bool pwmSetFrequency(int freq);

static int pwmGetIo(int channel)
{
  switch (channel) {
    case 0: return PWM_CHANNEL_0_IO;
    case 1: return PWM_CHANNEL_1_IO;
    case 2: return PWM_CHANNEL_2_IO;
    case 3: return PWM_CHANNEL_3_IO;
    default: return 0;
  }
}

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

  \section Frequency
  When the PWM system is started up, its frequency is set at 20 kHz.  You can override this
  by defining \b PWM_DEFAULT_FREQ in your config.h file.

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
  You can specify several pieces of configuration for the PWM channel.  All PWM channels
  share a baseline frequency, but each channel can be configured to run at some slower
  rate by specifying how many times to divide the central PWM frequency.

  @param channel Which PWM channel - valid options are 0, 1, 2, 3.
  @param divider (optional) How much to divide the baseline PWM frequency for this channel.
  @param center_aligned (optional) Whether this channel is center aligned, otherwise is left aligned.
  @param starts_high (optional) Whether this channel starts high, otherwise starts low.

  \b Example
  \code
  pwmEnableChannel(0); // enable channel 0
  \endcode
*/
bool pwmEnable(int channel, int frequency, bool center_aligned, bool starts_high)
{
  // configure to use peripheral A...all channels are on port B
  pinSetMode(pwmGetIo(channel), PERIPHERAL_A);

  // make sure the channel is disabled
  AT91C_BASE_PWMC->PWMC_DIS = 1 << channel;
  while ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) != 0);

  AT91S_PWMC_CH *pwm = &AT91C_BASE_PWMC->PWMC_CH[channel];
  // 0 is special value to just base PWM clock - otherwise try to find best frequency
  // as a divider of MCK
  pwm->PWMC_CMR = ((frequency == 0) ? AT91C_PWMC_CPRE_MCKA : pwmFindClockConfiguration(frequency, YES)) |
                  (center_aligned ? AT91C_PWMC_CALG : 0) |
                  (starts_high    ? AT91C_PWMC_CPOL : 0);

  pwm->PWMC_CPRDR = PWM_DUTY_MAX; // Set the Period register (sample size bit fied)
  pwmSetDuty(channel, 0);         // Set the duty cycle

  AT91C_BASE_PWMC->PWMC_ENA = (1 << channel); // enable this channel
  return true;
}

/**
  Disable a PWM channel.
  @param channel Which PWM channel - valid options are 0, 1, 2, 3.

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
  if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) == 0)
      AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CDTYR = duty;
  // Otherwise use update register
  else {
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CMR &= ~AT91C_PWMC_CPD;
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CUPDR = duty;
  }
}

/**
  Initialize the PWM system.
  Note - this is called automatically during startup with the value of \b PWM_DEFAULT_FREQ.
  You can override PWM_DEFAULT_FREQ in your config.h file to start up with a different frequency.
  @param frequency The base frequency for the PWM system.
*/
void pwmInit(int frequency)
{
  // turn on pwm power, disable all channels and configure clock A
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PWMC;
  AT91C_BASE_PWMC->PWMC_DIS = AT91C_PWMC_CHID0 | AT91C_PWMC_CHID1 | AT91C_PWMC_CHID2 | AT91C_PWMC_CHID3;
  pwmSetFrequency(frequency);
}

/**
  Deinitialize the PWM system.
*/
void pwmDeinit()
{
  AT91C_BASE_PMC->PMC_PCDR = 1 << AT91C_ID_PWMC; // disable the PWM clock
  AT91C_BASE_PWMC->PWMC_DIS = AT91C_PWMC_CHID0 | AT91C_PWMC_CHID1 | AT91C_PWMC_CHID2 | AT91C_PWMC_CHID3;
}

/** @} */

/*
  Set the frequency of the PWM system.
  Note that this will change the frequency for all PWM channels

  @param freq The frequency in Hz (cycles per second).
  @return True if the frequency was set successfully, false if not.

  \b Example
  \code
  pwmSetFrequency(500); // set it to 500 Hz
  \endcode
*/
bool pwmSetFrequency(int freq)
{
  unsigned int mode = 0;
  unsigned int result = pwmFindClockConfiguration(freq * 1000, NO);
  if (!result)
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

/*
  Try to find the best fit for a frequency, given our MCK.
  If we're doing this for a channel (forChannel), the default value if we fail
  should be clock A.  If we're doing it for the main PWM system, the default
  value if a valid clock isn't found should be 0.
  adapted from at91lib softpack.
*/
unsigned int pwmFindClockConfiguration(int frequency, bool forChannel)
{
  static const uint32_t divisors[11] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024 };
  uint8_t divisor = 0;
  uint32_t prescaler;

  // Find prescaler and divisor values
  prescaler = (MCK / divisors[divisor]) / frequency;
  while ((prescaler > 255) && (divisor < 11)) {
    divisor++;
    prescaler = (MCK / divisors[divisor]) / frequency;
  }

  if (forChannel)
    return (divisor < 11) ? divisor : AT91C_PWMC_CPRE_MCKA;
  else
    return (divisor < 11) ? prescaler | (divisor << 8) : 0;
}



