/*
	ctestee.h

*/

#ifndef CTESTEE_H
#define CTESTEE_H

int CTestee_SetTesteePower( int level );

/* OSC Interface */
const char* CTesteeOsc_GetName( void );
int CTesteeOsc_ReceiveMessage( int channel, char* message, int length );

#endif
