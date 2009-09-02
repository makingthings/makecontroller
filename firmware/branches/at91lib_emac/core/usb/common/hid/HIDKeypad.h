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
    Title: HIDKeypad

    About: Purpose
        Definitions of constants and methods for the HID keypad usage page.

    About: Usage
        1 - Use the constants declared in this file when instanciating a
            Report descriptor instance.
        2 - When implementing the functionality of an HID keyboard, use the
            key codes defined here to indicate keys that are being pressed and
            released.
*/

#ifndef HIDKEYPAD_H
#define HIDKEYPAD_H

//------------------------------------------------------------------------------
//         Constants
//------------------------------------------------------------------------------
/*
    Constant: HIDKeypad_PAGEID
        Identifier for the HID keypad usage page
*/
#define HIDKeypad_PAGEID                    0x07

/*
    Constant: Alphabetic keys
        HIDKeypad_A - Key code for 'a' and 'A'.
        HIDKeypad_B - Key code for 'b' and 'B'.
        HIDKeypad_C - Key code for 'c' and 'C'.
        HIDKeypad_D - Key code for 'd' and 'D'.
        HIDKeypad_E - Key code for 'e' and 'E'.
        HIDKeypad_F - Key code for 'f' and 'F'.
        HIDKeypad_G - Key code for 'g' and 'G'.
        HIDKeypad_H - Key code for 'h' and 'H'.
        HIDKeypad_I - Key code for 'i' and 'I'.
        HIDKeypad_J - Key code for 'j' and 'J'.
        HIDKeypad_K - Key code for 'k' and 'K'.
        HIDKeypad_L - Key code for 'l' and 'L'.
        HIDKeypad_M - Key code for 'm' and 'M'.
        HIDKeypad_N - Key code for 'n' and 'N'.
        HIDKeypad_O - Key code for 'o' and 'O'.
        HIDKeypad_P - Key code for 'p' and 'P'.
        HIDKeypad_Q - Key code for 'q' and 'Q'.
        HIDKeypad_R - Key code for 'r' and 'R'.
        HIDKeypad_S - Key code for 's' and 'S'.
        HIDKeypad_T - Key code for 't' and 'T'.
        HIDKeypad_U - Key code for 'u' and 'U'.
        HIDKeypad_V - Key code for 'v' and 'V'.
        HIDKeypad_W - Key code for 'w' and 'W'.
        HIDKeypad_X - Key code for 'x' and 'X'.
        HIDKeypad_Y - Key code for 'y' and 'Y'.
        HIDKeypad_Z - Key code for 'z' and 'Z'.
*/
#define HIDKeypad_A                     4
#define HIDKeypad_B                     5
#define HIDKeypad_C                     6
#define HIDKeypad_D                     7
#define HIDKeypad_E                     8
#define HIDKeypad_F                     9
#define HIDKeypad_G                     10
#define HIDKeypad_H                     11
#define HIDKeypad_I                     12
#define HIDKeypad_J                     13
#define HIDKeypad_K                     14
#define HIDKeypad_L                     15
#define HIDKeypad_M                     16
#define HIDKeypad_N                     17
#define HIDKeypad_O                     18
#define HIDKeypad_P                     19
#define HIDKeypad_Q                     20
#define HIDKeypad_R                     21
#define HIDKeypad_S                     22
#define HIDKeypad_T                     23
#define HIDKeypad_U                     24
#define HIDKeypad_V                     25
#define HIDKeypad_W                     26
#define HIDKeypad_X                     27
#define HIDKeypad_Y                     28
#define HIDKeypad_Z                     29

/*
    Constants: Numeric keys
        HIDKeypad_1 - Key code for '1' and '!'.
        HIDKeypad_2 - Key code for '2' and '@'.
        HIDKeypad_3 - Key code for '3' and '#'.
        HIDKeypad_4 - Key code for '4' and '$'.
        HIDKeypad_5 - Key code for '5' and '%'.
        HIDKeypad_6 - Key code for '6' and '^'.
        HIDKeypad_7 - Key code for '7' and '&'.
        HIDKeypad_8 - Key code for '8' and '*'.
        HIDKeypad_9 - Key code for '9' and '('.
        HIDKeypad_0 - Key code for '0' and ')'.
*/
#define HIDKeypad_1                     30
#define HIDKeypad_2                     31
#define HIDKeypad_3                     32
#define HIDKeypad_4                     33
#define HIDKeypad_5                     34
#define HIDKeypad_6                     35
#define HIDKeypad_7                     36
#define HIDKeypad_8                     37
#define HIDKeypad_9                     38
#define HIDKeypad_0                     39

/// Enter key code.
#define HIDKeypad_ENTER                 40
/// Escape key code.
#define HIDKeypad_ESCAPE                41
/// Backspace key code.
#define HIDKeypad_BACKSPACE             42
/// Tab key code.
#define HIDKeypad_TAB                   43
/// Spacebar key code.
#define HIDKeypad_SPACEBAR              44
/// Printscreen key code.
#define HIDKeypad_PRINTSCREEN           70
/// Scroll lock key code.
#define HIDKeypad_SCROLLLOCK            71
/// Num lock key code.
#define HIDKeypad_NUMLOCK               83

/*
    Constants: Modifier keys
        HIDKeypad_LEFTCONTROL - Key code for the left 'Control' key.
        HIDKeypad_LEFTSHIFT - Key code for the left 'Shift' key.
        HIDKeypad_LEFTALT - Key code for the left 'Alt' key.
        HIDKeypad_LEFTGUI - Key code for the left 'GUI' (e.g. Windows) key.
        HIDKeypad_RIGHTCONTROL - Key code for the right 'Control' key.
        HIDKeypad_RIGHTSHIFT - Key code for the right 'Shift' key.
        HIDKeypad_RIGHTALT - Key code for the right 'Alt' key.
        HIDKeypad_RIGHTGUI - Key code for the right 'GUI' key.
*/
#define HIDKeypad_LEFTCONTROL           224
#define HIDKeypad_LEFTSHIFT             225
#define HIDKeypad_LEFTALT               226
#define HIDKeypad_LEFTGUI               227
#define HIDKeypad_RIGHTCONTROL          228
#define HIDKeypad_RIGHTSHIFT            229
#define HIDKeypad_RIGHTALT              230
#define HIDKeypad_RIGHTGUI              231

/*
    Constants: Error codes
        HIDKeypad_ERRORROLLOVER - Indicates that too many keys have been pressed
            at the same time.
        HIDKeypad_POSTFAIL - ?
        HIDKeypad_ERRORUNDEFINED - Indicates an undefined error.
*/
#define HIDKeypad_ERRORROLLOVER         1
#define HIDKeypad_POSTFAIL              2
#define HIDKeypad_ERRORUNDEFINED        3

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: HIDKeypad_IsModifierKey
        Indicates if the given key code is associated with a modified key.

    Parameters:
        key - Key code.

    Returns:
        1 if the key code represents a modifier key; otherwise 0.
*/
extern unsigned char HIDKeypad_IsModifierKey(unsigned char key);

#endif //#ifndef HIDKEYPAD_H

