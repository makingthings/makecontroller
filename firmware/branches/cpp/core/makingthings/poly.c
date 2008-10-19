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

/** \file poly.c	
	Poly Functions.
	Functions for permitting the Make Controller Board perform simple functions without
  programming or any computer intervention at all.
*/

#include "stdlib.h"
#include "config.h"
#include "poly.h" 
#include "eeprom.h"

void Poly_LineInit( void );

void PolyTask( void* p );

//
// Top level Poly routines
// 
void  Poly_DeInit( int function, void* voidStruct );
void* Poly_Init( int function, int bank );
void  Poly_Run( int function, void* data, int ms, int readConstants );

typedef struct
{
  int mode0;
  int mode1;
  int running;
  int readContants;
} PolyStruct;

PolyStruct Poly;

//
// 1 - Timer stuff
//
void* Timer_Init( int bank );
void Timer_Run( void* timerVP, int ms, int readConstants );
void Timer_DeInit( void* voidStruct );

typedef struct
{
  int bank; // 0 or 1
  int baseIn;
  int baseOut;
  int baseAux;
 
  int timerSoFar;
	int timerDuration;
	int timerDurationHalf;
	int timerDurationQuarter;
	int lastPotValue;
	
	int lastIn0;
	int lastIn1;
	
	int timerState;
	int timerStateHalf;
	int timerStateQuarter;

  int readConstants;

  int sustainMode;
  int sustainState;
} TimerStruct;

void Timer_SaveConstants( TimerStruct *tp );

//
// 2 - Follower stuff
//
void* Follower_Init( int bank );
void Follower_Run( void* followerVP, int ms, int readConstants );
void Follower_DeInit( void* follower );

typedef struct
{
  int bank;
  int baseIn;
  int baseOut;
  int baseAux;

  float output;
  float rate;
  int lastRateValue;
  int input;

  int peak;
  int readConstants;

} FollowerStruct;

void Follower_SaveConstants( FollowerStruct *fp );


//
// 3 - Oscillator stuff
//

void* Oscillator_Init( int bank );
void Oscillator_Run( void* OscillatorP, int ms, int readConstants );
void Oscillator_DeInit( void* Oscillator );

typedef struct
{
  int bank;
  int baseIn;
  int baseOut;
  int baseAux;

  int state;
  float current;
  float periodOn;
  float periodOff;

  int periodOnRaw;
  int periodOffRaw;

  int readConstants;

} OscillatorStruct;

void Oscillator_SaveConstants( OscillatorStruct *fp );

/*
  Poly Function - pre-cooked behaviors for experimentation.

  4 ins & 4 outs for each function
  2 banks: 3 DIP switch spots select which function
  Each function has an integer index (0-7):
    0 - Timer
    1 - Follower
    etc.
*/

/* \defgroup Poly Poly
	The Poly subsystem controls onboard pre-baked functions.
* \ingroup Controller
* @{
*/

/**
	Sets whether the Poly subsystem is active.
	@param value An integer specifying the active state of the subsystem - 1 (on) or 0 (off).
	@return CONTROLLER_OK (0) on success.
*/
int Poly_SetActive( int value )
{
  if ( value ) 
  {  
    if ( !Poly.running )
    {
      Poly.running = true;
      // Poly.readConstants = false;
      Poly.mode0 = -1;
      Poly.mode1 = -1;
      TaskCreate( PolyTask, "Poly", 1600, NULL, 2 );
    }
  }
  else
    Poly.running = false;

  return CONTROLLER_OK;
}

int Poly_GetActive( )
{
  return Poly.running;
}

#define POLY_SLEEP 20

