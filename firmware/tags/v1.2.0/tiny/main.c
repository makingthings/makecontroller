/* 
   Make Controller Kit

   main.c

   This project is designed to show a minimal system doing trival things.  Some
   users may choose to use this project as a starting point for their own projects.
*/

#include "AT91SAM7X256.h"
#include "led.h"

int main( void )
{
  int i;

  Led_SetState( 1 );

  //int i;
  for ( i = 0; i < 1000000; i++ )
    ;

  while ( 1 )
  {
    Led_SetState( 1 );

    int i;
    for ( i = 0; i < 1000; i++ )
      ;

    for ( i = 0; i < 100000; i++ )
      ;
    Led_SetState( 0 );

    for ( i = 0; i < 100000; i++ )
      ;
  }

}

void swi_handler( void )
{

}
