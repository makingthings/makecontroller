/*
	ctester.h

*/

#ifndef CTESTER_H
#define CTESTER_H

#ifdef FACTORY_TESTING

int CTester_SetTesteePower( int level );
int CTester_GetTesteePower( void );
int CTester_GetTesteeCurrent( void );
int CTester_GetTesteeVoltage( void );
int CTester_SetIoPattern( int level );
int CTester_GetIoPattern( void );
int CTester_GetIoTest( void );
int CTester_GetCanIn( void );
int CTester_GetCanOut( void );
int CTester_SetCanOut( int mode );

/* OSC Interface */
const char* CTesterOsc_GetName( void );
int CTesterOsc_ReceiveMessage( int channel, char* message, int length );

#endif

#endif // FACTORY_TESTING
