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
	error.h
  Error return codes.
*/

#ifndef ERROR_H
#define ERROR_H

/**
\defgroup Error Error Return Codes
\ingroup Controller
@{
*/

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
#define CONTROLLER_ERROR_NO_NETWORK                 -23
#define CONTROLLER_ERROR_STRING_TOO_LONG            -24
#define CONTROLLER_ERROR_SYSTEM_NOT_ACTIVE          -25

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

/* @} */
#endif // ERROR_H


