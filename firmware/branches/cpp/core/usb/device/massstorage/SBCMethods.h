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
/// SCSI commands implementation.
/// 
/// !Usage
/// 
/// TODO
//------------------------------------------------------------------------------

#ifndef SBCMETHODS_H
#define SBCMETHODS_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "SBC.h"
#include "MSDLun.h"
#include "MSDDriver.h"

//------------------------------------------------------------------------------
//      Definitions
//------------------------------------------------------------------------------

//! \brief  Possible states of a SBC command.
#define SBC_STATE_READ                          0x01
#define SBC_STATE_WAIT_READ                     0x02
#define SBC_STATE_WRITE                         0x03
#define SBC_STATE_WAIT_WRITE                    0x04
#define SBC_STATE_NEXT_BLOCK                    0x05

//------------------------------------------------------------------------------
//      Exported functions
//------------------------------------------------------------------------------

void SBC_UpdateSenseData(SBCRequestSenseData *requestSenseData,
                         unsigned char senseKey,
                         unsigned char additionalSenseCode,
                         unsigned char additionalSenseCodeQualifier);

unsigned char SBC_GetCommandInformation(void          *command,
                               unsigned int  *length,
                               unsigned char *type,
                               MSDLun         *lun);

unsigned char SBC_ProcessCommand(MSDLun               *lun,
                                 MSDCommandState *commandState);

#endif //#ifndef SBCMETHODS_H