void PolyTask( void* p )
{
  (void)p;
  int newDipSwitch;
  int oldDipSwitch = -1;
  void* data0 = NULL;
  void* data1 = NULL;
  int readConstants = false;

  // Make sure everything is off
  Poly_LineInit();

  while( Poly.running )
  {
    newDipSwitch = DipSwitch_GetValue( );
    if( newDipSwitch != oldDipSwitch )
    {
      int newMode0;
      int newMode1;
      
      if ( !( newDipSwitch & 0x80 ) )
      {
        newMode0 = 0;
        newMode1 = 0;
      }
      else
      {
        newMode0 = (newDipSwitch & 0x0E) >> 1;
        newMode1 = (newDipSwitch & 0x70) >> 4;
      }

      readConstants = ( newDipSwitch & 0x01 ) ? 1 : 0;

      if( newMode0 != Poly.mode0 )
      {
        Poly_DeInit( Poly.mode0, data0 );
        data0 = Poly_Init( newMode0, 0 );
        Poly.mode0 = newMode0;
      }

      if( newMode1 != Poly.mode1 )
      {
        Poly_DeInit( Poly.mode1, data1 );
        data1 = Poly_Init( newMode1, 1 );
        Poly.mode1 = newMode1;
      }

      oldDipSwitch = newDipSwitch;
    }

    Poly_Run( Poly.mode0, data0, POLY_SLEEP, readConstants );
    Poly_Run( Poly.mode1, data1, POLY_SLEEP, readConstants );

    if ( Poly.mode0 == 0 && Poly.mode1 == 0 )
      Sleep( POLY_SLEEP * 20 );
    else
      Sleep( POLY_SLEEP );
  }

  TaskDelete( 0 );
}

void* Poly_Init( int function, int bank )
{
  switch( function )
  {
    case 1: // Timer
      return Timer_Init( bank );
      break;
    case 2: // Follower
      return Follower_Init( bank );
      break;
    case 3: // Oscillator
      return Oscillator_Init( bank );
      break;
    default:
      break;
  }

  return 0;
}

void Poly_DeInit( int function, void* data )
{
  switch( function )
  {
    case 1: // Timer
      Timer_DeInit( data );
      break;
    case 2: // Follower
      Follower_DeInit( data );
      break;
    case 3: // Oscillator
      Oscillator_DeInit( data );
      break;
    default:
      break;
  }
}

void Poly_Run( int function, void* data, int ms, int readConstants )
{
  switch( function )
  {
    case 1: // Timer
      Timer_Run( data, ms, readConstants );
      break;
    case 2: // Follower
      Follower_Run( data, ms, readConstants );
      break;
    case 3: // Oscillator
      Oscillator_Run( data, ms, readConstants );
      break;
  }
}

//
// 1 - Timer Routines
//

// TimerStruct Timer[2];  // temporary...debug malloc.

void* Timer_Init( int bank )
{
  TimerStruct* timer;   //create a pointer to a new TimerStruct
  timer = (TimerStruct*)Malloc( sizeof( TimerStruct ) );
  //timer = &Timer[bank];
  
  timer->bank = bank;
  timer->baseIn = bank * 4;
  timer->baseOut = ( 1 - bank ) * 4;
  timer->baseAux = bank * 2;

  Eeprom_Read( EEPROM_POLY_0_TIMER_DURATION + ( bank * 8 ), (unsigned char*)&timer->timerDuration, 4 );
  Eeprom_Read( EEPROM_POLY_0_TIMER_SUSTAIN + ( bank * 8 ), (unsigned char*)&timer->sustainMode, 4 );
  if ( timer->sustainMode )
    timer->sustainMode = 1;
  else
    timer->sustainMode = 0;

  timer->timerSoFar = 0;
  timer->timerDurationHalf = 0;
	timer->timerDurationQuarter = 0;
	timer->lastPotValue = 0;
	
	timer->lastIn0 = 0;
	timer->lastIn1 = 0;
	
	timer->timerState = 1;
	timer->timerStateHalf = 1;
	timer->timerStateQuarter = 1;
	
	// make sure the outputs are initialized properly
  DigitalOut_SetValue( timer->baseOut, 0 );
	int i;
	for( i = 1; i < 4; i++ )
		DigitalOut_SetValue( timer->baseOut + i, 1 );
  AppLed_SetState( timer->baseAux, 0 );
  AppLed_SetState( timer->baseAux + 1, 1 );
  Servo_SetPosition( timer->baseAux, 0 );
  Servo_SetPosition( timer->baseAux + 1, 0 );

  return (void*)timer;
}

