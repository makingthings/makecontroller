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

/** \file system.c	
  System Control.
	Functions for monitoring and controlling the system.
*/

#include "stdio.h"
#include "string.h"
#include "ctype.h"
#include "io.h"
#include "eeprom.h"
#include "system.h"
#include "config.h"
#include "AT91SAM7X256.h"
#include "portmacro.h"
#include "FreeRTOSConfig.h"
#include "network.h"

/* the Atmel header file doesn't define these. */
#ifdef AT91SAM7X256_H
# define AT91C_RSTC_KEY_PASSWORD	(0xa5 << 24)

# define AT91C_IROM			((char *)(0x3 << 20))
# define AT91C_IROM_SIZE		(8 << 10)
#endif

int PortFreeMemory( void );
void StackAuditTask( void* p );
void kill( void );
void vPortDisableAllInterrupts( void );
#define ASYNC_INIT -10
#define ASYNC_INACTIVE -1

typedef struct System_
{
  char name[ SYSTEM_MAX_NAME + 1 ]; // allotted EEPROM space is 100, but leave room for \0!
  int users;
  void* StackAuditPtr;
  #ifdef OSC
  char scratch1[ OSC_SCRATCH_SIZE ];
  int asyncDestination;
  int autoInterval;
  #endif // OSC
} SystemS;

SystemS* System;

/** \defgroup System System
    The System subsystem monitors and controls several aspects of the system. 

* \ingroup Core
* @{
*/

/**
	Sets whether the System subsystem is active.
	@param state An integer specifying the state system
	@return Zero on success.
	
	\b Example
	\code
	// enable the System subsystem
	System_SetActive(1);
	\endcode
*/
int System_SetActive( int state )
{
  if( state )
  {
    if( System == NULL )
    {
      System = MallocWait( sizeof( SystemS ), 100 );
      System->name[0] = 0;
      System->StackAuditPtr = NULL;
      #ifdef OSC
      System->asyncDestination = ASYNC_INIT;
      System->asyncDestination = System_GetAsyncDestination( );
      System->autoInterval = ASYNC_INIT;
      System->autoInterval = System_GetAutoSendInterval( );
      #endif
    }
    return CONTROLLER_OK;
  }
  else
  {
    if ( System != NULL )
    {
      Free( System );
      System = NULL;
    }
    return CONTROLLER_OK;
  }
}

/**
	Returns the active state of the subsystem.
	@return The active state of the subsystem - 1 (active) or 0 (inactive).
	
	\b Example
	\code
	if(System_GetActive())
	{
	  // System is active
	}
	else
	{
	  // System is inactive
	}
	\endcode
*/
int System_GetActive( )
{
  return System != NULL;
}

/**
	Returns the free size of the heap.
	The heap size is set in config.h in the constant CONTROLLER_HEAPSIZE.
	Any calls to Malloc will take their memory from the heap.  This allows
	you to check how much heap is remaining.
	@return The size free memory.
	
	\b Example
	\code
	// see how much memory is available
	int freemem = System_GetFreeMemory();
	\endcode
*/
int System_GetFreeMemory( void )
{
  return PortFreeMemory();
}

/**
	Gets the board's Serial Number.
	Each board has a serial number, although it's not necessarily unique
	as you can reset it as you please.
	
	The serial number is used to determine the Ethernet MAC address, so boards
	on the same network need to have unique serial numbers.
	@return CONTROLLER_OK ( = 0 ).
	
	\b Example
	\code
	// get this board's serial number
	int sernum = System_GetSerialNumber();
	\endcode
*/
int System_GetSerialNumber( void )
{
  int serial;
  if ( Eeprom_Read( EEPROM_SYSTEM_SERIAL_NUMBER, (uchar*)&serial, 4 ) == CONTROLLER_OK )
    return serial & 0xFFFF;
  return 0;
}

