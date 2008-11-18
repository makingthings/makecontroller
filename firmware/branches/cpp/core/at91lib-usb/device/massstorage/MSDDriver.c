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
//      Includes
//------------------------------------------------------------------------------

#include "MSDDriver.h"
#include "MSDDriverDescriptors.h"
#include "SBCMethods.h"
#include <utility/trace.h>
#include <usb/common/core/USBGenericRequest.h>
#include <usb/common/core/USBFeatureRequest.h>
#include <usb/device/core/USBD.h>
#include <usb/device/core/USBDDriver.h>

//------------------------------------------------------------------------------
//      Structures
//------------------------------------------------------------------------------

//! \brief  MSD driver state variables
//! \see    MSDCommandState
//! \see    S_std_class
//! \see    MSDLun
typedef struct {

    MSDLun *luns;
    MSDCommandState commandState;       //!< State of the currently executing command
    unsigned char maxLun;             //!< Maximum LUN index
    unsigned char state;              //!< Current state of the driver
    unsigned char waitResetRecovery; //!< Indicates if the driver is
                                             //!< waiting for a reset recovery
} MSDDriver;

//------------------------------------------------------------------------------
//         Internal variables
//------------------------------------------------------------------------------

/// Mass storage device driver instance.
static MSDDriver msdDriver;

/// Standard device driver instance.
static USBDDriver usbdDriver;

//------------------------------------------------------------------------------
//      Internal functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//! \brief  Returns the expected transfer length and direction (IN, OUT or don't
//!         care) from the host point-of-view.
//! \param  cbw    Pointer to the CBW to examinate
//! \param  pLength Expected length of command
//! \param  pType   Expected direction of command
//------------------------------------------------------------------------------
static void MSDDriver_GetCommandInformation(MSCbw *cbw,
                                            unsigned int  *length,
                                            unsigned char *type)
{
    // Expected host transfer direction and length
    (*length) = cbw->dCBWDataTransferLength;

    if (*length == 0) {

        (*type) = MSDDriver_NO_TRANSFER;
    }
    else if ((cbw->bmCBWFlags & MSD_CBW_DEVICE_TO_HOST) != 0) {

        (*type) = MSDDriver_DEVICE_TO_HOST;
    }
    else {

        (*type) = MSDDriver_HOST_TO_DEVICE;
    }
}

