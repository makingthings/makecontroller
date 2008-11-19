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
    Title: USBD implementation for a UDP controller

    About: Purpose
        Implementation of USB device functions on a UDP controller.
*/

//------------------------------------------------------------------------------
//      Headers
//------------------------------------------------------------------------------


#include "USBD.h"
#include "USBDCallbacks.h"
#include <board.h>
// #include <pio/pio.h>
// #include <utility/trace.h>
// #include <utility/led.h>
// #include <usb/common/core/USBEndpointDescriptor.h>
#include "USBEndpointDescriptor.h"
// #include <usb/common/core/USBGenericRequest.h>
#include "USBGenericRequest.h"

#if defined(BOARD_USB_UDP)

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: UDP register field values
        UDP_RXDATA - Bit mask for both banks of the UDP_CSR register.
*/
#define UDP_RXDATA              (AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1)

/*
    Constants: Endpoint states
        UDP_ENDPOINT_DISABLED - Endpoint is disabled.
        UDP_ENDPOINT_HALTED - Endpoint is halted (i.e. STALLs every request).
        UDP_ENDPOINT_IDLE - Endpoint is idle (i.e. ready for transmission).
        UDP_ENDPOINT_SENDING - Endpoint is sending data.
        UDP_ENDPOINT_RECEIVING - Endpoint is receiving data.
*/
#define UDP_ENDPOINT_DISABLED       0
#define UDP_ENDPOINT_HALTED         1
#define UDP_ENDPOINT_IDLE           2
#define UDP_ENDPOINT_SENDING        3
#define UDP_ENDPOINT_RECEIVING      4

/*
    Macros: UDP_CSR register access
        CLEAR_CSR - Clears the specified bit(s) in the UDP_CSR register.
        SET_CSR - Sets the specified bit(s) in the UDP_CSR register.
*/

#define CLEAR_CSR(endpoint, flags) \
    { \
        volatile unsigned int reg; \
        reg = AT91C_BASE_UDP->UDP_CSR[endpoint]; \
        reg &= ~(flags); \
        reg |= (AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1); \
        AT91C_BASE_UDP->UDP_CSR[endpoint] = reg; \
        while ( (AT91C_BASE_UDP->UDP_CSR[endpoint] & (flags)) == (flags) ); \
    }

#define SET_CSR(endpoint, flags) \
    { \
        volatile unsigned int reg; \
        reg = AT91C_BASE_UDP->UDP_CSR[endpoint] ; \
        reg |= (flags); \
        reg |= (AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1); \
        AT91C_BASE_UDP->UDP_CSR[endpoint] = reg; \
        while ( (AT91C_BASE_UDP->UDP_CSR[endpoint] & (flags)) != (flags) ); \
    }

//------------------------------------------------------------------------------
//      Types
//------------------------------------------------------------------------------

/*
    Type: UDP transfer
        Describes an ongoing transfer on a UDP endpoint.

    Variables:
        data - Pointer to a data buffer used for emission/reception.
        buffered - Number of bytes which have been written into the UDP internal
                   FIFO buffers.
        transferred - Number of bytes which have been sent/received.
        remaining - Number of bytes which have not been buffered/transferred yet.
        callback - Optional callback to invoke when the transfer completes.
        argument - Optional argument to the callback function.
*/
typedef struct {

    char *data;
    int buffered;
    int transferred;
    int remaining;
    TransferCallback callback;
    void *argument;

} Transfer;

/*
    Type: UDP endpoint
        Describes the state of an endpoint of the UDP controller.

    Variables:
        state - Current endpoint state.
        bank - Current reception bank (0 or 1).
        size - Maximum packet size for the endpoint.
        transfer - Describes an ongoing transfer (if current state is either
                   <UDP_ENDPOINT_SENDING> or <UDP_ENDPOINT_RECEIVING>).
*/
typedef struct {

    unsigned char state;
    unsigned char bank;
    unsigned short size;
    Transfer transfer;

} Endpoint;

//------------------------------------------------------------------------------
//         Internal variables
//------------------------------------------------------------------------------

/*
    Variables: 
        endpoints - Holds the internal state for each endpoint of the UDP.
        deviceState - Device current state.
        suspended - Indicates if device is currently suspended.
*/
static Endpoint endpoints[BOARD_USB_NUMENDPOINTS];
static unsigned char deviceState;
static unsigned char previousDeviceState;

//------------------------------------------------------------------------------
//      Internal Functions
//------------------------------------------------------------------------------
/*
    Functions: Peripheral clock
        EnablePeripheralClock - Enables the clock of the UDP peripheral.
        DisablePeripheralClock - Disables the UDP peripheral clock.
*/
static inline void UDP_EnablePeripheralClock(void)
{
    AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_UDP;
}

static inline void UDP_DisablePeripheralClock(void)
{
    AT91C_BASE_PMC->PMC_PCDR = 1 << AT91C_ID_UDP;
}

/*
    Functions: USB clock
        EnableUsbClock - Enables the 48MHz USB clock.
        DisableUsbClock - Disables the 48MHz USB clock.
*/
static inline void UDP_EnableUsbClock(void)
{
    AT91C_BASE_PMC->PMC_SCER = AT91C_PMC_UDP;
}

