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

#include "pwmout.h"
#include "pwm.h"
#include "core.h"

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
#if ( APPBOARD_VERSION >= 95 )
  #define PWMOUT_0_IO_A IO_PA24
  #define PWMOUT_0_IO_B IO_PA05
  #define PWMOUT_1_IO_A IO_PA06
  #define PWMOUT_1_IO_B IO_PA02
  #define PWMOUT_2_IO_A IO_PB25
  #define PWMOUT_2_IO_B IO_PA25
  #define PWMOUT_3_IO_A IO_PA26
  #define PWMOUT_3_IO_B IO_PB23
#endif

// static 
PwmOut::PwmOutInternal* PwmOut::pwmouts[] = {0, 0, 0, 0};

/**
  Create a new PwmOut object
  
  @param index Which channel to control - valid options are 0, 1, 2, 3
  
  \b Example
  \code
  PwmOut pwmout;
  // or allocate one...
  PwmOut* p = new PwmOut();
  \endcode
*/
PwmOut::PwmOut( int index )
{
  if ( index < 0 || index > 3 )
    return;
  _index = index;
  
  if( (pwmouts[_index]) == 0 )
  {
    pwmouts[_index] = new PwmOutInternal();
    PwmOutInternal* internal = pwmouts[_index];
    internal->count = 1;
    // Look up the IO's that will be used
    int ioAIdx;
    int ioBIdx;
    getIos( &ioAIdx, &ioBIdx );
    
    internal->ioA = new Io(ioAIdx);
    internal->ioB = new Io(ioBIdx);
    internal->ioA->on();
    internal->ioB->off();
    internal->pwm = new Pwm(_index);
  
    setDuty( 0 );
  }
  else
    pwmouts[_index]->count++;
}

/*
  If this was the last instance of this PwmOut, kill it
  and clean up.
*/
PwmOut::~PwmOut( )
{
  if( --(pwmouts[_index]->count) <= 0 )
  {
    setDuty(0);
    PwmOutInternal* internal = pwmouts[_index];
    internal->ioA->off();
    internal->ioB->off();
    delete internal->ioA;
    delete internal->ioB;
    delete internal->pwm;
    delete internal;
    pwmouts[_index] = 0;
  }
}

/** 
  Set the speed of a PwmOut.
  @param duty An integer (0 - 1023) specifying the duty.
  @return True on success, false on failure.
  
  \b Example
  \code
  PwmOut p(1);
  p.setDuty(512);
  \endcode
*/
bool PwmOut::setDuty( int duty )
{
  pwmouts[_index]->pwm->setDuty(duty);
  return true;
}

/** 
  Read the current duty of a PwmOut.
  @return The duty (0 - 1023).
  
  \b Example
  \code
  PwmOut p(1);
  int duty = p.duty();
  \endcode
*/
int PwmOut::duty( )
{
  return pwmouts[_index]->pwm->duty();
}

/** 
  Set whether the A channel associated with a PwmOut should be inverted.
  @param invert A character specifying the inversion - 1/non-zero (inverted) 0 (normal).
  @return True on success, false on failure;
  
  \b Example
  \code
  PwmOut p(1);
  p.setInvertedA(false); // channel A not inverted
  \endcode
*/
bool PwmOut::setInvertedA( bool invert )
{
  pwmouts[_index]->ioA->setValue( !invert );
  return true;
}

/** 
  Read whether the A channel of a PwmOut is inverted.
  @return True if inverted, false if not.
  
  \b Example
  \code
  PwmOut p(1);
  bool is_A_inverted = p.invertedA();
  \endcode
*/
bool PwmOut::invertedA( )
{
  return pwmouts[_index]->ioA->value( );
}

/** 
  Read whether the B channel of a PwmOut is inverted.
  @param invert A character specifying the inversion - 1/non-zero (inverted) 0 (normal).
  @return True on success, false on failure.
  
  \b Example
  \code
  PwmOut p(1);
  p.setInvertedB(false); // channel B not inverted
  \endcode
*/
bool PwmOut::setInvertedB( bool invert )
{
  pwmouts[_index]->ioB->setValue( !invert );
  return true;
}

/** 
  Read whether the B channel of a PwmOut is inverted.
  @return True if inverted, false if not.
  
  \b Example
  \code
  PwmOut p(1);
  bool is_B_inverted = p.invertedB();
  \endcode
*/
bool PwmOut::invertedB( )
{
  return pwmouts[_index]->ioB->value( );
}

