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
    Title: USBFeatureRequest

    About: Purpose
        Definition of a class for manipulating CLEAR_FEATURE and SET_FEATURE
        requests.
*/

#ifndef USBFEATUREREQUEST_H
#define USBFEATUREREQUEST_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "USBGenericRequest.h"

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: Feature selectors
        USBFeatureRequest_ENDPOINTHALT - Halt feature of an endpoint.
        USBFeatureRequest_DEVICEREMOTEWAKEUP - Remote wake-up feature of the
                                               device.
        USBFeatureRequest_TESTMODE - Test mode of the device.
*/
#define USBFeatureRequest_ENDPOINTHALT          0
#define USBFeatureRequest_DEVICEREMOTEWAKEUP    1
#define USBFeatureRequest_TESTMODE              2

/*
    Constants: Test mode selectors
        USBFeatureRequest_TESTJ - Tests the high-output drive level on the D+
                                  line.
        USBFeatureRequest_TESTK - Tests the high-output drive level on the D-
                                  line.
        USBFeatureRequest_TESTSE0NAK - Tests the output impedance, low-level
                                       output voltage and loading characteristics.
        USBFeatureRequest_TESTPACKET - Tests rise and fall times, eye patterns
                                       and jitter.
        USBFeatureRequest_TESTFORCEENABLE - Tests the hub disconnect detection.
        USBFeatureRequest_TESTSENDZLP - Send a ZLP in Test Mode.
*/
#define USBFeatureRequest_TESTJ                 1
#define USBFeatureRequest_TESTK                 2
#define USBFeatureRequest_TESTSE0NAK            3
#define USBFeatureRequest_TESTPACKET            4
#define USBFeatureRequest_TESTFORCEENABLE       5
#define USBFeatureRequest_TESTSENDZLP           6

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: USBFeatureRequest_GetFeatureSelector
        Returns the feature selector of a given CLEAR_FEATURE or SET_FEATURE
        request.

    Parameters:
        request - Pointer to a USBGenericRequest instance.

    Returns:
        Feature selector.
*/
extern unsigned char USBFeatureRequest_GetFeatureSelector(
    const USBGenericRequest *request);

/*
    Function: USBFeatureRequest_GetTestSelector
        Indicates the test that the device must undertake following a
        SET_FEATURE request.

    Parameters:
        request - Pointer to a USBGenericRequest instance.

    Returns:
        Test selector.
*/
extern unsigned char USBFeatureRequest_GetTestSelector(
    const USBGenericRequest *request);

#endif //#ifndef USBFEATUREREQUEST_H

