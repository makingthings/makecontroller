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
    Title: AUDDSpeakerDriver

    About: Purpose
        Definition of a USB audio speaker driver with two channels.

    About: Usage
        TODO
*/

#ifndef AUDDSPEAKERDRIVER_H
#define AUDDSPEAKERDRIVER_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <usb/common/core/USBGenericRequest.h>
#include <usb/device/core/USBD.h>

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: Audio stream information
        AUDDSpeakerDriver_SAMPLERATE - Sample rate in Hz.
        AUDDSpeakerDriver_NUMCHANNELS - Number of channels in audio stream.
        AUDDSpeakerDriver_BYTESPERSAMPLE - Number of bytes in one sample.
        AUDDSpeakerDriver_BITSPERSAMPLE - Number of bits in one sample.
        AUDDSpeakerDriver_SAMPLESPERFRAME - Number of samples in one USB frame.
        AUDDSpeakerDriver_BYTESPERFRAME - Number of bytes in one USB frame.
*/
#define AUDDSpeakerDriver_SAMPLERATE        48000
#define AUDDSpeakerDriver_NUMCHANNELS       2
#define AUDDSpeakerDriver_BYTESPERSAMPLE    2
#define AUDDSpeakerDriver_BITSPERSAMPLE     (AUDDSpeakerDriver_BYTESPERSAMPLE * 8)
#define AUDDSpeakerDriver_BYTESPERSUBFRAME  (AUDDSpeakerDriver_NUMCHANNELS * \
                                             AUDDSpeakerDriver_BYTESPERSAMPLE)
#define AUDDSpeakerDriver_SAMPLESPERFRAME   (AUDDSpeakerDriver_SAMPLERATE / 1000 \
                                             * AUDDSpeakerDriver_NUMCHANNELS)
#define AUDDSpeakerDriver_BYTESPERFRAME     (AUDDSpeakerDriver_SAMPLESPERFRAME * \
                                             AUDDSpeakerDriver_BYTESPERSAMPLE)

/*
    Constants: Channel numbers
        AUDDSpeakerDriver_MASTERCHANNEL - Master channel.
        AUDDSpeakerDriver_LEFTCHANNEL - Front left channel.
        AUDDSpeakerDriver_RIGHTCHANNEL - Front right channel.
*/
#define AUDDSpeakerDriver_MASTERCHANNEL     0
#define AUDDSpeakerDriver_LEFTCHANNEL       1
#define AUDDSpeakerDriver_RIGHTCHANNEL      2

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: AUDDSpeakerDriver_Initialize
        Initializes an USB audio speaker device driver, as well as the underlying
        USB controller.
*/
extern void AUDDSpeakerDriver_Initialize();

/*
    Function: AUDDSpeakerDriver_RequestHandler
        Handles USB audio-specific requests and forward standard requests to
        <USBDDriver_RequestHandler>.

    Parameters:
        request - Pointer to a USBGenericRequest instance.
*/
extern void AUDDSpeakerDriver_RequestHandler(const USBGenericRequest *request);

/*
    Function: AUDDSpeakerDriver_Read
        Reads incoming audio data sent by the USB host into the provided
        buffer. When the transfer is complete, an optional callback function is
        invoked.

    Parameters:
        buffer - Pointer to the data storage buffer.
        length - Size of the buffer in bytes.
        callback - Optional callback function.
        argument - Optional argument to the callback function.

    Returns:
        USBD_STATUS_SUCCESS if the transfer is started successfully; otherwise
        an error code.
*/
extern unsigned char AUDDSpeakerDriver_Read(void *buffer,
                                            unsigned int length,
                                            TransferCallback callback,
                                            void *argument);

#endif //#ifndef AUDDSPEAKERDRIVER_H

//#ifndef SPEAKER_DRIVER_H
//#define SPEAKER_DRIVER_H
//
////------------------------------------------------------------------------------
////         Definitions
////------------------------------------------------------------------------------
//
///*! Sampling frequency in Hz */
//#define SPEAKER_SAMPLERATE          48000
///*! Number of samples in one isochronous packet (1ms frame) */
//#define SPEAKER_SAMPLESPERPACKET     (SPEAKER_SAMPLERATE / 1000)
///*! Size of one sample (in bytes) */
//#define SPEAKER_SAMPLESIZE          2
///*! Number of channels */
//#define SPEAKER_NUMCHANNELS         2
///*! Size of one frame (number of bytes sent for one sample on all channels) */
//#define SPEAKER_FRAMESIZE           (SPEAKER_SAMPLESIZE * SPEAKER_NUMCHANNELS)
///*! Required bit rate given the sample frequency, sample size and number of
//    channels. */
//#define SPEAKER_BITRATE             (SPEAKER_SAMPLERATE * SPEAKER_FRAMESIZE)
///*! Size of one isochronous packet */
//#define SPEAKER_PACKETSIZE          (SPEAKER_SAMPLESPERPACKET * SPEAKER_FRAMESIZE)
//
////------------------------------------------------------------------------------
////         Structures
////------------------------------------------------------------------------------
///*!
//    Holds the speaker driver state.
// */
//typedef struct {
//
//    S_std_class   standardDriver;
//
//    unsigned char isOutStreamEnabled;
//    unsigned char isChannelMuted[SPEAKER_NUMCHANNELS+1];
//
//    Callback_f    outStreamStatusChanged;
//    Callback_f    outStreamMuteChanged;
//
//} __attribute__((packed)) S_speaker;
//
////------------------------------------------------------------------------------
////         Exported functions
////------------------------------------------------------------------------------
//
//extern void SPK_Init(S_speaker *speakerDriver, const S_usb *usbDriver);
//extern void SPK_SetCallbacks(S_speaker *speakerDriver,
//                             Callback_f outStreamStatusChanged,
//                             Callback_f outStreamMuteChanged);
//extern void SPK_RequestHandler(S_speaker *speakerDriver);
//extern char SPK_Read(S_speaker *speakerDriver,
//                     void *buffer,
//                     unsigned int length,
//                     Callback_f callback,
//                     void *argument);
//
//#endif //#ifndef SPEAKER_DRIVER_H
//
