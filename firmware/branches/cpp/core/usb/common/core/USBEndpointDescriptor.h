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
    Title: USBEndpointDescriptor

    About: Purpose
        Definition of a class for handling USB endpoint descriptors.
*/

#ifndef USBENDPOINTDESCRIPTOR_H
#define USBENDPOINTDESCRIPTOR_H

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: Endpoint directions
        USBEndpointDescriptor_OUT - Endpoint receives data from the host.
        USBEndpointDescriptor_IN - Endpoint sends data to the host.
*/
#define USBEndpointDescriptor_OUT           0
#define USBEndpointDescriptor_IN            1

/*
    Constants: Endpoint types
        USBEndpointDescriptor_CONTROL - Control endpoint type.
        USBEndpointDescriptor_ISOCHRONOUS - Isochronous endpoint type.
        USBEndpointDescriptor_BULK - Bulk endpoint type.
        USBEndpointDescriptor_INTERRUPT - Interrupt endpoint type.
*/
#define USBEndpointDescriptor_CONTROL       0
#define USBEndpointDescriptor_ISOCHRONOUS   1
#define USBEndpointDescriptor_BULK          2
#define USBEndpointDescriptor_INTERRUPT     3

/// Maximum size for a full-speed control endpoint.
#define USBEndpointDescriptor_MAXCTRLSIZE_FS                64
/// Maximum size for a high-speed control endpoint.
#define USBEndpointDescriptor_MAXCTRLSIZE_HS                64
/// Maximum size for a full-speed bulk endpoint.
#define USBEndpointDescriptor_MAXBULKSIZE_FS                64
/// Maximum size for a high-speed bulk endpoint.
#define USBEndpointDescriptor_MAXBULKSIZE_HS                512
/// Maximum size for a full-speed interrupt endpoint.
#define USBEndpointDescriptor_MAXINTERRUPTSIZE_FS           64
/// Maximum size for a high-speed interrupt endpoint.
#define USBEndpointDescriptor_MAXINTERRUPTSIZE_HS           1024
/// Maximum size for a full-speed isochronous endpoint.
#define USBEndpointDescriptor_MAXISOCHRONOUSSIZE_FS         1023
/// Maximum size for a high-speed isochronous endpoint.
#define USBEndpointDescriptor_MAXISOCHRONOUSSIZE_HS         1024

/*
    Macro: USBEndpointDescriptor_ADDRESS
        Calculates the address of an endpoint given its number and direction
        (see <Directions>).
*/
#define USBEndpointDescriptor_ADDRESS(direction, number) \
    (((direction & 0x01) << 7) | (number & 0xF))

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------

#ifdef __ICCARM__          // IAR
#pragma pack(1)            // IAR
#define __attribute__(...) // IAR
#endif                     // IAR

/*
    Type: USBEndpointDescriptor
        USB standard endpoint descriptor structure.

    Variables:
        bLength - Size of the descriptor in bytes.
        bDescriptorType - Descriptor type (<USBGenericDescriptor_ENDPOINT>).
        bEndpointAddress - Address and direction of the endpoint.
        bmAttributes - Endpoint type and additional characteristics (for
                       isochronous endpoints).
        wMaxPacketSize - Maximum packet size (in bytes) of the endpoint.
        bInterval - Polling rate of the endpoint.
*/
typedef struct {

   unsigned char bLength;         
   unsigned char bDescriptorType; 
   unsigned char bEndpointAddress;                   
   unsigned char bmAttributes;    
   unsigned short wMaxPacketSize;                      
   unsigned char bInterval;       
                                   
} __attribute__ ((packed)) USBEndpointDescriptor; // GCC

#ifdef __ICCARM__          // IAR
#pragma pack()             // IAR
#endif                     // IAR

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: USBEndpointDescriptor_GetNumber
        Returns the number of an endpoint given its descriptor.

    Parameters:
        endpoint - Pointer to a USBEndpointDescriptor instance.

    Returns:
        Endpoint number.
*/
extern unsigned char USBEndpointDescriptor_GetNumber(
    const USBEndpointDescriptor *endpoint);

/*
    Function: USBEndpointDescriptor_GetDirection
        Returns the direction of an endpoint given its descriptor.

    Parameters:
        endpoint - Pointer to a USBEndpointDescriptor instance.

    Returns:
        Endpoint direction (see <Endpoint directions>).
*/
extern unsigned char USBEndpointDescriptor_GetDirection(
    const USBEndpointDescriptor *endpoint);

/*
    Function: USBEndpointDescriptor_GetType
        Returns the type of an endpoint given its descriptor.

    Parameters:
        endpoint - Pointer to a USBEndpointDescriptor instance.

    Returns:
        Endpoint type (see <Endpoint types>).
*/
extern unsigned char USBEndpointDescriptor_GetType(
    const USBEndpointDescriptor *endpoint);

/*
    Function: USBEndpointDescriptor_GetMaxPacketSize
        Returns the maximum size of a packet (in bytes) on an endpoint given
        its descriptor.

    Parameters:
        endpoint - Pointer to a USBEndpointDescriptor instance.

    Returns:
        Maximum packet size of endpoint.
*/
extern unsigned short USBEndpointDescriptor_GetMaxPacketSize(
    const USBEndpointDescriptor *endpoint);

#endif //#ifndef USBENDPOINTDESCRIPTOR_H

