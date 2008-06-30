/*
  AinToServo - MakingThings, 2008
  
  Read values from the analog ins, and send those values
  to the servos.
*/
#include "config.h"
#include "servo.h"

void AinServoTask( void* p );

void Run( ) // this gets called as soon as we boot up.
{
  TaskCreate( AinServoTask, "AinServo", 1000, 0, 3 );
}

void AinServoTask( void* p )
{
  (void)p; // unused variable
  int i;
  int ain;

  while( true )
  {
    for( i = 0; i < 4; i++ ) // loop through 4 since we have 4 servos
    {
       ain = AnalogIn_GetValue(i); // read the analog in value
       Servo_SetPosition(i, ain);  // send it to the servo
    }
    Sleep(1);
  }
}


