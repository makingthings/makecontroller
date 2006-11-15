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

/*
	osc.h
*/

#include "types.h"

#ifndef OSC_H
#define OSC_H

#define OSC_SUBSYSTEM_COUNT  14

#define OSC_SCRATCH_SIZE    100

#define OSC_CHANNEL_UDP 0
#define OSC_CHANNEL_USB 1

// Top level functions
int Osc_SetActive( int state );
int Osc_GetActive( void );
int Osc_GetRunning( void );

int Osc_CreateMessage( int channel, char* address, char* format, ... );
int Osc_SendPacket( int channel );

int Osc_RegisterSubsystem( int subsystem,
                           const char *name, 
                           int (*Subsystem_ReceiveMessage)( int channel, char* buffer, int length ), 
                           int (*Subsystem_Poll)( int channel ) );

// Helpers for processing messages - they return error codes as spec'ed above
int Osc_IntReceiverHelper( int channel, char* message, int length, 
                           char* subsystemName,
                           int (*propertySet)( int property, int value ),
                           int (*propertyGet)( int property ),
                           char* propertyNames[] );

int Osc_IndexIntReceiverHelper( int channel, char* message, int length, 
                                int indexCount, char* subsystemName,
                                int (*propertySet)( int index, int property, int value ),
                                int (*propertyGet)( int index, int property ),
                                char* propertyNames[] );

int Osc_BlobReceiverHelper( int channel, char* message, int length, 
                           char* subsystemName, 
                           int (*blobPropertySet)( int property, uchar* buffer, int length ),
                           int (*blobPropertyGet)( int property, uchar* buffer, int size ),
                           char* blobPropertyNames[] );

int Osc_GeneralReceiverHelper( int channel, char* message, int length, 
                           char* subsystemName, 
                           int (*propertySet)( int property, char* typedata, int channel ),
                           int (*propertyGet)( int property, int channel ),
                           char* propertyNames[] );

int Osc_SendError( int channel, char* subsystemName, int error );


// Functions needed to process messages
int Osc_SubsystemError( int channel, char* subsystem, char* string );
int Osc_PropertyLookup( char** properties, char* property );
char *Osc_FindDataTag( char* message, int length );
int Osc_ExtractData( char* buffer, char* format, ... );
int  Osc_NumberMatch( int max, char* pattern, int* fancy );

// Pattern Match Stuff
bool Osc_PatternMatch(const char *  pattern, const char * test);

#endif
