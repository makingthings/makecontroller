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
    Title: HIDInterfaceDescriptor

    About: Purpose
        Constants used when declaring an HID interface.

    About: Usage
        Use the constants defined here when declaring a USBInterfaceDescriptor
        instance for a HID interface.
*/

#ifndef HIDINTERFACEDESCRIPTOR_H
#define HIDINTERFACEDESCRIPTOR_H

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: Interface class, subclass and protocol codes
        HIDInterfaceDescriptor_CLASS - HID interface class code.
        HIDInterfaceDescriptor_SUBCLASS_NONE - Indicates the interface does not
            implement a particular subclass.
        HIDInterfaceDescriptor_SUBCLASS_BOOT - Indicates the interface is
            compliant with the boot specification.
        HIDInterfaceDescriptor_PROTOCOL_NONE - Indicates the interface does not
            implement a particular protocol.
        HIDInterfaceDescriptor_PROTOCOL_KEYBOARD - Indicates the interface
            supports the boot specification as a keyboard.
        HIDInterfaceDescriptor_PROTOCOL_MOUSE - Indicates the interface supports
            the boot specification as a mouse.
*/
#define HIDInterfaceDescriptor_CLASS                0x03
#define HIDInterfaceDescriptor_SUBCLASS_NONE        0x00
#define HIDInterfaceDescriptor_SUBCLASS_BOOT        0x01
#define HIDInterfaceDescriptor_PROTOCOL_NONE        0x00
#define HIDInterfaceDescriptor_PROTOCOL_KEYBOARD    0x01
#define HIDInterfaceDescriptor_PROTOCOL_MOUSE       0x02

#endif //#ifndef HIDINTERFACEDESCRIPTOR_H

