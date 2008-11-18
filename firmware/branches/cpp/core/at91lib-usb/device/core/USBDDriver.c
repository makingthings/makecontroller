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
//      Headers
//------------------------------------------------------------------------------

#include "USBDDriver.h"
#include "USBDDriverCallbacks.h"
#include "USBD.h"
#include <board.h>
#include <utility/trace.h>
#include <usb/common/core/USBGenericDescriptor.h>
#include <usb/common/core/USBDeviceDescriptor.h>
#include <usb/common/core/USBConfigurationDescriptor.h>
#include <usb/common/core/USBDeviceQualifierDescriptor.h>
#include <usb/common/core/USBEndpointDescriptor.h>
#include <usb/common/core/USBFeatureRequest.h>
#include <usb/common/core/USBSetAddressRequest.h>
#include <usb/common/core/USBGetDescriptorRequest.h>
#include <usb/common/core/USBSetConfigurationRequest.h>
#include <usb/common/core/USBInterfaceRequest.h>

#include <string.h>

//------------------------------------------------------------------------------
//      Local functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Configures the device by setting it into the Configured state and
/// initializing all endpoints.
/// \param pDriver  Pointer to a USBDDriver instance.
/// \param cfgnum  Configuration number to set.
//------------------------------------------------------------------------------
static void SetConfiguration(USBDDriver *pDriver, unsigned char cfgnum)
{
    USBEndpointDescriptor *pEndpoints[BOARD_USB_NUMENDPOINTS+1];
    const USBConfigurationDescriptor *pConfiguration;

    // Use different descriptor depending on device speed
    if (USBD_IsHighSpeed()) {

        pConfiguration = pDriver->pDescriptors->pHsConfiguration;
    }
    else {

        pConfiguration = pDriver->pDescriptors->pFsConfiguration;
    }

    // Set & save the desired configuration
    USBD_SetConfiguration(cfgnum);
    pDriver->cfgnum = cfgnum;

    // If the configuration is not 0, configure endpoints
    if (cfgnum != 0) {
    
        // Parse configuration to get endpoint descriptors
        USBConfigurationDescriptor_Parse(pConfiguration, 0, pEndpoints, 0);
    
        // Configure endpoints
        int i = 0;
        while (pEndpoints[i] != 0) {
    
            USBD_ConfigureEndpoint(pEndpoints[i]);
            i++;
        }
    }

    // Acknowledge the request
    USBD_Write(0, // Endpoint #0
               0, // No data buffer
               0, // No data buffer
               (TransferCallback) USBDDriverCallbacks_ConfigurationChanged,
               (void *) (unsigned int) cfgnum);
}

//------------------------------------------------------------------------------
/// Sends the current configuration number to the host.
/// \param pDriver  Pointer to a USBDDriver instance.
//------------------------------------------------------------------------------
static void GetConfiguration(const USBDDriver *pDriver)
{
    USBD_Write(0, &(pDriver->cfgnum), 1, 0, 0);
}

//------------------------------------------------------------------------------
/// Sends the current status of the device to the host.
/// \param pDriver  Pointer to a USBDDriver instance.
//------------------------------------------------------------------------------
static void GetDeviceStatus(const USBDDriver *pDriver)
{
    unsigned short data = 0;
    const USBConfigurationDescriptor *pConfiguration;

    // Use different configuration depending on device speed
    if (USBD_IsHighSpeed()) {

        pConfiguration = pDriver->pDescriptors->pHsConfiguration;
    }
    else {

        pConfiguration = pDriver->pDescriptors->pFsConfiguration;
    }

    // Check current configuration for power mode (if device is configured)
    if (pDriver->cfgnum != 0) {

        if (USBConfigurationDescriptor_IsSelfPowered(pConfiguration)) {

            data |= 1;
        }
    }

    // Check if remote wake-up is enabled
    if (pDriver->isRemoteWakeUpEnabled) {

        data |= 2;
    }

    // Send the device status
    USBD_Write(0, &data, 2, 0, 0);
}

//------------------------------------------------------------------------------
/// Sends the current status of an endpoints to the USB host.
/// \param eptnum  Endpoint number.
//------------------------------------------------------------------------------
static void GetEndpointStatus(unsigned char eptnum)
{
    // Check if the endpoint exists
    if (eptnum > BOARD_USB_NUMENDPOINTS) {

        USBD_Stall(0);
    }
    else {

        unsigned short data = 0;

        // Check if the endpoint if currently halted
        if (USBD_IsHalted(eptnum)) {

            data = 1;
        }
        
        // Send the endpoint status
        USBD_Write(0, &data, 2, 0, 0);
    }
}

