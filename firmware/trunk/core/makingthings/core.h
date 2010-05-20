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

// helper macros for managing threads
/**
  Define a thread.
  @param name The name of your thread - use this in createThread().
  @param stack The amount of memory required for this task.  512 is a good default, then tune as needed.
  \ingroup rtos
*/
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

/**
  Create and run a new thread.
  @param name The name of your thread, as specified in threadLoop().
  @param priority Anywhere from 1 to 128.
  \ingroup rtos
*/
#define createThread(name, priority)                                                  \
{                                                                                     \
  name = chThdCreateStatic(name##_WA, sizeof(name##_WA), priority, name##_Thd, NULL); \
}

/**
  Delay the current thread.
  @param millis The number of milliseconds to sleep.
  \ingroup rtos
*/
#define sleep(millis) chThdSleepMilliseconds(millis)
/**
  Pass control to the next waiting thread.
  \ingroup rtos
*/
#define yield()  chThdYield()

#ifdef __cplusplus

extern "C" {
#endif
// C-only business in here

// for user code
void setup(void);
void loop(void);
void kill(void);

#include "ch.h"
#include "hal.h"

// all the basics
#include "types.h"
#include "error.h"
#include "config.h"
#include "pin.h"
#include "system.h"
#include "pwm.h"
#include "serial.h"
#include "spi.h"
#include "eeprom.h"
//#include "timer.h"
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

#ifdef __cplusplus
}
#endif

#endif // CORE_H