static inline void UDP_DisableUsbClock(void)
{
    AT91C_BASE_PMC->PMC_SCDR = AT91C_PMC_UDP;
}

/*
    Functions: Transceiver
        UDP_EnableTransceiver - Enables the UDP transceiver.
        UDP_DisableTransceiver - Disables the UDP transceiver.
*/
static inline void UDP_EnableTransceiver(void)
{
    AT91C_BASE_UDP->UDP_TXVC &= ~AT91C_UDP_TXVDIS;
}

static inline void UDP_DisableTransceiver(void)
{
    AT91C_BASE_UDP->UDP_TXVC |= AT91C_UDP_TXVDIS;
}

/*
    Function: UDP_EndOfTransfer
        Handles a completed transfer on the given endpoint, invoking the
        configured callback if any.

    Parameters:
        eptnum - Number of the endpoint for which the transfer has completed.
        status - Result of the USB transfer.
*/
static void UDP_EndOfTransfer(unsigned char eptnum, char status)
{
    Endpoint *endpoint = &(endpoints[eptnum]);
    Transfer *transfer = &(endpoint->transfer);

    // Check that endpoint was sending or receiving data
    if ((endpoint->state == UDP_ENDPOINT_RECEIVING)
        || (endpoint->state == UDP_ENDPOINT_SENDING)) {

        // trace_LOG(trace_INFO, "EoT ");

        // Endpoint returns in Idle state
        endpoint->state = UDP_ENDPOINT_IDLE;

        // Invoke callback is present
        if (transfer->callback != 0) {

            ((TransferCallback) transfer->callback)
                (transfer->argument,
                 status,
                 transfer->transferred,
                 transfer->remaining + transfer->buffered);
        }
    }
}

/*
    Function: UDP_ClearRxFlag
        Clears the correct reception flag (bank 0 or bank 1) of an endpoint.

    Parameters:
        eptnum - Number of an endpoint.
*/
static void UDP_ClearRxFlag(unsigned char eptnum)
{
    Endpoint *endpoint = &(endpoints[eptnum]);

    // Clear flag and change banks
    if (endpoint->bank == 0) {

        //CLEAR_CSR(eptnum, AT91C_UDP_RX_DATA_BK0);
        AT91C_BASE_UDP->UDP_CSR[eptnum] &= ~AT91C_UDP_RX_DATA_BK0;
        while ( (AT91C_BASE_UDP->UDP_CSR[eptnum] & AT91C_UDP_RX_DATA_BK0) == AT91C_UDP_RX_DATA_BK0 );

        // Swap bank if in dual-fifo mode
        if (BOARD_USB_ENDPOINTS_BANKS(eptnum) > 1) {

            endpoint->bank = 1;
        }
    }
    else {

        //CLEAR_CSR(eptnum, AT91C_UDP_RX_DATA_BK1);
        AT91C_BASE_UDP->UDP_CSR[eptnum] &= ~AT91C_UDP_RX_DATA_BK1;
        while ( (AT91C_BASE_UDP->UDP_CSR[eptnum] & AT91C_UDP_RX_DATA_BK1) == AT91C_UDP_RX_DATA_BK1 );
        endpoint->bank = 0;
    }
}

/*
    Function: UDP_WritePayload
        Writes a data payload into the current FIFO buffer of the UDP.

    Parameters:
        eptnum - Number of the endpoint which is sending data.
*/
static void UDP_WritePayload(unsigned char eptnum)
{
    Endpoint *endpoint = &(endpoints[eptnum]);
    Transfer *transfer = &(endpoint->transfer);
    signed int size;

    // Get the number of bytes to send
    size = endpoint->size;
    if (size > transfer->remaining) {

        size = transfer->remaining;
    }

    // Update transfer descriptor information
    transfer->buffered += size;
    transfer->remaining -= size;

    // Write packet in the FIFO buffer
    while (size > 0) {

        AT91C_BASE_UDP->UDP_FDR[eptnum] = *(transfer->data);
        transfer->data++;
        size--;
    }
}

/*
    Function: UDP_ReadPayload
        Reads a data payload from the current FIFO buffer of an endpoint.

    Parameters:
        eptnum - Endpoint number.
        size - Size of the data to read.
*/
static void UDP_ReadPayload(unsigned char eptnum, int size)
{
    Endpoint *endpoint = &(endpoints[eptnum]);
    Transfer *transfer = &(endpoint->transfer);

    // Check that the requested size is not bigger than the remaining transfer
    if (size > transfer->remaining) {

        transfer->buffered += size - transfer->remaining;
        size = transfer->remaining;
    }

    // Update transfer descriptor information
    transfer->remaining -= size;
    transfer->transferred += size;

    // Retrieve packet
    while (size > 0) {

        *(transfer->data) = (char) AT91C_BASE_UDP->UDP_FDR[eptnum];
        transfer->data++;
        size--;
    }
}