//------------------------------------------------------------------------------
/// Sends the requested USB descriptor to the host if available, or STALLs  the
/// request.
/// \param pDriver  Pointer to a USBDDriver instance.
/// \param type  Type of the requested descriptor
/// \param index  Index of the requested descriptor.
/// \param length  Maximum number of bytes to return.
//------------------------------------------------------------------------------
static void GetDescriptor(
    const USBDDriver *pDriver,
    unsigned char type,
    unsigned char index,
    unsigned int length)
{
    const USBDeviceDescriptor *pDevice;
    const USBConfigurationDescriptor *pConfiguration;
    const USBDeviceQualifierDescriptor *pQualifier;
    const USBConfigurationDescriptor *pOtherSpeed;
    const USBGenericDescriptor **pStrings =
        (const USBGenericDescriptor **) pDriver->pDescriptors->pStrings;
    unsigned char numStrings = pDriver->pDescriptors->numStrings;
    const USBGenericDescriptor *pString;

    // Use different set of descriptors depending on device speed
    if (USBD_IsHighSpeed()) {

        trace_LOG(trace_DEBUG, "HS ");
        pDevice = pDriver->pDescriptors->pHsDevice;
        pConfiguration = pDriver->pDescriptors->pHsConfiguration;
        pQualifier = pDriver->pDescriptors->pHsQualifier;
        pOtherSpeed = pDriver->pDescriptors->pHsOtherSpeed;
    }
    else {

        trace_LOG(trace_DEBUG, "FS ");
        pDevice = pDriver->pDescriptors->pFsDevice;
        pConfiguration = pDriver->pDescriptors->pFsConfiguration;
        pQualifier = pDriver->pDescriptors->pFsQualifier;
        pOtherSpeed = pDriver->pDescriptors->pFsOtherSpeed;
    }

    // Check the descriptor type
    switch (type) {
        
        case USBGenericDescriptor_DEVICE:
            trace_LOG(trace_INFO, "Dev ");

            // Adjust length and send descriptor
            if (length > USBGenericDescriptor_GetLength((USBGenericDescriptor *) pDevice)) {

                length = USBGenericDescriptor_GetLength((USBGenericDescriptor *) pDevice);
            }
            USBD_Write(0, pDevice, length, 0, 0);
            break;

        case USBGenericDescriptor_CONFIGURATION:
            trace_LOG(trace_INFO, "Cfg ");

            // Adjust length and send descriptor
            if (length > USBConfigurationDescriptor_GetTotalLength(pConfiguration)) {

                length = USBConfigurationDescriptor_GetTotalLength(pConfiguration);
            }
            USBD_Write(0, pConfiguration, length, 0, 0);
            break;

        case USBGenericDescriptor_DEVICEQUALIFIER:
            trace_LOG(trace_INFO, "Qua ");

            // Check if descriptor exists
            if (!pQualifier) {

                USBD_Stall(0);
            }
            else {

                // Adjust length and send descriptor
                if (length > USBGenericDescriptor_GetLength((USBGenericDescriptor *) pQualifier)) {

                    length = USBGenericDescriptor_GetLength((USBGenericDescriptor *) pQualifier);
                }
                USBD_Write(0, pQualifier, length, 0, 0);
            }
            break;

        case USBGenericDescriptor_OTHERSPEEDCONFIGURATION:
            trace_LOG(trace_INFO, "OSC ");

            // Check if descriptor exists
            if (!pOtherSpeed) {

                USBD_Stall(0);
            }
            else {

                // Adjust length and send descriptor
                if (length > USBConfigurationDescriptor_GetTotalLength(pOtherSpeed)) {

                    length = USBConfigurationDescriptor_GetTotalLength(pOtherSpeed);
                }
                USBD_Write(0, pOtherSpeed, length, 0, 0);
            }
            break;

        case USBGenericDescriptor_STRING:
            trace_LOG(trace_INFO, "Str%d ", index);

            // Check if descriptor exists
            if (index > numStrings) {

                USBD_Stall(0);
            }
            else {

                pString = pStrings[index];

                // Adjust length and send descriptor
                if (length > USBGenericDescriptor_GetLength(pString)) {

                    length = USBGenericDescriptor_GetLength(pString);
                }
                USBD_Write(0, pString, length, 0, 0);
            }
            break;

        default:
            trace_LOG(trace_WARNING,
                      "USBDDriver_GetDescriptor: Unknown descriptor type (%d)\n\r",
                      type);
            USBD_Stall(0);
    }
}