//------------------------------------------------------------------------------
//! \brief  Pre-processes a command by checking the differences between the
//!         host and device expectations in term of transfer type and length.
//!
//!         Once one of the thirteen cases is identified, the actions to do
//!         during the post-processing phase are stored in the dCase variable
//!         of the command state.
//! \param  pBot Pointer to a S_bot instance
//! \return 1 if the command is supported, false otherwise
//------------------------------------------------------------------------------
static unsigned char MSDDriver_PreProcessCommand()
{
    unsigned int        hostLength;
    unsigned int        deviceLength;
    unsigned char       hostType;
    unsigned char       deviceType;
    unsigned char                isCommandSupported;
    MSDCommandState *commandState = &(msdDriver.commandState);
    MSCsw           *csw = &(commandState->csw);
    MSCbw           *cbw = &(commandState->cbw);
    MSDLun               *lun = &(msdDriver.luns[(unsigned char) cbw->bCBWLUN]);

    // Get information about the command
    // Host-side
    MSDDriver_GetCommandInformation(cbw, &hostLength, &hostType);

    // Device-side
    isCommandSupported = SBC_GetCommandInformation(cbw->pCommand,
                                                   &deviceLength,
                                                   &deviceType,
                                                   lun);

    // Initialize data residue and result status
    csw->dCSWDataResidue = 0;
    csw->bCSWStatus = MSD_CSW_COMMAND_PASSED;

    // Check if the command is supported
    if (isCommandSupported) {

        // Identify the command case
        if(hostType == MSDDriver_NO_TRANSFER) {

            // Case 1  (Hn = Dn)
            if(deviceType == MSDDriver_NO_TRANSFER) {

                trace_LOG(trace_WARNING, "W: Case 1\n\r");
                commandState->postprocess = 0;
                commandState->length = 0;
            }
            else if(deviceType == MSDDriver_DEVICE_TO_HOST) {

                // Case 2  (Hn < Di)
                trace_LOG(trace_WARNING, "W: MSDDriver_PreProcessCommand: Case 2\n\r");
                commandState->postprocess = MSDDriver_CASE_PHASE_ERROR;
                commandState->length = 0;
            }
            else { //if(deviceType == MSDDriver_HOST_TO_DEVICE) {

                // Case 3  (Hn < Do)
                trace_LOG(trace_WARNING, "W: MSDDriver_PreProcessCommand: Case 3\n\r");
                commandState->postprocess = MSDDriver_CASE_PHASE_ERROR;
                commandState->length = 0;
            }
        }

        // Case 4  (Hi > Dn)
        else if(hostType == MSDDriver_DEVICE_TO_HOST) {

            if(deviceType == MSDDriver_NO_TRANSFER) {

                trace_LOG(trace_WARNING, "W: MSDDriver_PreProcessCommand: Case 4\n\r");
                commandState->postprocess = MSDDriver_CASE_STALL_IN;
                commandState->length = 0;
                csw->dCSWDataResidue = hostLength;
            }
            else if(deviceType == MSDDriver_DEVICE_TO_HOST) {

                if(hostLength > deviceLength) {

                    // Case 5  (Hi > Di)
                    trace_LOG(trace_WARNING, "W: MSDDriver_PreProcessCommand: Case 5\n\r");
                    commandState->postprocess = MSDDriver_CASE_STALL_IN;
                    commandState->length = deviceLength;
                    csw->dCSWDataResidue = hostLength - deviceLength;
                }
                else if(hostLength == deviceLength) {

                    // Case 6  (Hi = Di)
//                    trace_LOG(trace_WARNING, "W: Case 6\n\r");
                    commandState->postprocess = 0;
                    commandState->length = deviceLength;
                }
                else { //if(hostLength < deviceLength) {

                    // Case 7  (Hi < Di)
                    trace_LOG(trace_WARNING, "W: MSDDriver_PreProcessCommand: Case 7\n\r");
                    commandState->postprocess = MSDDriver_CASE_PHASE_ERROR;
                    commandState->length = hostLength;
                }
            }
            else { //if(deviceType == MSDDriver_HOST_TO_DEVICE) {

                // Case 8  (Hi <> Do)
                trace_LOG(trace_WARNING, "W: MSDDriver_PreProcessCommand: Case 8\n\r");
                commandState->postprocess = MSDDriver_CASE_STALL_IN | MSDDriver_CASE_PHASE_ERROR;
                commandState->length = 0;
            }
        }
        else if(hostType == MSDDriver_HOST_TO_DEVICE) {

            if(deviceType == MSDDriver_NO_TRANSFER) {

                // Case 9  (Ho > Dn)
                trace_LOG(trace_WARNING, "W: MSDDriver_PreProcessCommand: Case 9\n\r");
                commandState->postprocess = MSDDriver_CASE_STALL_OUT;
                commandState->length = 0;
                csw->dCSWDataResidue = hostLength;
            }
            else if(deviceType == MSDDriver_DEVICE_TO_HOST) {

                // Case 10 (Ho <> Di)
                trace_LOG(trace_WARNING, "W: MSDDriver_PreProcessCommand: Case 10\n\r");
                commandState->postprocess = MSDDriver_CASE_STALL_OUT | MSDDriver_CASE_PHASE_ERROR;
                commandState->length = 0;
            }
            else { //if(deviceType == MSDDriver_HOST_TO_DEVICE) {

                if(hostLength > deviceLength) {

                    // Case 11 (Ho > Do)
                    trace_LOG(trace_WARNING, "W: MSDDriver_PreProcessCommand: Case 11\n\r");
                    commandState->postprocess = MSDDriver_CASE_STALL_OUT;
                    commandState->length = deviceLength;
                    csw->dCSWDataResidue = hostLength - deviceLength;
                }
                else if(hostLength == deviceLength) {

                    // Case 12 (Ho = Do)
                    trace_LOG(trace_WARNING, "W: MSDDriver_PreProcessCommand: Case 12\n\r");
                    commandState->postprocess = 0;
                    commandState->length = deviceLength;
                }
                else { //if(hostLength < deviceLength) {

                    // Case 13 (Ho < Do)
                    trace_LOG(trace_WARNING, "W: MSDDriver_PreProcessCommand: Case 13\n\r");
                    commandState->postprocess = MSDDriver_CASE_PHASE_ERROR;
                    commandState->length = hostLength;
                }
            }
        }
    }

    return isCommandSupported;
}

