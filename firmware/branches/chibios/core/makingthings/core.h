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

#include "types.h"
#include "error.h"
#include "config.h"

#ifdef __cplusplus

#include "rtos.h"
#include "network.h"
#include "usb_serial.h"
#include "system.h"
#include "analogin.h"
#include "led.h"
#include "timer.h"
#include "fasttimer.h"
#include "pwm.h"

extern "C" {
#endif

// C-only business in here
void Run( void );
void kill( void );
void AIC_ConfigureIT(unsigned int source, unsigned int mode, void (*handler)( void ));

#ifdef __cplusplus
}
#endif

#endif // CORE_H