/**
	Sets the Serial Number. 
	Note that this can be changed by the user at 
  any time, but that it is used in the \ref Network subsystem to form the last 
  two bytes of the network MAC address, so ideally units on the same network
  should have unique serial numbers.
	@return 0 on success.
	
	\b Example
	\code
	// set the serial number to 12345
	System_SetSerialNumber(12345);
	\endcode
*/
int System_SetSerialNumber( int serial )
{
  serial &= 0xFFFF;
  return Eeprom_Write( EEPROM_SYSTEM_SERIAL_NUMBER, (uchar*)&serial, 4 );
}

/**
	Returns the board to SAM-BA mode.
	When a board is in SAM-BA mode, it is ready to have new firmware uploaded to it. 
	Upon successful completion, the board will be reset and begin running SAM-BA.  
	This function does not clear the GPNVM2 bit, so if you unplug/replug you'll be running
	your old code again.
  @param sure Confirm you're sure you want to do this.
	@return nonzero on failure.  Successful completion does not return.
	
	\b Example
	\code
	// prepare to be uploaded
	System_SetSamba(1);
	\endcode
*/
int System_SetSamba( int sure )
{
  if ( sure )
  {
    vPortDisableAllInterrupts();

    /* Disable the USB pullup. */
#if ( CONTROLLER_VERSION == 90 )
  AT91C_BASE_PIOB->PIO_PER = AT91C_PIO_PB11;
  AT91C_BASE_PIOB->PIO_OER = AT91C_PIO_PB11;
  AT91C_BASE_PIOB->PIO_SODR = AT91C_PIO_PB11;
#elif ( CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 )
  AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA11;
  AT91C_BASE_PIOA->PIO_OER = AT91C_PIO_PA11;
  AT91C_BASE_PIOA->PIO_CODR = AT91C_PIO_PA11;
#elif ( CONTROLLER_VERSION == 200 )
  AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA30;
  AT91C_BASE_PIOA->PIO_OER = AT91C_PIO_PA30;
  AT91C_BASE_PIOA->PIO_CODR = AT91C_PIO_PA30;
#endif

    /* Steal the PIT for the pullup disable delay. */
    AT91C_BASE_PITC->PITC_PIMR = (
      (configCPU_CLOCK_HZ + (16 * 1000 / 2))
      / (16 * 1000)
    ) | AT91C_PITC_PITEN
    ;

    /* Dummy read to clear picnt. */
    __asm__ __volatile__ ("ldr r3, %0" :: "m" (AT91C_BASE_PITC->PITC_PIVR) : "r3");

    /* Loop until picnt passes 200ms */
    while((AT91C_BASE_PITC->PITC_PIIR & AT91C_PITC_PICNT) < (200 << 20));

    /* Reset onboard and offboard peripherals, but not processor */
    while(AT91C_BASE_RSTC->RSTC_RSR & AT91C_RSTC_SRCMP);
    AT91C_BASE_RSTC->RSTC_RMR = AT91C_RSTC_KEY_PASSWORD;
    AT91C_BASE_RSTC->RSTC_RCR = AT91C_RSTC_KEY_PASSWORD
	| AT91C_RSTC_PERRST
    	| AT91C_RSTC_EXTRST
    ;
    while(AT91C_BASE_RSTC->RSTC_RSR & AT91C_RSTC_SRCMP);

    /*
       The ROM code copies itself to RAM, where it runs.
       That works fine when running SAM-BA in the usual way (booting with GPNVM2 clear).
       However, it is actually copying from the remap page; not the native ROM address.
       So with GPNVM2 set, that means the FLASH image (and only 8KB or so of that) gets copied, 
       which is not a recipe for success.
       This workaround would be unnecessary if the ROM just copied itself from 0x300000 instead of 0x0.
       If not for the fact that the ROM code doesn't appear to issue a remap command (the exception vectors 
       ordinarily remain in ROM instead of being remapped), this workaround would not be possible.  Either 
       it doesn't remap or it does it an even number of times.  To address the problem, we copy the ROM 
       image to the RAM, ourselves and issue a remap so that when we run the image, RAM will already be remapped, 
       and the image copy that it performs will be from the RAM to itself, therefore harmless. This does have 
       the side effect that when SAM-BA runs other code (via the G command), the remap page will be 
       remapped to RAM.  Any code run that way which assumes otherwise will break.  We are not using SAM-BA 
       in that way.
     */

    /* From here on, we have to be in asm to prevent the compiler from trying to use RAM in any way. */
    __asm__ __volatile__ (
    /* Copy the ROM image to RAM. */
    "	mov	r6, %0		\n"	/* save ROM address for later */
    "	b	2f		\n"
    "1:				\n"
    "	ldmia	%0!, {r7}	\n"
    "	str	r7, [%2]	\n"
    "	add	%2, %2, #4	\n"
    "2:				\n"
    "	cmp	%0, %1		\n"
    "	bmi	1b		\n"

    /* Remap so that image copy in SAM-BA is RAM to RAM. */
    /* We know that the remap page is not currently remapped because we just did an AT91C_RSTC_PERRST. */
    "	mov	r7, %4 		\n"
    "	str	r7, %3		\n"

    /* Start running the ROM. */
    "	bx	r6		\n"
    :
    :
    "r"(AT91C_IROM),
    "r"(AT91C_IROM + AT91C_IROM_SIZE),
    "r"(AT91C_ISRAM),
    "m"(AT91C_BASE_MC->MC_RCR),
    "n"(AT91C_MC_RCB)
    :
    "r7", "r6"
    );
  }

  // Never will do this
  return 1;
}

