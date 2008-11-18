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
/// Mass Storage class definitions.
/// 
/// !Usage
/// 
/// TODO
//------------------------------------------------------------------------------

#ifndef MSD_H
#define MSD_H

//------------------------------------------------------------------------------
//      Definitions
//------------------------------------------------------------------------------
// Class-specific requests
#define MSD_BULK_ONLY_RESET                     0xFF
#define MSD_GET_MAX_LUN                         0xFE

// Subclass codes
// Reduced Block Commands (RBC) T10
#define MSD_SUBCLASS_RBC                        0x01
// C/DVD devices
#define MSD_SUBCLASS_SFF_MCC                    0x02
// Tape device
#define MSD_SUBCLASS_QIC                        0x03
// Floppy disk drive (FDD) device
#define MSD_SUBCLASS_UFI                        0x04
// Floppy disk drive (FDD) device
#define MSD_SUBCLASS_SFF                        0x05
// SCSI transparent command set
#define MSD_SUBCLASS_SCSI                       0x06

// Table 3.1 - Mass Storage Transport Protocol (see usb_msc_overview_1.2.pdf)
#define MSD_PROTOCOL_CBI_COMPLETION             0x00
#define MSD_PROTOCOL_CBI                        0x01
#define MSD_PROTOCOL_BULK_ONLY                  0x50

//! Test unit control:
// TODO (jjoannic#1#): Document
#define CTRL_NOT_READY                          0x00
#define CTRL_GOOD                               0x01
#define CTRL_BUSY                               0x02

// Command block wrapper
#define MSD_CBW_SIZE                            31          //!< Command Block Wrapper Size
#define MSD_CBW_SIGNATURE                       0x43425355  //!< 'USBC' 0x43425355

// Command status wrapper
#define MSD_CSW_SIZE                            13
#define MSD_CSW_SIGNATURE                       0x53425355

//! Table 5.3 - Command Block Status Values (usbmassbulk_10.pdf)
#define MSD_CSW_COMMAND_PASSED                  0
#define MSD_CSW_COMMAND_FAILED                  1
#define MSD_CSW_PHASE_ERROR                     2

// CBW bmCBWFlags field
#define MSD_CBW_DEVICE_TO_HOST                  (1 << 7)

//------------------------------------------------------------------------------
//      Structures
//------------------------------------------------------------------------------

//! Table 5.1 - Command Block Wrapper (see usbmassbulk_10.pdf)
//! The CBW shall start on a packet boundary and shall end as a
//! short packet with exactly 31 (1Fh) bytes transferred.
typedef struct {

    unsigned int  dCBWSignature;          //!< 'USBC' 0x43425355 (little endian)
    unsigned int  dCBWTag;                //!< Must be the same as dCSWTag
    unsigned int  dCBWDataTransferLength; //!< Number of bytes transfer
    unsigned char bmCBWFlags;             //!< indicates the directin of the
                                              //!< transfer: 0x80=IN=device-to-host,
                                              //!< 0x00=OUT=host-to-device
    unsigned char bCBWLUN   :4,           //!< bits 0->3: bCBWLUN
                  bReserved1:4;           //!< reserved
    unsigned char bCBWCBLength:5,         //!< bits 0->4: bCBWCBLength
                  bReserved2  :3;         //!< reserved
    unsigned char pCommand[16];           // Command block

} MSCbw;

//! Table 5.2 - Command Status Wrapper (CSW) (usbmassbulk_10.pdf)
typedef struct
{
    unsigned int  dCSWSignature;   //!< 'USBS' 0x53425355 (little endian)
    unsigned int  dCSWTag;         //!< Must be the same as dCBWTag
    unsigned int  dCSWDataResidue; //!< For Data-Out the device shall report in the dCSWDataResidue the difference between the amount of
                                   // data expected as stated in the dCBWDataTransferLength, and the actual amount of data processed by
                                   // the device. For Data-In the device shall report in the dCSWDataResidue the difference between the
                                   // amount of data expected as stated in the dCBWDataTransferLength and the actual amount of relevant
                                   // data sent by the device. The dCSWDataResidue shall not exceed the value sent in the dCBWDataTransferLength.
    unsigned char bCSWStatus;     //!< Indicates the success or failure of the command.

} MSCsw;

#endif //#ifndef MSD_H

