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

#include "io.h"
#include "eeprom.h"
#include "system.h"
#include "config.h"
#include "AT91SAM7X256.h"

int System_Start( void );
int System_Stop( void );
int PortFreeMemory( void );

int System_users;

/** \defgroup System
    The System subsystem monitors and controls several aspects of the system. 
    From OSC this subsystem can be addressed as "system".  It has the following 
    properties:
    \li active (R/W) - not currently meaningful
    \li freememory (R) - returns the remaining free memory size
    \li samba (W) - requests the board erase itself and return to SAMBA state
    \li reset (W) - requests the board reboot
    \li serialnumber (R/W) - permits the reading and writing of the serial number 
    \li buildnumber (R) - permits the reading of the board's build number

* \ingroup Controller
* @{
*/

/**
	Sets whether the System control is active.   Presently this function has no material effect.
	@param state An integer specifying the state system
	@return Zero on success.
*/
int System_SetActive( int state )
{
  if ( state )
    return System_Start(  );
  else
    return System_Stop(  );
}

/**
	Returns the active state of the subsystem.
	@return The active state of the subsystem - 1 (active) or 0 (inactive).
*/
int System_GetActive( )
{
  return System_users > 0;
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
    return serial;
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
  return Eeprom_Write( EEPROM_SYSTEM_SERIAL_NUMBER, (uchar*)&serial, 4 );
}

/**
	Returns the build number of this codebase.
	@return The size free memory.
  \todo this should probably be just a compiler definition, not an EEPROM value
*/
int System_GetBuildNumber( void )
{
  int build;
  if ( Eeprom_Read( EEPROM_SYSTEM_BUILD_NUMBER, (uchar*)&build, 4 ) == CONTROLLER_OK )
    return build;
  return 0;
}

/**
	Sets the new Build Number.
	@return CONTROLLER_OK if OK.
*/
int System_SetBuildNumber( int serial )
{
  return Eeprom_Write( EEPROM_SYSTEM_BUILD_NUMBER, (uchar*)&serial, 4 );
}

/**
	Returns the board to SAMBA mode, erasing all of FLASH.
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
	Reset the board.
  Will reset the board if the parameter sure is true/
  @param sure confirms the request if true.
	@return CONTROLLER_OK if OK.
*/
int System_SetReset( int sure )
{
  if ( sure )
    AT91C_BASE_RSTC->RSTC_RCR = ( AT91C_RSTC_EXTRST | AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST | (0xA5 << 24 ) );

  return 1;
}

/** @}
*/

int System_Start()
{
  // int status;
  if ( System_users++ == 0 )
  {
  }
  return CONTROLLER_OK;
}

int System_Stop()
{
  if ( System_users <= 0 )
    return CONTROLLER_ERROR_TOO_MANY_STOPS;
  
  if ( --System_users == 0 )
  {
  }

  return CONTROLLER_OK;
}

/* SystemOsc Interface */

#include "osc.h"

static char* SystemOsc_Name = "system";
static char* SystemOsc_PropertyNames[] = { "active", "freememory", "samba", "reset", "serialnumber", "buildnumber", 0 }; // must have a trailing 0

int SystemOsc_PropertySet( int property, int value );
int SystemOsc_PropertyGet( int property );

const char* SystemOsc_GetName( void )
{
  return SystemOsc_Name;
}

int SystemOsc_ReceiveMessage( int channel, char* message, int length )
{
  return Osc_IntReceiverHelper( channel, message, length, 
                                SystemOsc_Name,
                                SystemOsc_PropertySet, SystemOsc_PropertyGet, 
                                SystemOsc_PropertyNames );
}

int SystemOsc_Poll( )
{
  return CONTROLLER_OK;
}

// Set the index LED, property with the value
int SystemOsc_PropertySet( int property, int value )
{
  switch ( property )
  {
    case 0: 
      System_SetActive( value );
      break;      
    case 2:
      System_SetSamba( value );
      break;
    case 3:
      System_SetReset( value );
      break;
    case 4:
      System_SetSerialNumber( value );
      break;
    case 5:
      System_SetBuildNumber( value );
      break;
  }
  return CONTROLLER_OK;
}

// Get the property
int SystemOsc_PropertyGet( int property )
{
  int value = 0;
  switch ( property )
  {
    case 0:
      value = System_GetActive( );
      break;
    case 1:
      value = System_GetFreeMemory( );
      break;
    case 4:
      value = System_GetSerialNumber( );
      break;  
    case 5:
      value = System_GetBuildNumber( );
      break;  
  }
  
  return value;
}

