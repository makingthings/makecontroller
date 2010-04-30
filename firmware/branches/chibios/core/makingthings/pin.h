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


#ifndef PIN_H
#define PIN_H

#include "config.h"
#include "types.h"
#include "ch.h"
#include "hal.h"

/**
 * A handler for use with pinAddInterruptHandler().
 */
typedef void (*PinInterruptHandler)(void*);

/**
 * Available pin modes.
 */
typedef enum {
  INPUT = PAL_MODE_INPUT,
  INPUT_PULLUP = PAL_MODE_INPUT_PULLUP,
  OUTPUT = PAL_MODE_OUTPUT_PUSHPULL,
  OUTPUT_OPENDRAIN = PAL_MODE_OUTPUT_OPENDRAIN,
  PERIPHERAL_A,
  PERIPHERAL_B,
  PULLUP_ON,
  PULLUP_OFF,
  GLITCH_FILTER_ON,
  GLITCH_FILTER_OFF
} PinMode;

#if defined(SIMULATOR)
typedef sim_vio_port_t* Group;
#else
typedef AT91S_PIO* Group;
#endif
#define GROUP_A IOPORT1
#define GROUP_B IOPORT2

/**
  \defgroup PinOptions Pin Options
  Pins for each of the processor's IO lines.
  \ingroup Core
  @{
*/
typedef enum {
  PIN_PA0  = 0,  /**< Pin 0, Port A */
  PIN_PA1  = 1,  /**< Pin 1, Port A */
  PIN_PA2  = 2,  /**< Pin 2, Port A */
  PIN_PA3  = 3,  /**< Pin 3, Port A */
  PIN_PA4  = 4,  /**< Pin 4, Port A */
  PIN_PA5  = 5,  /**< Pin 5, Port A */
  PIN_PA6  = 6,  /**< Pin 6, Port A */
  PIN_PA7  = 7,  /**< Pin 7, Port A */
  PIN_PA8  = 8,  /**< Pin 8, Port A */
  PIN_PA9  = 9,  /**< Pin 9, Port A */
  PIN_PA10 = 10, /**< Pin 10, Port A */
  PIN_PA11 = 11, /**< Pin 11, Port A */
  PIN_PA12 = 12, /**< Pin 12, Port A */
  PIN_PA13 = 13, /**< Pin 13, Port A */
  PIN_PA14 = 14, /**< Pin 14, Port A */
  PIN_PA15 = 15, /**< Pin 15, Port A */
  PIN_PA16 = 16, /**< Pin 16, Port A */
  PIN_PA17 = 17, /**< Pin 17, Port A */
  PIN_PA18 = 18, /**< Pin 18, Port A */
  PIN_PA19 = 19, /**< Pin 19, Port A */
  PIN_PA20 = 20, /**< Pin 20, Port A */
  PIN_PA21 = 21, /**< Pin 21, Port A */
  PIN_PA22 = 22, /**< Pin 22, Port A */
  PIN_PA23 = 23, /**< Pin 23, Port A */
  PIN_PA24 = 24, /**< Pin 24, Port A */
  PIN_PA25 = 25, /**< Pin 25, Port A */
  PIN_PA26 = 26, /**< Pin 26, Port A */
  PIN_PA27 = 27, /**< Pin 27, Port A */
  PIN_PA28 = 28, /**< Pin 28, Port A */
  PIN_PA29 = 29, /**< Pin 29, Port A */
  PIN_PA30 = 30, /**< Pin 30, Port A */
  PIN_PA31 = 31, /**< Pin 31, Port A */

  PIN_PB00 = 32, /**< Pin 0, Port B */
  PIN_PB01 = 33, /**< Pin 1, Port B */
  PIN_PB02 = 34, /**< Pin 2, Port B */
  PIN_PB03 = 35, /**< Pin 3, Port B */
  PIN_PB04 = 36, /**< Pin 4, Port B */
  PIN_PB05 = 37, /**< Pin 5, Port B */
  PIN_PB06 = 38, /**< Pin 6, Port B */
  PIN_PB07 = 39, /**< Pin 7, Port B */
  PIN_PB08 = 40, /**< Pin 8, Port B */
  PIN_PB09 = 41, /**< Pin 9, Port B */
  PIN_PB10 = 42, /**< Pin 10, Port B */
  PIN_PB11 = 43, /**< Pin 11, Port B */
  PIN_PB12 = 44, /**< Pin 12, Port B */
  PIN_PB13 = 45, /**< Pin 13, Port B */
  PIN_PB14 = 46, /**< Pin 14, Port B */
  PIN_PB15 = 47, /**< Pin 15, Port B */
  PIN_PB16 = 48, /**< Pin 16, Port B */
  PIN_PB17 = 49, /**< Pin 17, Port B */
  PIN_PB18 = 50, /**< Pin 18, Port B */
  PIN_PB19 = 51, /**< Pin 19, Port B */
  PIN_PB20 = 52, /**< Pin 20, Port B */
  PIN_PB21 = 53, /**< Pin 21, Port B */
  PIN_PB22 = 54, /**< Pin 22, Port B */
  PIN_PB23 = 55, /**< Pin 23, Port B */
  PIN_PB24 = 56, /**< Pin 24, Port B */
  PIN_PB25 = 57, /**< Pin 25, Port B */
  PIN_PB26 = 58, /**< Pin 26, Port B */
  PIN_PB27 = 59, /**< Pin 27, Port B */
  PIN_PB28 = 60, /**< Pin 28, Port B */
  PIN_PB29 = 61, /**< Pin 29, Port B */
  PIN_PB30 = 62, /**< Pin 30, Port B */
  PIN_PB31 = 63, /**< Pin 31, Port B */
} Pin;