//------------------------------------------------------------------------------
//! \brief  Post-processes a command given the case identified during the
//!         pre-processing step.
//!
//!         Depending on the case, one of the following actions can be done:
//!             - Bulk IN endpoint is stalled
//!             - Bulk OUT endpoint is stalled
//!             - CSW status set to phase error
//! \param  pBot Pointer to a S_bot instance
//------------------------------------------------------------------------------
static void MSDDriver_PostProcessCommand()
{
    MSDCommandState *commandState = &(msdDriver.commandState);
    MSCsw           *csw = &(commandState->csw);

    // STALL Bulk IN endpoint ?
    if ((commandState->postprocess & MSDDriver_CASE_STALL_IN) != 0) {

        trace_LOG(trace_INFO, "StallIn ");
        USBD_Halt(MSDDriverDescriptors_BULKIN);
    }

    // STALL Bulk OUT endpoint ?
    if ((commandState->postprocess & MSDDriver_CASE_STALL_OUT) != 0) {

        trace_LOG(trace_INFO, "StallOut ");
        USBD_Halt(MSDDriverDescriptors_BULKOUT);
    }

    // Set CSW status code to phase error ?
    if ((commandState->postprocess & MSDDriver_CASE_PHASE_ERROR) != 0) {

        trace_LOG(trace_INFO, "PhaseErr ");
        csw->bCSWStatus = MSD_CSW_PHASE_ERROR;
    }
}

//------------------------------------------------------------------------------
//! \brief  Processes the latest command received by the device.
//! \param  pBot Pointer to a S_bot instance
//! \return 1 if the command has been completed, false otherwise.
//------------------------------------------------------------------------------
static unsigned char MSDDriver_ProcessCommand()
{
    unsigned char       status;
    MSDCommandState *commandState = &(msdDriver.commandState);
    MSCbw           *cbw = &(commandState->cbw);
    MSCsw           *csw = &(commandState->csw);
    MSDLun               *lun = &(msdDriver.luns[(unsigned char) cbw->bCBWLUN]);
    unsigned char                isCommandComplete = 0;

    // Check if LUN is valid
    if (cbw->bCBWLUN > msdDriver.maxLun) {

        trace_LOG(trace_WARNING, "W: MSDDriver_ProcessCommand: Requested LUN does not exist\n\r");
        status = MSDDriver_STATUS_ERROR;
    }
    else {

        // Process command
        if (msdDriver.maxLun > 0) {

            trace_LOG(trace_INFO, "LUN%d ", cbw->bCBWLUN);
        }

        status = SBC_ProcessCommand(lun, commandState);
    }

    // Check command result code
    if (status == MSDDriver_STATUS_PARAMETER) {

        trace_LOG(trace_WARNING, "W: MSDDriver_ProcessCommand: Unknown command 0x%02X\n\r",
                      cbw->pCommand[0]);

        // Update sense data
        SBC_UpdateSenseData(&(lun->requestSenseData),
                            SBC_SENSE_KEY_ILLEGAL_REQUEST,
                            SBC_ASC_INVALID_FIELD_IN_CDB,
                            0);

        // Result codes
        csw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
        isCommandComplete = 1;

        // stall the request, IN or OUT
        if (((cbw->bmCBWFlags & MSD_CBW_DEVICE_TO_HOST) == 0)
            && (cbw->dCBWDataTransferLength > 0)) {

            // Stall the OUT endpoint : host to device
            USBD_Halt(MSDDriverDescriptors_BULKOUT);
            trace_LOG(trace_INFO, "StaOUT ");
        }
        else {

            // Stall the IN endpoint : device to host
            USBD_Halt(MSDDriverDescriptors_BULKIN);
            trace_LOG(trace_INFO, "StaIN ");
        }
    }
    else if (status == MSDDriver_STATUS_ERROR) {

        trace_LOG(trace_WARNING, "W: MSD_ProcessCommand: Command failed\n\r");

        // Update sense data
// TODO (jjoannic#1#): Change code
        SBC_UpdateSenseData(&(lun->requestSenseData),
                            SBC_SENSE_KEY_MEDIUM_ERROR,
                            SBC_ASC_INVALID_FIELD_IN_CDB,
                            0);

        // Result codes
        csw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
        isCommandComplete = 1;
    }
    else {

        // Update sense data
        SBC_UpdateSenseData(&(lun->requestSenseData),
                            SBC_SENSE_KEY_NO_SENSE,
                            0,
                            0);

        // Is command complete ?
        if (status == MSDDriver_STATUS_SUCCESS) {

            isCommandComplete = 1;
        }
    }

    // Check if command has been completed
    if (isCommandComplete) {

        trace_LOG(trace_INFO, "Cplt ");

        // Adjust data residue
        if (commandState->length != 0) {

            csw->dCSWDataResidue += commandState->length;

            // STALL the endpoint waiting for data
            if ((cbw->bmCBWFlags & MSD_CBW_DEVICE_TO_HOST) == 0) {

                // Stall the OUT endpoint : host to device
                USBD_Halt(MSDDriverDescriptors_BULKOUT);
                trace_LOG(trace_INFO, "StaOUT ");
            }
            else {

                // Stall the IN endpoint : device to host
                USBD_Halt(MSDDriverDescriptors_BULKIN);
                trace_LOG(trace_INFO, "StaIN ");
            }
        }

        // Reset command state
        commandState->state = 0;
    }

    return isCommandComplete;
}

