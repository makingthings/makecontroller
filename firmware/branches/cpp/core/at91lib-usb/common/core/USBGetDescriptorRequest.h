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
    Title: USBGetDescriptorRequest

    About: Purpose
        Definition of the USBGetDescriptorRequest class.
*/

#ifndef USBGETDESCRIPTORREQUEST_H
#define USBGETDESCRIPTORREQUEST_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "USBGenericRequest.h"

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: USBGetDescriptorRequest_GetDescriptorType
        Returns the type of the descriptor requested by the host given the
        corresponding GET_DESCRIPTOR request.

    Parameters:
        request - Pointer to a USBGenericDescriptor instance.

    Returns:
        Type of the requested descriptor.
*/
extern unsigned char USBGetDescriptorRequest_GetDescriptorType(
    const USBGenericRequest *request);

/*
    Function: USBGetDescriptorRequest_GetDescriptorIndex
        Returns the index of the requested descriptor, given the corresponding
        GET_DESCRIPTOR request.

    Parameters:
        request - Pointer to a USBGenericDescriptor instance.

    Returns:
        Index of the requested descriptor.
*/
extern unsigned char USBGetDescriptorRequest_GetDescriptorIndex(
    const USBGenericRequest *request);

#endif //#ifndef USBGETDESCRIPTORREQUEST_H

