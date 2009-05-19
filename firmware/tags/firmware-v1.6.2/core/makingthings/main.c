/*
	FreeRTOS V3.2.4 - copyright (C) 2003-2005 Richard Barry.

	This file is part of the FreeRTOS distribution.

	FreeRTOS is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	FreeRTOS is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FreeRTOS; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	A special exception to the GPL can be applied should you wish to distribute
	a combined work that includes FreeRTOS, without being obliged to provide
	the source code for any proprietary components.  See the licensing section
	of http://www.FreeRTOS.org for full details of how and when the exception
	can be applied.

	***************************************************************************
	See http://www.FreeRTOS.org for documentation, latest information, license
	and contact details.  Please ensure to read the configuration and relevant
	port sections of the online documentation.
	***************************************************************************
*/

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the application tasks, then starts the scheduler.
 * Main.c includes an idle hook function that simply periodically sends data
 * to the USB task for transmission.
 */

/*
	Changes from V3.2.2

	+ Modified the stack sizes used by some tasks to permit use of the 
	  command line GCC tools.
*/

/* Library includes. */
#include <string.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "portable.h"

/* MAKE CODE */
#include "config.h"

/* Hardware specific headers. */
#include "AT91SAM7X256.h"


// NORMAL PROGRAMMING RESUMES

/*-----------------------------------------------------------*/

/*
 * Configure the processor for use with the Atmel demo board.  This is very
 * minimal as most of the setup is performed in the startup code.
 */
static void prvSetupHardware( void );

/*
 * The idle hook.
 */
void vApplicationIdleHook( void );
/*-----------------------------------------------------------*/

/*
 * Setup hardware then start all the demo application tasks.
 */
void MakeStarterTask( void* parameters );

void MakeStarterTask( void* parameters )
{
 (void)parameters;
  Run( );
  TaskDelete( NULL );
}

int main( void )
{
	/* Setup the ports. */
	prvSetupHardware();

	/* Create the make task. */
	TaskCreate( MakeStarterTask, "Make", 1200, NULL, 4 );

	/*NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never get here! */
	return 0;
}
/*-----------------------------------------------------------*/


