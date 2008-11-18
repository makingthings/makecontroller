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
    Title: HIDDKeyboardInputReport

    About: Purpose
        Class for manipulating HID keyboard input reports.

    About: Usage
        1 - Initialize a newly created input report with
            <HIDDKeyboardInputReport_Initialize>.
        2 - Change the standard keys that are pressed and released using
            <HIDDKeyboardInputReport_PressStandardKey> and
            <HIDDKeyboardInputReport_ReleaseStandardKey>.
        3 - Change the modifier keys that are currently pressed and released
            using <HIDDKeyboardInputReport_PressModifierKey> and
            <HIDDKeyboardInputReport_ReleaseModifierKey>.
*/

#ifndef HIDDKEYBOARDINPUTREPORT_H
#define HIDDKEYBOARDINPUTREPORT_H

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: Keys
        HIDDKeyboardInputReport_MAXKEYPRESSES - Maximum number of simultaneous
            key presses.
*/
#define HIDDKeyboardInputReport_MAXKEYPRESSES       3

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------

#ifdef __ICCARM__          // IAR
#pragma pack(1)            // IAR
#define __attribute__(...) // IAR
#endif                     // IAR

/*
    Type: HIDDKeyboardInputReport
        HID input report structure used by the keyboard driver to notify the
        host of pressed keys.

        The first byte is used to report the state of modifier keys. The
        other three contains the keycodes of the currently pressed keys.

    Variables:
        bmModifierKeys - State of modifier keys.
        pressedKeys - Key codes of pressed keys.
*/
typedef struct {

    unsigned char bmModifierKeys:8;
    unsigned char pressedKeys[HIDDKeyboardInputReport_MAXKEYPRESSES];

} __attribute__ ((packed)) HIDDKeyboardInputReport; // GCC

#ifdef __ICCARM__          // IAR
#pragma pack()             // IAR
#endif                     // IAR

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: HIDDKeyboardInputReport_Initialize
        Initializes a keyboard input report instance.

    Parameters:
        report - Pointer to a HIDDKeyboardInputReport instance.
*/
extern void HIDDKeyboardInputReport_Initialize(HIDDKeyboardInputReport *report);

/*
    Function: HIDDKeyboardInputReport_PressStandardKey
        Reports a standard key as being pressed.

    Parameters:
        report - Pointer to a HIDDKeyboardInputReport instance.
        key - Key code of the standard key.
*/
extern void HIDDKeyboardInputReport_PressStandardKey(
    HIDDKeyboardInputReport *report,
    unsigned char key);

/*
    Function: HIDDKeyboardInputReport_ReleaseStandardKey
        Reports a standard key as not being pressed anymore.

    Parameters:
        report - Pointer to a HIDDKeyboardInputReport instance.
        key - Key code of the standard key
*/
extern void HIDDKeyboardInputReport_ReleaseStandardKey(
    HIDDKeyboardInputReport *report,
    unsigned char key);

/*
    Function: HIDDKeyboardInputReport_PressModifierKey
        Reports a modifier key as being currently pressed.

    Parameters:
        report - Pointer to a HIDDKeyboardInputReport instance.
        key - Key code of the modifier key.
*/
extern void HIDDKeyboardInputReport_PressModifierKey(
    HIDDKeyboardInputReport *report,
    unsigned char key);

/*
    Function: HIDDKeyboardInputReport_ReleaseModifierKey
        Reports a modifier key as not being pressed anymore.

    Parameters:
        report - Pointer to a HIDDKeyboardInputReport instance.
        key - Key code of the modifier key.
*/
extern void HIDDKeyboardInputReport_ReleaseModifierKey(
    HIDDKeyboardInputReport *report,
    unsigned char key);

#endif //#ifndef HIDDKEYBOARDINPUTREPORT_H

