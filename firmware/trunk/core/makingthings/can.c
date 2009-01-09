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

/** \file can.c	
	CAN - Control Area Network.
	Functions for interacting with the CAN Network.

  Note this subsystem is not implemented
*/

/* Library includes. */
#include <string.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Hardware specific headers. */
#include "Board.h"
#include "AT91SAM7X256.h"

#include "config.h"

#include "io.h"

#include "can.h"
#include "can_internal.h"


// CAN DISABLE
#if ( CONTROLLER_VERSION == 50 )
  #define CAN_ENABLE IO_PA02
  #define CAN_RX     IO_PA19
  #define CAN_TX     IO_PA20
#endif
#if ( CONTROLLER_VERSION == 90 )
  #define CAN_ENABLE IO_PB16
  #define CAN_RX     IO_PA19
  #define CAN_TX     IO_PA20
#endif
#if ( CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 )
  #define CAN_ENABLE IO_PA07
  #define CAN_RX     IO_PA19
  #define CAN_TX     IO_PA20
#endif

static int Can_Start( void );
static int Can_Stop( void );

static int Can_Init( void );

extern void ( CanIsr_Wrapper )( void );

struct Can_ Can;

/* \defgroup Can
* The CAN (Controller Area Network) subsystem allows for fast and reliable board-to-board communication.
* \ingroup Core
* @{
*/

/**
	Sets whether the CAN subsystem is active.
	@param state An integer specifying the active state - 1 (on) or 0 (off).
	@return 0 on success.
*/
int Can_SetActive( int state )
{
  if ( state )
    return Can_Start( );
  else
    return Can_Stop( );
}

/**
	Returns the active state of the CAN subsystem.
	@return State - 1/non-zero (on) or 0 (off).
*/
int Can_GetActive( )
{
  return Can.users > 0;
}

/**	
	Send a CAN message.
	@param id An integer specifying the message ID.
	@param message A pointer to the message to be sent.
	@param count ???
	@return Status - 0 on success.
*/
int Can_SendMessage( int id, char* message, int count )
{
  (void)id;
  (void)message;
  (void)count;

  if ( Can.users < 1 )
  {
    int status = Can_Start( );
    if ( status != CONTROLLER_OK )
      return status;
  }

  int value = 0;

  int i;
  int j;

  for ( i = 0; i < 20; i++ )
  {
    Io_SetValue( CAN_TX, 1 );
    for ( j = 0; j < 100; j++ )
      ;
    Io_SetValue( CAN_TX, 0 );
    for ( j = 0; j < 100; j++ )
      ;
  }

/*
  // This is the semaphore that lets only one instance get messages at a time
  if ( !xSemaphoreTake( Can.semaphore, 1000 ) )
    return CONTROLLER_ERROR_CANT_LOCK;

  // start the business

  // Busy wait - nah 
  // while ( !( AT91C_BASE_ADC->ADC_CHSR  & ( 1 << index ) ) );

  // This is the semaphore that signals that it's done
  if ( !xSemaphoreTake( Can.doneSemaphore, 1000 ) )
    return CONTROLLER_ERROR_TIMEOUT;

  // extract the message

  xSemaphoreGive( Can.semaphore );


 // Disable mailbox 0
  AT91C_BASE_CAN->CAN_MB0.CAN_MB_MMR = AT91C_CAN_MOT_DIS;  

    // Set ID to send
  AT91C_BASE_CAN->CAN_MB0.CAN_MB_MID = ( ( 0x55 << 18 ) & AT91C_CAN_MIDvA );

 // Set mailbox 0 up as a transmitter
  AT91C_BASE_CAN->CAN_MB0.CAN_MB_MMR = AT91C_CAN_MOT_TX;  

  // Set the length, and transmit
  AT91C_BASE_CAN->CAN_MB0.CAN_MB_MCR = ( ( 0x4 << 16 ) & AT91C_CAN_MDLC ) || AT91C_CAN_MTCR;

  while ( !AT91C_BASE_CAN->CAN_MB0.CAN_MB_MSR & AT91C_CAN_MRDY )
    ;
*/
  return value;
}

/**	
	Receive a CAN message.
	@param id An integer specifying the message ID.
	@param message A pointer to where the message should be saved.
	@param count ???
	@return Status - 0 on success.
*/
int Can_GetMessage( int* id, char* message, int* count )
{
  (void)id;
  (void)message;
  (void)count;

  if ( Can.users < 1 )
  {
    int status = Can_Start( );
    if ( status != CONTROLLER_OK )
      return status;
  }

  int value = 0;

  // This is the semaphore that lets only one instance get messages at a time
  if ( !xSemaphoreTake( Can.semaphore, 1000 ) )
    return CONTROLLER_ERROR_CANT_LOCK;

  // start the business

  /* Busy wait - nah */
  // while ( !( AT91C_BASE_ADC->ADC_CHSR  & ( 1 << index ) ) );

  // This is the semaphore that signals that it's done
  if ( !xSemaphoreTake( Can.doneSemaphore, 1000 ) )
    return CONTROLLER_ERROR_TIMEOUT;

  // extract the message

  xSemaphoreGive( Can.semaphore );

  return value;
}