/*
    Function: UDP_ReadRequest
        Reads a SETUP request from the FIFO buffer of Control endpoint 0 and
        stores it into the global <request> variable.
*/
static void UDP_ReadRequest(USBGenericRequest *request)
{
    unsigned char *data = (unsigned char *) request;
    unsigned int i;

    // Copy packet
    for (i = 0; i < 8; i++) {

        *data = (unsigned char) AT91C_BASE_UDP->UDP_FDR[0];
        data++;
    }
}

/*
    Function: UDP_ResetEndpoints
        Resets all the endpoints of the UDP peripheral.
*/
static void UDP_ResetEndpoints(void)
{
    Endpoint *endpoint;
    Transfer *transfer;
    unsigned char eptnum;

    // Reset the transfer descriptor of every endpoint
    for (eptnum = 0; eptnum < BOARD_USB_NUMENDPOINTS; eptnum++) {

        endpoint = &(endpoints[eptnum]);
        transfer = &(endpoint->transfer);

        // Reset endpoint transfer descriptor
        transfer->data = 0;
        transfer->transferred = -1;
        transfer->buffered = -1;
        transfer->remaining = -1;
        transfer->callback = 0;
        transfer->argument = 0;

        // Reset endpoint state
        endpoint->bank = 0;
        endpoint->state = UDP_ENDPOINT_DISABLED;
    }
}

/*
    Function: UDP_DisableEndpoints
        Disables all endpoints of the UDP peripheral except Control endpoint 0.
*/
static void UDP_DisableEndpoints(void)
{
    unsigned char eptnum;

    // Disable each endpoint, terminating any pending transfer
    for (eptnum = 1; eptnum < BOARD_USB_NUMENDPOINTS; eptnum++) {

        UDP_EndOfTransfer(eptnum, USBD_STATUS_ABORTED);
        endpoints[eptnum].state = UDP_ENDPOINT_DISABLED;
    }
}

/*
    Function: UDP_IsTransferFinished
        Checks if an ongoing transfer on an endpoint has been completed.

    Parameters:
        eptnum - Endpoint number.

    Returns:
        1 if the current transfer on the given endpoint is complete; otherwise
        0.
*/
static unsigned char UDP_IsTransferFinished(unsigned char eptnum)
{
    Endpoint *endpoint = &(endpoints[eptnum]);
    Transfer *transfer = &(endpoint->transfer);

    // Check if it is a Control endpoint
    //  -> Control endpoint must always finish their transfer with a zero-length
    //     packet
    if ((AT91C_BASE_UDP->UDP_CSR[eptnum] & AT91C_UDP_EPTYPE)
        == AT91C_UDP_EPTYPE_CTRL) {

        return (transfer->buffered < endpoint->size);
    }
    // Other endpoints only need to transfer all the data
    else {

        return (transfer->buffered <= endpoint->size)
               && (transfer->remaining == 0);
    }
}