/**
	Gives the board a name.
  The name must be alpha-numeric - only letters and numbers.  It can help you
  determine one board from another when there are more than one connected to 
  your system.
  @param name A string specifying the board's name
	@return 0 on success.
	
	\b Example
	\code
	// give the board a special name
	System_SetName("my very special controller");
	\endcode
*/
int System_SetName( char* name )
{
  System_SetActive( 1 );
  int length = strlen( name );
  if( length > SYSTEM_MAX_NAME )
    return CONTROLLER_ERROR_STRING_TOO_LONG;
  
  strcpy( System->name, name ); // update the name in our buffer
  int i;
  char* ptr = name;
  for( i = 0; i <= length; i++ ) // have to do this because Eeprom_Write can only go 32 at a time.
  {
    Eeprom_Write( EEPROM_SYSTEM_NAME + i, (uchar*)ptr++, 1 );
  }

  return CONTROLLER_OK;
}

/**
	Read the board's name.
	@return The board's name as a string.
	
	\b Example
	\code
	char* board_name = System_GetName();
	\endcode
*/
char* System_GetName( )
{
  System_SetActive( 1 );
  if( System->name[0] == 0 )
  {
    char* ptr;
    ptr = System->name;
    int i;
    bool legal = false;
    for( i = 0; i <= SYSTEM_MAX_NAME; i++ )
    {
      Eeprom_Read( EEPROM_SYSTEM_NAME + i, (uchar*)ptr, 1 );
      if( *ptr == 0 )
        break;
      if( !isalnum( *ptr ) && *ptr != ' ' )
      {
        legal = false;
        break;
      }
      legal = true;
      
      if( i == SYSTEM_MAX_NAME && *ptr != 0 )
        *ptr = 0;
      ptr++;
    }

    if( !legal )
    {
      strcpy( System->name, "Make Controller Kit" );
      System_SetName( System->name );
    }
  }

  return System->name;
}

/**
	Reset the board.
  Will reboot the board if the parameter sure is true.
  @param sure confirms the request is true.
	@return 0 on success.
	
	\b Example
	\code
	// reboot
	System_SetReset(1);
	\endcode
*/
int System_SetReset( int sure )
{
  if ( sure )
    kill( );

  return 1;
}

/** @}
*/

void kill( void )
{
  AT91C_BASE_RSTC->RSTC_RCR = ( AT91C_RSTC_EXTRST | AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST | (0xA5 << 24 ) );
}

