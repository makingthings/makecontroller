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

#include "FreeRTOS.h"
#include "task.h"
#include "config.h"
#include "AT91SAM7X256.h"

static void prvSetupHardware( void );
void vApplicationIdleHook( void );
void MakeStarterTask( void* parameters );

void MakeStarterTask( void* parameters )
{
 (void)parameters;
  Run( );
  TaskDelete( NULL );
}

int main( void )
{
	prvSetupHardware();
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
  /* 
    When using the JTAG debugger the hardware is not always initialised to
    the correct default state.  This line just ensures that this does not
    cause all interrupts to be masked at the start.
  */
  
  // Unstack nested interrupts
  unsigned int i;
  for (i = 0; i < 8 ; i++)
    AT91C_BASE_AIC->AIC_EOICR = 0;

  // Enable Protection mode
  AT91C_BASE_AIC->AIC_DCR = AT91C_AIC_DCR_PROT;
  
  /* 
    Note - Most setup is performed by the low level init function called from the
    startup asm file.
  */

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
}

int toggle;
void vApplicationIdleHook( void )
{
  // prevent the function from being optimized away?
  toggle = !toggle;
}

/*----------------------------------------------------------------------*/