static void prvSetupHardware( void )
{
	/* When using the JTAG debugger the hardware is not always initialised to
	the correct default state.  This line just ensures that this does not
	cause all interrupts to be masked at the start. */
	AT91C_BASE_AIC->AIC_EOICR = 0;
	
	/* Most setup is performed by the low level init function called from the
	startup asm file.*/

  // ENABLE HARDWARE RESET
  while((AT91C_BASE_RSTC->RSTC_RSR & (AT91C_RSTC_SRCMP | AT91C_RSTC_NRSTL)) != (AT91C_RSTC_NRSTL));
  AT91C_BASE_RSTC->RSTC_RMR = (0xa5 << 24)
    | AT91C_RSTC_URSTEN
    | ((12 - 1) << 8) // 125ms == (1 << 12) / 32768
  ;

  // EEPROM DISABLE
  #if ( CONTROLLER_VERSION == 90 )
    AT91C_BASE_PIOB->PIO_PER = 1 << 17; // Set PB17 - the EEPROM ~enable - in PIO mode
  	AT91C_BASE_PIOB->PIO_OER = 1 << 17; // Configure in Output
	  AT91C_BASE_PIOB->PIO_SODR = 1 << 17; // Set Output
  #elif ( CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 || CONTROLLER_VERSION == 200 )
    AT91C_BASE_PIOA->PIO_PER = 1 << 9; // Set PA9 - the EEPROM ~enable - in PIO mode
	  AT91C_BASE_PIOA->PIO_OER = 1 << 9; // Configure in Output
	  AT91C_BASE_PIOA->PIO_SODR = 1 << 9; // Set Output
  #endif

  // CAN DISABLE
  #if ( CONTROLLER_VERSION == 90 )
    AT91C_BASE_PIOB->PIO_PER = 1 << 16; // Set PB16 - the CAN ~enable - in PIO mode
    AT91C_BASE_PIOB->PIO_OER = 1 << 16; // Configure in Output
    AT91C_BASE_PIOB->PIO_SODR = 1 << 16; // Set Output
  #elif ( CONTROLLER_VERSION == 95  || CONTROLLER_VERSION == 100 || CONTROLLER_VERSION == 200 )
    AT91C_BASE_PIOA->PIO_PER = 1 << 7; // Set PA7 - the CAN ~enable - in PIO mode
    AT91C_BASE_PIOA->PIO_OER = 1 << 7; // Configure in Output
    AT91C_BASE_PIOA->PIO_SODR = 1 << 7; // Set Output
  #endif

	/* Enable the peripheral clock. */
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOA;
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOB;
	
  // Enable the EMAC
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_EMAC;

  #if ( APPBOARD_VERSION == 100 )
    // Kill the outputs
    // Outputs 0 - 7 are PA24, PA5, PA6, PA2, PB25, PA25, PA26, PB23
    int outputAMask = ( 1 << 24 ) | ( 1 << 5 ) | ( 1 << 6 ) | ( 1 << 2  ) | ( 1 << 25  ) | ( 1 << 26 );
    int outputBMask = ( 1 << 25 ) | ( 1 << 23 );
    // Set in peripheral mode
    AT91C_BASE_PIOA->PIO_PER = outputAMask;	
    AT91C_BASE_PIOB->PIO_PER = outputBMask;	
    // Set to Outputs
    AT91C_BASE_PIOA->PIO_OER = outputAMask;
    AT91C_BASE_PIOB->PIO_OER = outputBMask;
    // Turn Off
    AT91C_BASE_PIOA->PIO_CODR = outputAMask;
    AT91C_BASE_PIOB->PIO_CODR = outputBMask;
  #endif
    
  /* Turn the USB line into an input, kill the pull up */
  #if ( CONTROLLER_VERSION == 90 )
    AT91C_BASE_PIOB->PIO_PER = AT91C_PIO_PB10;	
    AT91C_BASE_PIOB->PIO_ODR = AT91C_PIO_PB10;
    AT91C_BASE_PIOB->PIO_PPUDR = AT91C_PIO_PB10;
  #elif ( CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 )
    AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA10;	
    AT91C_BASE_PIOA->PIO_ODR = AT91C_PIO_PA10;
    AT91C_BASE_PIOA->PIO_PPUDR = AT91C_PIO_PA10;
  #elif ( CONTROLLER_VERSION == 200 )
    AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA29;	
    AT91C_BASE_PIOA->PIO_ODR = AT91C_PIO_PA29;
    AT91C_BASE_PIOA->PIO_PPUDR = AT91C_PIO_PA29;
  #endif

  /* Setup the PIO for the USB pull up resistor. */
	/* Start low: no USB */
	#if ( CONTROLLER_VERSION == 90 )
		AT91C_BASE_PIOB->PIO_PER = AT91C_PIO_PB11;
    AT91C_BASE_PIOB->PIO_OER = AT91C_PIO_PB11;
    AT91C_BASE_PIOB->PIO_SODR = AT91C_PIO_PB11;
	#elif ( CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 )
		AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA11;
		AT91C_BASE_PIOA->PIO_OER = AT91C_PIO_PA11;
    AT91C_BASE_PIOA->PIO_CODR = AT91C_PIO_PA11; // had this round the wrong way...
  #elif ( CONTROLLER_VERSION == 200 )
		AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA30;
		AT91C_BASE_PIOA->PIO_OER = AT91C_PIO_PA30;
    AT91C_BASE_PIOA->PIO_CODR = AT91C_PIO_PA30;
	#endif
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

int toggle;
void vApplicationIdleHook( void )
{
  // prevent the function from being optimized away?
  toggle = !toggle;
}

/*----------------------------------------------------------------------*/