#ifdef OSC
#include "osc.h"

void System_StackAudit( int on_off )
{
  System_SetActive( 1 );
  if( System->StackAuditPtr == NULL && on_off )
    System->StackAuditPtr = TaskCreate( StackAuditTask, "StackAudit", 700, 0, 5 );
  
  if( System->StackAuditPtr != NULL && !on_off )
  {
    TaskDelete( System->StackAuditPtr );
    System->StackAuditPtr = NULL;
  }
}

void StackAuditTask( void* p )
{
  (void)p;
  void* task = NULL;
  while( 1 )
  {
    task = TaskGetNext( task );
    int stackremaining = TaskGetRemainingStack( task );
    if( stackremaining < 50 )
    {
      Led_SetState( 1 );
      Debug( DEBUG_WARNING, "Warning: Stack running low on task %s. %d bytes left.", TaskGetName( task ), stackremaining );
    }

    int freemem = System_GetFreeMemory( );
    if( freemem < 100 )
    {
      Led_SetState( 1 );
      Debug( DEBUG_WARNING, "Warning: System memory running low. %d bytes left.", freemem );
    }
    
    Sleep( 5 );
  }
}

void System_SetAsyncDestination( int dest )
{
  System_SetActive( 1 );
  if( dest < ASYNC_INACTIVE || dest > (OSC_CHANNEL_COUNT-1) )
    return;
  else
  {
    if( System->asyncDestination != dest )
    {
      System->asyncDestination = dest;
      Eeprom_Write( EEPROM_OSC_ASYNC_DEST, (uchar*)&dest, 4 );
    }
  }
}

int System_GetAsyncDestination( )
{
  System_SetActive( 1 );
  if( System->asyncDestination == ASYNC_INIT )
  {
    int async;
    Eeprom_Read( EEPROM_OSC_ASYNC_DEST, (uchar*)&async, 4 );
    if( async >= 0 && async <= (OSC_CHANNEL_COUNT-1) )
      System->asyncDestination = async;
    else
      System->asyncDestination = ASYNC_INACTIVE;
  }
  return System->asyncDestination;
}

void System_SetAutoSendInterval( int interval )
{
  System_SetActive( 1 );
  if( interval < 0 || interval > 5000 )
    return;
  else
  {
    if( System->autoInterval != interval )
    {
      System->autoInterval = interval;
      Eeprom_Write( EEPROM_OSC_ASYNC_INTERVAL, (uchar*)&interval, 4 );
    }
  }
}

int System_GetAutoSendInterval( )
{
  System_SetActive( 1 );
  if( System->autoInterval == ASYNC_INIT )
  {
    int interval;
    Eeprom_Read( EEPROM_OSC_ASYNC_INTERVAL, (uchar*)&interval, 4 );
    if( interval >= 0 && interval <= 5000 )
      System->autoInterval = interval;
    else
      System->autoInterval = 10;
  }
  return System->autoInterval;
}

