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
 *
 * A task defined by the function vBasicWEBServer is created.  This executes 
 * the lwIP stack and basic WEB server sample.  A task defined by the function
 * vUSBCDCTask.  This executes the USB to serial CDC example.  All the other 
 * tasks are from the set of standard demo tasks.  The WEB documentation 
 * provides more details of the standard demo application tasks.
 *
 * Main.c also creates a task called "Check".  This only executes every three
 * seconds but has the highest priority so is guaranteed to get processor time.
 * Its main function is to check the status of all the other demo application
 * tasks.  LED mainCHECK_LED is toggled every three seconds by the check task
 * should no error conditions be detected in any of the standard demo tasks.
 * The toggle rate increasing to 500ms indicates that at least one error has
 * been detected.
 *
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

/* lwIP includes. */
#include "lwip/api.h" 

/* MAKE CODE */
#include "makehooks.h"
#include "rtos.h"

#include "SAM7_EMAC.h"

/* lwIP includes. */
#include "lwip/api.h" 
#include "lwip/tcpip.h"
#include "lwip/memp.h" 
#include "lwip/stats.h"
#include "netif/loopif.h"

/* Hardware specific headers. */
#include "Board.h"
#include "config.h"
#include "AT91SAM7X256.h"

#include "led.h"

// NORMAL PROGRAMMING RESUMES

/* Priorities/stacks for the various tasks within the demo application. */
#define mainWEBSERVER_PRIORITY      ( tskIDLE_PRIORITY + 4 )


/*-----------------------------------------------------------*/

/*
 * Configure the processor for use with the Atmel demo board.  This is very
 * minimal as most of the setup is performed in the startup code.
 */
static void prvSetupHardware( void );

/*
 * The idle hook is just used to stream data to the USB port.
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
  MakeInit();
  TaskDelete( NULL );
}

void vlwIPInit( void );
void vBasicWEBServer( void *pvParameters );

void vlwIPInit( void )
{
    /* Initialize lwIP and its interface layer. */
	sys_init();
	mem_init();								
	memp_init();
	pbuf_init(); 
	netif_init();
	ip_init();
	tcpip_init( NULL, NULL );
}
/*------------------------------------------------------------*/

void vBasicWEBServer( void *pvParameters )
{
  struct ip_addr xIpAddr, xNetMast, xGateway;
  extern err_t ethernetif_init( struct netif *netif );
  static struct netif EMAC_if;

	/* Parameters are not used - suppress compiler error. */
	( void ) pvParameters;


	/* Create and configure the EMAC interface. */
	IP4_ADDR(&xIpAddr,emacIPADDR0,emacIPADDR1,emacIPADDR2,emacIPADDR3);
	IP4_ADDR(&xNetMast,emacNET_MASK0,emacNET_MASK1,emacNET_MASK2,emacNET_MASK3);
	IP4_ADDR(&xGateway,emacGATEWAY_ADDR0,emacGATEWAY_ADDR1,emacGATEWAY_ADDR2,emacGATEWAY_ADDR3);
	netif_add(&EMAC_if, &xIpAddr, &xNetMast, &xGateway, NULL, ethernetif_init, tcpip_input);

	/* make it the default interface */
    netif_set_default(&EMAC_if);

	/* bring it up */
    netif_set_up(&EMAC_if);

    while ( 1 )
      Sleep( 1000 );
}


int main( void )
{
	/* Setup the ports. */
	prvSetupHardware();

	/* Setup lwIP. */
  vlwIPInit();

	/* Create the lwIP task.  This uses the lwIP RTOS abstraction layer.*/
	sys_thread_new( vBasicWEBServer, ( void * ) NULL, mainWEBSERVER_PRIORITY );
                                                                                    
	/* Create the make task. */
	xTaskCreate( MakeStarterTask, "Make", 500, NULL, tskIDLE_PRIORITY + 1, NULL );

	/* Finally, start the scheduler. 

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
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

  // EEPROM DISABLE
  #if ( CONTROLLER_VERSION == 90 )
    AT91C_BASE_PIOB->PIO_PER = 1 << 17; // Set PB17 - the EEPROM ~enable - in PIO mode
  	AT91C_BASE_PIOB->PIO_OER = 1 << 17; // Configure in Output
	  AT91C_BASE_PIOB->PIO_SODR = 1 << 17; // Set Output
  #endif
  #if ( CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 )
    AT91C_BASE_PIOA->PIO_PER = 1 << 9; // Set PA9 - the EEPROM ~enable - in PIO mode
	  AT91C_BASE_PIOA->PIO_OER = 1 << 9; // Configure in Output
	  AT91C_BASE_PIOA->PIO_SODR = 1 << 9; // Set Output
  #endif

  // CAN DISABLE
  #if ( CONTROLLER_VERSION == 90 )
    AT91C_BASE_PIOB->PIO_PER = 1 << 16; // Set PB16 - the CAN ~enable - in PIO mode
    AT91C_BASE_PIOB->PIO_OER = 1 << 16; // Configure in Output
    AT91C_BASE_PIOB->PIO_SODR = 1 << 16; // Set Output
  #endif
  #if ( CONTROLLER_VERSION == 95  || CONTROLLER_VERSION == 100 )
    AT91C_BASE_PIOA->PIO_PER = 1 << 7; // Set PA7 - the CAN ~enable - in PIO mode
    AT91C_BASE_PIOA->PIO_OER = 1 << 7; // Configure in Output
    AT91C_BASE_PIOA->PIO_SODR = 1 << 7; // Set Output
  #endif

	/* Enable the peripheral clock. */
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOA;
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOB;
	
  #ifndef STRIP 
    AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_EMAC;
  #endif

  /* Turn the USB line into an input, kill the pull up */
  #if ( CONTROLLER_VERSION == 90 )
    AT91C_BASE_PIOB->PIO_PER = 1 << 10;	
    AT91C_BASE_PIOB->PIO_ODR = 1 << 10;
    AT91C_BASE_PIOB->PIO_PPUDR = 1 << 10;
  #endif
  #if ( CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 )
    AT91C_BASE_PIOA->PIO_PER = 1 << 10;	
    AT91C_BASE_PIOA->PIO_ODR = 1 << 10;
    AT91C_BASE_PIOA->PIO_PPUDR = 1 << 10;
  #endif

  /* Setup the PIO for high. */
	/* Start without the pullup - this will get set at the end of this 
	function. */
	#if ( CONTROLLER_VERSION == 90 )
		AT91C_BASE_PIOB->PIO_PER = AT91C_PIO_PB11;
    AT91C_BASE_PIOB->PIO_OER = AT91C_PIO_PB11;
    AT91C_BASE_PIOB->PIO_SODR = AT91C_PIO_PB11;
	#endif
	#if ( CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 )
		AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA11;
		AT91C_BASE_PIOA->PIO_OER = AT91C_PIO_PA11;
    AT91C_BASE_PIOA->PIO_SODR = AT91C_PIO_PA11;
	#endif
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{

 /*

  */

}

/*----------------------------------------------------------------------*/