void Timer_Run( void* timerVP, int ms, int readConstants )
{
  TimerStruct* tp = (TimerStruct*)timerVP;
  // Deal with current pulse
  if ( !tp->timerState  )	// if the timer is not expired
  { 
    tp->timerSoFar += ms;

    if ( tp->timerSoFar > tp->timerDuration )
      tp->timerSoFar = tp->timerDuration ;

    // Calculate servo positions
    int position = 2048 * tp->timerSoFar / tp->timerDuration;
    Servo_SetPosition( tp->baseAux, ( position >> 2 ) );
    int position1 = ( position < 1023) ? position : 2048 - position;
    Servo_SetPosition( tp->baseAux + 1, position1 );
    
    if ( !tp->timerStateHalf && tp->timerSoFar >= tp->timerDurationHalf )
    {
      DigitalOut_SetValue( tp->baseOut + 3, 1 );	// set the "timer half-expired" output
      AppLed_SetState( tp->baseAux + 1, 1 ); 
      tp->timerStateHalf = 1;    // and set the flag accordingly
    }
    
    if ( !tp->timerStateQuarter && tp->timerSoFar >= tp->timerDurationQuarter )
    {
      DigitalOut_SetValue( tp->baseOut + 2, 1 );	// set the "timer quarter-expired" output
      tp->timerStateQuarter = 1;
    }
    
    if ( tp->timerSoFar >= tp->timerDuration  )
    {
      DigitalOut_SetValue( tp->baseOut, 0 );	// turn off the "time remaining" output
      DigitalOut_SetValue( tp->baseOut + 1, 1 );	// turn on the "time expired" output
      AppLed_SetState( tp->baseAux, 0 ); 
      tp->timerState = 1;      
    }
  }
  
  // if we're authorized to read constants from inputs
  if ( readConstants )
  {
    // Deal with changing time 
    int newPotValue = AnalogIn_GetValue( tp->baseIn + 3 );    // assuming the pot jumper is connected...
    if ( newPotValue != tp->lastPotValue )  // if the value has changed
    {
      tp->lastPotValue = newPotValue;	// save it for next time
      
      // need to adjust scale - make sure we're not dividing by zero
      // potvalue squared seems right, but then divided by something that will
      // give good control.  If the poly system works on 5ms intervals, squared
      // and divided by 1000, gives 2.5s at half, 10s at full on an exponential
      // scale.
      tp->timerDuration = newPotValue * newPotValue / 50;
      if( tp->timerDuration == 0 )
        tp->timerDuration = 1;
      
      tp->timerDurationHalf = tp->timerDuration / 2;
      if( tp->timerDurationHalf == 0 )
        tp->timerDurationHalf = 1;
      
      tp->timerDurationQuarter = tp->timerDuration - tp->timerDuration / 4;
      if( tp->timerDurationQuarter == 0 )
        tp->timerDurationQuarter = 1;
      
      if ( tp->timerState && tp->timerSoFar > tp->timerDuration )
        tp->timerSoFar = tp->timerDuration - 1;
    }

    // Read sustain mode
    tp->sustainMode = DigitalIn_GetValue( tp->baseIn + 2 );
  }
  else
  {
    // If the switch is off, save the constants
    if ( tp->readConstants )
      Timer_SaveConstants( tp );
  }
  tp->readConstants = readConstants;
  
  // Check for new triggers
  int trigger = 0;
  int newIn0 = DigitalIn_GetValue( tp->baseIn ); 
  if ( newIn0 != tp->lastIn0 || tp->sustainMode )
  {
    if ( newIn0 == 1 )    // if the main trigger input is high
      trigger = 1;
    tp->lastIn0 = newIn0;
  }
  
  int newIn1 = DigitalIn_GetValue( tp->baseIn + 1 );
  if ( newIn1 != tp->lastIn1 || tp->sustainMode )
  {
    if ( newIn1 == 0 )  // if the "inverted trigger" drops to 0
      trigger = 1;
    tp->lastIn1 = newIn1;
  }

  if ( trigger )
  {
    // set all the outputs to off except the "time expired" output
    DigitalOut_SetValue( tp->baseOut, 1 );
    AppLed_SetState( tp->baseAux, 1 );
    AppLed_SetState( tp->baseAux + 1, 0 );
    int i;
    for( i = (tp->baseOut + 1); i < (tp->baseOut + 4); i++ )
      DigitalOut_SetValue( i, 0 );
    tp->timerState = 0;
    tp->timerStateHalf = 0;
    tp->timerStateQuarter = 0;
    tp->timerSoFar = 0;		// reset the timer count
  }
}

