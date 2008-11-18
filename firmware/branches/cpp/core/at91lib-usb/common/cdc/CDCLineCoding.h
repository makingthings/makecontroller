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
    Title: CDCLineCoding

    About: Purpose
        Line coding structure used for by the CDC GetLineCoding and SetLineCoding
        requests.

    About: Usage
        1 - Initialize a CDCLineCoding instance using <CDCLineCoding_Initialize>.
        2 - Send a CDCLineCoding object to the host in response to a GetLineCoding
            request.
        3 - Receive a CDCLineCoding object from the host after a SetLineCoding
            request.
*/

#ifndef CDCLINECODING_H
#define CDCLINECODING_H

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: Stop bits
        CDCLineCoding_ONESTOPBIT - The transmission protocol uses one stop bit.
        CDCLineCoding_ONE5STOPBIT - The transmission protocol uses 1.5 stop bit.
        CDCLineCoding_TWOSTOPBITS - The transmissin protocol uses two stop bits.
*/
#define CDCLineCoding_ONESTOPBIT            0
#define CDCLineCoding_ONE5STOPBIT           1
#define CDCLineCoding_TWOSTOPBITS           2

/*
    Constants: Parity checking
        CDCLineCoding_NOPARITY - No parity checking.
        CDCLineCoding_ODDPARITY - Odd parity checking.
        CDCLineCoding_EVENPARITY - Even parity checking.
        CDCLineCoding_MARKPARITY - Mark parity checking.
        CDCLineCoding_SPACEPARITY - Space parity checking.
*/
#define CDCLineCoding_NOPARITY              0
#define CDCLineCoding_ODDPARITY             1
#define CDCLineCoding_EVENPARITY            2
#define CDCLineCoding_MARKPARITY            3
#define CDCLineCoding_SPACEPARITY           4

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------

#ifdef __ICCARM__          // IAR
#pragma pack(1)            // IAR
#define __attribute__(...) // IAR
#endif                     // IAR

/*
    Type: CDCLineCoding
        Format of the data returned when a GetLineCoding request is received.

    Variables:
        dwDTERate - Data terminal rate in bits per second.
        bCharFormat - Number of stop bits (see <Stop bits>).
        bParityType - Type of parity checking used (see <Parity checking>).
        bDataBits - Number of data bits (5, 6, 7, 8 or 16).
*/
typedef struct {

    unsigned int dwDTERate;
    char bCharFormat;
    char bParityType;
    char bDataBits;

} __attribute__ ((packed)) CDCLineCoding; // GCC

#ifdef __ICCARM__          // IAR
#pragma pack()             // IAR
#endif                     // IAR

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: CDCLineCoding_Initialize
        Initializes the bitrate, number of stop bits, parity checking and
        number of data bits of a CDCLineCoding object.

    Parameters:
        lineCoding - Pointer to a CDCLineCoding instance.
        bitrate - Bitrate of the virtual COM connection.
        stopbits - Number of stop bits (see <Stop bits>).
        parity - Parity check type (see <Parity checking>).
        databits - Number of data bits.
*/
extern void CDCLineCoding_Initialize(CDCLineCoding *lineCoding,
                                     unsigned int bitrate,
                                     unsigned char stopbits,
                                     unsigned char parity,
                                     unsigned char databits);

#endif //#ifndef CDCLINECODING_H

