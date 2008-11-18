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

#include "USBD.h"
#include "USBDCallbacks.h"
#include "USBDDriver.h"
#include <board.h>
#include <pio/pio.h>
#include <utility/trace.h>
#include <utility/led.h>
#include <usb/common/core/USBEndpointDescriptor.h>
#include <usb/common/core/USBGenericRequest.h>
#include <usb/common/core/USBFeatureRequest.h>

#include <stdio.h>

#ifdef BOARD_USB_UDPHS

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
#define NUM_IT_MAX       (AT91C_BASE_UDPHS->UDPHS_IPFEATURES & AT91C_UDPHS_EPT_NBR_MAX)
#define NUM_IT_MAX_DMA   ((AT91C_BASE_UDPHS->UDPHS_IPFEATURES & AT91C_UDPHS_DMA_CHANNEL_NBR)>>4)

#define SHIFT_DMA        24
#define SHIFT_INTERUPT    8


#define DMA

/// Max size of the FMA FIFO
#define DMA_MAX_FIFO_SIZE 65536

// Constants: Endpoint states
//   UDP_ENDPOINT_DISABLED - Endpoint is disabled.
//   UDP_ENDPOINT_HALTED - Endpoint is halted (i.e. STALLs every request).
//   UDP_ENDPOINT_IDLE - Endpoint is idle (i.e. ready for transmission).
//   UDP_ENDPOINT_SENDING - Endpoint is sending data.
//   UDP_ENDPOINT_RECEIVING - Endpoint is receiving data.
#define UDP_ENDPOINT_DISABLED       0
#define UDP_ENDPOINT_HALTED         1
#define UDP_ENDPOINT_IDLE           2
#define UDP_ENDPOINT_SENDING        3
#define UDP_ENDPOINT_RECEIVING      4

//------------------------------------------------------------------------------
//      Structures
//------------------------------------------------------------------------------

// Type: UDP transfer
//   Describes an ongoing transfer on a UDP endpoint.
//
// Variables:
//   data - Pointer to a data buffer used for emission/reception.
//   buffered - Number of bytes which have been written into the UDP internal
//              FIFO buffers.
//   transferred - Number of bytes which have been sent/received.
//   remaining - Number of bytes which have not been buffered/transferred yet.
//   callback - Optional callback to invoke when the transfer completes.
//   argument - Optional argument to the callback function.
typedef struct
{
    char             *pData;
    volatile int     buffered;
    volatile int     transferred;
    volatile int     remaining;
    TransferCallback fCallback;
    void             *pArgument;
} Transfer;

// Type: UDP endpoint
//   Describes the state of an endpoint of the UDP controller.
//
// Variables:
//   state - Current endpoint state.
//   bank - Current reception bank (0 or 1).
//   size - Maximum packet size for the endpoint.
//   transfer - Describes an ongoing transfer (if current state is either
//              <UDP_ENDPOINT_SENDING> or <UDP_ENDPOINT_RECEIVING>).
typedef struct
{
    unsigned char  state;
    unsigned char  bank;
    unsigned short size;
    Transfer       transfer;
} Endpoint;

//------------------------------------------------------------------------------
//         Internal variables
//------------------------------------------------------------------------------

// Variables: 
//   endpoints - Holds the internal state for each endpoint of the UDP.
//   deviceState - Device current state.
//   suspended - Indicates if device is currently suspended.
static Endpoint      endpoints[BOARD_USB_NUMENDPOINTS];
static unsigned char deviceState;
static unsigned char previousDeviceState;

// 7.1.20 Test Mode Support
static const char test_packet_buffer[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,                // JKJKJKJK * 9
    0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,                     // JJKKJJKK * 8
    0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,                     // JJJJKKKK * 8
    0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // JJJJJJJKKKKKKK * 8
    0x7F,0xBF,0xDF,0xEF,0xF7,0xFB,0xFD,                          // JJJJJJJK * 8
    0xFC,0x7E,0xBF,0xDF,0xEF,0xF7,0xFB,0xFD,0x7E                 // {JKKKKKKK * 10}, JK
};

//------------------------------------------------------------------------------
//      Internal Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Disables the BIAS of the USB controller
//------------------------------------------------------------------------------
static inline void UDPHS_DisableBIAS( void )
{
    // For CAP9, SAM9RL, HS
#if !defined (BOARD_USB_NO_BIAS_COMMAND)
    AT91C_BASE_PMC->PMC_UCKR &= ~AT91C_CKGR_BIASEN_ENABLED;
#endif
}

//------------------------------------------------------------------------------
// Enables the BIAS of the USB controller
//------------------------------------------------------------------------------
static inline void UDPHS_EnableBIAS( void )
{
    // For CAP9, SAM9RL, HS
#if !defined (BOARD_USB_NO_BIAS_COMMAND)
    UDPHS_DisableBIAS();
    AT91C_BASE_PMC->PMC_UCKR |= AT91C_CKGR_BIASEN_ENABLED;
#endif
}

//------------------------------------------------------------------------------
// Enable UDPHS clock
// pUsb Pointer to a S_usb instance
//------------------------------------------------------------------------------
static inline void UDPHS_EnableUsbClock( void )
{
#if !defined (PMC_BY_HARD)
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_UDPHS);
    // Enable 480MHZ
    AT91C_BASE_CKGR->CKGR_UCKR |= (AT91C_CKGR_PLLCOUNT & (3 << 20)) | AT91C_CKGR_UPLLEN;
    // Wait until UTMI PLL is locked
    while ((AT91C_BASE_PMC->PMC_SR & AT91C_PMC_LOCKU) == 0);
#endif
}

//------------------------------------------------------------------------------
// Disable UDPHS clock
// pUsb Pointer to a S_usb instance
//------------------------------------------------------------------------------
static inline void UDPHS_DisableUsbClock( void )
{
#if !defined (PMC_BY_HARD)
    AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_UDPHS);
    // 480MHZ
    AT91C_BASE_CKGR->CKGR_UCKR &= ~AT91C_CKGR_UPLLEN;
#endif
}

//------------------------------------------------------------------------------
// Invokes the callback associated with a finished transfer on an
//         endpoint
// pEndpoint Pointer to a S_usb_endpoint instance
// bStatus   Status code returned by the transfer operation
//------------------------------------------------------------------------------
static void UDPHS_EndOfTransfer( unsigned char bEndpoint, char bStatus )
{
    Endpoint *pEndpoint = &(endpoints[bEndpoint]);
    Transfer *pTransfer = &(pEndpoint->transfer);

    // Check that endpoint was sending or receiving data
    if( (pEndpoint->state == UDP_ENDPOINT_RECEIVING)
     || (pEndpoint->state == UDP_ENDPOINT_SENDING) ) {

        trace_LOG(trace_DEBUG, "Eo");

        // Endpoint returns in Idle state
        pEndpoint->state = UDP_ENDPOINT_IDLE;

        // Invoke callback is present
        if (pTransfer->fCallback != 0) {

            ((TransferCallback) pTransfer->fCallback)
                (pTransfer->pArgument,
                 bStatus,
                 pTransfer->transferred,
                 pTransfer->remaining + pTransfer->buffered);
        }
    }
}

//------------------------------------------------------------------------------
// Clears the correct RX flag in an endpoint status register
// bEndpoint Index of endpoint
//------------------------------------------------------------------------------
static void UDPHS_ClearRxFlag( unsigned char bEndpoint )
{
    AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCLRSTA = AT91C_UDPHS_RX_BK_RDY;
}

//------------------------------------------------------------------------------
// Transfers a data payload from the current tranfer buffer to the endpoint FIFO
// bEndpoint Index of endpoint
//------------------------------------------------------------------------------
static void UDPHS_WritePayload( unsigned char bEndpoint )
{
    Endpoint *pEndpoint = &(endpoints[bEndpoint]);
    Transfer *pTransfer = &(pEndpoint->transfer);
    char     *pFifo;
    signed int   size;
    unsigned int dCtr;

    pFifo = (char*)&(AT91C_BASE_UDPHS_EPTFIFO->UDPHS_READEPT0[bEndpoint*16384]);

    // Get the number of bytes to send
    size = pEndpoint->size;
    if (size > pTransfer->remaining) {

        size = pTransfer->remaining;
    }

    // Update transfer descriptor information
    pTransfer->buffered += size;
    pTransfer->remaining -= size;

    // Write packet in the FIFO buffer
    dCtr = 0;
    while (size > 0) {

        pFifo[dCtr] = *(pTransfer->pData);
        pTransfer->pData++;
        size--;
        dCtr++;
    }
}