/*
    Function: UDP_EndpointHandler
        Endpoint interrupt handler. Manages IN, OUT & SETUP transaction, as well
        as the STALL condition.

    Parameters:
        eptnum - Number of the endpoint to handle interrupt for.
*/
static void UDP_EndpointHandler(unsigned char eptnum)
{
    Endpoint *endpoint = &(endpoints[eptnum]);
    Transfer *transfer = &(endpoint->transfer);
    unsigned int status = AT91C_BASE_UDP->UDP_CSR[eptnum];

    // trace_LOG(trace_INFO, "Ept%d ", eptnum);

    // Handle interrupts
    // IN packet sent
    if ((status & AT91C_UDP_TXCOMP) != 0) {

        // trace_LOG(trace_INFO, "Wr ");

        // Check that endpoint was in Sending state
        if (endpoint->state == UDP_ENDPOINT_SENDING) {

            // End of transfer ?
            if (UDP_IsTransferFinished(eptnum)) {

                // trace_LOG(trace_INFO, "%d ", transfer->buffered);

                transfer->transferred += transfer->buffered;
                transfer->buffered = 0;

                // Disable interrupt if this is not a control endpoint
                if ((status & AT91C_UDP_EPTYPE) != AT91C_UDP_EPTYPE_CTRL) {

                    AT91C_BASE_UDP->UDP_IDR = 1 << eptnum;
                }

                UDP_EndOfTransfer(eptnum, USBD_STATUS_SUCCESS);
                CLEAR_CSR(eptnum, AT91C_UDP_TXCOMP);
            }
            else {

                // Transfer remaining data
                // trace_LOG(trace_INFO, "%d ", endpoint->size);

                transfer->transferred += endpoint->size;
                transfer->buffered -= endpoint->size;

                // Send next packet
                if (BOARD_USB_ENDPOINTS_BANKS(eptnum) == 1) {

                    // No double buffering
                    UDP_WritePayload(eptnum);
                    SET_CSR(eptnum, AT91C_UDP_TXPKTRDY);
                    CLEAR_CSR(eptnum, AT91C_UDP_TXCOMP);
                }
                else {
                    // Double buffering
                    SET_CSR(eptnum, AT91C_UDP_TXPKTRDY);
                    CLEAR_CSR(eptnum, AT91C_UDP_TXCOMP);
                    UDP_WritePayload(eptnum);
                }
            }
        }
        else {
            // Acknowledge interrupt
            CLEAR_CSR(eptnum, AT91C_UDP_TXCOMP);
        }
    }

    // OUT packet received
    if ((status & UDP_RXDATA) != 0) {

        // trace_LOG(trace_INFO, "Rd ");

        // Check that the endpoint is in Receiving state
        if (endpoint->state != UDP_ENDPOINT_RECEIVING) {

            // Check if an ACK has been received on a Control endpoint
            if (((status & AT91C_UDP_EPTYPE) == AT91C_UDP_EPTYPE_CTRL)
                && ((status & AT91C_UDP_RXBYTECNT) == 0)) {

                // Acknowledge the data and finish the current transfer
                // trace_LOG(trace_INFO, "Ack ");
                UDP_ClearRxFlag(eptnum);
                UDP_EndOfTransfer(eptnum, USBD_STATUS_SUCCESS);
            }
            // Check if the data has been STALLed
            else if ((status & AT91C_UDP_FORCESTALL) != 0) {

                // Discard STALLed data
                // trace_LOG(trace_INFO, "Disc ");
                UDP_ClearRxFlag(eptnum);
            }
            // NAK the data
            else {

                // trace_LOG(trace_INFO, "Nak ");
                AT91C_BASE_UDP->UDP_IDR = 1 << eptnum;
            }
        }
        // Endpoint is in Read state
        else {

            // Retrieve data and store it into the current transfer buffer
            unsigned short size = (unsigned short) (status >> 16);
            // trace_LOG(trace_INFO, "%d ", size);
            UDP_ReadPayload(eptnum, size);
            UDP_ClearRxFlag(eptnum);

            // Check if the transfer is finished
            if ((transfer->remaining == 0) || (size < endpoint->size)) {

                // Disable interrupt if this is not a control endpoint
                if ((status & AT91C_UDP_EPTYPE) != AT91C_UDP_EPTYPE_CTRL) {

                    AT91C_BASE_UDP->UDP_IDR = 1 << eptnum;
                }

                UDP_EndOfTransfer(eptnum, USBD_STATUS_SUCCESS);
            }
        }
    }

    // SETUP packet received
    if ((status & AT91C_UDP_RXSETUP) != 0) {

        // trace_LOG(trace_INFO, "Stp ");

        // If a transfer was pending, complete it
        // Handles the case where during the status phase of a control write
        // transfer, the host receives the device ZLP and ack it, but the ack
        // is not received by the device
        if ((endpoint->state == UDP_ENDPOINT_RECEIVING)
            || (endpoint->state == UDP_ENDPOINT_SENDING)) {

            UDP_EndOfTransfer(eptnum, USBD_STATUS_SUCCESS);
        }
        USBGenericRequest request;
        UDP_ReadRequest(&request);

        // Set the DIR bit before clearing RXSETUP in Control IN sequence
        if (USBGenericRequest_GetDirection(&request) == USBGenericRequest_IN) {

            SET_CSR(eptnum, AT91C_UDP_DIR);
        }
        CLEAR_CSR(eptnum, AT91C_UDP_RXSETUP);

        // Forward the request to the upper layer
        USBDCallbacks_RequestReceived(&request);
    }

    // STALL sent
    if ((status & AT91C_UDP_STALLSENT) != 0) {

        // trace_LOG(trace_INFO, "Sta ");

        // If the endpoint is not halted, clear the STALL condition
        CLEAR_CSR(eptnum, AT91C_UDP_STALLSENT);
        if (endpoint->state != UDP_ENDPOINT_HALTED) {

            CLEAR_CSR(eptnum, AT91C_UDP_FORCESTALL);
        }
    }
}

//------------------------------------------------------------------------------
//      Exported functions
//------------------------------------------------------------------------------
/*
    Function: USBD_ConfigureEndpoint
        Configures an endpoint according to its Endpoint Descriptor.

    Parameters:
        descriptor - Pointer to an Endpoint descriptor.
*/
void USBD_ConfigureEndpoint(const USBEndpointDescriptor *descriptor)
{
    Endpoint *endpoint;
    unsigned char eptnum;
    unsigned char type;
    unsigned char direction;

    // NULL descriptor -> Control endpoint 0
    if (descriptor == 0) {

        eptnum = 0;
        endpoint = &(endpoints[eptnum]);
        type = USBEndpointDescriptor_CONTROL;
        direction = 0;
        endpoint->size = BOARD_USB_ENDPOINTS_MAXPACKETSIZE(0);
    }
    else {

        eptnum = USBEndpointDescriptor_GetNumber(descriptor);
        endpoint = &(endpoints[eptnum]);
        type = USBEndpointDescriptor_GetType(descriptor);
        direction = USBEndpointDescriptor_GetDirection(descriptor);
        endpoint->size = USBEndpointDescriptor_GetMaxPacketSize(descriptor);
    }

    // Abort the current transfer is the endpoint was configured and in
    // Write or Read state
    if ((endpoint->state == UDP_ENDPOINT_RECEIVING)
        || (endpoint->state == UDP_ENDPOINT_SENDING)) {

        UDP_EndOfTransfer(eptnum, USBD_STATUS_RESET);
    }
    endpoint->state = UDP_ENDPOINT_IDLE;

    // Reset Endpoint Fifos
    AT91C_BASE_UDP->UDP_RSTEP |= (1 << eptnum);
    AT91C_BASE_UDP->UDP_RSTEP &= ~(1 << eptnum);

    // Configure endpoint
    SET_CSR(eptnum, (unsigned char)AT91C_UDP_EPEDS | (type << 8) | (direction << 10));
    if (type == USBEndpointDescriptor_CONTROL) {

        AT91C_BASE_UDP->UDP_IER = (1 << eptnum);
    }

    // trace_LOG(trace_INFO, "CfgEpt%d ", eptnum);
}

