/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support 
 * ----------------------------------------------------------------------------
 * Copyright (c) 2008, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

/*
    Title: HIDDKeyboardDriverDescriptors

    About: Purpose
        Definitions of the descriptors required by the HID device keyboard
        driver.

    About: Usage
        1 - Use the <hiddKeyboardDriverDescriptors> variable to initialize a
            <USBDDriver> instance.
        2 - Send <hiddReportDescriptor> to the host when a GET_DESCRIPTOR request
            for the report descriptor is received.
*/

#ifndef HIDDKEYBOARDDRIVERDESCRIPTORS_H
#define HIDDKEYBOARDDRIVERDESCRIPTORS_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <usb/device/core/USBDDriverDescriptors.h>
#include <usb/common/hid/HIDKeypad.h>

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: Endpoints
        HIDDKeyboardDriverDescriptors_INTERRUPTIN - Interrupt IN endpoint number.
        HIDDKeyboardDriverDescriptors_INTERRUPTIN_POLLING - Interrupt IN endpoint
            polling rate (in milliseconds).
        HIDDKeyboardDriverDescriptors_INTERRUPTOUT - Interrupt OUT endpoint number.
        HIDDKeyboardDriverDescriptors_INTERRUPTOUT_POLLING - Interrupt OUT endpoint
            polling rate (in milliseconds).
*/
#define HIDDKeyboardDriverDescriptors_INTERRUPTIN           1
#define HIDDKeyboardDriverDescriptors_INTERRUPTIN_POLLING   10
#define HIDDKeyboardDriverDescriptors_INTERRUPTOUT          2
#define HIDDKeyboardDriverDescriptors_INTERRUPTOUT_POLLING  10

/*
    Constants: Keypad keys
        HIDDKeyboardDriverDescriptors_FIRSTMODIFIERKEY - Key code of the first
            accepted modifier key.
        HIDDKeyboardDriverDescriptors_LASTMODIFIERKEY - Key code of the last
            accepted modifier key.
        HIDDKeyboardDriverDescriptors_FIRSTSTANDARDKEY - Key code of the first
            accepted standard key.
        HIDDKeyboardDriverDescriptors_LASTSTANDARDKEY - Key code of the last
            accepted standard key.
*/
#define HIDDKeyboardDriverDescriptors_FIRSTMODIFIERKEY  HIDKeypad_LEFTCONTROL
#define HIDDKeyboardDriverDescriptors_LASTMODIFIERKEY   HIDKeypad_RIGHTGUI
#define HIDDKeyboardDriverDescriptors_FIRSTSTANDARDKEY  0
#define HIDDKeyboardDriverDescriptors_LASTSTANDARDKEY   HIDKeypad_NUMLOCK

/*
    Constants: Report descriptor
        HIDDKeyboardDriverDescriptors_REPORTSIZE - Size of the report descriptor
            in bytes.
*/
#define HIDDKeyboardDriverDescriptors_REPORTSIZE        61

//------------------------------------------------------------------------------
//         Exported variables
//------------------------------------------------------------------------------
/*
    Variables: HID keyboard driver descriptors
        hiddKeyboardDriverDescriptors - List of descriptors used by the HID
            keyboard driver.
        hiddReportDescriptor - Report descriptor used by the driver.
*/
extern USBDDriverDescriptors hiddKeyboardDriverDescriptors;
extern const unsigned char hiddReportDescriptor[];

#endif //#ifndef HIDDKEYBOARDDRIVERDESCRIPTORS_H