// Set all the subsystems we've used to inactive
// so the IO lines can be used by the next function
void Timer_DeInit( void* voidStruct )
{
  TimerStruct* tp = (TimerStruct*)voidStruct;
  if ( tp != NULL )
  {
    int i;
    for( i = tp->baseIn; i < tp->baseIn + 4; i++ )
    {
      DigitalIn_SetActive( i, 0 );
      AnalogIn_SetActive( i, 0 );
    }
    for( i = tp->baseOut; i < tp->baseOut + 4; i++ )
    {
      DigitalOut_SetValue( i, 0 );
      DigitalOut_SetActive( i, 0 );
    }
    AppLed_SetState( tp->baseAux, 0 );
    AppLed_SetState( tp->baseAux + 1, 0 );
    AppLed_SetActive( tp->baseAux, 0 );
    AppLed_SetActive( tp->baseAux + 1, 0 );

    Servo_SetActive( tp->baseAux, 0 );
    Servo_SetActive( tp->baseAux + 1, 0 );

    if ( tp->readConstants )
      Timer_SaveConstants( tp );

    Free( tp );
  }
  // eventually free() the TimerStruct instances once malloc is working...?
}

void Timer_SaveConstants( TimerStruct *tp )
{
  Eeprom_Write( EEPROM_POLY_0_TIMER_DURATION + ( tp->bank * 8 ), (unsigned char *)&tp->timerDuration, 4 );
  Eeprom_Write( EEPROM_POLY_0_TIMER_SUSTAIN + ( tp->bank * 8 ), (unsigned char *)&tp->sustainMode, 4 );
}

//
// 2 - Follower Routines
//

//FollowerStruct Follower[2]; // eventually put these on the heap...

void* Follower_Init( int bank )
{
  int i;

  //FollowerStruct* follower = &Follower[bank];
  FollowerStruct* follower = (FollowerStruct*)Malloc( sizeof( FollowerStruct ) );

  follower->bank = bank;

  follower->baseIn = bank * 4;
  follower->baseOut = ( 1 - bank ) * 2;
  follower->baseAux = bank * 2;

  follower->output = 0;
  follower->rate = 0;
  follower->lastRateValue = -2048;
  follower->input = 0;

  Eeprom_Read( EEPROM_POLY_0_FOLLOWER_RATE + ( bank * 8 ), (unsigned char*)&follower->rate, 4 );
  Eeprom_Read( EEPROM_POLY_0_FOLLOWER_PEAK + ( bank * 8 ), (unsigned char*)&follower->peak, 4 );

  if ( follower->peak )
    follower->peak = 1;
  else
    follower->peak = 0;
  
  // set up the PWM outs as inverted pairs, so we can use them to drive motors, etc.
  for( i = follower->baseOut; i < (follower->baseOut + 2); i++) //2 PWMs for each bank of 4 outputs
  {
    PwmOut_SetInvertA( i, 0 );
    PwmOut_SetInvertB( i, 1 );
    PwmOut_SetDuty( i, 0 ); //initialize the outputs to 0
  }

  AppLed_SetState( follower->baseAux, 0 );
  AppLed_SetState( follower->baseAux + 1, 0 );
  Servo_SetPosition( i, 0 );
  Servo_SetPosition( i + 1, 512 );

  return (void*)follower;
}