/*
    Function: USBD_InterruptHandler
        UDP interrupt handler. Manages device status changes.
*/
void USBD_InterruptHandler()
{
    unsigned int status;
    
    // Get interrupt status
    // Some interrupts may get masked depending on the device state
    status = AT91C_BASE_UDP->UDP_ISR;
    status &= AT91C_BASE_UDP->UDP_IMR;
    if (deviceState < USBD_STATE_POWERED) {

        status &= AT91C_UDP_WAKEUP | AT91C_UDP_RXRSM;
        AT91C_BASE_UDP->UDP_ICR = ~status;
    }

    // Return immediately if there is no interrupt to service
    if (status == 0) {

        return;
    }

    // Toggle USB LED if the device is active
    // trace_LOG(trace_INFO, "Hlr ");
    // if (deviceState >= USBD_STATE_POWERED) {
    // 
    //     LED_Set(USBD_LEDUSB);
    // }

    // Service interrupts

    //// Start Of Frame (SOF)
    //if (ISSET(dStatus, AT91C_UDP_SOFINT)) {
    //
    //    trace_LOG(trace_DEBUG, "SOF");
    //
    //    // Invoke the SOF callback
    //    USB_StartOfFrameCallback(pUsb);
    //
    //    // Acknowledge interrupt
    //    AT91C_BASE_UDP->UDP_ICR = AT91C_UDP_SOFINT;
    //    dStatus &= ~AT91C_UDP_SOFINT;
    //}

    // Suspend
    // This interrupt is always treated last (hence the '==')
    if (status == AT91C_UDP_RXSUSP) {

        // trace_LOG(trace_INFO, "Susp ");

        // Don't do anything if the device is already suspended
        if (deviceState != USBD_STATE_SUSPENDED) {

            // The device enters the Suspended state
            // Enable wakeup
            AT91C_BASE_UDP->UDP_IER = AT91C_UDP_WAKEUP | AT91C_UDP_RXRSM;

            // Acknowledge interrupt
            AT91C_BASE_UDP->UDP_ICR = AT91C_UDP_RXSUSP;

            // Switch to the Suspended state
            previousDeviceState = deviceState;
            deviceState = USBD_STATE_SUSPENDED;
            UDP_DisableTransceiver();
            UDP_DisablePeripheralClock();
            UDP_DisableUsbClock();

            // Invoke the Suspended callback
            USBDCallbacks_Suspended();
        }
    }
    // Resume
    else if ((status & (AT91C_UDP_WAKEUP | AT91C_UDP_RXRSM)) != 0) {

        // trace_LOG(trace_INFO, "Res ");

        // Invoke the Resume callback
        USBDCallbacks_Resumed();

        // Don't do anything if the device was not suspended
        if (deviceState == USBD_STATE_SUSPENDED) {

            // The device enters its previous state
            UDP_EnablePeripheralClock();
            UDP_EnableUsbClock();

            // Enable the transceiver if the device was past the Default
            // state
            deviceState = previousDeviceState;
            if (deviceState >= USBD_STATE_DEFAULT) {

                UDP_EnableTransceiver();
            }
        }
        
        // Clear and disable resume interrupts
        AT91C_BASE_UDP->UDP_ICR = AT91C_UDP_WAKEUP 
                                  | AT91C_UDP_RXRSM
                                  | AT91C_UDP_RXSUSP;
        AT91C_BASE_UDP->UDP_IDR = AT91C_UDP_WAKEUP | AT91C_UDP_RXRSM;
    }
    // End of bus reset
    else if ((status & AT91C_UDP_ENDBUSRES) != 0) {

        // trace_LOG(trace_INFO, "EoBRes ");

        // The device enters the Default state
        deviceState = USBD_STATE_DEFAULT;
        UDP_EnableTransceiver();
        UDP_ResetEndpoints();
        UDP_DisableEndpoints();
        USBD_ConfigureEndpoint(0);

        // Flush and enable the Suspend interrupt
        AT91C_BASE_UDP->UDP_ICR = AT91C_UDP_WAKEUP
                                  | AT91C_UDP_RXRSM
                                  | AT91C_UDP_RXSUSP;
        AT91C_BASE_UDP->UDP_IER = AT91C_UDP_RXSUSP;

        //// Enable the Start Of Frame (SOF) interrupt if needed
        //if (pUsb->pCallbacks->startOfFrame != 0) {
        //
        //    AT91C_BASE_UDP->UDP_IER = AT91C_UDP_SOFINT;
        //}

        // Invoke the Reset callback
        USBDCallbacks_Reset();

        // Acknowledge end of bus reset interrupt
        AT91C_BASE_UDP->UDP_ICR = AT91C_UDP_ENDBUSRES;
    }
    // Endpoint interrupts
    else {

        int eptnum = 0;
        while (status != 0) {

            // Check if endpoint has a pending interrupt
            if ((status & (1 << eptnum)) != 0) {
            
                UDP_EndpointHandler(eptnum);
                status &= ~(1 << eptnum);
                
                // if (status != 0) {
                // 
                //     trace_LOG(trace_INFO, "\n\r  - ");
                // }
            }
            eptnum++;
        }
    }

    // Toggle LED back to its previous state
    // trace_LOG(trace_INFO, "\n\r");
    // if (deviceState >= USBD_STATE_POWERED) {
    // 
    //     LED_Clear(USBD_LEDUSB);
    // }
}

