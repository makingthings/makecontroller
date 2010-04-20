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

/*
  General includes for core controller functionality.
  Be sure to avoid including c++ header files in c code.
*/

#ifndef CORE_H
#define CORE_H

#define UNUSED(x) (void)x;
#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

#ifdef __cplusplus

#include "rtos.h"
#include "timer.h"
#include "fasttimer.h"

extern "C" {
#endif
// C-only business in here

// all the basics
#include "ch.h"
#include "hal.h"
#include "types.h"
#include "error.h"
#include "config.h"
#include "pin.h"
#include "system.h"
#include "pwm.h"
#include "serial.h"
#include "spi.h"
#ifdef MAKE_CTRL_NETWORK
#include "network.h"
#include "udpsocket.h"
#include "tcpsocket.h"
#include "tcpserver.h"
#endif // MAKE_CTRL_NETWORK
#ifdef MAKE_CTRL_USB
#include "usbserial.h"
#endif
#include "led.h"
#include "analogin.h"

void Run( void );
void kill( void );

#ifdef __cplusplus
}
#endif

#endif // CORE_H
