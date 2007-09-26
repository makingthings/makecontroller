/*********************************************************************************

 Copyright 2006 MakingThings

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
#include "network.h"

int PortFreeMemory( void );
void StackAuditTask( void* p );
void kill( void );

typedef struct System_
{
  char name[ SYSTEM_MAX_NAME + 1 ]; // allotted EEPROM space is 100, but leave room for \0!
  int users;
  void* StackAuditPtr;
  #ifdef OSC
  char scratch1[ OSC_SCRATCH_SIZE ];
  #endif // OSC
} SystemS;

SystemS* System;

/** \defgroup System
    The System subsystem monitors and controls several aspects of the system. 

* \ingroup Controller
* @{
*/

/**
	Sets whether the System subsystem is active.
	@param state An integer specifying the state system
	@return Zero on success.
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
*/
int System_GetActive( )
{
  return System != NULL;
}

/**
	Returns the free size of the heap.
	@return The size free memory.
*/
int System_GetFreeMemory( void )
{
  return PortFreeMemory();
}

/**
	Gets the board's Serial Number.  
	@return CONTROLLER_OK ( = 0 ).
*/
int System_GetSerialNumber( void )
{
  int serial;
  if ( Eeprom_Read( EEPROM_SYSTEM_SERIAL_NUMBER, (uchar*)&serial, 4 ) == CONTROLLER_OK )
    return serial & 0xFFFF;
  return 0;
}

/**
	Sets the Serial Number. Note that this can be changed by the user at 
  any time, but that it is used in the \ref Network subsystem to form the last 
  two bytes of the network MAC address, so ideally units on the same network
  should have unique serial numbers.
	@return CONTROLLER_OK if OK.
*/
int System_SetSerialNumber( int serial )
{
  serial &= 0xFFFF;
  return Eeprom_Write( EEPROM_SYSTEM_SERIAL_NUMBER, (uchar*)&serial, 4 );
}

/**
	Returns the board to SAM-BA mode, erasing the flash memory.
  Leaves the board in a non-deterministic state awaiting a power cycle or reset.
	@return CONTROLLER_OK if OK.
*/
int System_SetSamba( int sure )
{
  if ( sure )
  {
    // Wait for End Of Programming
    while( !(AT91C_BASE_MC->MC_FSR & AT91C_MC_FRDY) );
    
    // Send Boot From Flash Command
    AT91C_BASE_MC->MC_FCR = (AT91C_MC_FCMD_CLR_GP_NVM |  (( (2) << 8) & AT91C_MC_PAGEN) | (0x5A << 24));

    AT91C_BASE_RSTC->RSTC_RCR = ( AT91C_RSTC_EXTRST | AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST | (0xA5 << 24 ) );

    // Wait for End Of Programming
    while( !(AT91C_BASE_MC->MC_FSR & AT91C_MC_FRDY) );
  
    if ( AT91C_BASE_MC->MC_FSR & AT91C_MC_PROGE )
      return 0;
  
    if ( AT91C_BASE_MC->MC_FSR & AT91C_MC_GPNVM2 )
      return 0;
  }

  // Never will do this
  return 1;
}

/**
	Gives the board a name.
  The name must be alpha-numeric - only letters and numbers.
  @param name A string specifying the board's name
	@return CONTROLLER_OK if OK.
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
	@return CONTROLLER_OK if OK.
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
                                            "name", "info-internal", "info", "stack-audit", "task-report", 0 }; // must have a trailing 0

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
  switch ( property )
  {
    case 0: // active
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, SystemOsc_Name, "Incorrect data - need an int" );

      System_SetActive( value );
      break;
    }
    case 2: // samba
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, SystemOsc_Name, "Incorrect data - need an int" );

      System_SetSamba( value );
      break;
    }
    case 3: // reset
    {
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, SystemOsc_Name, "Incorrect data - need an int" );

      System_SetReset( value );
      break;
    }
    case 4: // serialnumber
    {
      int value;
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
      int value;
      int count = Osc_ExtractData( typedata, "i", &value );
      if ( count != 1 )
        return Osc_SubsystemError( channel, SystemOsc_Name, "Incorrect data - need an int" );

      System_StackAudit( value );
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
      webserver = Network_GetWebServerEnabled( );
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
  }
  
  return CONTROLLER_OK;
}

#endif // OSC