/*
    Function: USBD_Write
        Sends data through a USB endpoint. Sets up the transfer descriptor,
        writes one or two data payloads (depending on the number of FIFO bank
        for the endpoint) and then starts the actual transfer. The operation is
        complete when all the data has been sent.

         *If the size of the buffer is greater than the size of the endpoint
         (or twice the size if the endpoint has two FIFO banks), then the buffer
         must be kept allocated until the transfer is finished*. This means that
         it is not possible to declare it on the stack (i.e. as a local variable
         of a function which returns after starting a transfer).

    Parameters:
        eptnum - Endpoint number.
        data - Pointer to a buffer with the data to send.
        size - Size of the data buffer.
        callback - Optional callback function to invoke when the transfer is
                   complete.
        argument - Optional argument to the callback function.

    Returns:
        USBD_STATUS_SUCCESS if the transfer has been started; otherwise, the
        corresponding error status code.
*/
char USBD_Write(unsigned char eptnum,
                const void *data,
                unsigned int size,
                TransferCallback callback,
                void *argument)
{
    Endpoint *endpoint = &(endpoints[eptnum]);
    Transfer *transfer = &(endpoint->transfer);

    // Check that the endpoint is in Idle state
    if (endpoint->state != UDP_ENDPOINT_IDLE) {

        return USBD_STATUS_LOCKED;
    }

    // trace_LOG(trace_INFO, "Write%d(%u) ", eptnum, size);

    // Setup the transfer descriptor
    transfer->data = (void *) data;
    transfer->remaining = size;
    transfer->buffered = 0;
    transfer->transferred = 0;
    transfer->callback = callback;
    transfer->argument = argument;

    // Send the first packet
    endpoint->state = UDP_ENDPOINT_SENDING;
    while((AT91C_BASE_UDP->UDP_CSR[eptnum]&AT91C_UDP_TXPKTRDY)==AT91C_UDP_TXPKTRDY);
    UDP_WritePayload(eptnum);
    SET_CSR(eptnum, AT91C_UDP_TXPKTRDY);

    // If double buffering is enabled and there is data remaining,
    // prepare another packet
    if ((BOARD_USB_ENDPOINTS_BANKS(eptnum) > 1) && (transfer->remaining > 0)) {

        UDP_WritePayload(eptnum);
    }

    // Enable interrupt on endpoint
    AT91C_BASE_UDP->UDP_IER = 1 << eptnum;

    return USBD_STATUS_SUCCESS;
}

/*
    Function: USBD_Read
        Reads incoming data on an USB endpoint This methods sets the transfer
        descriptor and activate the endpoint interrupt. The actual transfer is
        then carried out by the endpoint interrupt handler. The Read operation
        finishes either when the buffer is full, or a short packet (inferior to
        endpoint maximum  size) is received.

        *The buffer must be kept allocated until the transfer is finished*.

    Parameters:
        eptnum - Endpoint number.
        data - Pointer to a data buffer.
        size - Size of the data buffer in bytes.
        callback - Optional end-of-transfer callback function.
        argument - Optional argument to the callback function.

    Returns:
        USBD_STATUS_SUCCESS if the read operation has been started; otherwise,
        the corresponding error code.
*/
char USBD_Read(unsigned char eptnum,
               void *data,
               unsigned int size,
               TransferCallback callback,
               void *argument)
{
    Endpoint *endpoint = &(endpoints[eptnum]);
    Transfer *transfer = &(endpoint->transfer);

    // Return if the endpoint is not in IDLE state
    if (endpoint->state != UDP_ENDPOINT_IDLE) {

        return USBD_STATUS_LOCKED;
    }

    // trace_LOG(trace_INFO, "Read%u(%u) ", (unsigned int) eptnum, size);

    // Endpoint enters Receiving state
    endpoint->state = UDP_ENDPOINT_RECEIVING;

    // Set the transfer descriptor
    transfer->data = data;
    transfer->remaining = size;
    transfer->buffered = 0;
    transfer->transferred = 0;
    transfer->callback = callback;
    transfer->argument = argument;

    // Enable interrupt on endpoint
    AT91C_BASE_UDP->UDP_IER = 1 << eptnum;

    return USBD_STATUS_SUCCESS;
}