//------------------------------------------------------------------------------
/// Sets the active setting of the given interface if the configuration supports
/// it; otherwise, the control pipe is STALLed. If the setting of an interface
/// changes.
/// \parma pDriver  Pointer to a USBDDriver instance.
/// \parma infnum  Interface number.
/// \parma setting  New active setting for the interface.
//------------------------------------------------------------------------------
static void SetInterface(
    USBDDriver *pDriver,
    unsigned char infnum,
    unsigned char setting)
{
    // Make sure alternate settings are supported
    if (!pDriver->pInterfaces) {

        USBD_Stall(0);
    }
    else {

        // Change the current setting of the interface and trigger the callback 
        // if necessary
        if (pDriver->pInterfaces[infnum] != setting) {

            pDriver->pInterfaces[infnum] = setting;
            USBDDriverCallbacks_InterfaceSettingChanged(infnum, setting);
        }

        // Acknowledge the request
        USBD_Write(0, 0, 0, 0, 0);
    }
}

//------------------------------------------------------------------------------
/// Sends the currently active setting of the given interface to the USB
/// host. If alternate settings are not supported, this function STALLs the
/// control pipe.
/// \param pDriver  Pointer to a USBDDriver instance.
/// \param infnum  Interface number.
//------------------------------------------------------------------------------
static void GetInterface(
    const USBDDriver *pDriver,
    unsigned char infnum)
{
    // Make sure alternate settings are supported, or STALL the control pipe
    if (!pDriver->pInterfaces) {

        USBD_Stall(0);
    }
    else {

        // Sends the current interface setting to the host
        USBD_Write(0, &(pDriver->pInterfaces[infnum]), 1, 0, 0);
    }
}

#ifdef BOARD_USB_UDPHS
//------------------------------------------------------------------------------
// Performs the selected test on the USB device (high-speed only).
// \param test  Test selector value.
//------------------------------------------------------------------------------
static void USBDDriver_Test(unsigned char test)
{
    trace_LOG(trace_DEBUG, "UDPHS_Test\n\r");

    // the lower byte of wIndex must be zero
    // the most significant byte of wIndex is used to specify the specific test mode
    switch (test) {
        case USBFeatureRequest_TESTPACKET:
            //Test mode Test_Packet: 
            //Upon command, a port must repetitively transmit the following test packet until
            //the exit action is taken. This enables the testing of rise and fall times, eye 
            //patterns, jitter, and any other dynamic waveform specifications.
            //The test packet is made up by concatenating the following strings. 
            //(Note: For J/K NRZI data, and for NRZ data, the bit on the left is the first one 
            //transmitted. “S” indicates that a bit stuff occurs, which inserts an “extra” NRZI data bit. 
            //“* N” is used to indicate N occurrences of a string of bits or symbols.)
            //A port in Test_Packet mode must send this packet repetitively. The inter-packet timing 
            //must be no less than the minimum allowable inter-packet gap as defined in Section 7.1.18 and 
            //no greater than 125 us.
            // Send ZLP
            USBD_Test(USBFeatureRequest_TESTSENDZLP);
            // Tst PACKET
            USBD_Test(USBFeatureRequest_TESTPACKET);
            while (1);
            break;

        case USBFeatureRequest_TESTJ:
            //Test mode Test_J:
            //Upon command, a port’s transceiver must enter the high-speed J state and remain in that
            //state until the exit action is taken. This enables the testing of the high output drive
            //level on the D+ line.
            // Send ZLP
            USBD_Test(USBFeatureRequest_TESTSENDZLP);
            // Tst J
            USBD_Test(USBFeatureRequest_TESTJ);
            while (1);
            break;

        case USBFeatureRequest_TESTK:
            //Test mode Test_K:
            //Upon command, a port’s transceiver must enter the high-speed K state and remain in
            //that state until the exit action is taken. This enables the testing of the high output drive
            //level on the D- line.
            // Send a ZLP
            USBD_Test(USBFeatureRequest_TESTSENDZLP);
            USBD_Test(USBFeatureRequest_TESTK);
            while (1);
            break;

        case USBFeatureRequest_TESTSE0NAK:
            //Test mode Test_SE0_NAK:
            //Upon command, a port’s transceiver must enter the high-speed receive mode
            //and remain in that mode until the exit action is taken. This enables the testing
            //of output impedance, low level output voltage, and loading characteristics.
            //In addition, while in this mode, upstream facing ports (and only upstream facing ports)
            //must respond to any IN token packet with a NAK handshake (only if the packet CRC is
            //determined to be correct) within the normal allowed device response time. This enables testing of
            //the device squelch level circuitry and, additionally, provides a general purpose stimulus/response
            //test for basic functional testing.
            USBD_Test(USBFeatureRequest_TESTSE0NAK);
            // Send a ZLP
            USBD_Test(USBFeatureRequest_TESTSENDZLP);
            while (1);
            break;

        default:
            USBD_Stall( 0 );
            break;

    }
    // The exit action is to power cycle the device.
    // The device must be disconnected from the host
}
#endif

