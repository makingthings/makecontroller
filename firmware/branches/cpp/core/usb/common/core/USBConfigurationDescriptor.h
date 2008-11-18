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

/**
 *  \dir
 *  !!!Purpose
 * 
 *      Definitions of descriptor and request structures
 *      described by the USB specification.
 * 
 *  !!!Usage
 * 
 *      -# Do this
 *      -# Do that
 */

#ifndef USBCONFIGURATIONDESCRIPTOR_H
#define USBCONFIGURATIONDESCRIPTOR_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "USBGenericDescriptor.h"
#include "USBInterfaceDescriptor.h"
#include "USBEndpointDescriptor.h"

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: Attributes
        USBConfigurationDescriptor_BUSPOWERED_NORWAKEUP - Device is bus-powered
            and not support remote wake-up.
        USBConfigurationDescriptor_SELFPOWERED_NORWAKEUP - Device is self-powered
            and not support remote wake-up.
        USBConfigurationDescriptor_BUSPOWERED_RWAKEUP - Device is bus-powered
            and supports remote wake-up.
        USBConfigurationDescriptor_SELFPOWERED_RWAKEUP - Device is self-powered
            and supports remote wake-up.
*/
#define USBConfigurationDescriptor_BUSPOWERED_NORWAKEUP  0x80
#define USBConfigurationDescriptor_SELFPOWERED_NORWAKEUP 0xC0
#define USBConfigurationDescriptor_BUSPOWERED_RWAKEUP    0xA0
#define USBConfigurationDescriptor_SELFPOWERED_RWAKEUP   0xE0

/*
    Macros:
        USBConfigurationDescriptor_POWER - Calculates the value of the power
                                           consumption field given the value in
                                           mA.
*/
#define USBConfigurationDescriptor_POWER(power)     (power / 2)

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------

#ifdef __ICCARM__          // IAR
#pragma pack(1)            // IAR
#define __attribute__(...) // IAR
#endif                     // IAR

/*
    Type: USBConfigurationDescriptor
        USB standard configuration descriptor structure.

    Variables:
        bLength - Size of the descriptor in bytes.
        bDescriptorType - Descriptor type (<USBDESC_CONFIGURATION>).
        wTotalLength - Length of all descriptors returned along with this
                       configuration descriptor.
        bNumInterfaces - Number of interfaces in this configuration.
        bConfigurationValue - Value for selecting this configuration.
        iConfiguration - Index of the configuration string descriptor.
        bmAttributes - Configuration characteristics.
        bMaxPower - Maximum power consumption of the device when in this
                    configuration.
*/
typedef struct {

   unsigned char bLength;             
   unsigned char bDescriptorType;     
   unsigned short wTotalLength;            
   unsigned char bNumInterfaces;      
   unsigned char bConfigurationValue; 
   unsigned char iConfiguration;       
   unsigned char bmAttributes;         
   unsigned char bMaxPower;           
                                       
} __attribute__ ((packed)) USBConfigurationDescriptor; // GCC

#ifdef __ICCARM__          // IAR
#pragma pack()             // IAR
#endif                     // IAR

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: USBConfigurationDescriptor_GetTotalLength
        Returns the total length of a configuration, i.e. including the 
        descriptors following it.

    Parameters:
        configuration - Pointer to a USBConfigurationDescriptor instance.

    Returns:
        Total length (in bytes) of the configuration.
*/
extern unsigned int USBConfigurationDescriptor_GetTotalLength(
    const USBConfigurationDescriptor *configuration);

/*
    Function: USBConfigurationDescriptor_GetNumInterfaces
        Returns the number of interfaces in a configuration.

    Parameters:
        configuration - Pointer to a USBConfigurationDescriptor instance.

    Returns:
        Number of interfaces in configuration.
*/
extern unsigned char USBConfigurationDescriptor_GetNumInterfaces(
    const USBConfigurationDescriptor *configuration);

/*
    Function: USBConfigurationDescriptor_IsSelfPowered
        Indicates if the device is self-powered when in a given configuration.

    Parameters:
        configuration - Pointer to a USBConfigurationDescriptor instance.

    Returns:
        1 if the device is self-powered when in the given configuration;
        otherwise 0.
*/
extern unsigned char USBConfigurationDescriptor_IsSelfPowered(
    const USBConfigurationDescriptor *configuration);

/*
    Function: USBConfigurationDescriptor_Parse
        Parses the given Configuration descriptor (followed by relevant
        interface, endpoint and class-specific descriptors) into three arrays.

        *Each array must have its size equal or greater to the number of
        descriptors it stores plus one*. A null-value is inserted after the last
        descriptor of each type to indicate the array end.

        Note that if the pointer to an array is null (0), nothing is stored in
        it.

    Parameters:
        configuration - Pointer to the start of the whole Configuration descriptor.
        interfaces - Pointer to the Interface descriptor array.
        endpoints - Pointer to the Endpoint descriptor array.
        others - Pointer to the class-specific descriptor array.
*/
extern void USBConfigurationDescriptor_Parse(
    const USBConfigurationDescriptor *configuration,
    USBInterfaceDescriptor **interfaces,
    USBEndpointDescriptor **endpoints,
    USBGenericDescriptor **others);

#endif //#ifndef USBCONFIGURATIONDESCRIPTOR_H