void Follower_Run( void* followerVP, int ms, int readConstants )
{
  FollowerStruct* fp = (FollowerStruct*)followerVP;
  
  if ( readConstants )
  {
    // For pair zero, read the rate
    int newRateValue = AnalogIn_GetValue( fp->baseIn + 3 );
    if ( newRateValue != fp->lastRateValue )
    {
      fp->lastRateValue = newRateValue;
      if ( newRateValue == 0 )
        newRateValue = 1;
      if ( newRateValue > 1020 )
        fp->rate = 0;
      else
        fp->rate = ( 10 * 1024.0 ) / (newRateValue * newRateValue );
    }    
    fp->peak = DigitalIn_GetValue( fp->baseIn + 2 );
  }
  else
  {
    if ( fp->readConstants )
      Follower_SaveConstants( fp );
  }
  fp->readConstants = readConstants;

  int set = DigitalIn_GetValue( fp->baseIn + 1 );

  // Now read the position
	int input = AnalogIn_GetValue( fp->baseIn );

  int diff = input - fp->output;
  float change = fp->rate * (float)ms;
				
  if( diff != 0 ) // if the input has changed
  {
    if( diff > 0 )  // if the input is higher than last time
    {
      fp->output += change; // increment the output by the rate
      if( fp->output > input || fp->peak || set )
        fp->output = input;	// just set it to the input if it would otherwise be too high
      if ( diff > 1  )
      {
        AppLed_SetState( fp->baseAux, 1 );     
        AppLed_SetState( fp->baseAux + 1, 0 );     
      }
      PwmOut_SetAll( fp->baseOut + 1, diff, 0, 1 );
    }
    else  // the input is lower than last time
    {
      fp->output -= change; 
      if( fp->output < input || set )
        fp->output = input;
      if ( diff < -1  )
      {
        AppLed_SetState( fp->baseAux + 1, 1 );     
        AppLed_SetState( fp->baseAux, 0 );     
      }
      PwmOut_SetAll( fp->baseOut + 1, -diff, 1, 0 );
    }

    int diff2 = diff / 2;
    Servo_SetPosition( fp->baseAux + 1, 512 + diff2 );
             
    int out = (int)fp->output;
    PwmOut_SetDuty( fp->baseOut, out );
    Servo_SetPosition( fp->baseAux, out );
  }
  else
  {
    AppLed_SetState( fp->baseAux, 0 );
    AppLed_SetState( fp->baseAux + 1, 0 );
    PwmOut_SetDuty( fp->baseOut + 1, 0 );
    Servo_SetPosition( fp->baseAux + 1, 512 );
  }

}

void Follower_DeInit( void* follower )
{
  FollowerStruct* fp = (FollowerStruct*)follower;
  if ( fp != NULL )
  {
    int i;

    if ( fp->readConstants )
      Follower_SaveConstants( fp );

    AnalogIn_SetActive( 0, 0 );
    AnalogIn_SetActive( 3, 0 );
    DigitalIn_SetActive( 1, 0 );
    DigitalIn_SetActive( 2, 0 );

    for( i = fp->baseOut; i < fp->baseOut + 2; i++ )
    {
      PwmOut_SetDuty( i, 0 );
      PwmOut_SetActive( i, 0 );
    }
    for( i = fp->baseAux; i < fp->baseAux + 2; i++ )
    {
      AppLed_SetState( i, 0 );
      AppLed_SetActive( i, 0 );
      Servo_SetPosition( i, 0 );
      Servo_SetActive( i, 0 );
    }

    Free( fp );
  }
}

void Follower_SaveConstants( FollowerStruct *fp )
{
  Eeprom_Write( EEPROM_POLY_0_FOLLOWER_RATE + ( fp->bank * 8 ), (unsigned char *)&fp->rate, 4 );
  Eeprom_Write( EEPROM_POLY_0_FOLLOWER_PEAK + ( fp->bank * 8 ), (unsigned char *)&fp->peak, 4 );
}

//
// 3 - Oscillator Routines
//

//  int state;
//  float current;

//  float periodOn;
//  float periodOff;
//  int periodOnRaw;
//  int periodOffRaw;

// OscillatorStruct Oscillator[2]; // eventually put these on the heap...