/** \defgroup SystemOSC System - OSC
  System controls many of the logistics of the Controller Board via OSC.
  \ingroup OSC
   
    \section devices Devices
    There's only one System, so a device index is not used in OSC messages to it.
   
    \section properties Properties
    System has eight properties:
    - name
    - freememory
    - samba
    - reset
    - serialnumber
    - version
    - stack-audit
    - task-report
    - active

    \par Name
    The \b name property allows you to give a board its own name.  The name can only contain 
    alphabetic characters and numbers.
    To set your board's name, send the message
    \verbatim /system/name "My Board"\endverbatim
    To read the board's name, send the message
    \verbatim /system/name \endverbatim
    The board will respond by sending back an OSC message with the board's name.
    
    \par Free Memory
    The \b freememory property corresponds to the amount of free memory on the Controller Board.
    This value is read-only.  To get the amount of free memory, send the message
    \verbatim /system/freememory \endverbatim
    The board will respond by sending back an OSC message with the amount of free memory.
   
    \par Samba
    The \b samba property is a write-only value that returns the board to a state in which it's ready
    to receive new firmware via SAM-BA or mchelper.  Once you've set the board to SAM-BA state,
    unplug and replug the power on the board before uploading new firmware.
    \par
    To set the board in SAM-BA state, send the message
    \verbatim /system/samba 1 \endverbatim
    and don't forget to power cycle the board.  Remember the board won't be able to send/receive OSC
    messages until a new program is uploaded to it.
   
    \par Reset
    The \b reset property is a write-only value that reboots the board.
    To reset the board, send the message
    \verbatim /system/reset 1 \endverbatim
   
    \par Serial Number
    The \b serialnumber property corresponds to the unique serial number on each Controller Board.
    This value can be used in situations where a unique value needs to be used to identify a board.
    The serial number can be both read and written.
    \par
    To read the board's serial number, send the message
    \verbatim /system/serialnumber \endverbatim
   
    \par Version
    The \b version property corresponds to the of the firmware currently running on the board.
    This is read-only.
    \par
    To read the board's version, send the message
    \verbatim /system/version \endverbatim

    \par Stack Audit
    The \b stack-audit property can fire up a task that will monitor the stack usage
    of all tasks running on the board.  If the remaining stack of any of the tasks drops below 50 bytes,
    the board will attempt to send an OSC message back via the \ref Debug system to let you know.
    \par
    This uses up quite a lot of processor time, so it's really only designed to be used in a 
    debug context.
    \par
    To start up the stack audit, send the message
    \verbatim /system/stack-audit 1 \endverbatim
    \par
    and turn it off by sending 
    \verbatim /system/stack-audit 0 \endverbatim
    
    \par Task Report
    The \b task-report property is a read-only property that will generate a list of all the tasks running 
    on your Make Controller, first giving the name of the task followed by the task's remaining stack.
    \par
    To see the tasks running on your board, send the message
    \verbatim /system/task-report \endverbatim
   
    \par Active
    The \b active property corresponds to the active state of System.
    If System is set to be inactive, it will not respond to any other OSC messages. 
    If you're not seeing appropriate
    responses to your messages to System, check the whether it's
    active by sending a message like
    \verbatim /system/active \endverbatim
    \par
    You can set the active flag by sending
    \verbatim /system/active 1 \endverbatim
*/

static char* SystemOsc_Name = "system";
static char* SystemOsc_PropertyNames[] = { "active", "freememory", "samba", "reset", 
                                            "serialnumber", "version",
                                            "name", "info-internal", "info", "stack-audit", 
                                            "task-report", "autosend-usb", "autosend-udp", "autosend-interval", 0 }; // must have a trailing 0

int SystemOsc_PropertySet( int property, char* typedata, int channel );
int SystemOsc_PropertyGet( int property, int channel );

const char* SystemOsc_GetName( void )
{
  return SystemOsc_Name;
}
// need to allow this to accept non-int arguments
int SystemOsc_ReceiveMessage( int channel, char* message, int length )
{
  int status = Osc_GeneralReceiverHelper( channel, message, length, 
                                SystemOsc_Name,
                                SystemOsc_PropertySet, SystemOsc_PropertyGet, 
                                SystemOsc_PropertyNames );

  if ( status != CONTROLLER_OK )
    return Osc_SendError( channel, SystemOsc_Name, status );

  return CONTROLLER_OK;
}

int SystemOsc_Poll( )
{
  return CONTROLLER_OK;
}