//------------------------------------------------------------------------------
//      Exported functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Initializes a USBDDriver instance with a list of descriptors. If
/// interfaces can have multiple alternate settings, an array to store the
/// current setting for each interface must be provided.
/// \param pDriver  Pointer to a USBDDriver instance.
/// \param pDescriptors  Pointer to a USBDDriverDescriptors instance.
/// \param pInterfaces  Pointer to an array for storing the current alternate
///                     setting of each interface (optional).
//------------------------------------------------------------------------------
void USBDDriver_Initialize(
    USBDDriver *pDriver,
    const USBDDriverDescriptors *pDescriptors,
    unsigned char *pInterfaces)
{

    pDriver->cfgnum = 0;
#if (BOARD_USB_BMATTRIBUTES == USBConfigurationDescriptor_SELFPOWERED_RWAKEUP) \
    || (BOARD_USB_BMATTRIBUTES == USBConfigurationDescriptor_BUSPOWERED_RWAKEUP)
    pDriver->isRemoteWakeUpEnabled = 1;
#else
    pDriver->isRemoteWakeUpEnabled = 0;
#endif

    pDriver->pDescriptors = pDescriptors;
    pDriver->pInterfaces = pInterfaces;

    // Initialize interfaces array if not null
    if (pInterfaces != 0) {
    
        memset(pInterfaces, sizeof(pInterfaces), 0);
    }
}