/** @} */

#ifdef __cplusplus
extern "C" {
#endif
bool pinValue(Pin pin);
int  pinGroupValue(Group port);
void pinSetValue(Pin pin, bool value);
void pinGroupSetValue(Group port, int pins, bool value);
void pinOn(Pin pin);
void pinGroupOn(Group port, int pins);
void pinOff(Pin pin);
void pinGroupOff(Group port, int pins);
void pinSetMode(Pin pin, PinMode mode);
void pinGroupSetMode(Group port, int pins, PinMode mode);
#ifndef PIN_NO_ISR
bool pinAddInterruptHandler(Pin pin, PinInterruptHandler h, void* arg);
void pinDisableHandler(Pin pin);
void pinEnableHandler(Pin pin);
#endif // PIN_NO_ISR
#ifdef __cplusplus
}
#endif

#ifdef OSC
#include "osc.h"
extern const OscNode pinOsc;
#endif

/**
  \defgroup PinBits Pin Bits
  \ingroup Core
  @{
*/
#define PIN_PA0_BIT  (1 << 0)  /**< Pin 0, Port A */
#define PIN_PA1_BIT  (1 << 1)  /**< Pin 1, Port A */
#define PIN_PA2_BIT  (1 << 2)  /**< Pin 2, Port A */
#define PIN_PA3_BIT  (1 << 3)  /**< Pin 3, Port A */
#define PIN_PA4_BIT  (1 << 4)  /**< Pin 4, Port A */
#define PIN_PA5_BIT  (1 << 5)  /**< Pin 5, Port A */
#define PIN_PA6_BIT  (1 << 6)  /**< Pin 6, Port A */
#define PIN_PA7_BIT  (1 << 7)  /**< Pin 7, Port A */
#define PIN_PA8_BIT  (1 << 8)  /**< Pin 8, Port A */
#define PIN_PA9_BIT  (1 << 9)  /**< Pin 9, Port A */
#define PIN_PA10_BIT (1 << 10) /**< Pin 10, Port A */
#define PIN_PA11_BIT (1 << 11) /**< Pin 11, Port A */
#define PIN_PA12_BIT (1 << 12) /**< Pin 12, Port A */
#define PIN_PA13_BIT (1 << 13) /**< Pin 13, Port A */
#define PIN_PA14_BIT (1 << 14) /**< Pin 14, Port A */
#define PIN_PA15_BIT (1 << 15) /**< Pin 15, Port A */
#define PIN_PA16_BIT (1 << 16) /**< Pin 16, Port A */
#define PIN_PA17_BIT (1 << 17) /**< Pin 17, Port A */
#define PIN_PA18_BIT (1 << 18) /**< Pin 18, Port A */
#define PIN_PA19_BIT (1 << 19) /**< Pin 19, Port A */
#define PIN_PA20_BIT (1 << 20) /**< Pin 20, Port A */
#define PIN_PA21_BIT (1 << 21) /**< Pin 21, Port A */
#define PIN_PA22_BIT (1 << 22) /**< Pin 22, Port A */
#define PIN_PA23_BIT (1 << 23) /**< Pin 23, Port A */
#define PIN_PA24_BIT (1 << 24) /**< Pin 24, Port A */
#define PIN_PA25_BIT (1 << 25) /**< Pin 25, Port A */
#define PIN_PA26_BIT (1 << 26) /**< Pin 26, Port A */
#define PIN_PA27_BIT (1 << 27) /**< Pin 27, Port A */
#define PIN_PA28_BIT (1 << 28) /**< Pin 28, Port A */
#define PIN_PA29_BIT (1 << 29) /**< Pin 29, Port A */
#define PIN_PA30_BIT (1 << 30) /**< Pin 30, Port A */
#define PIN_PA31_BIT (1 << 31) /**< Pin 31, Port A */