/*
    Function: USBD_Halt
        Sets the HALT feature on the given endpoint (if not already in this
        state).

    Parameters:
        eptnum - Endpoint number.
*/
void USBD_Halt(unsigned char eptnum)
{
    Endpoint *endpoint = &(endpoints[eptnum]);
    
    // Check that endpoint is enabled and not already in Halt state
    if ((endpoint->state != UDP_ENDPOINT_DISABLED)
        && (endpoint->state != UDP_ENDPOINT_HALTED)) {

        // trace_LOG(trace_INFO, "Halt%d ", eptnum);

        // Abort the current transfer if necessary
        UDP_EndOfTransfer(eptnum, USBD_STATUS_ABORTED);

        // Put endpoint into Halt state
        SET_CSR(eptnum, AT91C_UDP_FORCESTALL);
        endpoint->state = UDP_ENDPOINT_HALTED;

        // Enable the endpoint interrupt
        AT91C_BASE_UDP->UDP_IER = 1 << eptnum;
    }
}

/*
    Function: USBD_Unhalt
        Clears the Halt feature on the given endpoint.

    Parameters:
        eptnum - Endpoint number.
*/
void USBD_Unhalt(unsigned char eptnum)
{
    Endpoint *endpoint = &(endpoints[eptnum]);

    // Check if the endpoint is enabled
    if (endpoint->state != UDP_ENDPOINT_DISABLED) {

        // trace_LOG(trace_INFO, "Unhalt%d ", eptnum);

        // Return endpoint to Idle state
        endpoint->state = UDP_ENDPOINT_IDLE;

        // Clear FORCESTALL flag
        CLEAR_CSR(eptnum, AT91C_UDP_FORCESTALL);

        // Reset Endpoint Fifos, beware this is a 2 steps operation
        AT91C_BASE_UDP->UDP_RSTEP |= 1 << eptnum;
        AT91C_BASE_UDP->UDP_RSTEP &= ~(1 << eptnum);
    }
}
    
/*
    Function: USBD_IsHalted
        Returns the current Halt status of an endpoint.

    Parameters:
        eptnum - Endpoint number.

    Returns:
        1 if the endpoint is currently halted; otherwise 0.
*/
unsigned char USBD_IsHalted(unsigned char eptnum)
{
    Endpoint *endpoint = &(endpoints[eptnum]);
    if (endpoint->state == UDP_ENDPOINT_HALTED) {

        return 1;
    }
    else {

        return 0;
    }
}

/*
    Function: USBD_Stall
        Causes the given endpoint to acknowledge the next packet it receives
        with a STALL handshake.

    Parameters:
        eptnum - Endpoint number.

    Returns:
        USBD_STATUS_SUCCESS or USBD_STATUS_LOCKED.
*/
unsigned char USBD_Stall(unsigned char eptnum)
{
    Endpoint *endpoint = &(endpoints[eptnum]);

    // Check that endpoint is in Idle state
    if (endpoint->state != UDP_ENDPOINT_IDLE) {

        // trace_LOG(trace_WARNING, "USBD_Stall: Endpoint%d locked\n\r", eptnum);
        return USBD_STATUS_LOCKED;
    }

    // trace_LOG(trace_INFO, "Stall%d ", eptnum);
    SET_CSR(eptnum, AT91C_UDP_FORCESTALL);

    return USBD_STATUS_SUCCESS;
}

/*
    Function: USBD_RemoteWakeUp
        Starts a remote wake-up procedure.
*/
void USBD_RemoteWakeUp()
{
    UDP_EnablePeripheralClock();
    UDP_EnableUsbClock();
    UDP_EnableTransceiver();

    // trace_LOG(trace_INFO, "RWUp ");

    // Activates a remote wakeup (edge on ESR), then clear ESR
    AT91C_BASE_UDP->UDP_GLBSTATE |= AT91C_UDP_ESR;
    AT91C_BASE_UDP->UDP_GLBSTATE &= ~AT91C_UDP_ESR;
}

/*
    Function: USBD_SetAddress
        Sets the device address to the given value.

    Parameters:
        address - New device address.
*/
void USBD_SetAddress(unsigned char address)
{
    // trace_LOG(trace_INFO, "SetAddr(%d) ", address);

    // Set address
    AT91C_BASE_UDP->UDP_FADDR = AT91C_UDP_FEN | address;

    // If the address is 0, the device returns to the Default state
    if (address == 0) {

        AT91C_BASE_UDP->UDP_GLBSTATE = 0;
        deviceState = USBD_STATE_DEFAULT;
    }
    // If the address is non-zero, the device enters the Address state
    else {

        AT91C_BASE_UDP->UDP_GLBSTATE = AT91C_UDP_FADDEN;
        deviceState = USBD_STATE_ADDRESS;
    }
}