//------------------------------------------------------------------------------
//! \brief  Resets the state of the BOT driver
//! \param  pBot Pointer to a S_bot instance
//! \see    S_bot
//------------------------------------------------------------------------------
void MSDDriver_Reset()
{
    trace_LOG(trace_INFO, "MSDReset ");

    msdDriver.state = MSDDriver_STATE_READ_CBW;
    msdDriver.waitResetRecovery = 0;
    msdDriver.commandState.state = 0;
}

//------------------------------------------------------------------------------
//         Callback re-implementation
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// Invoked when a new SETUP request is received from the host. Forwards the
/// request to the Mass Storage device driver handler function.
/// \param request  Pointer to a USBGenericRequest instance.
//------------------------------------------------------------------------------
void USBDCallbacks_RequestReceived(const USBGenericRequest *request)
{
    MSDDriver_RequestHandler(request);
}

//------------------------------------------------------------------------------
/// Invoked when the configuration of the device changes. Resets the mass
/// storage driver.
//------------------------------------------------------------------------------
void USBDDriverCallbacks_ConfigurationChanged(unsigned char cfgnum)
{
    if (cfgnum > 0) {

        MSDDriver_Reset();
    }
}

//------------------------------------------------------------------------------
//      Exported functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//! \brief  Initializes a BOT driver and the associated USB driver.
//! \param  pBot    Pointer to a S_bot instance
//! \param  pUsb    USB driver to use
//! \param  lun    Pointer to a list of LUNs
//! \param  numLuns Number of LUN in list
//! \see    S_bot
//! \see    S_usb
//------------------------------------------------------------------------------
void MSDDriver_Initialize(MSDLun *luns, unsigned char numLuns)
{
    trace_LOG(trace_INFO, "I: MSD init\n\r");

    // Command state initialization
    msdDriver.commandState.state = 0;
    msdDriver.commandState.postprocess = 0;
    msdDriver.commandState.length = 0;
    msdDriver.commandState.transfer.semaphore = 0;

    // LUNs
    msdDriver.luns = luns;
    msdDriver.maxLun = (unsigned char) (numLuns - 1);

    // Reset BOT driver
    MSDDriver_Reset();

    // Init the USB driver
    USBDDriver_Initialize(&usbdDriver, &msdDriverDescriptors, 0);
    USBD_Init();
}

