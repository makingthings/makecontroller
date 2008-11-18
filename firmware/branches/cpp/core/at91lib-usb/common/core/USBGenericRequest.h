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
    Title: USBGenericRequest

    About: Purpose
        Definition of the USBGenericRequest class and its methods.
*/

#ifndef USBGENERICREQUEST_H
#define USBGENERICREQUEST_H

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: Codes
        USBGenericRequest_GETSTATUS - GET_STATUS request code.
        USBGenericRequest_CLEARFEATURE - CLEAR_FEATURE request code.
        USBGenericRequest_SETFEATURE - SET_FEATURE request code.
        USBGenericRequest_SETADDRESS - SET_ADDRESS request code.
        USBGenericRequest_GETDESCRIPTOR - GET_DESCRIPTOR request code.
        USBGenericRequest_SETDESCRIPTOR - SET_DESCRIPTOR request code.
        USBGenericRequest_GETCONFIGURATION - GET_CONFIGURATION request code.
        USBGenericRequest_SETCONFIGURATION - SET_CONFIGURATION request code.
        USBGenericRequest_GETINTERFACE - GET_INTERFACE request code.
        USBGenericRequest_SETINTERFACE - SET_INTERFACE request code.
        USBGenericRequest_SYNCHFRAME - SYNCH_FRAME request code.
*/
#define USBGenericRequest_GETSTATUS             0
#define USBGenericRequest_CLEARFEATURE          1
#define USBGenericRequest_SETFEATURE            3
#define USBGenericRequest_SETADDRESS            5
#define USBGenericRequest_GETDESCRIPTOR         6
#define USBGenericRequest_SETDESCRIPTOR         7
#define USBGenericRequest_GETCONFIGURATION      8
#define USBGenericRequest_SETCONFIGURATION      9
#define USBGenericRequest_GETINTERFACE          10
#define USBGenericRequest_SETINTERFACE          11
#define USBGenericRequest_SYNCHFRAME            12

/*
    Constants: Recipients
        USBGenericRequest_DEVICE - Recipient is the whole device.
        USBGenericRequest_INTERFACE - Recipient is an interface.
        USBGenericRequest_ENDPOINT - Recipient is an endpoint.
        USBGenericRequest_OTHER - Recipient is another entity.
*/
#define USBGenericRequest_DEVICE                0
#define USBGenericRequest_INTERFACE             1
#define USBGenericRequest_ENDPOINT              2
#define USBGenericRequest_OTHER                 3

/*
    Constants: Types
        USBGenericRequest_STANDARD - Request is standard.
        USBGenericRequest_CLASS - Request is class-specific.
        USBGenericRequest_VENDOR - Request is vendor-specific.      
*/
#define USBGenericRequest_STANDARD              0
#define USBGenericRequest_CLASS                 1
#define USBGenericRequest_VENDOR                2

/*
    Constants: Directions
        USBGenericRequest_IN - Transfer occurs from device to the host.
        USBGenericRequest_OUT - Transfer occurs from the host to the device.
*/
#define USBGenericRequest_OUT                   0
#define USBGenericRequest_IN                    1

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------

/*
    Type: USBGenericRequest
        Generic USB SETUP request sent over Control endpoints.

    Variables:
        bmRequestType - Type of request (see <Recipients>, <Types> and
            <Direction>).
        bRequest - Request code (see <Codes>).
        wValue - Request-specific value parameter.
        wIndex - Request-specific index parameter.
        wLength - Expected length (in bytes) of the data phase.
*/
typedef struct {

    unsigned char bmRequestType:8;
    unsigned char bRequest:8;
    unsigned short wValue:16;
    unsigned short wIndex:16;
    unsigned short wLength:16;

} USBGenericRequest;

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: USBGenericRequest_GetType
        Returns the type of the given request.

    Parameters:
        request - Pointer to a USBGenericRequest instance.

    Returns:
        Request type.

    See also:
        <Types>.
*/
extern unsigned char USBGenericRequest_GetType(const USBGenericRequest *request);

/*
    Function: USBGenericRequest_GetRequest
        Returns the request code of the given request.

    Parameters:
        request - Pointer to a USBGenericRequest instance.

    Returns:
        Request code.

    See also:
        <Codes>.
*/
extern unsigned char USBGenericRequest_GetRequest(
    const USBGenericRequest *request);

/*
    Function: USBGenericRequest_GetValue
        Returns the wValue field of the given request.

    Parameters:
        request - Pointer to a USBGenericRequest instance.

    Returns:
        Request value.
*/
extern unsigned short USBGenericRequest_GetValue(
    const USBGenericRequest *request);

/*
    Function: USBGenericRequest_GetIndex
        Returns the wIndex field of the given request.

    Parameters:
        request - Pointer to a USBGenericRequest instance.

    Returns:
        Request index;
*/
extern unsigned short USBGenericRequest_GetIndex(
    const USBGenericRequest *request);

/*
    Function: USBGenericRequest_GetLength
        Returns the expected length of the data phase following a request.

    Parameters:
        request - Pointer to a USBGenericRequest instance.

    Returns:
        Length of data phase.
*/
extern unsigned short USBGenericRequest_GetLength(
    const USBGenericRequest *request);

/*
    Function: USBGenericRequest_GetEndpointNumber
        Returns the endpoint number targetted by a given request.

    Parameters:
        request - Pointer to a USBGenericRequest instance.

    Returns:
        Endpoint number.
*/
extern unsigned char USBGenericRequest_GetEndpointNumber(
    const USBGenericRequest *request);

/*
    Function: USBGenericRequest_GetRecipient
        Returns the intended recipient of a given request.

    Parameters:
        request - Pointer to a USBGenericRequest instance.

    Returns:
        Request recipient.
*/
extern unsigned char USBGenericRequest_GetRecipient(
    const USBGenericRequest *request);

/*
    Function: USBGenericRequest_GetDirection
        Returns the direction of the data transfer following the given request.

    Parameters:
        request - Pointer to a USBGenericRequest instance.

    Returns:
        Transfer direction.
*/
extern unsigned char USBGenericRequest_GetDirection(
    const USBGenericRequest *request);

#endif //#ifndef USBGENERICREQUEST_H

