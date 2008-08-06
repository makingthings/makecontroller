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

/*
	error.h
  Error return codes.
*/

#ifndef ERROR_H
#define ERROR_H

/**
\defgroup Error Error Return Codes
Error return values for Make Controller API calls.
\ingroup Core
@{
*/

/** All's well here */
#define CONTROLLER_OK                                0
/** Can't get a lock on a resource */
#define CONTROLLER_ERROR_CANT_LOCK                  -1
/** There are too many other users on this resource */
#define CONTROLLER_ERROR_TOO_MANY_USERS             -2
/** This resource has already been stopped too many times */
#define CONTROLLER_ERROR_TOO_MANY_STOPS             -3
/** Lock attempt was unsuccessful */
#define CONTROLLER_ERROR_NOT_LOCKED                 -4
/** The requested index is not valid */
#define CONTROLLER_ERROR_ILLEGAL_INDEX              -5
/** The requested ID is not valid */
#define CONTROLLER_ERROR_ILLEGAL_ID                 -6
/** Parameter is not valid */
#define CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE    -7
/** Resource is not open */
#define CONTROLLER_ERROR_NOT_OPEN                   -8
/** Not enough memory available */
#define CONTROLLER_ERROR_INSUFFICIENT_RESOURCES     -9
/** Error reading data */
#define CONTROLLER_ERROR_BAD_DATA                   -10
/** Not enough space for the requested operation */
#define CONTROLLER_ERROR_NO_SPACE                   -11
/** Resource is missing */
#define CONTROLLER_ERROR_RESOURCE_MISSING           -12
/** Board doesn't have an address */
#define CONTROLLER_ERROR_NO_ADDRESS                 -13
/** The operation has timed out */
#define CONTROLLER_ERROR_TIMEOUT                    -14
/** Address is not valid */
#define CONTROLLER_ERROR_BAD_ADDRESS                -15
/** Data passed in the wrong format */
#define CONTROLLER_ERROR_BAD_FORMAT                 -16
/** Couldn't complete operation because the subsystem is inactive */
#define CONTROLLER_ERROR_SUBSYSTEM_INACTIVE         -17
/** An error with the queue */
#define CONTROLLER_ERROR_QUEUE_ERROR                -18
/** Property specified is not valid */
#define CONTROLLER_ERROR_UNKNOWN_PROPERTY           -19
/** OSC data type is not valid */
#define CONTROLLER_ERROR_INCORRECT_DATA_TYPE        -20
/** Couldn't find the type tag in this OSC message */
#define CONTROLLER_ERROR_NO_TYPE_TAG                -21
/** No OSC property specified */
#define CONTROLLER_ERROR_NO_PROPERTY                -22
/** The network is not up */
#define CONTROLLER_ERROR_NO_NETWORK                 -23
/** Specified string is too long */
#define CONTROLLER_ERROR_STRING_TOO_LONG            -24
/** The system is not active */
#define CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE          -25
/** A requested write was unsuccessful */
#define CONTROLLER_ERROR_WRITE_FAILED               -26
/** Not enough memory available for the requested operation */
#define CONTROLLER_ERROR_INSUFFICIENT_MEMORY        -27
/** Task did not start successfully */
#define CONTROLLER_ERROR_CANT_START_TASK            -28

/** Unexpected count returned */
#define CONTROLLER_ERROR_COUNT_MISMATCH             -100
/** Requested operation failed to start */
#define CONTROLLER_ERROR_START_FAILED               -101
/** Requested stop operation failed */
#define CONTROLLER_ERROR_STOP_FAILED                -102
/** The user count is not valid */
#define CONTROLLER_ERROR_WRONG_USER_COUNT           -103
/** A data structure's size was not valid */
#define CONTROLLER_ERROR_DATA_STRUCTURE_SIZE_WRONG  -104
/** Initialization was not successful */
#define CONTROLLER_ERROR_INCORRECT_INIT             -105
/** De-initialization was not successful */
#define CONTROLLER_ERROR_INCORRECT_DEINIT           -106
/** An error occurred while trying to lock */
#define CONTROLLER_ERROR_LOCK_ERROR                 -107
/** An error occurred when granting a lock */
#define CONTROLLER_ERROR_LOCK_GRANTED_ERROR         -108
/** A use error occurred */
#define CONTROLLER_ERROR_USE_GRANTED_ERROR          -109
/** Initialization was not successful */
#define CONTROLLER_ERROR_INITIALIZATION             -110

/* @} */
#endif // ERROR_H