//------------------------------------------------------------------------------
// Transfers a data payload from an endpoint FIFO to the current transfer buffer
// bEndpoint   Index of endpoint
// wPacketSize Size of received data packet
//------------------------------------------------------------------------------
static void UDPHS_ReadPayload( unsigned char bEndpoint, int wPacketSize )
{
    Endpoint *pEndpoint = &(endpoints[bEndpoint]);
    Transfer *pTransfer = &(pEndpoint->transfer);
    char     *pFifo;
    unsigned char dBytes=0;

    pFifo = (char*)&(AT91C_BASE_UDPHS_EPTFIFO->UDPHS_READEPT0[bEndpoint*16384]);

    // Check that the requested size is not bigger than the remaining transfer
    if (wPacketSize > pTransfer->remaining) {

        pTransfer->buffered += wPacketSize - pTransfer->remaining;
        wPacketSize = pTransfer->remaining;
    }

    // Update transfer descriptor information
    pTransfer->remaining -= wPacketSize;
    pTransfer->transferred += wPacketSize;

    // Retrieve packet
    while (wPacketSize > 0) {

        *(pTransfer->pData) = pFifo[dBytes];
        pTransfer->pData++;
        wPacketSize--;
        dBytes++;
    }
}


//------------------------------------------------------------------------------
// Transfers a received SETUP packet from endpoint 0 FIFO to the S_usb_request
// structure of an USB driver
//------------------------------------------------------------------------------
static void UDPHS_ReadRequest( USBGenericRequest *pRequest )
{
    unsigned int *pData = (unsigned int *)pRequest;
    unsigned int fifo;

    fifo = (AT91C_BASE_UDPHS_EPTFIFO->UDPHS_READEPT0[0]);
    *pData = fifo;
    fifo = (AT91C_BASE_UDPHS_EPTFIFO->UDPHS_READEPT0[0]);
    pData++;
    *pData = fifo;
    //trace_LOG(trace_ERROR, "SETUP: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n\r", pData[0],pData[1],pData[2],pData[3],pData[4],pData[5],pData[6],pData[7]);
}

//------------------------------------------------------------------------------
// This function reset all endpoint transfer descriptors
//------------------------------------------------------------------------------
static void UDPHS_ResetEndpoints( void )
{
    Endpoint *pEndpoint;
    Transfer *pTransfer;
    unsigned char bEndpoint;

    // Reset the transfer descriptor of every endpoint
    for( bEndpoint = 0; bEndpoint < BOARD_USB_NUMENDPOINTS; bEndpoint++ ) {

        pEndpoint = &(endpoints[bEndpoint]);
        pTransfer = &(pEndpoint->transfer);

        // Reset endpoint transfer descriptor
        pTransfer->pData = 0;
        pTransfer->transferred = -1;
        pTransfer->buffered = -1;
        pTransfer->remaining = -1;
        pTransfer->fCallback = 0;
        pTransfer->pArgument = 0;

        // Reset endpoint state
        pEndpoint->bank = 0;
        pEndpoint->state = UDP_ENDPOINT_DISABLED;
    }
}


//------------------------------------------------------------------------------
// Disable all endpoints (except control endpoint 0), aborting current transfers
// if necessary.
//------------------------------------------------------------------------------
static void UDPHS_DisableEndpoints( void )
{
    unsigned char bEndpoint;

    // Disable each endpoint, terminating any pending transfer
    // Control endpoint 0 is not disabled
    for( bEndpoint = 1; bEndpoint < BOARD_USB_NUMENDPOINTS; bEndpoint++ ) {

        UDPHS_EndOfTransfer( bEndpoint, USBD_STATUS_ABORTED );
        endpoints[bEndpoint].state = UDP_ENDPOINT_DISABLED;
    }
}

//------------------------------------------------------------------------------
// Endpoint interrupt handler.
//         Handle IN/OUT transfers, received SETUP packets and STALLing
// bEndpoint Index of endpoint
//------------------------------------------------------------------------------
static void UDPHS_EndpointHandler( unsigned char bEndpoint )
{
    Endpoint *pEndpoint = &(endpoints[bEndpoint]);
    Transfer *pTransfer = &(pEndpoint->transfer);
    unsigned int   status = AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTSTA;
    unsigned short wPacketSize;
    USBGenericRequest request;
    unsigned char sendZLP = 0;

    trace_LOG(trace_DEBUG, "E%d ", bEndpoint);

    // Handle interrupts
    // IN packet sent
    if( (AT91C_UDPHS_TX_PK_RDY == (AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCTL & AT91C_UDPHS_TX_PK_RDY))
     && (0 == (status & AT91C_UDPHS_TX_PK_RDY )) ) {

        trace_LOG(trace_DEBUG, "Wr ");

        // Check that endpoint was in Sending state
        if( pEndpoint->state == UDP_ENDPOINT_SENDING ) {

            if (pTransfer->buffered > 0) {
                pTransfer->transferred += pTransfer->buffered;
                pTransfer->buffered = 0;
            }

            if(  ((pTransfer->buffered)==0)
               &&((pTransfer->transferred)==0)
               &&((pTransfer->remaining)==0)) {
                sendZLP = 1;
            }

            // End of transfer ?
            if( (pTransfer->remaining > 0) ) {
                trace_LOG(trace_DEBUG, "\n\r1pTransfer->buffered %d \n\r", pTransfer->buffered);
                trace_LOG(trace_DEBUG, "1pTransfer->transferred %d \n\r", pTransfer->transferred);
                trace_LOG(trace_DEBUG, "1pTransfer->remaining %d \n\r", pTransfer->remaining);

               // Transfer remaining data
                trace_LOG(trace_DEBUG, " %d ", pEndpoint->size);

                // Send next packet
                UDPHS_WritePayload(bEndpoint);
                AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTSETSTA = AT91C_UDPHS_TX_PK_RDY;
            }
            else {
                if( sendZLP == 1 ) {
                    AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTSETSTA = AT91C_UDPHS_TX_PK_RDY;
                }
                trace_LOG(trace_DEBUG, "\n\r0pTransfer->buffered %d \n\r", pTransfer->buffered);
                trace_LOG(trace_DEBUG, "0pTransfer->transferred %d \n\r", pTransfer->transferred);
                trace_LOG(trace_DEBUG, "0pTransfer->remaining %d \n\r", pTransfer->remaining);

                trace_LOG(trace_DEBUG, " %d ", pTransfer->transferred);

                // Disable interrupt if this is not a control endpoint
                if( AT91C_UDPHS_EPT_TYPE_CTL_EPT != (AT91C_UDPHS_EPT_TYPE&(AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCFG)) ) {

                    AT91C_BASE_UDPHS->UDPHS_IEN &= ~(1<<SHIFT_INTERUPT<<bEndpoint);
                }
                AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCTLDIS = AT91C_UDPHS_TX_PK_RDY;

                UDPHS_EndOfTransfer(bEndpoint, USBD_STATUS_SUCCESS);
            }
        }
        else {

            trace_LOG(trace_FATAL, "Error Wr");
        }
    }

    // OUT packet received
    if( AT91C_UDPHS_RX_BK_RDY == (status & AT91C_UDPHS_RX_BK_RDY) ) {

        trace_LOG(trace_DEBUG, "Rd ");

        // Check that the endpoint is in Receiving state
        if (pEndpoint->state != UDP_ENDPOINT_RECEIVING) {

            // Endpoint is NOT in Read state
            if( (0 == (AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCFG & AT91C_UDPHS_EPT_TYPE))
             && (0 == (status & AT91C_UDPHS_BYTE_COUNT)) ) {

                // Control endpoint, 0 bytes received
                // Acknowledge the data and finish the current transfer
                trace_LOG(trace_DEBUG, "Ack ");
                UDPHS_ClearRxFlag(bEndpoint);

                UDPHS_EndOfTransfer(bEndpoint, USBD_STATUS_SUCCESS);
            }
            // Check if the data has been STALLed
            else if( AT91C_UDPHS_FRCESTALL == (status & AT91C_UDPHS_FRCESTALL)) {

                // Discard STALLed data
                trace_LOG(trace_DEBUG, "Discard ");
                UDPHS_ClearRxFlag(bEndpoint);
            }
            // NAK the data
            else {

                trace_LOG(trace_DEBUG, "Nak ");
                AT91C_BASE_UDPHS->UDPHS_IEN &= ~(1<<SHIFT_INTERUPT<<bEndpoint);
            }
        }
        else {

            // Endpoint is in Read state
            // Retrieve data and store it into the current transfer buffer
            wPacketSize = (unsigned short)((status & AT91C_UDPHS_BYTE_COUNT)>>20);

            trace_LOG(trace_DEBUG, "%d ", wPacketSize);
            UDPHS_ReadPayload(bEndpoint, wPacketSize);
            UDPHS_ClearRxFlag(bEndpoint);

            // Check if the transfer is finished
            if ((pTransfer->remaining == 0) || (wPacketSize < pEndpoint->size)) {

                AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCTLDIS = AT91C_UDPHS_RX_BK_RDY;

                // Disable interrupt if this is not a control endpoint
                if( AT91C_UDPHS_EPT_TYPE_CTL_EPT != (AT91C_UDPHS_EPT_TYPE & (AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCFG)) ) {

                    AT91C_BASE_UDPHS->UDPHS_IEN &= ~(1<<SHIFT_INTERUPT<<bEndpoint);
                }
                UDPHS_EndOfTransfer(bEndpoint, USBD_STATUS_SUCCESS);
            }
        }
    }

    // STALL sent
    if( AT91C_UDPHS_STALL_SNT == (status & AT91C_UDPHS_STALL_SNT) ) {

        trace_LOG(trace_WARNING, "Sta 0x%X [%d] ", status, bEndpoint);

        // Acknowledge the stall flag
        AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCLRSTA = AT91C_UDPHS_STALL_SNT;

        // If the endpoint is not halted, clear the STALL condition
        if (pEndpoint->state != UDP_ENDPOINT_HALTED) {

            trace_LOG(trace_WARNING, "_ " );
            AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCLRSTA = AT91C_UDPHS_FRCESTALL;
        }
    }

    // SETUP packet received
    if( AT91C_UDPHS_RX_SETUP == (status & AT91C_UDPHS_RX_SETUP) )  {

        trace_LOG(trace_DEBUG, "Stp ");

        // If a transfer was pending, complete it
        // Handles the case where during the status phase of a control write
        // transfer, the host receives the device ZLP and ack it, but the ack
        // is not received by the device
        if ((pEndpoint->state == UDP_ENDPOINT_RECEIVING)
            || (pEndpoint->state == UDP_ENDPOINT_SENDING)) {

            UDPHS_EndOfTransfer(bEndpoint, USBD_STATUS_SUCCESS);
        }
        // Copy the setup packet
        UDPHS_ReadRequest(&request);

        // Acknowledge setup packet
        AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCLRSTA = AT91C_UDPHS_RX_SETUP;

        // Forward the request to the upper layer
        USBDCallbacks_RequestReceived(&request);
    }

}

