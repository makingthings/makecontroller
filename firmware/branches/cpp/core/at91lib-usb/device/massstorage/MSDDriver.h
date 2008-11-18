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
/// \unit
/// !Purpose
/// 
/// Mass storage device driver implementation.
/// 
/// !Usage
/// TODO
//------------------------------------------------------------------------------

#ifndef MSDDRIVER_H
#define MSDDRIVER_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "MSD.h"
#include "MSDLun.h"
#include <utility/trace.h>

//------------------------------------------------------------------------------
//      Definitions
//------------------------------------------------------------------------------


//! \brief  Possible states of the MSD driver
//! \brief  Driver is expecting a command block wrapper
#define MSDDriver_STATE_READ_CBW              (1 << 0)

//! \brief  Driver is waiting for the transfer to finish
#define MSDDriver_STATE_WAIT_CBW              (1 << 1)

//! \brief  Driver is processing the received command
#define MSDDriver_STATE_PROCESS_CBW           (1 << 2)

//! \brief  Driver is starting the transmission of a command status wrapper
#define MSDDriver_STATE_SEND_CSW              (1 << 3)

//! \brief  Driver is waiting for the CSW transmission to finish
#define MSDDriver_STATE_WAIT_CSW              (1 << 4)

//! \brief  Result codes for MSD functions
//! \brief  Method was successful
#define MSDDriver_STATUS_SUCCESS              0x00

//! \brief  There was an error when trying to perform a method
#define MSDDriver_STATUS_ERROR                0x01

//! \brief  No error was encountered but the application should call the
//!         method again to continue the operation
#define MSDDriver_STATUS_INCOMPLETE           0x02

//! \brief  A wrong parameter has been passed to the method
#define MSDDriver_STATUS_PARAMETER            0x03

//! \brief  Actions to perform during the post-processing phase of a command
//! \brief  Indicates that the CSW should report a phase error
#define MSDDriver_CASE_PHASE_ERROR            (1 << 0)

//! \brief  The driver should halt the Bulk IN pipe after the transfer
#define MSDDriver_CASE_STALL_IN               (1 << 1)

//! \brief  The driver should halt the Bulk OUT pipe after the transfer
#define MSDDriver_CASE_STALL_OUT              (1 << 2)

//! \brief  Possible direction values for a data transfer
#define MSDDriver_DEVICE_TO_HOST              0
#define MSDDriver_HOST_TO_DEVICE              1
#define MSDDriver_NO_TRANSFER                 2

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------
//! \brief  Structure for holding the result of a USB transfer
//! \see    MSD_Callback
typedef struct {

    unsigned int  transferred; //!< Number of bytes transferred
    unsigned int  remaining;   //!< Number of bytes not transferred
    unsigned char semaphore;        //!< Semaphore to indicate transfer completion
    unsigned char status;           //!< Operation result code

} MSDTransfer;

//! \brief  Status of an executing command
//! \see    MSDCbw
//! \see    MSDCsw
//! \see    MSDTransfer
typedef struct {

    MSDTransfer transfer; //!< Current transfer status
    MSCbw      cbw;      //!< Received CBW
    MSCsw      csw;      //!< CSW to send
    unsigned char  state;    //!< Current command state
    unsigned char  postprocess;     //!< Actions to perform when command is complete
    unsigned int   length;   //!< Remaining length of command

} MSDCommandState;

//------------------------------------------------------------------------------
//      Inline functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//! \brief  This function is to be used as a callback for USB or LUN transfers.
//!
//!         A S_bot_transfer structure is updated with the method results.
//! \param  S_bot_transfer    Pointer to the transfer structure to update
//! \param  bStatus           Operation result code
//! \param  dBytesTransferred Number of bytes transferred by the command
//! \param  dBytesRemaining   Number of bytes not transferred
//------------------------------------------------------------------------------
static inline void MSDDriver_Callback(MSDTransfer *transfer,
                                      unsigned char status,
                                      unsigned int transferred,
                                      unsigned int remaining)
{
    trace_LOG(trace_DEBUG, "Cbk ");
    transfer->semaphore++;
    transfer->status = status;
    transfer->transferred = transferred;
    transfer->remaining = remaining;
}

//------------------------------------------------------------------------------
//      Global functions
//------------------------------------------------------------------------------

extern void MSDDriver_Initialize(MSDLun *luns, unsigned char numLuns);

extern void MSDDriver_RequestHandler(const USBGenericRequest *request);

extern void MSDDriver_StateMachine(void);

extern void MSDDriver_RemoteWakeUp(void);

#endif // #ifndef MSDDRIVER_H

