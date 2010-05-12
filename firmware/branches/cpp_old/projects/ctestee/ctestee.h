/*
	ctestee.h

*/

#ifndef CTESTEE_H
#define CTESTEE_H

int CTestee_SetIoPattern( int pattern );
int CTestee_GetIoPattern( void );
int CTestee_GetCanIn( void );
int CTestee_GetCanOut( void );
int CTestee_SetCanOut( int mode );

/* OSC Interface */
const char* CTesteeOsc_GetName( void );
int CTesteeOsc_ReceiveMessage( int channel, char* message, int length );

#endif 
