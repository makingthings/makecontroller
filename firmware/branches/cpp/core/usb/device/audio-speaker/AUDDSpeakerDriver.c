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

#include "AUDDSpeakerDriver.h"
#include "AUDDSpeakerDriverDescriptors.h"
#include "AUDDSpeakerChannel.h"
#include <utility/trace.h>
#include <utility/led.h>
#include <usb/common/audio/AUDGenericRequest.h>
#include <usb/common/audio/AUDFeatureUnitRequest.h>
#include <usb/device/core/USBDDriver.h>

//------------------------------------------------------------------------------
//         Internal types
//------------------------------------------------------------------------------
/*
    Type: AUDDSpeakerDriver
        Audio speaker driver internal state.

    Variables:
        usbdDriver - USB device driver instance.
        channels - List of AUDDSpeakerChannel instances.
*/
typedef struct {

    USBDDriver usbdDriver;
    AUDDSpeakerChannel channels[AUDDSpeakerDriver_NUMCHANNELS+1];

} AUDDSpeakerDriver;

//------------------------------------------------------------------------------
//         Internal variables
//------------------------------------------------------------------------------
/*
    Variables: Internal variables
        auddSpeakerDriver - Global USB audio speaker driver instance.
        auddSpeakerDriverInterfaces - Array for storing the current setting of
            each interface.
        muted - Intermediate storage variable for the mute status of a channel.
*/
static AUDDSpeakerDriver auddSpeakerDriver;
static unsigned char auddSpeakerDriverInterfaces[2];
static unsigned char muted;

//------------------------------------------------------------------------------
//         Internal functions
//------------------------------------------------------------------------------
/*
    Function: AUDDSpeakerDriver_MuteReceived
        Callback triggered after the new mute status of a channel has been read
        by <AUDDSpeakerDriver_SetFeatureCurrentValue>. Changes the mute status
        of the given channel accordingly.

    Parameters:
        channel - Number of the channel whose mute status has changed.
*/
static void AUDDSpeakerDriver_MuteReceived(unsigned char channel)
{
    if (muted) {
    
        AUDDSpeakerChannel_Mute(&(auddSpeakerDriver.channels[channel]));
    }
    else {

        AUDDSpeakerChannel_Unmute(&(auddSpeakerDriver.channels[channel]));
    }

    USBD_Write(0, 0, 0, 0, 0);
}

/*
    Function: AUDDSpeakerDriver_SetFeatureCurrentValue
        Sets the current value of a particular Feature control of a channel.

    Parameters:
        channel - Channel number.
        control - Control selector value (see TODO).
        length - Length of the data containing the new value.
*/
static void AUDDSpeakerDriver_SetFeatureCurrentValue(unsigned char channel,
                                                     unsigned char control,
                                                     unsigned short length)
{
    trace_LOG(trace_INFO, "sFeature ");
    trace_LOG(trace_DEBUG, "\b(CS%d, CN%d, L%d) ", control, channel, length);

    // Check the the requested control is supported
    // Mute control on master channel
    if ((control == AUDFeatureUnitRequest_MUTE)
        && (channel < (AUDDSpeakerDriver_NUMCHANNELS+1))
        && (length == 1)) {

        unsigned int argument = channel; // Avoids compiler warning
        USBD_Read(0, // Endpoint #0
                  &muted,
                  sizeof(muted),
                  (TransferCallback) AUDDSpeakerDriver_MuteReceived,
                  (void *) argument);
    }
    // Control/channel combination not supported
    else {

        USBD_Stall(0);
    }
}