void* Oscillator_Init( int bank )
{
  int i;

  OscillatorStruct* op = (OscillatorStruct*)Malloc( sizeof( OscillatorStruct ) );

  op->bank = bank;

  op->baseIn = bank * 4;
  op->baseOut = ( 1 - bank ) * 4;
  op->baseAux = bank * 2;

  op->state = 0;
  op->current = 0.0;

  Eeprom_Read( EEPROM_POLY_0_OSCILLATOR_PERIODON + ( bank * 8 ), (unsigned char*)&op->periodOn, 4 );
  Eeprom_Read( EEPROM_POLY_0_OSCILLATOR_PERIODOFF + ( bank * 8 ), (unsigned char*)&op->periodOff, 4 );
  
  for( i = op->baseOut; i < (op->baseOut + 4); i++)
    DigitalOut_SetValue( i, 0 );

  AppLed_SetState( op->baseAux, 1 );
  AppLed_SetState( op->baseAux + 1, 0 );
  Servo_SetPosition( op->baseAux, 512 );

  return (void*)op;
}

void Oscillator_Run( void* oscillatorVP, int ms, int readConstants )
{
  OscillatorStruct* op = (OscillatorStruct*)oscillatorVP;
  
  if ( readConstants )
  {
    int periodOffRaw = AnalogIn_GetValue( op->baseIn + 3 );
    if ( periodOffRaw != op->periodOffRaw )
    {
      op->periodOffRaw = periodOffRaw;
      op->periodOff = (float)( periodOffRaw * periodOffRaw ) / 50.0;
    }
    int periodOnRaw = AnalogIn_GetValue( op->baseIn + 2 );
    if ( periodOnRaw != op->periodOnRaw )
    {
      op->periodOnRaw = periodOnRaw;
      if ( periodOnRaw < 2 )
        op->periodOn = op->periodOff;
      else
        op->periodOn = (float)( periodOnRaw * periodOffRaw ) / 50.0;
    }
    else
    {
      if ( periodOnRaw < 2 )
        op->periodOn = op->periodOff;
    }
  }
  else
  {
    if ( op->readConstants )
      Oscillator_SaveConstants( op );
  }
  op->readConstants = readConstants;

  int setOn = DigitalIn_GetValue( op->baseIn + 0 );
  int setOff = DigitalIn_GetValue( op->baseIn + 1 );

  int stateNew = op->state;
  if ( setOn )
  {
    stateNew = 1;
    op->current = 0.0;
  }
  if ( setOff )
  {
    stateNew = 0;
    op->current = 0.0;
  }

  op->current += ms;
 
  if ( op->current > ( ( stateNew ) ? op->periodOn : op->periodOff ) )
  {
    stateNew = !stateNew;
    op->current = 0.0;
  }

  if ( stateNew != op->state )
  {
    if ( stateNew )
    {
      AppLed_SetState( op->baseAux, 0 );     
      AppLed_SetState( op->baseAux + 1, 1 );     
      DigitalOut_SetValue( op->baseOut, 1 );
      DigitalOut_SetValue( op->baseOut + 1, 0 );
    }
    else
    {
      AppLed_SetState( op->baseAux, 1 );     
      AppLed_SetState( op->baseAux + 1, 0 );     
      DigitalOut_SetValue( op->baseOut, 0 );
      DigitalOut_SetValue( op->baseOut + 1, 1 );
      Servo_SetPosition( op->baseAux, 0 );
      Servo_SetPosition( op->baseAux + 1, 0 );
    }
  }

  if ( stateNew )
  {
    int position = 2048 * op->current / op->periodOn;
    Servo_SetPosition( op->baseAux, ( position >> 2 ) );
    int position1 = ( position < 1023) ? position : 2048 - position;
    Servo_SetPosition( op->baseAux + 1, position1 );
  }

  op->state = stateNew;
}

void Oscillator_DeInit( void* Oscillator )
{
  OscillatorStruct* op = (OscillatorStruct*)Oscillator;
  if ( op != NULL )
  {
    int i;

    if ( op->readConstants )
      Oscillator_SaveConstants( op );

    DigitalIn_SetActive( 0, 0 );
    DigitalIn_SetActive( 1, 0 );
    AnalogIn_SetActive( 2, 0 );
    AnalogIn_SetActive( 3, 0 );

    for( i = op->baseOut; i < op->baseOut + 4; i++ )
    {
      DigitalOut_SetValue( i, 0 );
      DigitalOut_SetActive( i, 0 );
    }

    for( i = op->baseAux; i < op->baseAux + 2; i++ )
    {
      AppLed_SetState( i, 0 );
      AppLed_SetActive( i, 0 );
      Servo_SetPosition( i, 0 );
      Servo_SetActive( i, 0 );
    }

    Free( op );
  }
}