/*
    Function: USBD_SetConfiguration
        Sets the current device configuration.

    Parameters:
        cfgnum - Configuration number to set.
*/
void USBD_SetConfiguration(unsigned char cfgnum)
{
    // trace_LOG(trace_INFO, "SetCfg(%d) ", cfgnum);

    // If the configuration number if non-zero, the device enters the
    // Configured state
    if (cfgnum != 0) {

        deviceState = USBD_STATE_CONFIGURED;
        AT91C_BASE_UDP->UDP_GLBSTATE |= AT91C_UDP_CONFG;
    }
    // If the configuration number is zero, the device goes back to the Address
    // state
    else {

        deviceState = USBD_STATE_ADDRESS;
        AT91C_BASE_UDP->UDP_GLBSTATE = AT91C_UDP_FADDEN;

        // Abort all transfers
        UDP_DisableEndpoints();
    }
}

/*
    Function: USBD_Connect
        Connects the pull-up on the D+ line of the USB.
*/
void USBD_Connect()
{
    // trace_LOG(trace_DEBUG, "Conn ");

#if defined(BOARD_USB_PULLUP_EXTERNAL)
    const Pin pinPullUp = PIN_USB_PULLUP;
    if (pinPullUp.attribute == PIO_OUTPUT_0) {

        PIO_Set(&pinPullUp);
    }
    else {

        PIO_Clear(&pinPullUp);
    }
#elif defined(BOARD_USB_PULLUP_INTERNAL)
    AT91C_BASE_UDP->UDP_TXVC |= AT91C_UDP_PUON;
#elif defined(BOARD_USB_PULLUP_MATRIX)
    AT91C_BASE_MATRIX->MATRIX_USBPCR |= AT91C_MATRIX_USBPCR_PUON;
#elif !defined(BOARD_USB_PULLUP_ALWAYSON)
    #error Unsupported pull-up type.
#endif
  AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA10;
  AT91C_BASE_PIOA->PIO_ODR = AT91C_PIO_PA10;
  
  AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA11;
  AT91C_BASE_PIOA->PIO_OER = AT91C_PIO_PA11;
  AT91C_BASE_PIOA->PIO_CODR = AT91C_PIO_PA11;
}

/*
    Function: USBD_Disconnect
        Disconnects the pull-up from the D+ line of the USB.
*/
void USBD_Disconnect()
{
    // trace_LOG(trace_DEBUG, "Disc ");

#if defined(BOARD_USB_PULLUP_EXTERNAL)
    const Pin pinPullUp = PIN_USB_PULLUP;
    if (pinPullUp.attribute == PIO_OUTPUT_0) {

        PIO_Clear(&pinPullUp);
    }
    else {

        PIO_Set(&pinPullUp);
    }
#elif defined(BOARD_USB_PULLUP_INTERNAL)
    AT91C_BASE_UDP->UDP_TXVC &= ~AT91C_UDP_PUON;
#elif defined(BOARD_USB_PULLUP_MATRIX)
    AT91C_BASE_MATRIX->MATRIX_USBPCR &= ~AT91C_MATRIX_USBPCR_PUON;
#elif !defined(BOARD_USB_PULLUP_ALWAYSON)
    #error Unsupported pull-up type.
#endif

    // Device returns to the Powered state
    if (deviceState > USBD_STATE_POWERED) {
    
        deviceState = USBD_STATE_POWERED;
    }
}

/*
    Function: USBD_Init
        Initializes the USB driver.
*/
void USBD_Init()
{
    // trace_LOG(trace_INFO, "USBD_Init\n\r");

    // Reset endpoint structures
    UDP_ResetEndpoints();

    // Configure the pull-up on D+ and disconnect it
#if defined(BOARD_USB_PULLUP_EXTERNAL)
    const Pin pinPullUp = PIN_USB_PULLUP;
    PIO_Configure(&pinPullUp, 1);
#elif defined(BOARD_USB_PULLUP_INTERNAL)
    AT91C_BASE_UDP->UDP_TXVC &= ~AT91C_UDP_PUON;
#elif defined(BOARD_USB_PULLUP_MATRIX)
    AT91C_BASE_MATRIX->MATRIX_USBPCR &= ~AT91C_MATRIX_USBPCR_PUON;
#elif !defined(BOARD_USB_PULLUP_ALWAYSON)
    #error Missing pull-up definition.
#endif

    // Device is in the Attached state
    deviceState = USBD_STATE_SUSPENDED;
    previousDeviceState = USBD_STATE_POWERED;
    UDP_EnablePeripheralClock();
    UDP_EnableUsbClock();
    AT91C_BASE_UDP->UDP_IER = AT91C_UDP_WAKEUP;

    // Configure interrupts
    USBDCallbacks_Initialized();
}

/*
    Function: USBD_GetState
        Returns the current state of the USB device.

    Returns:
        Device current state.
*/
unsigned char USBD_GetState()
{
    return deviceState;
}

//------------------------------------------------------------------------------
/// Indicates if the device is running in high or full-speed. Always returns 0
/// since UDP does not support high-speed mode.
//------------------------------------------------------------------------------
unsigned char USBD_IsHighSpeed(void)
{
    return 0;
}


#endif //#if defined(BOARD_USB_UDP)

