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
    Title: USBD

    About: Purpose
        Collection of methods for using the USB device controller on AT91
        microcontrollers.

    About: Usage
        Please refer to the corresponding application note.
*/

#ifndef USBD_H
#define USBD_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <usb/common/core/USBEndpointDescriptor.h>
#include <usb/common/core/USBGenericRequest.h>

//------------------------------------------------------------------------------
//      Definitions
//------------------------------------------------------------------------------
/*
    Constants: USB device API return values
        USBD_STATUS_SUCCESS - Indicates the operation was successful.
        USBD_STATUS_LOCKED - Endpoint/device is already busy.
        USBD_STATUS_ABORTED - Operation has been aborted.
        USBD_STATUS_RESET - Operation has been aborted because the device has
            been reset.
*/
#define USBD_STATUS_SUCCESS             0
#define USBD_STATUS_LOCKED              1
#define USBD_STATUS_ABORTED             2
#define USBD_STATUS_RESET               3

/*
    Constants: USB device states
        USBD_STATE_SUSPENDED - The device is currently suspended.
        USBD_STATE_ATTACHED - USB cable is plugged into the device.
        USBD_STATE_POWERED - Host is providing +5V through the USB cable.
        USBD_STATE_DEFAULT - Device has been reset.
        USBD_STATE_ADDRESS - The device has been given an address on the bus.
        USBD_STATE_CONFIGURED - A valid configuration has been selected.  
*/
#define USBD_STATE_SUSPENDED            0
#define USBD_STATE_ATTACHED             1
#define USBD_STATE_POWERED              2
#define USBD_STATE_DEFAULT              3
#define USBD_STATE_ADDRESS              4
#define USBD_STATE_CONFIGURED           5

/*
    Constants: USB device LEDs
        USBD_LEDPOWER - LED for indicating that the device is powered.
        USBD_LEDUSB - LED for indicating USB activity.
        USBD_LEDOTHER - LED for custom usage.
*/
#define USBD_LEDPOWER                   0
#define USBD_LEDUSB                     1
#define USBD_LEDOTHER                   2

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Callback used by transfer functions (<USBD_Read> & <USBD_Write>) to notify
/// that a transaction is complete.
//------------------------------------------------------------------------------
typedef void (*TransferCallback)(void *pArg,
                                 unsigned char status,
                                 unsigned int transferred,
                                 unsigned int remaining);

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

extern void USBD_InterruptHandler(void);

extern void USBD_RemoteWakeUp( void );

extern void USBD_Init(void);

extern void USBD_Connect(void);

extern void USBD_Disconnect(void);

extern char USBD_Write(
    unsigned char eptnum,
    const void *pData,
    unsigned int size,
    TransferCallback callback,
    void *pArg);

extern char USBD_Read(
    unsigned char eptnum,
    void *pData,
    unsigned int size,
    TransferCallback callback,
    void *pArg);

extern unsigned char USBD_Stall(unsigned char eptnum);

extern void USBD_Halt(unsigned char eptnum);

extern void USBD_Unhalt(unsigned char eptnum);

extern void USBD_ConfigureEndpoint(const USBEndpointDescriptor *pDescriptor);

extern unsigned char USBD_IsHalted(unsigned char eptnum);

extern void USBD_RemoteWakeUp(void);

extern void USBD_SetAddress(unsigned char address);

extern void USBD_SetConfiguration(unsigned char cfgnum);

extern unsigned char USBD_GetState(void);

extern unsigned char USBD_IsHighSpeed(void);

extern void USBD_Test(unsigned char bIndex);

#endif //#ifndef USBD_H

