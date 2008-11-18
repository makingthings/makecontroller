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

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "HIDDKeyboardDriver.h"
#include "HIDDKeyboardDriverDescriptors.h"
#include "HIDDKeyboardCallbacks.h"
#include "HIDDKeyboardInputReport.h"
#include "HIDDKeyboardOutputReport.h"
#include <utility/trace.h>
#include <usb/common/core/USBGetDescriptorRequest.h>
#include <usb/common/hid/HIDGenericDescriptor.h>
#include <usb/common/hid/HIDDescriptor.h>
#include <usb/common/hid/HIDGenericRequest.h>
#include <usb/common/hid/HIDReportRequest.h>
#include <usb/common/hid/HIDIdleRequest.h>
#include <usb/common/hid/HIDKeypad.h>
#include <usb/device/core/USBD.h>
#include <usb/device/core/USBDDriver.h>

//------------------------------------------------------------------------------
//         Internal types
//------------------------------------------------------------------------------
/*
    Type: HIDDKeyboardDriver
        Driver structure for an HID device implementing keyboard functionalities.

    Variables:
        usbdDriver - Standard USB device driver instance.
        inputReportIdleRate - Idle rate (in milliseconds) of the input report.
        inputReport - Input report instance.
        outputReport - Output report instance.
*/
typedef struct {

    USBDDriver usbdDriver;
    unsigned char inputReportIdleRate;
    HIDDKeyboardInputReport inputReport;
    HIDDKeyboardOutputReport outputReport;

} HIDDKeyboardDriver;

//------------------------------------------------------------------------------
//         Internal variables
//------------------------------------------------------------------------------
/*
    Variable: hiddKeyboardDriver
        Static instance of the HID keyboard device driver.
*/
static HIDDKeyboardDriver hiddKeyboardDriver;

//------------------------------------------------------------------------------
//         Internal functions
//------------------------------------------------------------------------------
/*
    Function: HIDDKeyboardDriver_GetDescriptor
        Returns the descriptor requested by the host.

    Parameters:
        type - Descriptor type.
        length - Maximum number of bytes to send.

    Returns:
        1 if the request has been handled by this function, otherwise 0.
*/
static unsigned char HIDDKeyboardDriver_GetDescriptor(unsigned char type,
                                                      unsigned char length)
{
    const USBConfigurationDescriptor *pConfiguration;
    HIDDescriptor *hidDescriptor;

    switch (type) {

        case HIDGenericDescriptor_REPORT:
            trace_LOG(trace_INFO, "Report ");

            // Adjust length and send report descriptor
            if (length > HIDDKeyboardDriverDescriptors_REPORTSIZE) {

                length = HIDDKeyboardDriverDescriptors_REPORTSIZE;
            }
            USBD_Write(0, &hiddReportDescriptor, length, 0, 0);
            break;

        case HIDGenericDescriptor_HID:
            trace_LOG(trace_INFO, "HID ");

            // Configuration descriptor is different depending on configuration
            if (USBD_IsHighSpeed()) {

                pConfiguration = hiddKeyboardDriver.usbdDriver.pDescriptors->pHsConfiguration;
            }
            else {

                pConfiguration = hiddKeyboardDriver.usbdDriver.pDescriptors->pFsConfiguration;
            }

            // Parse the device configuration to get the HID descriptor
            USBConfigurationDescriptor_Parse(pConfiguration, 0, 0, (USBGenericDescriptor **) &hidDescriptor);

            // Adjust length and send HID descriptor
            if (length > sizeof(HIDDescriptor)) {

                length = sizeof(HIDDescriptor);
            }
            USBD_Write(0, hidDescriptor, length, 0, 0);
            break;

        default:
            return 0;
    }

    return 1;
}

/*
    Function: HIDDKeyboardDriver_GetIdle
        Sends the current Idle rate of the input report to the host.

*/
static void HIDDKeyboardDriver_GetIdle()
{
    trace_LOG(trace_INFO, "gIdle ");

    USBD_Write(0, &(hiddKeyboardDriver.inputReportIdleRate), 1, 0, 0);
}

/*
    Function: HIDDKeyboardDriver_SetIdle
        Retrieves the new idle rate of the input report from the USB host.

    Parameters:
        idleRate - New input report idle rate.
*/
static void HIDDKeyboardDriver_SetIdle(unsigned char idleRate)
{
    trace_LOG(trace_INFO, "sIdle(%d) ", idleRate);

    hiddKeyboardDriver.inputReportIdleRate = idleRate;
    USBD_Write(0, 0, 0, 0, 0);
}

