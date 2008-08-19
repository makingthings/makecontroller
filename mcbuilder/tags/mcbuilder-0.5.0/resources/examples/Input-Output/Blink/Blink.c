/*
  Blink - MakingThings, 2008

  Blink the LED on the Make Controller Board.
*/
#include "config.h"

void BlinkTask( void* p );

void Run( ) // this task gets called as soon as we boot up.
{
  TaskCreate( BlinkTask, "Get", 1000, 0, 3 );
}

void BlinkTask( void* p )
{
  (void)p; // unused variable

  while( true ) // forever...
  {
    Led_SetState(0);
    Sleep(990);
    Led_SetState(1);
    Sleep(10);
  }
}


