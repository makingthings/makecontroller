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
    Title: HIDReportRequest

    About: Purpose
        Definition of a class for manipulating HID-specific GET_REPORT and
        SET_REPORT requests.

    About: Usage
        1 - Receive a GET_REPORT or SET_REPORT request from the host.
        2 - Retrieve the report type using <HIDReportRequest_GetReportType>.
        3 - Retrieve the report ID using <HIDReportRequest_GetReportId>.
*/

#ifndef HIDREPORTREQUEST_H
#define HIDREPORTREQUEST_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <usb/common/core/USBGenericRequest.h>

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: Report types
        HIDReportRequest_INPUT - Input report.
        HIDReportRequest_OUTPUT - Output report.
        HIDReportRequest_FEATURE - Feature report.
*/
#define HIDReportRequest_INPUT                  1
#define HIDReportRequest_OUTPUT                 2
#define HIDReportRequest_FEATURE                3

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: HIDReportRequest_GetReportType
        Indicates the type of report targetted by a SET_REPORT or GET_REPORT
        request.

    Parameters:
        request - Pointer to a USBGenericRequest instance.

    Returns:
        Requested report type (see <Report types>).
*/
extern unsigned char HIDReportRequest_GetReportType(
    const USBGenericRequest *request);

/*
    Function: HIDReportRequest_GetReportId
        Indicates the ID of the report targetted by a SET_REPORT or GET_REPORT
        request. This value should be 0 if report IDs are not used.

    Parameters:
        request - Pointer to a USBGenericRequest instance.

    Returns:
        Requested report ID.
*/
extern unsigned char HIDReportRequest_GetReportId(
    const USBGenericRequest *request);

#endif //#ifndef HIDREPORTREQUEST_H