//------------------------------------------------------------------------------
/// Handles the given request if it is standard, otherwise STALLs it.
/// \param pDriver  Pointer to a USBDDriver instance.
/// \param pRequest  Pointer to a USBGenericRequest instance.
//------------------------------------------------------------------------------
void USBDDriver_RequestHandler(
    USBDDriver *pDriver,
    const USBGenericRequest *pRequest)
{
    unsigned char cfgnum;
    unsigned char infnum;
    unsigned char eptnum;
    unsigned char setting;
    unsigned char type;
    unsigned char index;
    unsigned int length;
    unsigned int address;

    trace_LOG(trace_INFO, "Std ");

    // Check request code
    switch (USBGenericRequest_GetRequest(pRequest)) {

        case USBGenericRequest_GETDESCRIPTOR:
            trace_LOG(trace_INFO, "gDesc ");

            // Send the requested descriptor
            type = USBGetDescriptorRequest_GetDescriptorType(pRequest);
            index = USBGetDescriptorRequest_GetDescriptorIndex(pRequest);
            length = USBGenericRequest_GetLength(pRequest);
            GetDescriptor(pDriver, type, index, length);
            break;

        case USBGenericRequest_SETADDRESS:
            trace_LOG(trace_INFO, "sAddr ");

            // Sends a zero-length packet and then set the device address
            address = USBSetAddressRequest_GetAddress(pRequest);

            if( USBD_IsHighSpeed() ) {

                USBD_SetAddress( address );
            }
            else {
                USBD_Write(0, 0, 0, (TransferCallback) USBD_SetAddress, (void *) address);
            }
            break;

        case USBGenericRequest_SETCONFIGURATION:
            trace_LOG(trace_INFO, "sCfg ");

            // Set the requested configuration
            cfgnum = USBSetConfigurationRequest_GetConfiguration(pRequest);
            SetConfiguration(pDriver, cfgnum);
            break;

        case USBGenericRequest_GETCONFIGURATION:
            trace_LOG(trace_INFO, "gCfg ");

            // Send the current configuration number
            GetConfiguration(pDriver);
            break;

        case USBGenericRequest_GETSTATUS:
            trace_LOG(trace_INFO, "gSta ");
    
            // Check who is the recipient
            switch (USBGenericRequest_GetRecipient(pRequest)) {
    
                case USBGenericRequest_DEVICE:
                    trace_LOG(trace_INFO, "Dev ");
    
                    // Send the device status
                    GetDeviceStatus(pDriver);
                    break;
    
                case USBGenericRequest_ENDPOINT:
                    trace_LOG(trace_INFO, "Ept ");
    
                    // Send the endpoint status
                    eptnum = USBGenericRequest_GetEndpointNumber(pRequest);
                    GetEndpointStatus(eptnum);
                    break;
    
                default:
                    trace_LOG(trace_WARNING,
                              "W: USBDDriver_RequestHandler: Unknown recipient (%d)\n\r",
                              USBGenericRequest_GetRecipient(pRequest));
                    USBD_Stall(0);
            }
            break;

        case USBGenericRequest_CLEARFEATURE:
            trace_LOG(trace_INFO, "cFeat ");

            // Check which is the requested feature
            switch (USBFeatureRequest_GetFeatureSelector(pRequest)) {

                case USBFeatureRequest_ENDPOINTHALT:
                    trace_LOG(trace_INFO, "Hlt ");

                    // Unhalt endpoint and send a zero-length packet
                    USBD_Unhalt(USBGenericRequest_GetEndpointNumber(pRequest));
                    USBD_Write(0, 0, 0, 0, 0);
                    break;

                case USBFeatureRequest_DEVICEREMOTEWAKEUP:
                    trace_LOG(trace_INFO, "RmWU ");

                    // Disable remote wake-up and send a zero-length packet
                    pDriver->isRemoteWakeUpEnabled = 0;
                    USBD_Write(0, 0, 0, 0, 0);
                    break;

                default:
                    trace_LOG(trace_WARNING,
                              "USBDDriver_RequestHandler: Unknown feature selector (%d)\n\r",
                              USBFeatureRequest_GetFeatureSelector(pRequest));
                    USBD_Stall(0);
            }
            break;

    case USBGenericRequest_SETFEATURE:
        trace_LOG(trace_INFO, "sFeat ");

        // Check which is the selected feature
        switch (USBFeatureRequest_GetFeatureSelector(pRequest)) {

            case USBFeatureRequest_DEVICEREMOTEWAKEUP:
                trace_LOG(trace_INFO, "RmWU ");

                // Enable remote wake-up and send a ZLP
                pDriver->isRemoteWakeUpEnabled = 1;
                USBD_Write(0, 0, 0, 0, 0);
                break;

            case USBFeatureRequest_ENDPOINTHALT:
                trace_LOG(trace_INFO, "Ept ");

                // Halt endpoint
                USBD_Halt(USBGenericRequest_GetEndpointNumber(pRequest));
                USBD_Write(0, 0, 0, 0, 0);
                break;

#if defined(BOARD_USB_UDPHS)

            case USBFeatureRequest_TESTMODE:
                // 7.1.20 Test Mode Support
                if ((USBGenericRequest_GetType(pRequest) == USBGenericRequest_DEVICE)
                    && ((USBGenericRequest_GetIndex(pRequest) & 0x000F) == 0)) {

                    // Handle test request
                    USBDDriver_Test(USBFeatureRequest_GetTestSelector(pRequest));
                }
                else {

                    USBD_Stall(0);
                }
                break;
#endif

            default:
                trace_LOG(trace_WARNING,
                          "USBDDriver_RequestHandler: Unknown feature selector (%d)\n\r",
                          USBFeatureRequest_GetFeatureSelector(pRequest));
                USBD_Stall(0);
        }
        break;

    case USBGenericRequest_SETINTERFACE:
        trace_LOG(trace_INFO, "sInterface ");

        infnum = USBInterfaceRequest_GetInterface(pRequest);
        setting = USBInterfaceRequest_GetAlternateSetting(pRequest);
        SetInterface(pDriver, infnum, setting);
        break;

    case USBGenericRequest_GETINTERFACE:
        trace_LOG(trace_INFO, "gInterface ");

        infnum = USBInterfaceRequest_GetInterface(pRequest);
        GetInterface(pDriver, infnum);
        break;

    default:
        trace_LOG(trace_WARNING,
                  "USBDDriver_RequestHandler: Unknown request code (%d)\n\r",
                  USBGenericRequest_GetRequest(pRequest));
        USBD_Stall(0);
    }
}


//------------------------------------------------------------------------------
/// Returns 1 if remote wake up has been enabled by the host; otherwise, returns
/// 0.
/// \param pDriver  Pointer to an USBDDriver instance.
//------------------------------------------------------------------------------
unsigned char USBDDriver_IsRemoteWakeUpEnabled(const USBDDriver *pDriver)
{
    return pDriver->isRemoteWakeUpEnabled;
}


