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
    Title: AUDDSpeakerChannel

    About: Purpose
        Manipulation of the channels of an USB audio speaker device.

    About: Usage
        1 - Initialize a AUDDSpeakerChannel instance using
            <AUDDSpeakerChannel_Initialize>.
        2 - Retrieves the current status of a channel with the
            <AUDDSpeakerChannel_IsMuted> method.
        3 - Re-implement the <AUDDSpeakerChannel_MuteChanged> callback to get
            notified when the status of a channel changes.
*/

#ifndef AUDDSPEAKERCHANNEL_H
#define AUDDSPEAKERCHANNEL_H

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------
/*
    Type: AUDDSpeakerChannel
        Modelizes a channel of an USB audio speaker device.

    Variables:
        number - Zero-based channel number in the audio function.
        muted - Indicates if the channel is currently muted.
*/
typedef struct {

    unsigned char number;
    unsigned char muted;

} AUDDSpeakerChannel;

//------------------------------------------------------------------------------
//         Callbacks
//------------------------------------------------------------------------------
/*
    Function: AUDDSpeakerChannel_MuteChanged
        Callback triggered when the mute status of a channel changes. This is
        a default implementation which does nothing and must be overriden.

    Parameters:
        channel - Pointer to a AUDDSpeakerChannel instance.
        muted - Indicates the new mute status of the channel.
*/
extern void AUDDSpeakerChannel_MuteChanged(AUDDSpeakerChannel *channel,
                                           unsigned char muted);

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: AUDDSpeakerChannel_Initialize
        Initializes the member variables of an AUDDSpeakerChannel object to the
        given values.

    Parameters:
        channel - Pointer to an AUDDSpeakerChannel instance.
        number - Channel number in the audio function.
        muted - Indicates if the channel is muted.
*/
extern void AUDDSpeakerChannel_Initialize(AUDDSpeakerChannel *channel,
                                          unsigned char number,
                                          unsigned char muted);

/*
    Function: AUDDSpeakerChannel_GetNumber
        Indicates the number of a channel.

    Parameters:
        channel - Pointer to an AUDDSpeakerChannel instance.

    Returns:
        Channel number.
*/
extern unsigned char AUDDSpeakerChannel_GetNumber(
    const AUDDSpeakerChannel *channel);

/*
    Function: AUDDSpeakerChannel_Mute
        Mutes the given channel and triggers the MuteChanged callback if
        necessary.

    Parameters:
        channel - Pointer to an AUDDSpeakerChannelInstance.
*/
extern void AUDDSpeakerChannel_Mute(AUDDSpeakerChannel *channel);

/*
    Function: AUDDSpeakerChannel_Unute
        Unmutes the given channel and triggers the MuteChanged callback if
        necessary.

    Parameters:
        channel - Pointer to an AUDDSpeakerChannelInstance.
*/
extern void AUDDSpeakerChannel_Unmute(AUDDSpeakerChannel *channel);

/*
    Function: AUDDSpeakerChannel_IsMuted
        Indicates if the given channel is currently muted or not.

    Parameters:
        channel - Pointer an AUDDSpeakerChannel instance.

    Returns:
        1 if the channel is muted; otherwise 0.
*/
extern unsigned char AUDDSpeakerChannel_IsMuted(
    const AUDDSpeakerChannel *channel);

#endif //#ifndef AUDDSPEAKERCHANNEL_H