//------------------------------------------------------------------------------
//      Interrupt service routine
//------------------------------------------------------------------------------
#ifdef DMA
//----------------------------------------------------------------------------
// \fn    UDPHS_DmaHandler
// \brief This function (ISR) handles dma interrupts
//----------------------------------------------------------------------------
static void UDPHS_DmaHandler( unsigned char bEndpoint )
{
    Endpoint     *pEndpoint = &(endpoints[bEndpoint]);
    Transfer     *pTransfer = &(pEndpoint->transfer);
    unsigned int  status;
    unsigned char result = USBD_STATUS_SUCCESS;

    status = AT91C_BASE_UDPHS->UDPHS_DMA[bEndpoint].UDPHS_DMASTATUS;
    trace_LOG(trace_DEBUG, "Dma Ept%d ", bEndpoint);

    // Disable DMA interrupt to avoid receiving 2 interrupts (B_EN and TR_EN)
    AT91C_BASE_UDPHS->UDPHS_DMA[bEndpoint].UDPHS_DMACONTROL &=
        ~(AT91C_UDPHS_END_TR_EN | AT91C_UDPHS_END_B_EN);

    AT91C_BASE_UDPHS->UDPHS_IEN &= ~(1 << SHIFT_DMA << bEndpoint);

    if( AT91C_UDPHS_END_BF_ST == (status & AT91C_UDPHS_END_BF_ST) ) {

        trace_LOG(trace_DEBUG, "EndBuffer ");

        // BUFF_COUNT holds the number of untransmitted bytes.
        // BUFF_COUNT is equal to zero in case of good transfer
        pTransfer->transferred = pTransfer->buffered
                                 - ((status & AT91C_UDPHS_BUFF_COUNT) >> 16);
        pTransfer->buffered = ((status & AT91C_UDPHS_BUFF_COUNT) >> 16);

        pTransfer->remaining -= pTransfer->transferred;

        trace_LOG(trace_DEBUG, "\n\rR:%d ", pTransfer->remaining );
        trace_LOG(trace_DEBUG, "B:%d ", pTransfer->buffered );
        trace_LOG(trace_DEBUG, "T:%d ", pTransfer->transferred );

        if( (pTransfer->remaining + pTransfer->buffered) > 0 ) {

            // Prepare an other transfer
            if( pTransfer->remaining > DMA_MAX_FIFO_SIZE ) {

                pTransfer->buffered = DMA_MAX_FIFO_SIZE;    
            }
            else {
                pTransfer->buffered = pTransfer->remaining;
            }

            AT91C_BASE_UDPHS->UDPHS_DMA[bEndpoint].UDPHS_DMAADDRESS = 
                    (unsigned int)((pTransfer->pData)+(pTransfer->buffered));

            // Clear unwanted interrupts
            AT91C_BASE_UDPHS->UDPHS_DMA[bEndpoint].UDPHS_DMASTATUS;

            // Enable DMA endpoint interrupt
            AT91C_BASE_UDPHS->UDPHS_IEN |= (1 << SHIFT_DMA << bEndpoint);
            // DMA config for receive the good size of buffer, or an error buffer

            AT91C_BASE_UDPHS->UDPHS_DMA[bEndpoint].UDPHS_DMACONTROL = 0; // raz
            AT91C_BASE_UDPHS->UDPHS_DMA[bEndpoint].UDPHS_DMACONTROL =
                                     ( ((pTransfer->buffered << 16) & AT91C_UDPHS_BUFF_COUNT)
                                       | AT91C_UDPHS_END_TR_EN
                                       | AT91C_UDPHS_END_TR_IT
                                       | AT91C_UDPHS_END_B_EN
                                       | AT91C_UDPHS_END_BUFFIT
                                       | AT91C_UDPHS_CHANN_ENB );
        }
    }
    else if( AT91C_UDPHS_END_TR_ST == (status & AT91C_UDPHS_END_TR_ST) ) {

        trace_LOG(trace_DEBUG, "EndTransf ");

        pTransfer->transferred = pTransfer->buffered
                                 - ((status & AT91C_UDPHS_BUFF_COUNT) >> 16);
        pTransfer->remaining = 0;
        trace_LOG(trace_DEBUG, "\n\rR:%d ", pTransfer->remaining );
        trace_LOG(trace_DEBUG, "B:%d ", pTransfer->buffered );
        trace_LOG(trace_DEBUG, "T:%d ", pTransfer->transferred );
    }
    else {

        trace_LOG(trace_ERROR, "UDPHS_DmaHandler: Error (0x%08X)\n\r", status);
        result = USBD_STATUS_ABORTED;
    }

    // Invoke callback
    if( pTransfer->remaining == 0 ) {

        trace_LOG(trace_DEBUG, "EOT ");
        UDPHS_EndOfTransfer(bEndpoint, result);
    }
}
#endif