//------------------------------------------------------------------------------
//! \brief  Handler for incoming SETUP requests on default Control endpoint 0.
//!
//!         Standard requests are forwarded to the STD_RequestHandler method.
//! \param  pBot Pointer to a S_bot instance
//------------------------------------------------------------------------------
void MSDDriver_RequestHandler(const USBGenericRequest *request)
{
    trace_LOG(trace_INFO, "NewReq ");

    // Handle requests
    switch (USBGenericRequest_GetRequest(request)) {
    //---------------------
    case USBGenericRequest_CLEARFEATURE:
    //---------------------
        trace_LOG(trace_INFO, "ClrFeat ");

        switch (USBFeatureRequest_GetFeatureSelector(request)) {

        //---------------------
        case USBFeatureRequest_ENDPOINTHALT:
        //---------------------
            trace_LOG(trace_INFO, "Hlt ");

            // Do not clear the endpoint halt status if the device is waiting
            // for a reset recovery sequence
            if (!msdDriver.waitResetRecovery) {

                // Forward the request to the standard handler
                USBDDriver_RequestHandler(&usbdDriver, request);
            }
            else {

                trace_LOG(trace_INFO, "No ");
            }

            USBD_Write(0, 0, 0, 0, 0);
            break;

        //------
        default:
        //------
            // Forward the request to the standard handler
            USBDDriver_RequestHandler(&usbdDriver, request);
        }
        break;

    //-------------------
    case MSD_GET_MAX_LUN:
    //-------------------
        trace_LOG(trace_INFO, "gMaxLun ");

        // Check request parameters
        if ((request->wValue == 0)
            && (request->wIndex == 0)
            && (request->wLength == 1)) {

            USBD_Write(0, &(msdDriver.maxLun), 1, 0, 0);

        }
        else {

            trace_LOG(trace_WARNING, "W: MSDDriver_RequestHandler: GetMaxLUN(%d,%d,%d)\n\r",
                          request->wValue, request->wIndex, request->wLength);
            USBD_Stall(0);
        }
        break;

    //-----------------------
    case MSD_BULK_ONLY_RESET:
    //-----------------------
        trace_LOG(trace_INFO, "Rst ");

        // Check parameters
        if ((request->wValue == 0)
            && (request->wIndex == 0)
            && (request->wLength == 0)) {

            // Reset the MSD driver
            MSDDriver_Reset();
            USBD_Write(0, 0, 0, 0, 0);
        }
        else {

            trace_LOG(trace_WARNING, "W: MSDDriver_RequestHandler: Reset(%d,%d,%d)\n\r",
                          request->wValue, request->wIndex, request->wLength);
            USBD_Stall(0);
        }
        break;

    //------
    default:
    //------
        // Forward request to standard handler
        USBDDriver_RequestHandler(&usbdDriver, request);

        break;
    }
}