/*
    Function: HIDDKeyboardDriver_GetReport
        Sends the requested report to the host.

    Parameters:
        type - Report type.
        length - Maximum number of bytes to send.
*/
static void HIDDKeyboardDriver_GetReport(unsigned char type,
                                         unsigned short length)
{
    trace_LOG(trace_INFO, "gReport ");

    // Check report type
    switch (type) {

        case HIDReportRequest_INPUT:
            trace_LOG(trace_INFO, "In ");

            // Adjust size and send report
            if (length > sizeof(HIDDKeyboardInputReport)) {

                length = sizeof(HIDDKeyboardInputReport);
            }
            USBD_Write(0, // Endpoint #0
                       &(hiddKeyboardDriver.inputReport),
                       length,
                       0, // No callback
                       0);
            break;

        case HIDReportRequest_OUTPUT:
            trace_LOG(trace_INFO, "Out ");

            // Adjust size and send report
            if (length > sizeof(HIDDKeyboardOutputReport)) {

                length = sizeof(HIDDKeyboardOutputReport);
            }
            USBD_Write(0, // Endpoint #0
                       &(hiddKeyboardDriver.outputReport),
                       length,
                       0, // No callback
                       0);
            break;

        default:
            USBD_Stall(0);
    }
}

/*
    Function: HIDDKeyboardDriver_ReportReceived
        Callback invoked when an output report has been received from the host.
        Forward the new status of the LEDs to the user program via the
        <HIDDKeyboardCallbacks_LedsChanged> callback.
*/
static void HIDDKeyboardDriver_ReportReceived()
{
    trace_LOG(trace_INFO, "oReport ");

    // Trigger callback
    HIDDKeyboardCallbacks_LedsChanged(
        HIDDKeyboardOutputReport_GetNumLockStatus(&(hiddKeyboardDriver.outputReport)),
        HIDDKeyboardOutputReport_GetCapsLockStatus(&(hiddKeyboardDriver.outputReport)),
        HIDDKeyboardOutputReport_GetScrollLockStatus(&(hiddKeyboardDriver.outputReport)));

    // Restart transfer
    USBD_Read(HIDDKeyboardDriverDescriptors_INTERRUPTOUT,
              &(hiddKeyboardDriver.outputReport),
              sizeof(HIDDKeyboardOutputReport),
              (TransferCallback) HIDDKeyboardDriver_ReportReceived,
              0); // No argument for callback function
}

/*
    Function: HIDDKeyboardDriver_SetReport
        Retrieves the new value of a report from the host and saves it.

    Parameters:
        type - Report type.
        length - Report length.
*/
static void HIDDKeyboardDriver_SetReport(unsigned char type,
                                         unsigned short length)
{
    trace_LOG(trace_INFO, "sReport ");

    // Check report type
    switch (type) {
    
        case HIDReportRequest_INPUT:
            // SET_REPORT requests on input reports are ignored
            USBD_Stall(0);
            break;

        case HIDReportRequest_OUTPUT:
            // Check report length
            if (length != sizeof(HIDDKeyboardOutputReport)) {

                USBD_Stall(0);
            }
            else {
            
                USBD_Read(0, // Endpoint #0
                          &(hiddKeyboardDriver.outputReport),
                          length,
                          (TransferCallback) HIDDKeyboardDriver_ReportReceived,
                          0); // No argument to the callback function
            }
            break;

        default:
            USBD_Stall(0);
    }
}

//------------------------------------------------------------------------------
//         Optional RequestReceived() callback re-implementation
//------------------------------------------------------------------------------
#if !defined(NOAUTOCALLBACK)

void USBDCallbacks_RequestReceived(const USBGenericRequest *request)
{
    HIDDKeyboardDriver_RequestHandler(request);
}

#endif

//------------------------------------------------------------------------------
//         ConfigurationChanged() callback re-implementation
//------------------------------------------------------------------------------
void USBDDriverCallbacks_ConfigurationChanged(unsigned char cfgnum)
{
    if (cfgnum > 0) {
    
        // Start receiving output reports
        USBD_Read(HIDDKeyboardDriverDescriptors_INTERRUPTOUT,
                  &(hiddKeyboardDriver.outputReport),
                  sizeof(HIDDKeyboardOutputReport),
                  (TransferCallback) HIDDKeyboardDriver_ReportReceived,
                  0); // No argument for callback function
    }
}

//------------------------------------------------------------------------------
//      Exported functions
//------------------------------------------------------------------------------
/*
    Function: HIDDKeyboardDriver_Initialize
        Initializes the HID keyboard device driver.
*/
void HIDDKeyboardDriver_Initialize()
{
    hiddKeyboardDriver.inputReportIdleRate = 0;
    HIDDKeyboardInputReport_Initialize(&(hiddKeyboardDriver.inputReport));
    HIDDKeyboardOutputReport_Initialize(&(hiddKeyboardDriver.outputReport));
    USBDDriver_Initialize(&(hiddKeyboardDriver.usbdDriver),
                          &hiddKeyboardDriverDescriptors,
                          0); // Multiple interface settings not supported
    USBD_Init();
}