//------------------------------------------------------------------------------
//      Exported functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// UDP interrupt handler
//         Manages device resume, suspend, end of bus reset. Forwards endpoint
//         interrupts to the appropriate handler.
//------------------------------------------------------------------------------
void USBD_InterruptHandler(void)
{
    unsigned int  status;
    unsigned char numIT;

    if (deviceState >= USBD_STATE_POWERED) {

        LED_Set(USBD_LEDUSB);
    }

    // Get interrupts status
    status = AT91C_BASE_UDPHS->UDPHS_INTSTA & AT91C_BASE_UDPHS->UDPHS_IEN;

    // Handle all UDPHS interrupts
    trace_LOG(trace_DEBUG, "H");
    while (status != 0) {

        // Start Of Frame (SOF)
        if ((status & AT91C_UDPHS_IEN_SOF) != 0) {

            trace_LOG(trace_DEBUG, "SOF ");

            // Invoke the SOF callback
            //USB_StartOfFrameCallback(pUsb);

            // Acknowledge interrupt
            AT91C_BASE_UDPHS->UDPHS_CLRINT = AT91C_UDPHS_IEN_SOF;
            status &= ~AT91C_UDPHS_IEN_SOF;
        }
        // Suspend
        // This interrupt is always treated last (hence the '==')
        else if (status == AT91C_UDPHS_DET_SUSPD) {

            trace_LOG(trace_DEBUG, "S");

            // The device enters the Suspended state
            // MCK + UDPCK must be off
            // Pull-Up must be connected
            // Transceiver must be disabled

            LED_Clear(USBD_LEDUSB);

            UDPHS_DisableBIAS();

            // Enable wakeup
            AT91C_BASE_UDPHS->UDPHS_IEN |= AT91C_UDPHS_WAKE_UP | AT91C_UDPHS_ENDOFRSM;
            AT91C_BASE_UDPHS->UDPHS_IEN &= ~AT91C_UDPHS_DET_SUSPD;

            // Acknowledge interrupt
            AT91C_BASE_UDPHS->UDPHS_CLRINT = AT91C_UDPHS_DET_SUSPD | AT91C_UDPHS_WAKE_UP;
            previousDeviceState = deviceState;
            deviceState = USBD_STATE_SUSPENDED;
            UDPHS_DisableUsbClock();

            // Invoke the Suspend callback
            USBDCallbacks_Suspended();
        }
        // Resume
        else if( ((status & AT91C_UDPHS_WAKE_UP) != 0)      // line activity
              || ((status & AT91C_UDPHS_ENDOFRSM) != 0))  { // pc wakeup

//JCB
#ifdef NOT_DEFINED
#if !defined(PIN_USB_VBUS)
            // Configure PIO
            PIO_Configure(&pinVbus, 1);

            // Check current level on VBus
            if (PIO_Get(&pinVbus) == 1)    // Protection
#endif
#endif
            {
                // Invoke the Resume callback
                USBDCallbacks_Resumed();

                trace_LOG(trace_DEBUG, "R");

                UDPHS_EnableUsbClock();
                UDPHS_EnableBIAS();

                // The device enters Configured state
                // MCK + UDPCK must be on
                // Pull-Up must be connected
                // Transceiver must be enabled

                deviceState = previousDeviceState;

                AT91C_BASE_UDPHS->UDPHS_CLRINT = AT91C_UDPHS_WAKE_UP | AT91C_UDPHS_ENDOFRSM | AT91C_UDPHS_DET_SUSPD;

                AT91C_BASE_UDPHS->UDPHS_IEN |= AT91C_UDPHS_ENDOFRSM | AT91C_UDPHS_DET_SUSPD;
                AT91C_BASE_UDPHS->UDPHS_CLRINT = AT91C_UDPHS_WAKE_UP | AT91C_UDPHS_ENDOFRSM;
                AT91C_BASE_UDPHS->UDPHS_IEN &= ~AT91C_UDPHS_WAKE_UP;
            }
// jcb !!!
#ifdef NOT_DEFINED
#if !defined(PIN_USB_VBUS)
            else {

                // No VBUS
                // Disconnect the pull-up
                USBD_Disconnect();
                AT91C_BASE_UDPHS->UDPHS_CLRINT = AT91C_UDPHS_WAKE_UP;
            }
#endif
#endif
        }
        // End of bus reset
        else if ((status & AT91C_UDPHS_ENDRESET) == AT91C_UDPHS_ENDRESET) {

//            trace_LOG(trace_DEBUG, "EoB ");

            // The device enters the Default state
            deviceState = USBD_STATE_DEFAULT;
            //      MCK + UDPCK are already enabled
            //      Pull-Up is already connected
            //      Transceiver must be enabled
            //      Endpoint 0 must be enabled

            UDPHS_ResetEndpoints();
            UDPHS_DisableEndpoints();
            USBD_ConfigureEndpoint(0);

            // Flush and enable the Suspend interrupt
            AT91C_BASE_UDPHS->UDPHS_CLRINT = AT91C_UDPHS_WAKE_UP | AT91C_UDPHS_DET_SUSPD;

            //// Enable the Start Of Frame (SOF) interrupt if needed
            //if (pCallbacks->startOfFrame != 0)
            //{
            //    AT91C_BASE_UDPHS->UDPHS_IEN |= AT91C_UDPHS_IEN_SOF;
            //}

            // Invoke the Reset callback
            USBDCallbacks_Reset();

            // Acknowledge end of bus reset interrupt
            AT91C_BASE_UDPHS->UDPHS_CLRINT = AT91C_UDPHS_ENDRESET;

            AT91C_BASE_UDPHS->UDPHS_IEN |= AT91C_UDPHS_DET_SUSPD;
        }
        // Handle upstream resume interrupt
        else if (status & AT91C_UDPHS_UPSTR_RES) {

            trace_LOG(trace_DEBUG, "ExtRes ");

            // - Acknowledge the IT
            AT91C_BASE_UDPHS->UDPHS_CLRINT = AT91C_UDPHS_UPSTR_RES;
        }
        // Endpoint interrupts
        else {
#ifndef DMA
            // Handle endpoint interrupts
            for (numIT = 0; numIT < NUM_IT_MAX; numIT++) {

                if ((status & (1 << SHIFT_INTERUPT << numIT)) != 0) {

                    UDPHS_EndpointHandler(numIT);
                }
            }
#else
            // Handle endpoint control interrupt
            if ((status & (1 << SHIFT_INTERUPT << 0)) != 0) {

                UDPHS_EndpointHandler( 0 );
            }
            else {

                numIT = 1;
                while((status&(0x7E<<SHIFT_DMA)) != 0) {

                    // Check if endpoint has a pending interrupt
                    if ((status & (1 << SHIFT_DMA << numIT)) != 0) {

                        UDPHS_DmaHandler(numIT);
                        status &= ~(1 << SHIFT_DMA << numIT);
                        if (status != 0) {

                            trace_LOG(trace_INFO, "\n\r  - ");
                        }
                    }
                    numIT++;
                }
            }
#endif
        }

        // Retrieve new interrupt status
        status = AT91C_BASE_UDPHS->UDPHS_INTSTA & AT91C_BASE_UDPHS->UDPHS_IEN;

        trace_LOG(trace_DEBUG, "\n\r");
        if (status != 0) {

            trace_LOG(trace_DEBUG, "  - ");
        }
    }

    if (deviceState >= USBD_STATE_POWERED) {

        LED_Clear(USBD_LEDUSB);
    }
}