/** @}
*/

int Can_Start()
{
  // int status;
  if ( Can.users++ == 0 )
  {
    Can_Init();
  }
  return CONTROLLER_OK;
}

int Can_Stop()
{
  if ( Can.users <= 0 )
    return CONTROLLER_ERROR_TOO_MANY_STOPS;
  
  if ( --Can.users == 0 )
    Can_Stop();

  return CONTROLLER_OK;
}

int Can_Init()
{
  // Try to lock the enable pin
  int status = Io_Start( CAN_ENABLE, true );
  if ( status != CONTROLLER_OK )
    return status;

  // Try to lock the tx pin
  status = Io_Start( CAN_TX, true );
  if ( status != CONTROLLER_OK )
  {
    Io_Stop( CAN_ENABLE );
    return status;
  }

  // Try to lock the rx pin
  status = Io_Start( CAN_RX, true );
  if ( status != CONTROLLER_OK )
  {
    Io_Stop( CAN_ENABLE );
    Io_Stop( CAN_TX );
    return status;
  }

  Io_SetDirection( CAN_TX, true );
  Io_SetDirection( CAN_RX, false );
  Io_SetPullup( CAN_RX, false );

  Io_SetDirection( CAN_ENABLE, true );
  Io_SetValue( CAN_ENABLE, false );

/*
  // Enable the peripheral clock
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_CAN;

  // Make sure the pins are running on the right peripheral (A in this case)
  Io_SetPeripheralA( CAN_TX );
  Io_SetPeripheralA( CAN_RX );

  Io_SetOutput( CAN_ENABLE );
  Io_SetValue( CAN_ENABLE, false );

  // Set some random bit timing up.
  AT91C_BASE_CAN->CAN_BR = 0x0053255;
    
  // Enable the CAN system
  AT91C_BASE_CAN->CAN_MR = AT91C_CAN_CANEN;

  // Do the OS stuff
  vSemaphoreCreateBinary( Can.semaphore );

  // Create the sempahore that will be used to wake the calling process up 
  vSemaphoreCreateBinary( Can.doneSemaphore );
  xSemaphoreTake( Can.doneSemaphore, 0 );

  // Do the hardware init

  // Initialize the interrupts

  // WAS AT91F_AIC_ConfigureIt( AT91C_ID_ADC, 3, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, ( void (*)( void ) ) CanIsr_Wrapper );
  // Which is defined at the bottom of the AT91SAM7X256.h file
  unsigned int mask ;													
																			
  mask = 0x1 << AT91C_ID_CAN;		
                        
  // Disable the interrupt on the interrupt controller					
  AT91C_BASE_AIC->AIC_IDCR = mask ;										
  // Save the interrupt handler routine pointer and the interrupt priority
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_CAN ] = (unsigned int)CanIsr_Wrapper;			
  // Store the Source Mode Register		
  AT91C_BASE_AIC->AIC_SMR[ AT91C_ID_CAN ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 4  ;				
  // Clear the interrupt on the interrupt controller
  AT91C_BASE_AIC->AIC_ICCR = mask ;					

  // AT91C_BASE_AIC->CAN_IER = 0; // AT91C_ADC_DRDY; 

	AT91C_BASE_AIC->AIC_IECR = mask;
*/

  return CONTROLLER_OK;
}

#ifdef OSC

#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* CanOsc_Name = "can";
static char* CanOsc_PropertyNames[] = { "active", "value", 0 }; // must have a trailing 0

int CanOsc_PropertySet( int property, int value );
int CanOsc_PropertyGet( int property );

// Returns the name of the subsystem
const char* CanOsc_GetName( )
{
  return CanOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int CanOsc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_IntReceiverHelper( channel, message, length, 
                                      CanOsc_Name,
                                      CanOsc_PropertySet, CanOsc_PropertyGet, 
                                      CanOsc_PropertyNames );

  // the can system is complex it will need another kind of helper - one that
  // can handle
  //     /can/active 1  as well as
  //     /can/0/id 21213
  // also the can getters and setters will need to be able to handle complex data

  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, CanOsc_Name, status );
  return CONTROLLER_OK;
}

// Set the index LED, property with the value
int CanOsc_PropertySet( int property, int value )
{
  switch ( property )
  {
    case 0: 
      Can_SetActive( value );
      break;      
  }
  return CONTROLLER_OK;
}

// Get the property
int CanOsc_PropertyGet( int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      value = Can_GetActive( );
      break;
  }
  
  return value;
}

#endif // OSC



