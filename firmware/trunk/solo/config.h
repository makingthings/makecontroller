/*
	config.h

  Select which features & hardware you're using.

  MakingThings
*/

#ifndef CONFIG_H
#define CONFIG_H

/* 
   Define the subsystems 
*/

#define OSC

#include "controller.h"   // ...everybody uses the MAKE Controller Board

#define FIRMWARE_VERSION_NUMBER 110
#define FIRMWARE_BUILD_NUMBER     1

#define CONTROLLER_HEAPSIZE 16000

/********************************************************
  Uncomment the revision of the MAKE Controller Board you're using.
*********************************************************/
//#define CONTROLLER_VERSION    50    // Atmel SAM7X eval. board
//#define CONTROLLER_VERSION    90     
//#define CONTROLLER_VERSION  95       
#define CONTROLLER_VERSION  100         

// Error message defines
#define CONTROLLER_OK                                0
#define CONTROLLER_ERROR_CANT_LOCK                  -1
#define CONTROLLER_ERROR_TOO_MANY_USERS             -2
#define CONTROLLER_ERROR_TOO_MANY_STOPS             -3
#define CONTROLLER_ERROR_NOT_LOCKED                 -4
#define CONTROLLER_ERROR_ILLEGAL_INDEX              -5
#define CONTROLLER_ERROR_ILLEGAL_ID                 -6
#define CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE    -7
#define CONTROLLER_ERROR_NOT_OPEN                   -8
#define CONTROLLER_ERROR_INSUFFICIENT_RESOURCES     -9
#define CONTROLLER_ERROR_BAD_DATA                   -10
#define CONTROLLER_ERROR_NO_SPACE                   -11
#define CONTROLLER_ERROR_RESOURCE_MISSING           -12
#define CONTROLLER_ERROR_NO_ADDRESS                 -13
#define CONTROLLER_ERROR_TIMEOUT                    -14
#define CONTROLLER_ERROR_BAD_ADDRESS                -15
#define CONTROLLER_ERROR_BAD_FORMAT                 -16
#define CONTROLLER_ERROR_SUBSYSTEM_INACTIVE         -17
#define CONTROLLER_ERROR_QUEUE_ERROR                -18
#define CONTROLLER_ERROR_UNKNOWN_PROPERTY           -19
#define CONTROLLER_ERROR_INCORRECT_DATA_TYPE        -20
#define CONTROLLER_ERROR_NO_TYPE_TAG                -21
#define CONTROLLER_ERROR_NO_PROPERTY                -22

#define CONTROLLER_ERROR_COUNT_MISMATCH             -100
#define CONTROLLER_ERROR_START_FAILED               -101
#define CONTROLLER_ERROR_STOP_FAILED                -102
#define CONTROLLER_ERROR_WRONG_USER_COUNT           -103
#define CONTROLLER_ERROR_DATA_STRUCTURE_SIZE_WRONG  -104
#define CONTROLLER_ERROR_INCORRECT_INIT             -105
#define CONTROLLER_ERROR_INCORRECT_DEINIT           -106
#define CONTROLLER_ERROR_LOCK_ERROR                 -107
#define CONTROLLER_ERROR_LOCK_GRANTED_ERROR         -108
#define CONTROLLER_ERROR_USE_GRANTED_ERROR          -109
#define CONTROLLER_ERROR_INITIALIZATION             -110

#endif /* CONFIG_H */