//------------------------------------------------------------------------------
//! \brief  State machine for the BOT driver
//! \param  pBot Pointer to a S_bot instance
//------------------------------------------------------------------------------
void MSDDriver_StateMachine(void)
{
    MSDCommandState *commandState = &(msdDriver.commandState);
    MSCbw           *cbw = &(commandState->cbw);
    MSCsw           *csw = &(commandState->csw);
    MSDTransfer      *transfer = &(commandState->transfer);
    unsigned char       status;

    // Identify current driver state
    switch (msdDriver.state) {
    //----------------------
    case MSDDriver_STATE_READ_CBW:
    //----------------------
        // Start the CBW read operation
        transfer->semaphore = 0;
        status = USBD_Read(MSDDriverDescriptors_BULKOUT,
                           cbw,
                           MSD_CBW_SIZE,
                           (TransferCallback) MSDDriver_Callback,
                           (void *) transfer);

        // Check operation result code
        if (status == USBD_STATUS_SUCCESS) {

            // If the command was successful, wait for transfer
            msdDriver.state = MSDDriver_STATE_WAIT_CBW;
        }
        break;

    //----------------------
    case MSDDriver_STATE_WAIT_CBW:
    //----------------------
        // Check transfer semaphore
        if (transfer->semaphore > 0) {

            // Take semaphore and terminate transfer
            transfer->semaphore--;

            // Check if transfer was successful
            if (transfer->status == USBD_STATUS_SUCCESS) {

                trace_LOG(trace_INFO, "------------------------------\n\r");

                // Process received command
                msdDriver.state = MSDDriver_STATE_PROCESS_CBW;
            }
            else if (transfer->status == USBD_STATUS_RESET) {

                trace_LOG(trace_INFO, "I: MSDDriver_StateMachine: Endpoint resetted\n\r");
                msdDriver.state = MSDDriver_STATE_READ_CBW;
            }
            else {

                trace_LOG(trace_WARNING, "W: MSDDriver_StateMachine: Failed to read CBW\n\r");
                msdDriver.state = MSDDriver_STATE_READ_CBW;
            }
        }
        break;

    //-------------------------
    case MSDDriver_STATE_PROCESS_CBW:
    //-------------------------
        // Check if this is a new command
        if (commandState->state == 0) {

            // Copy the CBW tag
            csw->dCSWTag = cbw->dCBWTag;

            // Check that the CBW is 31 bytes long
            if ((transfer->transferred != MSD_CBW_SIZE) ||
                (transfer->remaining != 0)) {

                trace_LOG(trace_WARNING, "W: MSDDriver_StateMachine: Invalid CBW (too short or too long)\n\r");

                // Wait for a reset recovery
                msdDriver.waitResetRecovery = 1;

                // Halt the Bulk-IN and Bulk-OUT pipes
                USBD_Halt(MSDDriverDescriptors_BULKOUT);
                USBD_Halt(MSDDriverDescriptors_BULKIN);

                csw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
                msdDriver.state = MSDDriver_STATE_READ_CBW;

            }
            // Check the CBW Signature
            else if (cbw->dCBWSignature != MSD_CBW_SIGNATURE) {

                trace_LOG(trace_WARNING, "W: MSD_BOTStateMachine: Invalid CBW (Bad signature)\n\r");

                // Wait for a reset recovery
                msdDriver.waitResetRecovery = 1;

                // Halt the Bulk-IN and Bulk-OUT pipes
                USBD_Halt(MSDDriverDescriptors_BULKOUT);
                USBD_Halt(MSDDriverDescriptors_BULKIN);

                csw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
                msdDriver.state = MSDDriver_STATE_READ_CBW;
            }
            else {

                // Pre-process command
                MSDDriver_PreProcessCommand();
            }
        }

        // Process command
        if (csw->bCSWStatus == MSDDriver_STATUS_SUCCESS) {

            if (MSDDriver_ProcessCommand()) {

                // Post-process command if it is finished
                MSDDriver_PostProcessCommand();
                msdDriver.state = MSDDriver_STATE_SEND_CSW;
            }
            trace_LOG(trace_INFO, "\n\r");
        }

        break;

    //----------------------
    case MSDDriver_STATE_SEND_CSW:
    //----------------------
        // Set signature
        csw->dCSWSignature = MSD_CSW_SIGNATURE;

        // Start the CSW write operation
        status = USBD_Write(MSDDriverDescriptors_BULKIN,
                            csw,
                            MSD_CSW_SIZE,
                            (TransferCallback) MSDDriver_Callback,
                            (void *) transfer);

        // Check operation result code
        if (status == USBD_STATUS_SUCCESS) {

            trace_LOG(trace_INFO, "SendCSW ");

            // Wait for end of transfer
            msdDriver.state = MSDDriver_STATE_WAIT_CSW;
        }
        break;

    //----------------------
    case MSDDriver_STATE_WAIT_CSW:
    //----------------------
        // Check transfer semaphore
        if (transfer->semaphore > 0) {

            // Take semaphore and terminate transfer
            transfer->semaphore--;

            // Check if transfer was successful
            if (transfer->status == USBD_STATUS_RESET) {

                trace_LOG(trace_INFO, "MSDDriver_StateMachine: Endpoint resetted\n\r");
            }
            else if (transfer->status == USBD_STATUS_ABORTED) {

                trace_LOG(trace_WARNING, "W: MSDDriver_StateMachine: Failed to send CSW\n\r");
            }
            else {

                trace_LOG(trace_INFO, "ok");
            }

            // Read new CBW
            msdDriver.state = MSDDriver_STATE_READ_CBW;
        }
        break;
    }
}

//------------------------------------------------------------------------------
/// Starts a remote wake-up sequence if the host has explicitely enabled it
/// by sending the appropriate SET_FEATURE request.
//------------------------------------------------------------------------------
void MSDDriver_RemoteWakeUp(void)
{
    // Remote wake-up has been enabled
    if (USBDDriver_IsRemoteWakeUpEnabled(&usbdDriver)) {

        USBD_RemoteWakeUp();
    }
    // Remote wake-up NOT enabled
    else {

        trace_LOG(trace_WARNING, "-W- MSDDriver_RemoteWakeUp: Host has not enabled remote wake-up\n\r");
    }
}

