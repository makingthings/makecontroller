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
    Title: HIDDKeyboardDriver

    About: Purpose
        Definition of methods for using a HID keyboard device driver.

    About: Usage
        1 - Re-implement the <USBDCallbacks_RequestReceived> callback to forward
            requests to <HIDDKeyboardDriver_RequestHandler>. This is done
            automatically unless the NOAUTOCALLBACK symbol is defined during
            compilation.
        2 - Initialize the driver using <HIDDKeyboardDriver_Initialize>. The
            USB driver is automatically initialized by this method.
        3 - Call the <HIDDKeyboardDriver_ChangeKeys> method when one or more
            keys are pressed/released.
*/

#ifndef HIDDKEYBOARDDRIVER_H
#define HIDDKEYBOARDDRIVER_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <usb/common/core/USBGenericRequest.h>

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: HIDDKeyboardDriver_Initialize
        Initializes the HID keyboard device driver.
*/
extern void HIDDKeyboardDriver_Initialize();

/*
    Function: HIDDKeyboardDriver_RequestHandler
        Handles HID-specific SETUP request sent by the host.

    Parameters:
        request - Pointer to a USBGenericRequest instance.
*/
extern void HIDDKeyboardDriver_RequestHandler(const USBGenericRequest *request);

/*
    Function: HIDDKeyboardDriver_ChangeKeys
        Reports a change in which keys are currently pressed or release to the
        host.

    Parameters:
        pressedKeys - Pointer to an array of key codes indicating keys that have
            been pressed since the last call to <HIDDKeyboardDriver_ChangeKeys>.
        pressedKeysSize - Number of key codes in the pressedKeys array.
        releasedKeys - Pointer to an array of key codes indicates keys that have
            been released since the last call to <HIDDKeyboardDriver_ChangeKeys>.
        releasedKeysSize - Number of key codes in the releasedKeys array.

    Returns:
        <USBD_STATUS_SUCCESS> if the report has been sent to the host;
        otherwise an error code.
*/
extern unsigned char HIDDKeyboardDriver_ChangeKeys(
    unsigned char *pressedKeys,
    unsigned char pressedKeysSize,
    unsigned char *releasedKeys,
    unsigned char releasedKeysSize);

extern void HIDDKeyboardDriver_RemoteWakeUp(void);

#endif //#ifndef HIDDKEYBOARDDRIVER_H

