/*
    ChibiOS/RT - Copyright (C) 2006-2007 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _BOARD_H_
#define _BOARD_H_

#define BOARD_MAKE_CONTROLLER
#define SAM7_PLATFORM SAM7X256
#include "config.h"
#include "at91sam7.h"

#define CLK             18432000
#define MCK             48054857

#define PIOB_PHY_PD_MASK AT91C_PB18_EF100
#define PHY_HARDWARE PHY_DAVICOM_9161

// Initial I/O setup - everything as input with pullup, except LED (PA12)
#define VAL_PIOA_ODSR           0x00001000      // Output data.
#define VAL_PIOA_OSR            0x00001000      // Direction.
#define VAL_PIOA_PUSR           0xFFFFFFFF      // Pull-up.
// Port B - everything as input with pullup
#define VAL_PIOB_ODSR           0x00000000      // Output data.
#define VAL_PIOB_OSR            0x00000000      // Direction.
#define VAL_PIOB_PUSR           0xFFFFFFFF      // Pull-up.

#define BOARD_USB_UDP
#define BOARD_USB_PULLUP_ALWAYSON
#define BOARD_USB_NUMENDPOINTS                  6
#define BOARD_USB_ENDPOINTS_MAXPACKETSIZE(i)    ((((i) == 4) || ((i) == 5)) ? 256 : (((i) == 0) ? 8 : 64))
#define BOARD_USB_ENDPOINTS_BANKS(i)            ((((i) == 0) || ((i) == 3)) ? 1 : 2)
#define BOARD_USB_BMATTRIBUTES                  USBConfigurationDescriptor_BUSPOWERED_NORWAKEUP

// MakingThings
#if (CONTROLLER_VERSION <= 100)
  #define USB_PULLUP PIN_PA11
  #define USB_DETECT PIN_PA10
#elif (CONTROLLER_VERSION >= 200)
  #define USB_PULLUP PIN_PA30
  #define USB_DETECT PIN_PA29
#endif

#endif // _BOARD_H_
