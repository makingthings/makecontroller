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

#ifndef _SYSTEM_H
#define _SYSTEM_H

#include "core.h"

#define SYSTEM_MAX_NAME 99 

/** 
  Monitors and controls several aspects of the system. 

*/
class System
{
  public:
    static System* get();

    int samba( int sure );
    int reset( int sure );

    int serialNumber();
    int setSerialNumber( int serial );

    char* name( );
    int setName( char* name );
    
    void stackAudit( int on_off );
    int freeMemory();

    int autosendDestination( );
    void setAutosendDestination( int dest );
    
    int autosendInterval( );
    void setAutosendInterval( int interval );
  
  protected:
    System();
    static System* _instance;
    char _name[SYSTEM_MAX_NAME + 1];
    Task* stackAuditTask;
    #ifdef OSC
//    char scratch1[ OSC_SCRATCH_SIZE ];
    int _autoDestination;
    int _autoInterval;
    #endif
};

///* SystemOsc Interface */
//
//const char* SystemOsc_GetName( void );
//int SystemOsc_ReceiveMessage( int channel, char* message, int length );
//int SystemOsc_Poll( void );

#endif