/*
    Function: AUDDSpeakerDriver_GetFeatureCurrentValue
        Sends the current value of a particular channel Feature to the USB host.

    Parameters:
        channel - Channel number.
        control - Control selector value (see TODO).
        length - Length of the data to return.
*/
static void AUDDSpeakerDriver_GetFeatureCurrentValue(unsigned char channel,
                                                     unsigned char control,
                                                     unsigned char length)
{
    trace_LOG(trace_INFO, "gFeature ");
    trace_LOG(trace_DEBUG, "\b(CS%d, CN%d, L%d) ", control, channel, length);

    // Check that the requested control is supported
    // Master channel mute control
    if ((control == AUDFeatureUnitRequest_MUTE)
        && (channel < (AUDDSpeakerDriver_NUMCHANNELS+1))
        && (length == 1)) {

        muted = AUDDSpeakerChannel_IsMuted(&(auddSpeakerDriver.channels[channel]));
        USBD_Write(0, &muted, sizeof(muted), 0, 0);
    }
    else {

        USBD_Stall(0);
    }
}

///*
//    Function: AUDDSpeakerDriver_SetInterface
//        Changes the current active alternate setting of the given interface.
//
//    Parameters:
//        
//*/
//TRACE_DEBUG_M("SetInterface(%d,%d) ", setup->wIndex, setup->wValue);
//
//            /* Check that interface is AudioStreaming OUT */
//            if (setup->wIndex == INTERFACE_ID_AUDIOSTREAMING_OUT) {
//
//                /* Check desired alternate setting */
//                switch (setup->wValue) {
//                    case 0:
//                    case 1:
//                        if (speakerDriver->isOutStreamEnabled != setup->wValue) {
//                        
//                            speakerDriver->isOutStreamEnabled = setup->wValue;
//                            SPK_OutStreamStatusChanged(speakerDriver);
//                            LED_SetGlowing(LED_OTHER, setup->wValue);
//                        }
//                        USB_SendZLP0(usbDriver, 0, 0);
//                        break;
//
//                    default:
//                        USB_Stall(usbDriver, 0);
//                }
//            }
//            else {
//
//                USB_Stall(usbDriver, 0);
//            }
//

//------------------------------------------------------------------------------
//         Callback re-implementation
//------------------------------------------------------------------------------
/*
    Function: USBDCallbacks_RequestReceived
        Triggered when the USB host emits a new SETUP request. The request is
        forward to <AUDDSpeakerDriver_RequestHandler>.

    Parameters:
        request - Pointer to a USBGenericRequest instance.
*/
#if !defined(NOAUTOCALLBACK)
void USBDCallbacks_RequestReceived(const USBGenericRequest *request)
{
    AUDDSpeakerDriver_RequestHandler(request);
}
#endif

/*
    Function: USBDDriverCallbacks_InterfaceSettingChanged
        Invoked whenever the active setting of an interface is changed by the
        host. Changes the status of the third LED accordingly.

    Parameters:
        interface - Interface number.
        setting - Newly active setting.
*/
void USBDDriverCallbacks_InterfaceSettingChanged(unsigned char interface,
                                                 unsigned char setting)
{
    if ((interface == AUDDSpeakerDriverDescriptors_STREAMING)
        && (setting == 0)) {

        LED_Clear(USBD_LEDOTHER);
    }
    else {

        LED_Set(USBD_LEDOTHER);
    }
}

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: AUDDSpeakerDriver_Initialize
        Initializes an USB audio speaker device driver, as well as the underlying
        USB controller.
*/
void AUDDSpeakerDriver_Initialize()
{
    // Initialize speaker channels
    AUDDSpeakerChannel_Initialize(&(auddSpeakerDriver.channels[0]),
                                  AUDDSpeakerDriver_MASTERCHANNEL,
                                  0);
    AUDDSpeakerChannel_Initialize(&(auddSpeakerDriver.channels[1]),
                                  AUDDSpeakerDriver_LEFTCHANNEL,
                                  0);
    AUDDSpeakerChannel_Initialize(&(auddSpeakerDriver.channels[2]),
                                  AUDDSpeakerDriver_RIGHTCHANNEL,
                                  0);
    
    // Initialize the USB driver
    USBDDriver_Initialize(&(auddSpeakerDriver.usbdDriver),
                          &auddSpeakerDriverDescriptors,
                          auddSpeakerDriverInterfaces);
    USBD_Init();

    // Initialize the third LED to indicate when the audio interface is active
    LED_Configure(USBD_LEDOTHER);
}