/** 
  Set all the parameters of a PwmOut at once.
  @param duty The duty (0-1023)
  @param invertA Whether channel A should be inverted
  @param invertB Whether channel B should be inverted
  @return True on success, otherwise false.
  
  \b Example
  \code
  PwmOut p(1);
  p.setAll(1023, false, true);
  \endcode
*/
bool PwmOut::setAll( int duty, bool invertA, bool invertB )
{
  PwmOutInternal* internal = pwmouts[_index];
  internal->pwm->setDuty(duty);
  internal->ioA->setValue( !invertA );
  internal->ioB->setValue( !invertB );
  return true;
}

void PwmOut::getIos( int* ioA, int* ioB )
{ 
  switch( _index )
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

//#include "osc.h"
//#include "string.h"
//#include "stdio.h"
//
//// Need a list of property names
//// MUST end in zero
//static char* PwmOutOsc_Name = "pwmout";
//static char* PwmOutOsc_PropertyNames[] = { "active", "duty", "invA", "invB", 
//             "dividerAValue", "dividerAMux",
//             "dividerBValue", "dividerBMux",
//             "clockSource", "alignment",
//             "polarity",
//             0 }; // must have a trailing 0
//
//int PwmOutOsc_PropertySet( int index, int property, int value );
//int PwmOutOsc_PropertyGet( int index, int property );
//
//// Returns the name of the subsystem
//const char* PwmOutOsc_GetName( )
//{
//  return PwmOutOsc_Name;
//}
//
//// Now getting a message.  This is actually a part message, with the first
//// part (the subsystem) already parsed off.
//int PwmOutOsc_ReceiveMessage( int channel, char* message, int length )
//{
//  int status = Osc_IndexIntReceiverHelper( channel, message, length, 
//                                     PWMOUT_COUNT, PwmOutOsc_Name,
//                                     PwmOutOsc_PropertySet, PwmOutOsc_PropertyGet, 
//                                     PwmOutOsc_PropertyNames );
//                                     
//  if ( status != CONTROLLER_OK )
//    return Osc_SendError( channel, PwmOutOsc_Name, status );
//  return CONTROLLER_OK;
//}
//
//// Set the index LED, property with the value
//int PwmOutOsc_PropertySet( int index, int property, int value )
//{
//  switch ( property )
//  {
//    case 0: 
//      PwmOut_SetActive( index, value );
//      break;      
//    case 1:
//      PwmOut_SetDuty( index, value );
//      break;
//    case 2:
//      PwmOut_SetInvertA( index, value );
//      break;
//    case 3:
//      PwmOut_SetInvertB( index, value );
//      break;
//    case 4:
//      PwmOut_SetDividerAValue(value);
//      break;
//    case 5:
//      PwmOut_SetDividerAMux(value);
//      break;
//    case 6:
//      PwmOut_SetDividerBValue(value);
//      break;
//    case 7:
//      PwmOut_SetDividerBMux(value);
//      break;
//    case 8:
//      Pwm_SetClockSource( index, value );
//      break;
//    case 9:
//      PwmOut_SetWaveformAlignment( index, value );
//      break;
//    case 10:
//      PwmOut_SetWaveformPolarity( index, value );
//      break;
//  }
//  return CONTROLLER_OK;
//}
//
//// Get the index LED, property
//int PwmOutOsc_PropertyGet( int index, int property )
//{
//  int value = 0;
//  switch ( property )
//  {
//    case 0:
//      value = PwmOut_GetActive( index );
//      break;
//    case 1:
//      value = PwmOut_GetDuty( index );
//      break;
//    case 2:
//      value = PwmOut_GetInvertA( index );
//      break;
//    case 3:
//      value = PwmOut_GetInvertB( index );
//      break;
//    case 4:
//      value = PwmOut_GetDividerAValue();
//      break;
//    case 5:
//      value = PwmOut_GetDividerAMux();
//      break;
//    case 6:
//      value = PwmOut_GetDividerBValue();
//      break;
//    case 7:
//      value = PwmOut_GetDividerBMux();
//      break;
//    case 8:
//      value = Pwm_GetClockSource( index );
//      break;
//    case 9:
//      value = PwmOut_GetWaveformAlignment( index );
//      break;
//    case 10:
//      value = PwmOut_GetWaveformPolarity( index );
//      break;
//  }
//  return value;
//}


#endif
