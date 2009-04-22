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

#include "AT91SAM7X256.h"
#include "Board.h"
#include "io.h"
#include "pwm.h"
#include "error.h"

#define PWM_DUTY_MAX 1024
#define PWM_COUNT 4
#define PWM_DEFAULT_FREQ 350
#define PWM_CHANNEL_0_IO IO_PB19
#define PWM_CHANNEL_1_IO IO_PB20
#define PWM_CHANNEL_2_IO IO_PB21
#define PWM_CHANNEL_3_IO IO_PB22

// statics
int Pwm::activeChannels = 0;
int Pwm::_frequency = 0;

/**
  Create a new PWM channel.

  @param channel Which channel to use - valid options are 0, 1, 2, 3.

  \b Example
  \code
  Pwm p(0); // make a new Pwm for channel 0
  p.setDuty(512);
  \endcode
*/
Pwm::Pwm( int channel )
{
  this->channel = channel; // might as well write this in here so it's invalid instead of possibly defaulting to 0
  if ( channel < 0 || channel >= PWM_COUNT )
    return;
  _duty = _period = 0;
  // IO line should use peripheral A
  Io pwmPin( getIo( channel ), Io::A );

  unsigned int mask = 1 << channel;
  if( !(activeChannels & mask) )
  {
    if(!activeChannels)
      baseInit();
    
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
    activeChannels |= mask; // mark it as used
  }
}

Pwm::~Pwm( )
{
  Io pwmPin( getIo( channel ) );
  pwmPin.off(); // turn it off

  unsigned int c = 1 << channel;
  AT91C_BASE_PWMC->PWMC_DIS = c; // disable this channel
  activeChannels &= ~c; // mark it as unused
  if(!activeChannels) // if that was our last channel, turn everything off
    baseDeinit();
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
void Pwm::setDuty( int duty )
{
  _duty = duty;
  // If channel is disabled, write to CDTY
  if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) == 0)
      AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CDTYR = duty;
  // Otherwise use update register
  else
  {
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CMR &= ~AT91C_PWMC_CPD;
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CUPDR = _duty;
  }
}

/**	
	Read the current duty of a PWM device.
  @return The duty - (0 - 1023).

  \b Example
  \code
  Pwm myPwm(3);
  int d = myPwm.duty();
  \endcode
*/
int Pwm::duty( )
{
  return _duty;
}

int Pwm::baseInit()
{
  // turn on pwm power, disable all channels and configure clock A
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PWMC;
  AT91C_BASE_PWMC->PWMC_DIS = AT91C_PWMC_CHID0 | AT91C_PWMC_CHID1 | AT91C_PWMC_CHID2 | AT91C_PWMC_CHID3;
  setFrequency(PWM_DEFAULT_FREQ);
  return CONTROLLER_OK;
}

int Pwm::baseDeinit()
{
  // Deconfigure PMC by disabling the PWM clock
  AT91C_BASE_PMC->PMC_PCDR = 1 << AT91C_ID_PWMC;

  return CONTROLLER_OK;
}

int Pwm::getIo( int channel )
{  
  switch ( channel )
  {
    case 0:
      return PWM_CHANNEL_0_IO;
    case 1:
      return PWM_CHANNEL_1_IO;
    case 2:
      return PWM_CHANNEL_2_IO;
    case 3:
      return PWM_CHANNEL_3_IO;
    default:
      return 0;
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
void Pwm::setWaveform( bool left_aligned, bool starts_low )
{
  unsigned int alignment = left_aligned ? 0 : 1;
  unsigned int polarity = starts_low ? 0 : 1;
  // Disable channel (effective at the end of the current period)
  if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) != 0)
  {
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
bool Pwm::setFrequency(int freq) // static
{
  unsigned int mode = 0;
  unsigned int result = findClockConfiguration( freq * 1000 );
  if(!result)
    return false;
  mode |= result;
  _frequency = freq;
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
  Read the frequency for this PWM channel.

  @return The frequency in Hz (cycles per second).

  \b Example
  \code
  int freq = Pwm::frequency( );
  \endcode
*/
int Pwm::frequency() // static
{
  return _frequency;
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
bool Pwm::setPeriod( int period )
{
  _period = period;
  // If channel is disabled, write to CPRD
  if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) == 0)
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CPRDR = period;
  // Otherwise use update register
  else
  {
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CMR |= AT91C_PWMC_CPD;
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CUPDR = period;
  }
  return true;
}

/**
  Read the period of this Pwm channel

  @return The period of this channel.

  \b Example
  \code
  Pwm p(1); // channel 1
  int period = p.period( );
  \endcode
*/
int Pwm::period( )
{
  return _period;
}

// from at91lib softpack
int Pwm::findClockConfiguration(int frequency)
{
  unsigned int divisors[11] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
  unsigned char divisor = 0;
  unsigned int prescaler;

//    SANITY_CHECK(frequency < mck);

  // Find prescaler and divisor values
  prescaler = (MCK / divisors[divisor]) / frequency;
  while ((prescaler > 255) && (divisor < 11))
  {
    divisor++;
    prescaler = (MCK / divisors[divisor]) / frequency;
  }

  // Return result
  if (divisor < 11)
//      TRACE_DEBUG("Found divisor=%u and prescaler=%u for freq=%uHz\n\r",
//                divisors[divisor], prescaler, frequency);
    return prescaler | (divisor << 8);
  else
    return 0;
}