/*
    Function: HIDDKeyboardDriver_RequestHandler
        Handles HID-specific SETUP request sent by the host.

    Parameters:
        request - Pointer to a USBGenericRequest instance.
*/
void HIDDKeyboardDriver_RequestHandler(const USBGenericRequest *request)
{
    trace_LOG(trace_INFO, "NewReq ");

    // Check if this is a standard request
    if (USBGenericRequest_GetType(request) == USBGenericRequest_STANDARD) {

        // This is a standard request
        switch (USBGenericRequest_GetRequest(request)) {
        
            case USBGenericRequest_GETDESCRIPTOR:
                // Check if this is a HID descriptor, otherwise forward it to
                // the standard driver
                if (!HIDDKeyboardDriver_GetDescriptor(
                        USBGetDescriptorRequest_GetDescriptorType(request),
                        USBGenericRequest_GetLength(request))) {

                    USBDDriver_RequestHandler(&(hiddKeyboardDriver.usbdDriver),
                                              request);
                }
                break;

            default:
                USBDDriver_RequestHandler(&(hiddKeyboardDriver.usbdDriver),
                                              request);
        }
    }
    // Check if this is a class request
    else if (USBGenericRequest_GetType(request) == USBGenericRequest_CLASS) {

        // This is a class-specific request
        switch (USBGenericRequest_GetRequest(request)) {

            case HIDGenericRequest_GETIDLE:
                HIDDKeyboardDriver_GetIdle();
                break;

            case HIDGenericRequest_SETIDLE:
                HIDDKeyboardDriver_SetIdle(HIDIdleRequest_GetIdleRate(request));
                break;

            case HIDGenericRequest_GETREPORT:
                HIDDKeyboardDriver_GetReport(
                    HIDReportRequest_GetReportType(request),
                    USBGenericRequest_GetLength(request));
                break;

            case HIDGenericRequest_SETREPORT:
                HIDDKeyboardDriver_SetReport(
                    HIDReportRequest_GetReportType(request),
                    USBGenericRequest_GetLength(request));
                break;

            default:
                trace_LOG(trace_WARNING,
                          "HIDDKeyboardDriver_RequestHandler: Unknown request 0x%02X\n\r",
                          USBGenericRequest_GetRequest(request));
                USBD_Stall(0);
        }
    }
    else {

        // Vendor request ?
        USBD_Stall(0);
    }
}

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
unsigned char HIDDKeyboardDriver_ChangeKeys(unsigned char *pressedKeys,
                                            unsigned char pressedKeysSize,
                                            unsigned char *releasedKeys,
                                            unsigned char releasedKeysSize)
{
    // Press keys
    while (pressedKeysSize > 0) {

        // Check if this is a standard or modifier key
        if (HIDKeypad_IsModifierKey(*pressedKeys)) {

            // Set the corresponding bit in the input report
            HIDDKeyboardInputReport_PressModifierKey(
                &(hiddKeyboardDriver.inputReport),
                *pressedKeys);
        }
        else {

            HIDDKeyboardInputReport_PressStandardKey(
                &(hiddKeyboardDriver.inputReport),
                *pressedKeys);
        }

        pressedKeysSize--;
        pressedKeys++;
    }

    // Release keys
    while (releasedKeysSize > 0) {

        // Check if this is a standard or modifier key
        if (HIDKeypad_IsModifierKey(*releasedKeys)) {

            // Set the corresponding bit in the input report
            HIDDKeyboardInputReport_ReleaseModifierKey(
                &(hiddKeyboardDriver.inputReport),
                *releasedKeys);
        }
        else {

            HIDDKeyboardInputReport_ReleaseStandardKey(
                &(hiddKeyboardDriver.inputReport),
                *releasedKeys);
        }

        releasedKeysSize--;
        releasedKeys++;
    }

    // Send input report through the interrupt IN endpoint
    return USBD_Write(HIDDKeyboardDriverDescriptors_INTERRUPTIN,
                      &(hiddKeyboardDriver.inputReport),
                      sizeof(HIDDKeyboardInputReport),
                      0,
                      0);
}

//------------------------------------------------------------------------------
/// Starts a remote wake-up sequence if the host has explicitely enabled it
/// by sending the appropriate SET_FEATURE request.
//------------------------------------------------------------------------------
void HIDDKeyboardDriver_RemoteWakeUp(void)
{
    // Remote wake-up has been enabled
    if (USBDDriver_IsRemoteWakeUpEnabled(&(hiddKeyboardDriver.usbdDriver))) {

        USBD_RemoteWakeUp();
    }
}