//------------------------------------------------------------------------------
// Configure an endpoint with the provided endpoint descriptor
// pDdescriptor Pointer to the endpoint descriptor
// 
//------------------------------------------------------------------------------
void USBD_ConfigureEndpoint(const USBEndpointDescriptor *pDescriptor)
{
    Endpoint *pEndpoint;
    unsigned char bEndpoint;
    unsigned char bType;
    unsigned char bEndpointDir;
    unsigned char bSizeEpt = 0;

    // NULL descriptor -> Control endpoint 0
    if (pDescriptor == 0) {

        bEndpoint = 0;
        pEndpoint = &(endpoints[bEndpoint]);
        bType = USBEndpointDescriptor_CONTROL;
        bEndpointDir = 0;
        pEndpoint->size = BOARD_USB_ENDPOINTS_MAXPACKETSIZE(0);
        pEndpoint->bank = BOARD_USB_ENDPOINTS_BANKS(0);
    }
    else  {

        // The endpoint number
        bEndpoint = USBEndpointDescriptor_GetNumber(pDescriptor);
        pEndpoint = &(endpoints[bEndpoint]);
        // Transfer type: Control, Isochronous, Bulk, Interrupt
        bType = USBEndpointDescriptor_GetType(pDescriptor);
        // Direction, ignored for control endpoints
        bEndpointDir = USBEndpointDescriptor_GetDirection(pDescriptor);
        pEndpoint->size = USBEndpointDescriptor_GetMaxPacketSize(pDescriptor);
        pEndpoint->bank = BOARD_USB_ENDPOINTS_BANKS(bEndpoint);
    }

    // Abort the current transfer is the endpoint was configured and in
    // Write or Read state
    if( (pEndpoint->state == UDP_ENDPOINT_RECEIVING)
     || (pEndpoint->state == UDP_ENDPOINT_SENDING) ) {

        UDPHS_EndOfTransfer(bEndpoint, USBD_STATUS_RESET);
    }
    pEndpoint->state = UDP_ENDPOINT_IDLE;

    // Disable endpoint
    AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCTLDIS = AT91C_UDPHS_SHRT_PCKT
                                                           | AT91C_UDPHS_BUSY_BANK
                                                           | AT91C_UDPHS_NAK_OUT
                                                           | AT91C_UDPHS_NAK_IN
                                                           | AT91C_UDPHS_STALL_SNT
                                                           | AT91C_UDPHS_RX_SETUP
                                                           | AT91C_UDPHS_TX_PK_RDY
                                                           | AT91C_UDPHS_TX_COMPLT
                                                           | AT91C_UDPHS_RX_BK_RDY
                                                           | AT91C_UDPHS_ERR_OVFLW
                                                           | AT91C_UDPHS_MDATA_RX
                                                           | AT91C_UDPHS_DATAX_RX
                                                           | AT91C_UDPHS_NYET_DIS
                                                           | AT91C_UDPHS_INTDIS_DMA
                                                           | AT91C_UDPHS_AUTO_VALID
                                                           | AT91C_UDPHS_EPT_DISABL;

    // Reset Endpoint Fifos
    AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCLRSTA = AT91C_UDPHS_TOGGLESQ | AT91C_UDPHS_FRCESTALL;
    AT91C_BASE_UDPHS->UDPHS_EPTRST = 1<<bEndpoint;

    // Configure endpoint
    if( pEndpoint->size == 8 )  {
        bSizeEpt = 0;
    } 
    else if ( pEndpoint->size == 16 ) {
        bSizeEpt = 1;
    }
    else if ( pEndpoint->size == 32 ) {
        bSizeEpt = 2;
    }
    else if ( pEndpoint->size == 64 ) {
        bSizeEpt = 3;
    }
    else if ( pEndpoint->size == 128 ) {
        bSizeEpt = 4;
    }
    else if ( pEndpoint->size == 256 ) {
        bSizeEpt = 5;
    }
    else if ( pEndpoint->size == 512 )  {
        bSizeEpt = 6;
    }
    else if ( pEndpoint->size == 1024 ) {
        bSizeEpt = 7;
    } //else {
    //  sizeEpt = 0; // control endpoint
    //}

    // Configure endpoint
    if (bType == USBEndpointDescriptor_CONTROL) {

        // Enable endpoint IT for control endpoint
        AT91C_BASE_UDPHS->UDPHS_IEN |= (1<<SHIFT_INTERUPT<<bEndpoint);
    }


    AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCFG = bSizeEpt 
                                                        | (bEndpointDir << 3) 
                                                        | (bType << 4) 
                                                        | ((pEndpoint->bank) << 6);

    while( (signed int)AT91C_UDPHS_EPT_MAPD != (signed int)((AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCFG) & AT91C_UDPHS_EPT_MAPD) ) {

        // resolved by clearing the reset IT in good place
        trace_LOG(trace_ERROR, "PB bEndpoint: 0x%X\n\r", bEndpoint);
        trace_LOG(trace_ERROR, "PB bSizeEpt: 0x%X\n\r", bSizeEpt);
        trace_LOG(trace_ERROR, "PB bEndpointDir: 0x%X\n\r", bEndpointDir);
        trace_LOG(trace_ERROR, "PB bType: 0x%X\n\r", bType);
        trace_LOG(trace_ERROR, "PB pEndpoint->bank: 0x%X\n\r", pEndpoint->bank);
        trace_LOG(trace_ERROR, "PB UDPHS_EPTCFG: 0x%X\n\r", AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCFG);
        for(;;);
    }

    if (bType == USBEndpointDescriptor_CONTROL) {

        AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCTLENB = AT91C_UDPHS_RX_BK_RDY 
                                                               | AT91C_UDPHS_RX_SETUP
                                                               | AT91C_UDPHS_EPT_ENABL;
    }
    else {
#ifndef DMA
        AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCTLENB = AT91C_UDPHS_EPT_ENABL;
#else
        AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCTLENB = AT91C_UDPHS_AUTO_VALID 
                                                               | AT91C_UDPHS_EPT_ENABL;
#endif
    }

}

//------------------------------------------------------------------------------
// Sends data through an USB endpoint (IN)
//         Sets up the transfer descriptor, write one or two data payloads
//         (depending on the number of FIFO banks for the endpoint) and then
//         starts the actual transfer. The operation is complete when all
//         the data has been sent.
//------------------------------------------------------------------------------
char USBD_Write( unsigned char    bEndpoint,
                 const void       *pData,
                 unsigned int     dLength,
                 TransferCallback fCallback,
                 void             *pArgument )
{
    Endpoint *pEndpoint = &(endpoints[bEndpoint]);
    Transfer *pTransfer = &(pEndpoint->transfer);

    // Return if the endpoint is not in IDLE state
    if (pEndpoint->state != UDP_ENDPOINT_IDLE)  {

        return USBD_STATUS_LOCKED;
    }

    trace_LOG(trace_DEBUG, "Write%d(%d) ", bEndpoint, dLength);

    // Setup the transfer descriptor
    pTransfer->pData = (void *) pData;
    pTransfer->remaining = dLength;
    pTransfer->buffered = 0;
    pTransfer->transferred = 0;
    pTransfer->fCallback = fCallback;
    pTransfer->pArgument = pArgument;
    
    // Send one packet
    pEndpoint->state = UDP_ENDPOINT_SENDING;

#ifdef DMA
    // Test if endpoint type control
    if(AT91C_UDPHS_EPT_TYPE_CTL_EPT == (AT91C_UDPHS_EPT_TYPE&(AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCFG)))  {
#endif
        // Enable endpoint IT
        AT91C_BASE_UDPHS->UDPHS_IEN |= (1 << SHIFT_INTERUPT << bEndpoint);
        AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCTLENB = AT91C_UDPHS_TX_PK_RDY;

#ifdef DMA
    }
    else {

        if( pTransfer->remaining == 0 ) {
            // DMA not handle ZLP
            AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTSETSTA = AT91C_UDPHS_TX_PK_RDY;
            // Enable endpoint IT
            AT91C_BASE_UDPHS->UDPHS_IEN |= (1 << SHIFT_INTERUPT << bEndpoint);
            AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCTLENB = AT91C_UDPHS_TX_PK_RDY;
        }
        else {
            // Others endpoints (not control)
            if( pTransfer->remaining > DMA_MAX_FIFO_SIZE ) {

                // Transfer the max
                pTransfer->buffered = DMA_MAX_FIFO_SIZE;    
            }
            else {
                // Transfer the good size
                pTransfer->buffered = pTransfer->remaining;
            }

            trace_LOG(trace_DEBUG, "\n\r_WR:%d ", pTransfer->remaining );
            trace_LOG(trace_DEBUG, "B:%d ", pTransfer->buffered );
            trace_LOG(trace_DEBUG, "T:%d ", pTransfer->transferred );

            AT91C_BASE_UDPHS->UDPHS_DMA[bEndpoint].UDPHS_DMAADDRESS = (unsigned int)(pTransfer->pData);

            // Clear unwanted interrupts
            AT91C_BASE_UDPHS->UDPHS_DMA[bEndpoint].UDPHS_DMASTATUS;
            // Enable DMA endpoint interrupt
            AT91C_BASE_UDPHS->UDPHS_IEN |= (1 << SHIFT_DMA << bEndpoint);
            // DMA config
            AT91C_BASE_UDPHS->UDPHS_DMA[bEndpoint].UDPHS_DMACONTROL = 0; // raz
            AT91C_BASE_UDPHS->UDPHS_DMA[bEndpoint].UDPHS_DMACONTROL =
                                              ( ((pTransfer->buffered << 16) & AT91C_UDPHS_BUFF_COUNT)
                                                | AT91C_UDPHS_END_B_EN
                                                | AT91C_UDPHS_END_BUFFIT
                                                | AT91C_UDPHS_CHANN_ENB );
        }
    }
#endif

    return USBD_STATUS_SUCCESS;
}

