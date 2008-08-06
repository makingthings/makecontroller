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

/** \file pwmout.c	
	PWM Out.
	Library of functions for the Make Application Board's PwmOut Subsystem.
*/

#include "pwmout.h"
#include "pwm.h"
#include "config.h"

#include "AT91SAM7X256.h"

#if ( APPBOARD_VERSION == 50 )
  #define PWMOUT_0_IO_A IO_PA02
  #define PWMOUT_0_IO_B IO_PA02
  #define PWMOUT_1_IO_A IO_PA02
  #define PWMOUT_1_IO_B IO_PA02
  #define PWMOUT_2_IO_A IO_PA02
  #define PWMOUT_2_IO_B IO_PA02
  #define PWMOUT_3_IO_A IO_PA02
  #define PWMOUT_3_IO_B IO_PA02
#endif
#if ( APPBOARD_VERSION == 90 )
  #define PWMOUT_0_IO_A IO_PB23
  #define PWMOUT_0_IO_B IO_PA26
  #define PWMOUT_1_IO_A IO_PA25
  #define PWMOUT_1_IO_B IO_PB25
  #define PWMOUT_2_IO_A IO_PA02
  #define PWMOUT_2_IO_B IO_PA06
  #define PWMOUT_3_IO_A IO_PA05
  #define PWMOUT_3_IO_B IO_PA24
#endif
#if ( APPBOARD_VERSION == 95 || APPBOARD_VERSION == 100 )
  #define PWMOUT_0_IO_A IO_PA24
  #define PWMOUT_0_IO_B IO_PA05
  #define PWMOUT_1_IO_A IO_PA06
  #define PWMOUT_1_IO_B IO_PA02
  #define PWMOUT_2_IO_A IO_PB25
  #define PWMOUT_2_IO_B IO_PA25
  #define PWMOUT_3_IO_A IO_PA26
  #define PWMOUT_3_IO_B IO_PB23
#endif

#define PWMOUT_COUNT 4

static int PwmOut_Start( int motor );
static int PwmOut_Stop( int motor );

static int PwmOut_users[ PWMOUT_COUNT ];  

static void PwmOut_GetIos( int index, int* ioA, int* ioB );

/** \defgroup PwmOut PWM Out
	The PWM Out subsystem underlies the DC Motor subsystem and controls the 4 PWM signals on the SAM7X.
	Each PWM device controls a pair of Digital Outs - an A and a B channel:
	- PwmOut 0 - Digital Outs 0 (A) and 1 (B).
	- PwmOut 1 - Digital Outs 2 (A) and 3 (B).
	- PwmOut 2 - Digital Outs 4 (A) and 5 (B).
	- PwmOut 3 - Digital Outs 6 (A) and 7 (B).
	
	The A and B channels of a PWM device can be set independently to be inverted, or not, from one another
	in order to control motors, lights, etc.
	
	The simplest way to get started is simply with a call to PwmOut_SetActive() and then to 
	PwmOut_SetDuty() - this will allow you to control simple dimming and motors.  If you need to adjust timing, 
	inversion or other parameters, delve a bit deeper into the API above.
	
	PwmOut relies on the internal \ref Pwm system.  See that page for more info on the timing issues 
	involved with the PWM Out system.
	\ingroup Libraries
	@{
*/

/**
	Sets whether the specified PWM device is active.
	@param index An integer specifying which PWM device (0-3).
	@param state An integer specifying the active state - 1 (active) or 0 (inactive).
	@return Zero on success.
*/
int PwmOut_SetActive( int index, int state )
{
  if ( index < 0 || index >= PWMOUT_COUNT )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( state )
    return PwmOut_Start( index );
  else
    return PwmOut_Stop( index );
}

/**
	Returns the active state of the LED.
	@param index An integer specifying which PWM device (0-3).
	@return Zero on success.
*/
int PwmOut_GetActive( int index )
{
  if ( index < 0 || index >= PWMOUT_COUNT )
    return false;
  return PwmOut_users[ index ] > 0;
}

