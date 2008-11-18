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
    Title: USBGenericDescriptor

    About: Purpose
        Definition of a generic USB descriptor class.
*/

#ifndef USBGENERICDESCRIPTOR_H
#define USBGENERICDESCRIPTOR_H

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: Descriptor types
        USBGenericDescriptor_DEVICE - Device descriptor type.
        USBGenericDescriptor_CONFIGURATION - Configuration descriptor type.
        USBGenericDescriptor_STRING - String descriptor type.
        USBGenericDescriptor_INTERFACE - Interface descriptor type.
        USBGenericDescriptor_ENDPOINT - Endpoint descriptor type.
        USBGenericDescriptor_DEVICEQUALIFIER - Device qualifier descriptor type.
        USBGenericDescriptor_OTHERSPEEDCONFIGURATION - Other speed configuration
                                                       descriptor type.
        USBGenericDescriptor_INTERFACEPOWER - Interface power descriptor type.
*/
#define USBGenericDescriptor_DEVICE                     1
#define USBGenericDescriptor_CONFIGURATION              2
#define USBGenericDescriptor_STRING                     3
#define USBGenericDescriptor_INTERFACE                  4
#define USBGenericDescriptor_ENDPOINT                   5
#define USBGenericDescriptor_DEVICEQUALIFIER            6
#define USBGenericDescriptor_OTHERSPEEDCONFIGURATION    7
#define USBGenericDescriptor_INTERFACEPOWER             8

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------

#ifdef __ICCARM__          // IAR
#pragma pack(1)            // IAR
#define __attribute__(...) // IAR
#endif                     // IAR

/*
    Type: USBGenericDescriptor
        Holds the few fields shared by all USB descriptors.

    Variables:
        bLength - Length of the descriptor in bytes.
        bDescriptorType - Descriptor type.
*/
typedef struct {

    unsigned char bLength;
    unsigned char bDescriptorType;

} __attribute__ ((packed)) USBGenericDescriptor; // GCC

#ifdef __ICCARM__          // IAR
#pragma pack()             // IAR
#endif                     // IAR

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: USBGenericDescriptor_GetLength
        Returns the length of a descriptor.

    Parameters:
        descriptor - Pointer to a USBGenericDescriptor instance.

    Returns:
        Length of descriptor in bytes.
*/
extern unsigned int USBGenericDescriptor_GetLength(
    const USBGenericDescriptor *descriptor);

/*
    Function: USBGenericDescriptor_GetType
        Returns the type of a descriptor.

    Parameters:
        descriptor - Pointer to a USBGenericDescriptor instance.

    Returns:
        Type of descriptor.
*/
extern unsigned char USBGenericDescriptor_GetType(
    const USBGenericDescriptor *descriptor);

/*
    Function: USBGenericDescriptor_GetNextDescriptor
        Returns a pointer to the descriptor right after the given one, when
        parsing a Configuration descriptor.

    Parameters:
        descriptor - Pointer to a USBGenericDescriptor instance.

    Returns:
        Pointer to the next descriptor.
*/
extern USBGenericDescriptor *USBGenericDescriptor_GetNextDescriptor(
    const USBGenericDescriptor *descriptor);

#endif //#ifndef USBGENERICDESCRIPTOR_H

