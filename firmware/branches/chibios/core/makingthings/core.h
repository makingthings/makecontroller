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

#define sleep(m) chThdSleepMilliseconds(m)
#define yield()  chThdYield()

// helper macros for managing threads
#define threadLoop(name, stack)         \
WORKING_AREA(name##_WA, stack);         \
static void name##Function(void);       \
Thread* name;                           \
static msg_t name##_Thd(void *arg)      \
{                                       \
  (void)arg;                            \
  while (!chThdShouldTerminate())       \
    name##Function();                   \
  return 0;                             \
}                                       \
void name##Function()

#define createThread(name, priority)                                                  \
{                                                                                     \
  name = chThdCreateStatic(name##_WA, sizeof(name##_WA), priority, name##_Thd, NULL); \
}

#ifdef __cplusplus

#include "rtos.h"
#include "timer.h"

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
#include "fasttimer.h"
#include "led.h"
#include "analogin.h"
#ifdef MAKE_CTRL_NETWORK
#include "network.h"
#include "udpsocket.h"
#include "tcpsocket.h"
#include "tcpserver.h"
#endif // MAKE_CTRL_NETWORK
#ifdef MAKE_CTRL_USB
#include "usbserial.h"
#endif

void Run( void );
void kill( void );

#ifdef __cplusplus
}
#endif

#endif // CORE_H