// Set the index LED, property with the value
int SystemOsc_PropertySet( int property, char* typedata, int channel )
{
  int value = 0;
  switch ( property )
  {
    case 0: // active
    {
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, SystemOsc_Name, "Incorrect data - need an int" );

      System_SetActive( value );
      break;
    }
    case 2: // samba
    {
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, SystemOsc_Name, "Incorrect data - need an int" );

      System_SetSamba( value );
      break;
    }
    case 3: // reset
    {
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, SystemOsc_Name, "Incorrect data - need an int" );

      System_SetReset( value );
      break;
    }
    case 4: // serialnumber
    {
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, SystemOsc_Name, "Incorrect data - need an int" );

      System_SetSerialNumber( value );
      break;
    }
    case 6: // name
    {
      char* address;
      int count = Osc_ExtractData( typedata, "s", &address );
      if ( count != 1 ) 
        return Osc_SubsystemError( channel, SystemOsc_Name, "Incorrect data - need a string" );

      System_SetName( address );
      break;
    }
    case 9: // stack-audit
    {
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, SystemOsc_Name, "Incorrect data - need an int" );

      System_StackAudit( value );
      break;
    }
    case 11: // autosend-usb
    {
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, SystemOsc_Name, "Incorrect data - need an int" );
      if( value )
        System_SetAsyncDestination( OSC_CHANNEL_USB );
      else
      {
        if( System_GetAsyncDestination( ) == OSC_CHANNEL_USB )
          System_SetAsyncDestination( ASYNC_INACTIVE );
      }
      break;
    }
    case 12: // autosend-udp
    {
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, SystemOsc_Name, "Incorrect data - need an int" );
      if( value )
        System_SetAsyncDestination( OSC_CHANNEL_UDP );
      else
      {
        if( System_GetAsyncDestination( ) == OSC_CHANNEL_UDP )
          System_SetAsyncDestination( ASYNC_INACTIVE );
      }
      break;
    }
    case 13: // autosend-interval
    {
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, SystemOsc_Name, "Incorrect data - need an int" );

      System_SetAutoSendInterval( value );
      break;
    }
  }
  return CONTROLLER_OK;
}