/*
    Function: AUDDSpeakerDriver_RequestHandler
        Handles audio-specific USB requests sent by the host, and forwards
        standard ones to the USB device driver.

    Parameters:
        request - Pointer to a USBGenericRequest instance.
*/
void AUDDSpeakerDriver_RequestHandler(const USBGenericRequest *request)
{
    unsigned char entity;
    unsigned char interface;

    trace_LOG(trace_INFO, "NewReq ");

    // Check if this is a class request
    if (USBGenericRequest_GetType(request) == USBGenericRequest_CLASS) {

        // Check if the request is supported
        switch (USBGenericRequest_GetRequest(request)) {

            case AUDGenericRequest_SETCUR:
                trace_LOG(trace_INFO,
                          "sCur(0x%04X) ",
                          USBGenericRequest_GetIndex(request));
    
                // Check the target interface and entity
                entity = AUDGenericRequest_GetEntity(request);
                interface = AUDGenericRequest_GetInterface(request);
                if ((entity == AUDDSpeakerDriverDescriptors_FEATUREUNIT)
                    && (interface == AUDDSpeakerDriverDescriptors_CONTROL)) {
    
                    AUDDSpeakerDriver_SetFeatureCurrentValue(
                        AUDFeatureUnitRequest_GetChannel(request),
                        AUDFeatureUnitRequest_GetControl(request),
                        USBGenericRequest_GetLength(request));
                }
                else {
    
                    trace_LOG(trace_WARNING,
                              "AUDDSpeakerDriver_RequestHandler: Unsupported entity/interface combination (0x%04X)\n\r",
                              USBGenericRequest_GetIndex(request));
                    USBD_Stall(0);
                }
                break;
    
            case AUDGenericRequest_GETCUR:
                trace_LOG(trace_INFO,
                          "gCur(0x%04X) ",
                          USBGenericRequest_GetIndex(request));
    
                // Check the target interface and entity
                entity = AUDGenericRequest_GetEntity(request);
                interface = AUDGenericRequest_GetInterface(request);
                if ((entity == AUDDSpeakerDriverDescriptors_FEATUREUNIT)
                    && (interface == AUDDSpeakerDriverDescriptors_CONTROL)) {
    
                    AUDDSpeakerDriver_GetFeatureCurrentValue(
                        AUDFeatureUnitRequest_GetChannel(request),
                        AUDFeatureUnitRequest_GetControl(request),
                        USBGenericRequest_GetLength(request));
                }
                else {
    
                    trace_LOG(trace_WARNING,
                              "AUDDSpeakerDriver_RequestHandler: Unsupported entity/interface combination (0x%04X)\n\r",
                              USBGenericRequest_GetIndex(request));
                    USBD_Stall(0);
                }
                break;
    
            default:
    
                trace_LOG(trace_WARNING,
                          "AUDDSpeakerDriver_RequestHandler: Unsupported request (%d)\n\r",
                          USBGenericRequest_GetRequest(request));
                USBD_Stall(0);
        }
    }
    // Check if this is a standard request
    else if (USBGenericRequest_GetType(request) == USBGenericRequest_STANDARD) {

        // Forward request to the standard handler
        USBDDriver_RequestHandler(&(auddSpeakerDriver.usbdDriver), request);
    }
    // Unsupported request type
    else {

        trace_LOG(trace_WARNING,
                  "AUDDSpeakerDriver_RequestHandler: Unsupported request type (%d)\n\r",
                  USBGenericRequest_GetType(request));
        USBD_Stall(0);
    }
}

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
unsigned char AUDDSpeakerDriver_Read(void *buffer,
                                     unsigned int length,
                                     TransferCallback callback,
                                     void *argument)
{
    return USBD_Read(AUDDSpeakerDriverDescriptors_DATAOUT,
                     buffer,
                     length,
                     callback,
                     argument);
}