//------------------------------------------------------------------------------
// Reads incoming data on an USB endpoint (OUT)
//------------------------------------------------------------------------------
char USBD_Read( unsigned char    bEndpoint,
                void             *pData,
                unsigned int     dLength,
                TransferCallback fCallback,
                void             *pArgument )
{
    Endpoint *pEndpoint = &(endpoints[bEndpoint]);
    Transfer *pTransfer = &(pEndpoint->transfer);
  
    // Return if the endpoint is not in IDLE state
    if (pEndpoint->state != UDP_ENDPOINT_IDLE) {

        return USBD_STATUS_LOCKED;
    }

    trace_LOG(trace_DEBUG, "Read%d(%d) ", bEndpoint, dLength);

    // Endpoint enters Receiving state
    pEndpoint->state = UDP_ENDPOINT_RECEIVING;

    // Set the transfer descriptor
    pTransfer->pData = pData;
    pTransfer->remaining = dLength;
    pTransfer->buffered = 0;
    pTransfer->transferred = 0;
    pTransfer->fCallback = fCallback;
    pTransfer->pArgument = pArgument;

#ifdef DMA
    // Test if endpoint type control
    if(AT91C_UDPHS_EPT_TYPE_CTL_EPT == (AT91C_UDPHS_EPT_TYPE&(AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCFG))) {
#endif
        // Control endpoint
        // Enable endpoint IT
        AT91C_BASE_UDPHS->UDPHS_IEN |= (1 << SHIFT_INTERUPT << bEndpoint);
        AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCTLENB = AT91C_UDPHS_RX_BK_RDY;
#ifdef DMA
    }
    else {

        trace_LOG(trace_DEBUG, "Read%d(%d) ", bEndpoint, dLength);

        // Others endpoints (not control)
        if( pTransfer->remaining > DMA_MAX_FIFO_SIZE ) {

            // Transfer the max
            pTransfer->buffered = DMA_MAX_FIFO_SIZE;    
        }
        else {
            // Transfer the good size
            pTransfer->buffered = pTransfer->remaining;
        }

        AT91C_BASE_UDPHS->UDPHS_DMA[bEndpoint].UDPHS_DMAADDRESS = (unsigned int)(pTransfer->pData);

        // Clear unwanted interrupts
        AT91C_BASE_UDPHS->UDPHS_DMA[bEndpoint].UDPHS_DMASTATUS;

        // Enable DMA endpoint interrupt
        AT91C_BASE_UDPHS->UDPHS_IEN |= (1 << SHIFT_DMA << bEndpoint);

        trace_LOG(trace_DEBUG, "\n\r_RR:%d ", pTransfer->remaining );
        trace_LOG(trace_DEBUG, "B:%d ", pTransfer->buffered );
        trace_LOG(trace_DEBUG, "T:%d ", pTransfer->transferred );

        // DMA config
        AT91C_BASE_UDPHS->UDPHS_DMA[bEndpoint].UDPHS_DMACONTROL = 0; // raz
        AT91C_BASE_UDPHS->UDPHS_DMA[bEndpoint].UDPHS_DMACONTROL =
                                 ( ((pTransfer->buffered << 16) & AT91C_UDPHS_BUFF_COUNT)
                                   | AT91C_UDPHS_END_TR_EN
                                   | AT91C_UDPHS_END_TR_IT
                                   | AT91C_UDPHS_END_B_EN
                                   | AT91C_UDPHS_END_BUFFIT
                                   | AT91C_UDPHS_CHANN_ENB );
    }
#endif

    return USBD_STATUS_SUCCESS;
}

//------------------------------------------------------------------------------
// Clears, sets or returns the Halt state on specified endpoint
//
//         When in Halt state, an endpoint acknowledges every received packet
//         with a STALL handshake. This continues until the endpoint is
//         manually put out of the Halt state by calling this function.
// pUsb Pointer to a S_usb instance
// bEndpoint Index of endpoint
// bRequest  Request to perform
//                   -> USB_SET_FEATURE, USB_CLEAR_FEATURE, USB_GET_STATUS
// \return true if the endpoint is currently Halted, false otherwise
// 
//------------------------------------------------------------------------------
void USBD_Halt( unsigned char bEndpoint )
{
    Endpoint *pEndpoint = &(endpoints[bEndpoint]);

    // Check that endpoint is enabled and not already in Halt state
    if( (pEndpoint->state != UDP_ENDPOINT_DISABLED)
     && (pEndpoint->state != UDP_ENDPOINT_HALTED) ) {

        trace_LOG(trace_DEBUG, "Halt%d ", bEndpoint);

        // Abort the current transfer if necessary
        UDPHS_EndOfTransfer(bEndpoint, USBD_STATUS_ABORTED);

        // Put endpoint into Halt state
        AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTSETSTA = AT91C_UDPHS_FRCESTALL;
        pEndpoint->state = UDP_ENDPOINT_HALTED;

#ifdef DMA
        // Test if endpoint type control
        if(AT91C_UDPHS_EPT_TYPE_CTL_EPT == (AT91C_UDPHS_EPT_TYPE&(AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCFG)))  {
#endif
            // Enable the endpoint interrupt
            AT91C_BASE_UDPHS->UDPHS_IEN |= (1<<SHIFT_INTERUPT<<bEndpoint);
#ifdef DMA
        }
        else {
            // Enable IT DMA
            AT91C_BASE_UDPHS->UDPHS_IEN |= (1<<SHIFT_DMA<<bEndpoint);
        }
#endif
   }
}

//------------------------------------------------------------------------------
//    Function: USBD_Unhalt
//        Clears the Halt feature on the given endpoint.
//------------------------------------------------------------------------------
void USBD_Unhalt( unsigned char bEndpoint )
{
    Endpoint *pEndpoint = &(endpoints[bEndpoint]);

    // Check if the endpoint is enabled
    if (pEndpoint->state != UDP_ENDPOINT_DISABLED) {

        trace_LOG(trace_DEBUG, "Unhalt%d ", bEndpoint);

        // Return endpoint to Idle state
        pEndpoint->state = UDP_ENDPOINT_IDLE;

        // Clear FORCESTALL flag
        AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTCLRSTA = AT91C_UDPHS_TOGGLESQ | AT91C_UDPHS_FRCESTALL;

        // Reset Endpoint Fifos
        AT91C_BASE_UDPHS->UDPHS_EPTRST = (1<<bEndpoint);
    }
}

//------------------------------------------------------------------------------
//    Function: USBD_IsHalted
//        Returns the current Halt status of an endpoint.
//
//    Parameters:
//        bEndpoint - Endpoint number.
//
//    Returns:
//        1 if the endpoint is currently halted; otherwise 0.
//------------------------------------------------------------------------------
unsigned char USBD_IsHalted( unsigned char bEndpoint )
{
    Endpoint *pEndpoint = &(endpoints[bEndpoint]);
    unsigned char status = 0;

    if (pEndpoint->state == UDP_ENDPOINT_HALTED) {
        status = 1;
    }
    return( status );
}

//------------------------------------------------------------------------------
// IS High Speed device working in High Speed ?
//------------------------------------------------------------------------------
unsigned char USBD_IsHighSpeed( void )
{
    unsigned char status = 0;

    if( AT91C_UDPHS_SPEED == (AT91C_BASE_UDPHS->UDPHS_INTSTA & AT91C_UDPHS_SPEED) )
    {
        // High Speed
        trace_LOG(trace_DEBUG, "High Speed\n\r");
        status = 1;
    }
    else {
        trace_LOG(trace_DEBUG, "Full Speed\n\r");
    }
    return( status );
}
 