// Get the property
int SystemOsc_PropertyGet( int property, int channel )
{
  int value = 0;
  //char address[ OSC_SCRATCH_SIZE ];
  switch ( property )
  {
    case 0: // active
      value = System_GetActive( );
      snprintf( System->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", SystemOsc_Name, SystemOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, System->scratch1, ",i", value ); 
      break;
    case 1: // freememory
      value = System_GetFreeMemory( );
      snprintf( System->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", SystemOsc_Name, SystemOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, System->scratch1, ",i", value ); 
      break;
    case 4: // serialnumber
      value = System_GetSerialNumber( );
      snprintf( System->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", SystemOsc_Name, SystemOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, System->scratch1, ",i", value ); 
      break;  
    case 5: // version
    {
      char versionString[50];
      snprintf( versionString, 50, "%s %d.%d.%d", FIRMWARE_NAME, FIRMWARE_MAJOR_VERSION, FIRMWARE_MINOR_VERSION, FIRMWARE_BUILD_NUMBER );
      snprintf( System->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", SystemOsc_Name, SystemOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, System->scratch1, ",s", versionString ); 
      break;
    }
    case 6: // name
    {
      char* name;
      name = System_GetName( );
      snprintf( System->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", SystemOsc_Name, SystemOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, System->scratch1, ",s", name ); 
      break;
    }
    case 7: // info-internal
    case 8: // info
    {
      int a0, a1, a2, a3;
			{ // put these in their own context so the local variables aren't lying around on the stack for the whole message
				char ipAddr[25];
				char* sysName = System_GetName( );
				int serialnum = System_GetSerialNumber( );
				char sysVersion[25];
				snprintf( sysVersion, 25, "%s %d.%d.%d", FIRMWARE_NAME, FIRMWARE_MAJOR_VERSION, FIRMWARE_MINOR_VERSION, FIRMWARE_BUILD_NUMBER );
				int freemem = System_GetFreeMemory( );
				#ifdef MAKE_CTRL_NETWORK
				if( Network_GetAddress( &a0, &a1, &a2, &a3 ) != CONTROLLER_OK )
					a0 = a1 = a2 = a3 = -1;
				#else
				a0 = a1 = a2 = a3 = -1;
				#endif // MAKE_CTRL_NETWORK
				snprintf( ipAddr, 25, "%d.%d.%d.%d", a0, a1, a2, a3 );
				snprintf( System->scratch1, OSC_SCRATCH_SIZE, "/%s/%s-a", SystemOsc_Name, SystemOsc_PropertyNames[ property ] ); 
				Osc_CreateMessage( channel, System->scratch1, ",sissi", sysName, serialnum, ipAddr, sysVersion, freemem );
			}
			{
			char gateway[25];
      char mask[25];
			int dhcp;
      int webserver;
			int oscUdpListen;
			int oscUdpSend; 
      #ifdef MAKE_CTRL_NETWORK
      if( Network_GetGateway( &a0, &a1, &a2, &a3 ) != CONTROLLER_OK )
        a0 = a1 = a2 = a3 = -1;
			snprintf( gateway, 25, "%d.%d.%d.%d", a0, a1, a2, a3 );
      if( Network_GetMask( &a0, &a1, &a2, &a3 ) != CONTROLLER_OK )
        a0 = a1 = a2 = a3 = -1;
			snprintf( mask, 25, "%d.%d.%d.%d", a0, a1, a2, a3 );
      dhcp = Network_GetDhcpEnabled( );
      webserver = 0; // webserver no longer in core. TODO - update system-info to not use webserver
      oscUdpListen = NetworkOsc_GetUdpListenPort( );
      oscUdpSend = NetworkOsc_GetUdpSendPort( );
      #else
      a0 = a1 = a2 = a3 = -1;
			snprintf( gateway, 25, "%d.%d.%d.%d", a0, a1, a2, a3 );
			snprintf( mask, 25, "%d.%d.%d.%d", a0, a1, a2, a3 );
      dhcp = 0;
      webserver = 0;
      oscUdpListen = 0;
      oscUdpSend = 0;
      #endif // MAKE_CTRL_NETWORK
      
			snprintf( System->scratch1, OSC_SCRATCH_SIZE, "/%s/%s-b", SystemOsc_Name, SystemOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, System->scratch1, ",iissii", dhcp, webserver, gateway, mask, oscUdpListen, oscUdpSend );
			}
      break;
    }
    case 9: // stack-audit
      if( System->StackAuditPtr == NULL )
        value = 0;
      else
        value = 1;
      snprintf( System->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", SystemOsc_Name, SystemOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, System->scratch1, ",i", value ); 
      break;
    case 10: // task-report
    {
      int numOfTasks = GetNumberOfTasks( ) - 1;  // don't count the IDLE task
      int i;
      void* task = NULL;
      char* taskName = "";
      snprintf( System->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", SystemOsc_Name, SystemOsc_PropertyNames[ property ] ); 
      
      for( i = 0; i < numOfTasks; i++ )
      {
      	task = TaskGetNext( task );
      	value = TaskGetRemainingStack( task );
      	taskName = TaskGetName( task );
      	Osc_CreateMessage( channel, System->scratch1, ",si", taskName, value ); 
      }
      break;
    }
    case 11: // autosend-usb
      value = ( System_GetAsyncDestination( ) == OSC_CHANNEL_USB ) ? 1 : 0;
      snprintf( System->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", SystemOsc_Name, SystemOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, System->scratch1, ",i", value ); 
      break;
    case 12: // autosend-udp
      value = ( System_GetAsyncDestination( ) == OSC_CHANNEL_UDP ) ? 1 : 0;
      snprintf( System->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", SystemOsc_Name, SystemOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, System->scratch1, ",i", value ); 
      break;
    case 13: // autosend-interval
      value = System_GetAutoSendInterval( );
      snprintf( System->scratch1, OSC_SCRATCH_SIZE, "/%s/%s", SystemOsc_Name, SystemOsc_PropertyNames[ property ] ); 
      Osc_CreateMessage( channel, System->scratch1, ",i", value ); 
      break; 
  }
  
  return CONTROLLER_OK;
}

#endif // OSC