#define PIN_PB0_BIT  (1 << 0)  /**< Pin 0, Port B */
#define PIN_PB1_BIT  (1 << 1)  /**< Pin 1, Port B */
#define PIN_PB2_BIT  (1 << 2)  /**< Pin 2, Port B */
#define PIN_PB3_BIT  (1 << 3)  /**< Pin 3, Port B */
#define PIN_PB4_BIT  (1 << 4)  /**< Pin 4, Port B */
#define PIN_PB5_BIT  (1 << 5)  /**< Pin 5, Port B */
#define PIN_PB6_BIT  (1 << 6)  /**< Pin 6, Port B */
#define PIN_PB7_BIT  (1 << 7)  /**< Pin 7, Port B */
#define PIN_PB8_BIT  (1 << 8)  /**< Pin 8, Port B */
#define PIN_PB9_BIT  (1 << 9)  /**< Pin 9, Port B */
#define PIN_PB10_BIT (1 << 10) /**< Pin 10, Port B */
#define PIN_PB11_BIT (1 << 11) /**< Pin 11, Port B */
#define PIN_PB12_BIT (1 << 12) /**< Pin 12, Port B */
#define PIN_PB13_BIT (1 << 13) /**< Pin 13, Port B */
#define PIN_PB14_BIT (1 << 14) /**< Pin 14, Port B */
#define PIN_PB15_BIT (1 << 15) /**< Pin 15, Port B */
#define PIN_PB16_BIT (1 << 16) /**< Pin 16, Port B */
#define PIN_PB17_BIT (1 << 17) /**< Pin 17, Port B */
#define PIN_PB18_BIT (1 << 18) /**< Pin 18, Port B */
#define PIN_PB19_BIT (1 << 19) /**< Pin 19, Port B */
#define PIN_PB20_BIT (1 << 20) /**< Pin 20, Port B */
#define PIN_PB21_BIT (1 << 21) /**< Pin 21, Port B */
#define PIN_PB22_BIT (1 << 22) /**< Pin 22, Port B */
#define PIN_PB23_BIT (1 << 23) /**< Pin 23, Port B */
#define PIN_PB24_BIT (1 << 24) /**< Pin 24, Port B */
#define PIN_PB25_BIT (1 << 25) /**< Pin 25, Port B */
#define PIN_PB26_BIT (1 << 26) /**< Pin 26, Port B */
#define PIN_PB27_BIT (1 << 27) /**< Pin 27, Port B */
#define PIN_PB28_BIT (1 << 28) /**< Pin 28, Port B */
#define PIN_PB29_BIT (1 << 29) /**< Pin 29, Port B */
#define PIN_PB30_BIT (1 << 30) /**< Pin 30, Port B */
#define PIN_PB31_BIT (1 << 31) /**< Pin 31, Port B */
/** @} */

#endif