//------------------------------------------------------------------------------
// Causes the endpoint to acknowledge the next received packet with
//         a STALL handshake.
//         Further packets are then handled normally.
// bEndpoint Index of endpoint
// \return Operation result code
//------------------------------------------------------------------------------
unsigned char USBD_Stall( unsigned char bEndpoint )
{
    Endpoint *pEndpoint = &(endpoints[bEndpoint]);

    // Check that endpoint is in Idle state
    if (pEndpoint->state != UDP_ENDPOINT_IDLE) {

        trace_LOG(trace_WARNING, "W: UDP_Stall: Endpoint%d locked\n\r", bEndpoint);
        return USBD_STATUS_LOCKED;
    }

    trace_LOG(trace_DEBUG, "Stall%d ", bEndpoint);

    AT91C_BASE_UDPHS->UDPHS_EPT[bEndpoint].UDPHS_EPTSETSTA = AT91C_UDPHS_FRCESTALL;

    return USBD_STATUS_SUCCESS;
}

//------------------------------------------------------------------------------
// Activates a remote wakeup procedure
//------------------------------------------------------------------------------
void USBD_RemoteWakeUp(void)
{
    trace_LOG(trace_DEBUG, "Remote WakeUp\n\r");

    // Device is currently suspended
    if (deviceState == USBD_STATE_SUSPENDED) {

        trace_LOG(trace_DEBUG, "RW\n\r");
        UDPHS_EnableUsbClock();

        // Activates a remote wakeup
        AT91C_BASE_UDPHS->UDPHS_CTRL |= AT91C_UDPHS_REWAKEUP;

        while ((AT91C_BASE_UDPHS->UDPHS_CTRL&AT91C_UDPHS_REWAKEUP) == AT91C_UDPHS_REWAKEUP) {

            trace_LOG(trace_DEBUG, "W");
        }
        UDPHS_EnableBIAS();
    }
    // Device is NOT suspended
    else {

        trace_LOG(trace_WARNING, "-W- USBD_RemoteWakeUp: Device is not suspended\n\r");
    }
}

//------------------------------------------------------------------------------
// Sets or unsets the device address
//         This function directly accesses the S_usb_request instance located
//         in the S_usb structure to extract its new address.
//------------------------------------------------------------------------------
void USBD_SetAddress( unsigned char address )
{
    volatile unsigned int i;

    trace_LOG(trace_DEBUG, "SetAddr(%d) ", address);

    if( USBD_IsHighSpeed() ) {
        // Timeout after 6 HUBs
        AT91C_BASE_UDPHS->UDPHS_EPT[0].UDPHS_EPTSETSTA = AT91C_UDPHS_TX_PK_RDY;
        i=0;
        while( (0 != (AT91C_BASE_UDPHS->UDPHS_EPT[0].UDPHS_EPTSTA & AT91C_UDPHS_TX_PK_RDY ))
            && (i< BOARD_MCK/10000) ) {  
            i++; // around 5 ms
        }
    }

    // Set address
    AT91C_BASE_UDPHS->UDPHS_CTRL &= ~AT91C_UDPHS_DEV_ADDR; // RAZ Address
    AT91C_BASE_UDPHS->UDPHS_CTRL |= address | AT91C_UDPHS_FADDR_EN;

    // If the address is 0, the device returns to the Default state
    if (address == 0) {
        deviceState = USBD_STATE_DEFAULT;
    }
    // If the address is non-zero, the device enters the Address state
    else {
        deviceState = USBD_STATE_ADDRESS;
    }
}

//------------------------------------------------------------------------------
// Changes the device state from Address to Configured, or from
//         Configured to Address.
//         This method directly access the last received SETUP packet to
//         decide on what to do.
//------------------------------------------------------------------------------
void USBD_SetConfiguration( unsigned char cfgnum )
{
    trace_LOG(trace_DEBUG, "SetCfg(%d) ", cfgnum);

    // Check the request
    if( cfgnum != 0 ) {

        // Enter Configured state
        deviceState = USBD_STATE_CONFIGURED;
    }
    // If the configuration number is zero, the device goes back to the Address
    // state
    else  {

        // Go back to Address state
        deviceState = USBD_STATE_ADDRESS;

        // Abort all transfers
        UDPHS_DisableEndpoints();
    }
}

//------------------------------------------------------------------------------
// Enables the pull-up on the D+ line to connect the device to the USB.
//------------------------------------------------------------------------------
void USBD_Connect( void )
{
    trace_LOG(trace_DEBUG, "Conn ");
#if defined(BOARD_USB_PULLUP_INTERNAL)
    AT91C_BASE_UDPHS->UDPHS_CTRL &= ~AT91C_UDPHS_DETACH;   // Pull Up on DP
    AT91C_BASE_UDPHS->UDPHS_CTRL |= AT91C_UDPHS_PULLD_DIS; // Disable Pull Down

#elif defined(BOARD_USB_PULLUP_INTERNAL_BY_MATRIX)
    trace_LOG(trace_DEBUG, "PUON 1\n\r");
    AT91C_BASE_MATRIX->MATRIX_USBPCR |= AT91C_MATRIX_USBPCR_PUON;

#elif defined(BOARD_USB_PULLUP_EXTERNAL)

#ifdef PIN_USB_PULLUP
    const Pin pinPullUp = PIN_USB_PULLUP;
    if( pinPullUp.attribute == PIO_OUTPUT_0 ) {

        PIO_Set(&pinPullUp);
    }
    else {

        PIO_Clear(&pinPullUp);
    }
#else
    #error unsupported now
#endif

#elif !defined(BOARD_USB_PULLUP_ALWAYSON)
    #error Unsupported pull-up type.

#endif
}

//------------------------------------------------------------------------------
// Disables the pull-up on the D+ line to disconnect the device from the bus.
//------------------------------------------------------------------------------
void USBD_Disconnect( void )
{
    trace_LOG(trace_DEBUG, "Disc ");

#if defined(BOARD_USB_PULLUP_INTERNAL)
    AT91C_BASE_UDPHS->UDPHS_CTRL |= AT91C_UDPHS_DETACH; // detach
    AT91C_BASE_UDPHS->UDPHS_CTRL &= ~AT91C_UDPHS_PULLD_DIS; // Enable Pull Down

#elif defined(BOARD_USB_PULLUP_INTERNAL_BY_MATRIX)
    AT91C_BASE_MATRIX->MATRIX_USBPCR &= ~AT91C_MATRIX_USBPCR_PUON;

#elif defined(BOARD_USB_PULLUP_EXTERNAL)

#ifdef PIN_USB_PULLUP
    const Pin pinPullUp = PIN_USB_PULLUP;
    if (pinPullUp.attribute == PIO_OUTPUT_0) {

        PIO_Clear(&pinPullUp);
    }
    else {

        PIO_Set(&pinPullUp);
    }
#else
    #error unsupported now
#endif

#elif !defined(BOARD_USB_PULLUP_ALWAYSON)
    #error Unsupported pull-up type.

#endif

    // Device returns to the Powered state
    if (deviceState > USBD_STATE_POWERED) {    

        deviceState = USBD_STATE_POWERED;
    }
}