void Oscillator_SaveConstants( OscillatorStruct *op )
{
  Eeprom_Write( EEPROM_POLY_0_OSCILLATOR_PERIODOFF + ( op->bank * 8 ), (unsigned char *)&op->periodOn, 4 );
  Eeprom_Write( EEPROM_POLY_0_OSCILLATOR_PERIODON + ( op->bank * 8 ), (unsigned char *)&op->periodOff, 4 );
}

#if ( APPBOARD_VERSION == 50 )
  #define DIGITALOUT_0_IO IO_PA02
  #define DIGITALOUT_1_IO IO_PA02
  #define DIGITALOUT_2_IO IO_PA02
  #define DIGITALOUT_3_IO IO_PA02
  #define DIGITALOUT_4_IO IO_PA02
  #define DIGITALOUT_5_IO IO_PA02
  #define DIGITALOUT_6_IO IO_PA02
  #define DIGITALOUT_7_IO IO_PA02
  #define DIGITALOUT_01_ENABLE IO_PA02
  #define DIGITALOUT_23_ENABLE IO_PA02
  #define DIGITALOUT_45_ENABLE IO_PA02
  #define DIGITALOUT_67_ENABLE IO_PA02
#endif

#if ( APPBOARD_VERSION == 90 )
  #define DIGITALOUT_0_IO IO_PB23
  #define DIGITALOUT_1_IO IO_PA26
  #define DIGITALOUT_2_IO IO_PA25
  #define DIGITALOUT_3_IO IO_PB25
  #define DIGITALOUT_4_IO IO_PA02
  #define DIGITALOUT_5_IO IO_PA06
  #define DIGITALOUT_6_IO IO_PA05
  #define DIGITALOUT_7_IO IO_PA24
  #define DIGITALOUT_01_ENABLE IO_PB19
  #define DIGITALOUT_23_ENABLE IO_PB20
  #define DIGITALOUT_45_ENABLE IO_PB21
  #define DIGITALOUT_67_ENABLE IO_PB22
#endif
#if ( APPBOARD_VERSION == 95 || APPBOARD_VERSION == 100 )

  #define DIGITALOUT_0_IO IO_PA24
  #define DIGITALOUT_1_IO IO_PA05
  #define DIGITALOUT_2_IO IO_PA06
  #define DIGITALOUT_3_IO IO_PA02
  #define DIGITALOUT_4_IO IO_PB25
  #define DIGITALOUT_5_IO IO_PA25
  #define DIGITALOUT_6_IO IO_PA26
  #define DIGITALOUT_7_IO IO_PB23
  #define DIGITALOUT_01_ENABLE IO_PB19
  #define DIGITALOUT_23_ENABLE IO_PB20
  #define DIGITALOUT_45_ENABLE IO_PB21
  #define DIGITALOUT_67_ENABLE IO_PB22
#endif

int Poly_Ios[] =
{
  DIGITALOUT_0_IO,
  DIGITALOUT_1_IO, 
  DIGITALOUT_2_IO, 
  DIGITALOUT_3_IO, 
  DIGITALOUT_4_IO, 
  DIGITALOUT_5_IO, 
  DIGITALOUT_6_IO, 
  DIGITALOUT_7_IO, 
  0
};

int Poly_Enables[] =
{
  DIGITALOUT_01_ENABLE, 
  DIGITALOUT_23_ENABLE, 
  DIGITALOUT_45_ENABLE, 
  DIGITALOUT_67_ENABLE,
  0
};

void Poly_LineInit()
{
  // Lines OFF
  int* piop = Poly_Ios;
  while ( *piop )
    Io_SetFalse( *piop++ );

  // ENABLES ON
  piop = Poly_Enables;
  while ( *piop )
    Io_SetTrue( *piop++ );
}

/** @}
  */

