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

#ifndef _SYSTEM_H
#define _SYSTEM_H

#define SYSTEM_MAX_NAME 99 


class System
{
  public:
    static System* get();
    int setSamba( int sure );
    int reset( int sure );
    int setSerialNumber( int serial );
    int getSerialNumber();
    int setName( char* name );
    char* getName( void );
    void stackAudit( int on_off );
    int getFreeMemory();
  
  protected:
    System();
    static System* _instance;
//    static char[] name;
};

//
//int System_SetActive( int state );
//int System_GetActive( void );
//int System_GetFreeMemory( void );
//int System_GetSerialNumber( void );
//
//
//void System_SetAsyncDestination( int dest );
//int System_GetAsyncDestination( void );
//void System_SetAutoSendInterval( int interval );
//int System_GetAutoSendInterval( void );
//
//
//
///* SystemOsc Interface */
//
//const char* SystemOsc_GetName( void );
//int SystemOsc_ReceiveMessage( int channel, char* message, int length );
//int SystemOsc_Poll( void );

#endif