//------------------------------------------------------------------------------
// Certification test for High Speed device.
// bIndex char for the test choice
//------------------------------------------------------------------------------
void USBD_Test( unsigned char bIndex )
{
    char          *pFifo;
    unsigned char i;

    AT91C_BASE_UDPHS->UDPHS_IEN &= ~AT91C_UDPHS_DET_SUSPD; // remove suspend for TEST
    AT91C_BASE_UDPHS->UDPHS_TST |= AT91C_UDPHS_SPEED_CFG_HS; // force High Speed (remove suspend)

    switch( bIndex ) {

        case USBFeatureRequest_TESTPACKET:
            trace_LOG(trace_DEBUG, "TEST_PACKET ");

            AT91C_BASE_UDPHS->UDPHS_DMA[1].UDPHS_DMACONTROL = 0;
            AT91C_BASE_UDPHS->UDPHS_DMA[2].UDPHS_DMACONTROL = 0;

            // Configure endpoint 2, 64 bytes, direction IN, type BULK, 1 bank
            AT91C_BASE_UDPHS->UDPHS_EPT[2].UDPHS_EPTCFG = AT91C_UDPHS_EPT_SIZE_64 | AT91C_UDPHS_EPT_DIR_IN | AT91C_UDPHS_EPT_TYPE_BUL_EPT | AT91C_UDPHS_BK_NUMBER_1;
            while( (signed int)(AT91C_BASE_UDPHS->UDPHS_EPT[2].UDPHS_EPTCFG & AT91C_UDPHS_EPT_MAPD) != (signed int)AT91C_UDPHS_EPT_MAPD ) {}

            AT91C_BASE_UDPHS->UDPHS_EPT[2].UDPHS_EPTCTLENB =  AT91C_UDPHS_EPT_ENABL;

            // Write FIFO
            pFifo = (char*)((unsigned int *)(AT91C_BASE_UDPHS_EPTFIFO->UDPHS_READEPT0) + (16384 * 2));
            //pFifo = (char*)&(AT91C_BASE_UDPHS_EPTFIFO->UDPHS_READEPT0[bEndpoint*16384]);
            for( i=0; i<sizeof(test_packet_buffer); i++) {
                pFifo[i] = test_packet_buffer[i];
            }
            // Tst PACKET
            AT91C_BASE_UDPHS->UDPHS_TST |= AT91C_UDPHS_TST_PKT;
            // Send packet
            AT91C_BASE_UDPHS->UDPHS_EPT[2].UDPHS_EPTSETSTA = AT91C_UDPHS_TX_PK_RDY;
            break;

        case USBFeatureRequest_TESTJ:
            trace_LOG(trace_DEBUG, "TEST_J ");
            AT91C_BASE_UDPHS->UDPHS_TST = AT91C_UDPHS_TST_J;
            break;

        case USBFeatureRequest_TESTK:
            trace_LOG(trace_DEBUG, "TEST_K ");
            AT91C_BASE_UDPHS->UDPHS_TST = AT91C_UDPHS_TST_K;
            break;

        case USBFeatureRequest_TESTSE0NAK:
            trace_LOG(trace_DEBUG, "TEST_SEO_NAK ");
            AT91C_BASE_UDPHS->UDPHS_IEN = 0;  // for test
            break;

        case USBFeatureRequest_TESTSENDZLP:
            //while( 0 != (AT91C_BASE_UDPHS->UDPHS_EPT[0].UDPHS_EPTSTA & AT91C_UDPHS_TX_PK_RDY ) ) {}
            AT91C_BASE_UDPHS->UDPHS_EPT[0].UDPHS_EPTSETSTA = AT91C_UDPHS_TX_PK_RDY;
            //while( 0 != (AT91C_BASE_UDPHS->UDPHS_EPT[0].UDPHS_EPTSTA & AT91C_UDPHS_TX_PK_RDY ) ) {}
            trace_LOG(trace_DEBUG, "SEND_ZLP ");
            break;
    }
    trace_LOG(trace_DEBUG, "\n\r");
}


//------------------------------------------------------------------------------
// Initializes the specified USB driver
//         This function initializes the current FIFO bank of endpoints,
//         configures the pull-up and VBus lines, disconnects the pull-up and
//         then trigger the Init callback.
//------------------------------------------------------------------------------
void USBD_Init( void )
{
    unsigned char i;

    trace_LOG(trace_DEBUG, "USBD Init()\n\r");

    // Reset endpoint structures
    UDPHS_ResetEndpoints();

    // Enables the USB Clock
    UDPHS_EnableUsbClock();

    // Configure the pull-up on D+ and disconnect it
#if defined(BOARD_USB_PULLUP_INTERNAL)
    AT91C_BASE_UDPHS->UDPHS_CTRL |= AT91C_UDPHS_DETACH; // detach
    AT91C_BASE_UDPHS->UDPHS_CTRL |= AT91C_UDPHS_PULLD_DIS; // Disable Pull Down

#elif defined(BOARD_USB_PULLUP_INTERNAL_BY_MATRIX)
    trace_LOG(trace_DEBUG, "PUON 0\n\r");
    AT91C_BASE_MATRIX->MATRIX_USBPCR &= ~AT91C_MATRIX_USBPCR_PUON;

#elif defined(BOARD_USB_PULLUP_EXTERNAL)
#ifdef PIN_USB_PULLUP
    const Pin pinPullUp = PIN_USB_PULLUP;
    PIO_Configure(&pinPullUp, 1);
    if (pinPullUp.attribute == PIO_OUTPUT_0) {

        PIO_Clear(&pinPullUp);
    }
    else {

        PIO_Set(&pinPullUp);
    }
#else
    #error unsupported now
#endif
#elif !defined(BOARD_USB_PULLUP_ALWAYSON)
    #error Unsupported pull-up type.

#endif

    // Reset and enable IP UDPHS
    AT91C_BASE_UDPHS->UDPHS_CTRL &= ~AT91C_UDPHS_EN_UDPHS;
    AT91C_BASE_UDPHS->UDPHS_CTRL |= AT91C_UDPHS_EN_UDPHS;
    // Enable and disable of the transceiver is automaticaly done by the IP.

    // With OR without DMA !!!
    // Initialization of DMA
    for( i=1; i<=((AT91C_BASE_UDPHS->UDPHS_IPFEATURES & AT91C_UDPHS_DMA_CHANNEL_NBR)>>4); i++ ) {

        // RESET endpoint canal DMA:
        // DMA stop channel command
        AT91C_BASE_UDPHS->UDPHS_DMA[i].UDPHS_DMACONTROL = 0;  // STOP command

        // Disable endpoint
        AT91C_BASE_UDPHS->UDPHS_EPT[i].UDPHS_EPTCTLDIS = AT91C_UDPHS_SHRT_PCKT
                                                       | AT91C_UDPHS_BUSY_BANK
                                                       | AT91C_UDPHS_NAK_OUT
                                                       | AT91C_UDPHS_NAK_IN
                                                       | AT91C_UDPHS_STALL_SNT
                                                       | AT91C_UDPHS_RX_SETUP
                                                       | AT91C_UDPHS_TX_PK_RDY
                                                       | AT91C_UDPHS_TX_COMPLT
                                                       | AT91C_UDPHS_RX_BK_RDY
                                                       | AT91C_UDPHS_ERR_OVFLW
                                                       | AT91C_UDPHS_MDATA_RX
                                                       | AT91C_UDPHS_DATAX_RX
                                                       | AT91C_UDPHS_NYET_DIS
                                                       | AT91C_UDPHS_INTDIS_DMA
                                                       | AT91C_UDPHS_AUTO_VALID
                                                       | AT91C_UDPHS_EPT_DISABL;

        // Reset endpoint config
        AT91C_BASE_UDPHS->UDPHS_EPT[i].UDPHS_EPTCTLENB = 0;

        // Reset DMA channel (Buff count and Control field)
        AT91C_BASE_UDPHS->UDPHS_DMA[i].UDPHS_DMACONTROL = AT91C_UDPHS_LDNXT_DSC;  // NON STOP command

        // Reset DMA channel 0 (STOP)
        AT91C_BASE_UDPHS->UDPHS_DMA[i].UDPHS_DMACONTROL = 0;  // STOP command

        // Clear DMA channel status (read the register for clear it)
        AT91C_BASE_UDPHS->UDPHS_DMA[i].UDPHS_DMASTATUS = AT91C_BASE_UDPHS->UDPHS_DMA[i].UDPHS_DMASTATUS;

    }

    AT91C_BASE_UDPHS->UDPHS_TST = 0;
    AT91C_BASE_UDPHS->UDPHS_IEN = 0;
    AT91C_BASE_UDPHS->UDPHS_CLRINT = AT91C_UDPHS_UPSTR_RES
                                   | AT91C_UDPHS_ENDOFRSM
                                   | AT91C_UDPHS_WAKE_UP
                                   | AT91C_UDPHS_ENDRESET
                                   | AT91C_UDPHS_IEN_SOF
                                   | AT91C_UDPHS_MICRO_SOF
                                   | AT91C_UDPHS_DET_SUSPD;
    
    // Device is in the Attached state
    deviceState = USBD_STATE_SUSPENDED;
    previousDeviceState = USBD_STATE_POWERED;

    // Disable interrupts
    AT91C_BASE_UDPHS->UDPHS_IEN = AT91C_UDPHS_ENDOFRSM
                                | AT91C_UDPHS_WAKE_UP
                                | AT91C_UDPHS_DET_SUSPD;

    // Disable USB clocks
    UDPHS_DisableUsbClock();

    // Configure interrupts
    USBDCallbacks_Initialized();
}

//------------------------------------------------------------------------------
//    Function: USBD_GetState
//        Returns the current state of the USB device.
//    Returns:
//        Device current state.
//------------------------------------------------------------------------------
unsigned char USBD_GetState( void )
{
    return deviceState;
}

#endif // BOARD_USB_UDPHS