/**	
	Set the speed of a PWM device.
	@param index An integer specifying which PWM device (0-3).
	@param duty An integer (0 - 1023) specifying the duty.
  @return Zero on success.
*/
int PwmOut_SetDuty( int index, int duty )
{
  if ( index < 0 || index > 3 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( PwmOut_users[ index ] < 1 )
  {
    int status = PwmOut_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  return Pwm_Set( index, duty );
}

/**	
	Read the current duty of a PWM device.
	@param index An integer specifying which PWM device (0-3).
  @return The duty (0 - 1023).
*/
int PwmOut_GetDuty( int index )
{
  if ( index < 0 || index > 3 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( PwmOut_users[ index ] < 1 )
  {
    int status = PwmOut_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  return Pwm_Get( index );
}

/**	
	Set whether the A channel associated with a PWM out should be inverted.
	@param index An integer specifying which PWM device (0-3).
  @param invert A character specifying the inversion - 1/non-zero (inverted) 0 (normal).
  @return Zero on success.
*/
int PwmOut_SetInvertA( int index, char invert )
{
  if ( index < 0 || index > 3 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( PwmOut_users[ index ] < 1 )
  {
    int status = PwmOut_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  // Look up the IO that are involved
  int ioA;
  int ioB;
  PwmOut_GetIos( index, &ioA, &ioB );
    
  Io_SetValue( ioA, !invert );

  return CONTROLLER_OK;
}

/**	
	Read whether the A channel of a PWM device is inverted.
	@param index An integer specifying which PWM device (0-3).
  @return The inversion - 1/non-zero (inverted) or 0 (normal).
*/
int PwmOut_GetInvertA( int index )
{
  if ( index < 0 || index > 3 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( PwmOut_users[ index ] < 1 )
  {
    int status = PwmOut_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  // Look up the IO that are involved
  int ioA;
  int ioB;
  PwmOut_GetIos( index, &ioA, &ioB );
    
  return Io_GetValue( ioA );
}

/**	
	Read whether the B channel of a PWM out is inverted.
	@param index An integer specifying which PWM device (0-3).
  @param invert A character specifying the inversion - 1/non-zero (inverted) 0 (normal).
  @return Zero on success.
*/
int PwmOut_SetInvertB( int index, char invert )
{
  if ( index < 0 || index > 3 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( PwmOut_users[ index ] < 1 )
  {
    int status = PwmOut_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  // Look up the IO that are involved
  int ioA;
  int ioB;
  PwmOut_GetIos( index, &ioA, &ioB );
    
  Io_SetValue( ioB, !invert );

  return CONTROLLER_OK;
}

/**	
	Read whether the B channel of a PWM device is inverted.
	@param index An integer specifying which PWM device (0-3).
  @return The inversion - 1/non-zero (inverted) or 0 (normal).
*/
int PwmOut_GetInvertB( int index )
{
  if ( index < 0 || index > 3 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( PwmOut_users[ index ] < 1 )
  {
    int status = PwmOut_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  // Look up the IO that are involved
  int ioA;
  int ioB;
  PwmOut_GetIos( index, &ioA, &ioB );
    
  return Io_GetValue( ioB );
}


/**	
	Set whether the 2 channels of a PWM device are inverted.
  This is a convenience function, and is equivalent to making separate calls to PwmOut_SetInvertA() and PwmOut_SetInvertB().
	@param index An integer specifying which PWM device (0-3).
  @param invertA A character specifying the inversion of the A channel - 1/non-zero (inverted) or 0 (normal).
  @param invertB A character specifying the inversion of the B channel - 1/non-zero (inverted) or 0 (normal).
  @return Zero on success.
*/
int PwmOut_SetInvert( int index, char invertA, char invertB )
{
  if ( index < 0 || index > 3 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( PwmOut_users[ index ] < 1 )
  {
    int status = PwmOut_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  // Look up the IO that are involved
  int ioA;
  int ioB;
  PwmOut_GetIos( index, &ioA, &ioB );
    
  Io_SetValue( ioA, !invertA );
  Io_SetValue( ioB, !invertB );

  return CONTROLLER_OK;
}

/**	
	Set the duty and the inversion of both channels of a PWM device.
	This is a convenience function to set all the properties of a PWM device in a single call.\n
	It is equivalent to separate calls to PwmOut_SetInvert() and PwmOut_SetDuty().
	@param index An integer specifying which PWM out.
	@param duty An integer specifying the duty (0 - 1023).
  @param invertA A character specifying the inversion of the A channel - 1/non-zero (inverted) or 0 (normal).
  @param invertB A character specifying the inversion of the B channel - 1/non-zero (inverted) or 0 (normal).
  @return Zero on success
*/
int PwmOut_SetAll( int index, int duty, char invertA, char invertB )
{
  if ( index < 0 || index > 3 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( PwmOut_users[ index ] < 1 )
  {
    int status = PwmOut_Start( index );
    if ( status != CONTROLLER_OK )
      return status;
  }

  // Look up the IO that are involved
  int ioA;
  int ioB;
  PwmOut_GetIos( index, &ioA, &ioB );
    
  Io_SetValue( ioA, !invertA );
  Io_SetValue( ioB, !invertB );

  return Pwm_Set( index, duty );;
}

int PwmOut_Start( int index )
{
  int status;

  if ( index < 0 || index > 3 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;

  if ( PwmOut_users[ index ]++ == 0 )
  {
    // Look up the IO's that will be used
    int ioA;
    int ioB;
    PwmOut_GetIos( index, &ioA, &ioB );
  
    // Try to get the IO's
    status = Io_Start( ioA, true );
    if ( status != CONTROLLER_OK )
    {
      PwmOut_users[ index ]--;
      return status;
    }
  
    status = Io_Start( ioB, true );
    if ( status != CONTROLLER_OK )
    {
      PwmOut_users[ index ]--;
      // better give back ioA since we did get that
      Io_Stop( ioA );
      return status;
    }
    
    // Make sure we can get the PWM channel
    status = Pwm_Start( index );
    if ( status != CONTROLLER_OK )
    {
      PwmOut_users[ index ]--;
      // better give back the io's since we did get them
      Io_Stop( ioA );
      Io_Stop( ioB );
      return status;
    }

    // Set all the io's right
  
    // Enable as PIOs
    Io_SetPio( ioA, true );
    Io_SetPio( ioB, true );
  
    // Switch them off
    Io_SetValue( ioA, false );
    Io_SetValue( ioB, false );
  
    // Turn them into outputs
    Io_SetDirection( ioA, IO_OUTPUT );
    Io_SetDirection( ioB, IO_OUTPUT );

    PwmOut_SetDuty( index, 0 );
    Io_SetValue( ioA, true );
  }

  return CONTROLLER_OK;
}

int PwmOut_Stop( int index )
{
  if ( index < 0 || index > 3 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;
  
  if ( PwmOut_users[ index ] <= 0 )
    return CONTROLLER_ERROR_NOT_LOCKED;

  if ( --PwmOut_users[ index ] == 0 )
  {
    // Look up the IO's that are involved
    int ioA;
    int ioB;
    PwmOut_GetIos( index, &ioA, &ioB );
  
    // Turn them off
    Io_SetValue( ioA, false );
    Io_SetValue( ioB, false );

    Io_Stop( ioA );
    Io_Stop( ioB );

    Pwm_Stop( index );
  }

  return CONTROLLER_OK;
}

void PwmOut_GetIos( int index, int* ioA, int* ioB )
{ 
  switch ( index )
  {
    case 0:
      *ioA = PWMOUT_0_IO_A;
      *ioB = PWMOUT_0_IO_B;
      break;
    case 1:
      *ioA = PWMOUT_1_IO_A;
      *ioB = PWMOUT_1_IO_B;
      break;
    case 2:
      *ioA = PWMOUT_2_IO_A;
      *ioB = PWMOUT_2_IO_B;
      break;
    case 3:
      *ioA = PWMOUT_3_IO_A;
      *ioB = PWMOUT_3_IO_B;
      break;
    default:
      *ioA = 0;
      *ioB = 0;
      break;
  }
}

/**
  Set the divider A value.
  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @param val The value (between 0 and 255)
  @return 0 on success.
*/
int PwmOut_SetDividerAValue(int val)
{
   if( val < 0 || val > 255 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;
   
   int v = Pwm_GetDividerA();
   
   //strip off the divider Value
   v &= 0x00000f00;
   
   //add on the new Value
   v |= val;

   return Pwm_SetDividerA( v );
}

/**
  Read the divider A value.
  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @return the divider A value (0 - 255)
  @see PwmOut_SetDividerAValue( )
*/
int PwmOut_GetDividerAValue()
{
  return ( Pwm_GetDividerA() & 0x000000ff );
}

/**
  Set the divider A mux value.
  @param mux The mux value (between 0 and 10)
  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @return 0 on success.
*/
int PwmOut_SetDividerAMux(int mux)
{
   if( mux < 0 || mux > 10 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;
   
   int v = Pwm_GetDividerA();
   
   //strip off the divider Value
   v &= 0x000000ff;
   
   //add on the new Value
   v |= ( mux << 8 );

   return Pwm_SetDividerA( v );
}
/**
  Read the divider A mux value.
  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @return the divider A mux value (0 - 10)
  @see PwmOut_SetDividerAMux( )
*/
int PwmOut_GetDividerAMux()
{
  return ( ( Pwm_GetDividerA() >> 8 ) & 0x0000000f );
}

/**
  Set the divider B value.
  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @param val The value (between 0 and 255)
  @return 0 on success.
*/
int PwmOut_SetDividerBValue(int val)
{
   if( val < 0 || val > 255 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;
   
   int v = Pwm_GetDividerB();
   
   //strip off the divider Value
   v &= 0x00000f00;
   
   //add on the new Value
   v |= val;

   return Pwm_SetDividerA( v );
}

/**
  Read the divider B value.
  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @return the divider B value (0 - 255)
  @see PwmOut_SetDividerBValue( )
*/
int PwmOut_GetDividerBValue()
{
  return ( Pwm_GetDividerB() & 0x000000ff );
}

/**
  Set the divider B mux value.
  @param mux The mux value (between 0 and 10)
  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @return 0 on success.
*/
int PwmOut_SetDividerBMux(int mux)
{
   if( mux < 0 || mux > 10 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;
   
   int v = Pwm_GetDividerB();
   
   //strip off the divider Value
   v &= 0x000000ff;
   
   //add on the new Value
   v |= ( mux << 8 );

   return Pwm_SetDividerB( v );
}

/**
  Read the divider B mux value.
  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @return the divider B mux value (0 - 10)
  @see PwmOut_SetDividerBMux( )
*/
int PwmOut_GetDividerBMux()
{
  return ( ( Pwm_GetDividerB() >> 8 ) & 0x0000000f );
}

/**
  Set the alignment of a channel's waveform.
  Valid values are:
  - 0 = Left Aligned (default)
  - 1 = Center Aligned

  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @param index The PWM channel (0-3) that you'd like to configure.
  @param val The alignment value, as described above.
  @return 0 on success.
*/
int PwmOut_SetWaveformAlignment(int index, int val)
{
  if ( index < 0 || index > 3 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;
  if ( val > 1 || val < 0 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  int currval = Pwm_GetWaveformProperties( index ) & 0x00000006; //Leave bits 2 & 3 alone
  currval |= val;
  return Pwm_SetWaveformProperties( index, currval );
}

/**
  Read the configured waveform alignment for a given PWM channel.

  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @param index The PWM channel (0-3) you'd like to read from.
  @return the waveform alignment - see PwmOut_SetWaveformAlignment( )
*/
int PwmOut_GetWaveformAlignment( int index )
{
  int val = Pwm_GetWaveformProperties( index );
  if ( val < 0 )
    return val;
  
  return val  & 0x00000001;
}

/**
  Set the Waveform Polarity for a PWM channel.
  Valid values are:
   - 0 = Start Waveform Low
   - 1 = Start Waveform High (default)

  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @param index The PWM channel (0-3) that you'd like to configure.
  @param val The value, as described above.
  @return 0 on success.
*/
int PwmOut_SetWaveformPolarity(int index, int val)
{
  if ( index < 0 || index > 3 )
    return CONTROLLER_ERROR_ILLEGAL_INDEX;
  if ( val > 1 || val < 0 )
    return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

  int currval = Pwm_GetWaveformProperties( index ) & 0x00000005; //Leave bits 3 & 1 alone
  currval |= (val << 1);
  return Pwm_SetWaveformProperties( index, currval );
}

/**
  Read the waveform polarity for a given channel.

  Contributed by TheStigg - http://www.makingthings.com/author/thestigg
  @param index The PWM channel (0-3) that you'd like to read from.
  @return The configured polarity for that channel.
  @see PwmOut_SetWaveformPolarity( )
*/
int PwmOut_GetWaveformPolarity( int index )
{
  int val = Pwm_GetWaveformProperties( index );
  if ( val < 0 )
    return val;
  
  return val  & 0x00000002;
}

/** @}
*/

#ifdef OSC

/** \defgroup pwmout_osc PWM Out - OSC
  Generate PWM signals with the Application Board via OSC.
  \ingroup OSC
	
	\section devices Devices
	There are 4 PWM controllers available on the Application Board, numbered 0 - 3.
	
	Each PWM Out controls a pair of Digital Outs - an A and a B channel:
	- PwmOut 0 - Digital Outs 0 (A) and 1 (B).
	- PwmOut 1 - Digital Outs 2 (A) and 3 (B).
	- PwmOut 2 - Digital Outs 4 (A) and 5 (B).
	- PwmOut 3 - Digital Outs 6 (A) and 7 (B).
	
	Each channel can also be set to invert the given PWM signal.
	
	\section properties Properties
	Each PWM Out has the following properties:
  - duty
  - invA
  - invB
  - active
  - dividerAValue
  - dividerAMux
  - dividerBValue
  - dividerBMux
  - clockSource
  - alignment
  - polarity

	\subsection Duty
	The \b duty property corresponds to the duty at which a load connected to the output is being driven.
	This value can be both read and written.  The range of values expected by the board
	is from 0 - 1023.

  To generate a 75% on PWM signal, send a message like
	\verbatim /pwmout/1/duty 768 \endverbatim
	Leave the argument value off to read the duty:
	\verbatim /pwmout/1/duty \endverbatim
	
	\subsection invA
	The \b invA property corresponds to the inversion of the A channel of a PWM Out.
	This value can be both read and written, and the range of values expected is simply 
	0 or 1.  1 means inverted and 0 means normal.  0 is the default.
	
	To set the A channel of the second PWM Out as inverted, send the message
	\verbatim /pwmout/1/invA 1 \endverbatim
	Note that the A channel of PWM Out 1 is Digital Out 2.
	
	\subsection invB
	The \b invB property corresponds to the inversion of the B channel of a PWM Out.
	This value can be both read and written, and the range of values expected is simply 
	0 or 1.  1 means inverted and 0 means normal.  0 is the default.
	
	To set the B channel of the fourth PWM Out back to normal, send the message
	\verbatim /pwmout/1/invB 0 \endverbatim
	Note that the A channel of PWM Out 1 is Digital Out 7.
	
	\subsection Active
	The \b active property corresponds to the active state of the PWM Out.
	If the device is set to be active, no other tasks will be able to
	use its 2 digital out lines.  If you're not seeing appropriate
	responses to your messages to the PWM Out, check the whether it's 
	locked by sending a message like
	\verbatim /pwmout/0/active \endverbatim
	
	If you're no longer using the PWM Out, it's a good idea to free the 2 Digital Outs by 
	sending the message
	\verbatim /pwmout/0/active 0 \endverbatim

	\section p_adjust Period adjustment of the PWM unit

	The below values allow you to adjust the period length to all possible values as supported
	by the physical limits of the Make Controller.  Before altering the values, it is good to understand
	a little about the subsustem of the PWM module.  Please see the general introduction in the \ref Pwm section.

	\subsection DividerBValue
  The \b dividerXValue property corresponds to the linear divider value within clock divider module X of the 
  PWM Out.  This value can be between 0 and 255, and linearly divides the clock that is fed into it.  You can 
  change the incomming clock value by altering the DividerAMux parameter.  Higher linear divider values mean 
  a longer period.
  
  Default value of DividerA is 4. Default value of DividerB is 0.

	\subsection divs DividerAMux & DividerBMux
  The \b dividerAMux and \b dividerBMux properties select the incoming clock value for divider module A or B (respectively)
  of the PWM Out.  This value ranges between 0 and 10.  This is similar to the clock source value, eg. a DividerAMux 
  value of 5 = a clock rate of MasterClock/(2^5)
  
  Increasing this value will substantially increase the period of the PWM.  Use the DividerXValue parameter 
  to more finely tune the value of the period.  
  
  Default value of MuxA is 4.  Default value of MuxB is 0.		    

	\subsection ClockSource
  The \b clockSource property selects the incoming clock value for the specified PWM channel. A clock 
  source of \b 0-10 represents the Master clock divided by 2 to the power of the Clock Source Value, eg. a clock 
  source value of 5 = a clock rate of MasterClock/(2^5).  A value of \b 11 sets the Clock source to be 
  generated by clock Divider A.  A value of \b 12 sets the Clock source to be generated by clock Divider B
  
  The default value is 11 (Divider A).

	\subsection WaveformAlignment
  The \b waveformAlignment property chooses whether the channel is left aligned or center aligned.
  The following values are valid:
   - 0 = Left aligned
   - 1 = Center aligned
  
  A left aligned channel counts up to the maximum duty cycle, then resets the counter.  A center aligned 
  channel counts up to the maximum duty cycle, then counts down to zero, etc. The default is left aligned.

	\subsection WaveformPolarity
  The \b polarity property chooses whether the channel begins its period high or low.  The following values are valid:
   - 0 = Normal Polarity
   - 1 = Inverted Polarity
  
  Normal polarity starts low, then goes high.  Inverted polarity starts high, then goes low.  The default is inverted polarity.
*/

#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* PwmOutOsc_Name = "pwmout";
static char* PwmOutOsc_PropertyNames[] = { "active", "duty", "invA", "invB", 
					   "dividerAValue", "dividerAMux",
					   "dividerBValue", "dividerBMux",
					   "clockSource", "alignment",
					   "polarity",
					   0 }; // must have a trailing 0

int PwmOutOsc_PropertySet( int index, int property, int value );
int PwmOutOsc_PropertyGet( int index, int property );

// Returns the name of the subsystem
const char* PwmOutOsc_GetName( )
{
  return PwmOutOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int PwmOutOsc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
                                     PWMOUT_COUNT, PwmOutOsc_Name,
                                     PwmOutOsc_PropertySet, PwmOutOsc_PropertyGet, 
                                     PwmOutOsc_PropertyNames );
                                     
  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, PwmOutOsc_Name, status );
  return CONTROLLER_OK;
}

// Set the index LED, property with the value
int PwmOutOsc_PropertySet( int index, int property, int value )
{
  switch ( property )
  {
    case 0: 
      PwmOut_SetActive( index, value );
      break;      
    case 1:
      PwmOut_SetDuty( index, value );
      break;
    case 2:
      PwmOut_SetInvertA( index, value );
      break;
    case 3:
      PwmOut_SetInvertB( index, value );
      break;
    case 4:
      PwmOut_SetDividerAValue(value);
      break;
    case 5:
      PwmOut_SetDividerAMux(value);
      break;
    case 6:
      PwmOut_SetDividerBValue(value);
      break;
    case 7:
      PwmOut_SetDividerBMux(value);
      break;
    case 8:
      Pwm_SetClockSource( index, value );
      break;
    case 9:
      PwmOut_SetWaveformAlignment( index, value );
      break;
    case 10:
      PwmOut_SetWaveformPolarity( index, value );
      break;
  }
  return CONTROLLER_OK;
}

// Get the index LED, property
int PwmOutOsc_PropertyGet( int index, int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      value = PwmOut_GetActive( index );
      break;
    case 1:
      value = PwmOut_GetDuty( index );
      break;
    case 2:
      value = PwmOut_GetInvertA( index );
      break;
    case 3:
      value = PwmOut_GetInvertB( index );
      break;
    case 4:
      value = PwmOut_GetDividerAValue();
      break;
    case 5:
      value = PwmOut_GetDividerAMux();
      break;
    case 6:
      value = PwmOut_GetDividerBValue();
      break;
    case 7:
      value = PwmOut_GetDividerBMux();
      break;
    case 8:
      value = Pwm_GetClockSource( index );
      break;
    case 9:
      value = PwmOut_GetWaveformAlignment( index );
      break;
    case 10:
      value = PwmOut_GetWaveformPolarity( index );
      break;
  }
  return value;
}


#endif
