/*
	FreeRTOS.org V4.6.0 - Copyright (C) 2003-2007 Richard Barry.

	This file is part of the FreeRTOS.org distribution.

	FreeRTOS.org is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	FreeRTOS.org is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FreeRTOS.org; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	A special exception to the GPL can be applied should you wish to distribute
	a combined work that includes FreeRTOS.org, without being obliged to provide
	the source code for any proprietary components.  See the licensing section
	of http://www.FreeRTOS.org for full details of how and when the exception
	can be applied.

	***************************************************************************
	See http://www.FreeRTOS.org for documentation, latest information, license
	and contact details.  Please ensure to read the configuration and relevant
	port sections of the online documentation.

	Also see http://www.SafeRTOS.com for an IEC 61508 compliant version along
	with commercial development and support options.
	***************************************************************************
*/

/*
	USB Communications Device Class driver.
	Implements task vUSBCDCTask and provides an Abstract Control Model serial 
	interface.  Control is through endpoint 0, device-to-host notification is 
	provided by interrupt-in endpoint 3, and raw data is transferred through 
	bulk endpoints 1 and 2.

	- developed from original FreeRTOS HID example by Scott Miller
	- modified to support 3.2 GCC by najay
*/

/* MakingThings */
#include "config.h"
#ifdef MAKE_CTRL_USB

#include <string.h>
#include <stdio.h>
#include "Board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "USB-CDC.h"
#include "descriptors.h"

#define usbNO_BLOCK ( ( portTickType ) 0 )

/* Reset all endpoints */
static void prvResetEndPoints( void );

/* Clear pull up resistor to detach device from host */
static void vDetachUSBInterface( void );

/* Set up interface and initialize variables */
static void vInitUSBInterface( void );

/* Handle control endpoint events. */
static void prvProcessEndPoint0Interrupt( xISRStatus *pxMessage );

/* Handle standard device requests. */
static void prvHandleStandardDeviceRequest( xUSB_REQUEST *pxRequest );

/* Handle standard interface requests. */
static void prvHandleStandardInterfaceRequest( xUSB_REQUEST *pxRequest );

/* Handle endpoint requests. */
static void prvHandleStandardEndPointRequest( xUSB_REQUEST *pxRequest );

/* Handle class interface requests. */
static void prvHandleClassInterfaceRequest( xUSB_REQUEST *pxRequest );

/* Prepare control data transfer.  prvSendNextSegment starts transfer. */
static void prvSendControlData( unsigned portCHAR *pucData, unsigned portSHORT usRequestedLength, unsigned portLONG ulLengthLeftToSend, portLONG lSendingDescriptor );

/* Send next segment of data for the control transfer */
static void prvSendNextSegment( void );

/* Send stall - used to respond to unsupported requests */
static void prvSendStall( void );

/* Send a zero-length (null) packet */
static void prvSendZLP( int endpoint );

/* Handle requests for standard interface descriptors */
static void prvGetStandardInterfaceDescriptor( xUSB_REQUEST *pxRequest );

/*------------------------------------------------------------*/

/* File scope static variables */
static unsigned portCHAR ucUSBConfig = ( unsigned portCHAR ) 0;
static unsigned portLONG ulReceivedAddress = ( unsigned portLONG ) 0;
static eDRIVER_STATE eDriverState = eNOTHING;

/* Incoming and outgoing control data structures */
static xCONTROL_MESSAGE pxControlTx;
static xCONTROL_MESSAGE pxControlRx;

/* Queue holding pointers to pending messages */
xQueueHandle xUSBInterruptQueue; 

/* Queues used to hold received characters, and characters waiting to be
transmitted.  Rx queue must be larger than FIFO size. */
xQueueHandle xRxCDC; 
xQueueHandle xTxCDC;

/* Line coding - 115,200 baud, N-8-1 */
static const unsigned portCHAR pxLineCoding[] = { 0x00, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x08 };

/* Status variables. */
static unsigned portCHAR ucControlState;
static unsigned int uiCurrentBank;

/*------------------------------------------------------------*/
extern int OscBusy;
extern int Usb_Running;

void vUSBCDCTask( void *pvParameters )
{
xISRStatus *pxMessage;
unsigned portLONG ulStatus;
unsigned portLONG ulRxBytes;

	( void ) pvParameters;

	/* Disconnect USB device from hub.  For debugging - causes host to register reset */
	portENTER_CRITICAL();
		 vDetachUSBInterface();
	portEXIT_CRITICAL();
	
	vTaskDelay( portTICK_RATE_MS * 60 );

	/* Init USB interface */
	portENTER_CRITICAL();
		vInitUSBInterface();
	portEXIT_CRITICAL();
	
	/* Main task loop.  Process incoming endpoint 0 interrupts, handle data transfers. */
	 
  /* MAKINGTHINGS: USB RUNNING */
  Usb_Running = 1;
  /* MakingThings - if we sent a packet that was a multiple of EP_FIFO,
     we need to send a zero length packet to force the transfer
  */
  bool sentEvenPacket = false; 

	for( ;; )
	{
		/* Look for data coming from the ISR. */
		if( xQueueReceive( xUSBInterruptQueue, &pxMessage, usbSHORTEST_DELAY ) )
		{
			if( pxMessage->ulISR & AT91C_UDP_EPINT0 )
			{
				/* All endpoint 0 interrupts are handled here. */
				prvProcessEndPoint0Interrupt( pxMessage );
			}

			if( pxMessage->ulISR & AT91C_UDP_ENDBUSRES )
			{
				/* End of bus reset - reset the endpoints and de-configure. */
				prvResetEndPoints();		
			}
		}

    if(sentEvenPacket) // if we sent a packet with a length multiple of EP_FIFO
    {
      prvSendZLP(usbEND_POINT_2);
      sentEvenPacket = false;
    }
		
		/* See if we're ready to send and receive data. */
		if( eDriverState == eREADY_TO_SEND && ucControlState ) 
		{
			if( ( !(AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_2 ] & AT91C_UDP_TXPKTRDY) ) && uxQueueMessagesWaiting( xTxCDC ) )
			{
        xBULKBUFFER Queue2USB;
        if( xQueueReceive( xTxCDC, &Queue2USB, 0 ) ) 
        {
          unsigned char out = 0;
          sentEvenPacket = ( Queue2USB.Count == EP_FIFO);

          while (out < Queue2USB.Count)
            AT91C_BASE_UDP->UDP_FDR[ usbEND_POINT_2 ] = Queue2USB.Data[out++];
        }
				AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_2 ] |= AT91C_UDP_TXPKTRDY;
			}

			/* Check for incoming data (host-to-device) on endpoint 1. */
			while( AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_1 ] & (AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1) )
			{
				/* Only process FIFO if there's room to store it in the queue */
				if ( USB_CDC_QUEUE_SIZE > uxQueueMessagesWaiting( xRxCDC ))
				{	
          xBULKBUFFER USB2Queue;

          ulRxBytes = (AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_1 ] >> 16) & usbRX_COUNT_MASK;
          // collect however many bytes are in endpoint ... may be a whole osc packet
          // may just be a fragment.
          USB2Queue.Count = 0;
					while( ulRxBytes-- )
            USB2Queue.Data[USB2Queue.Count++] = AT91C_BASE_UDP->UDP_FDR[ usbEND_POINT_1 ];

          // put it in the queue
          xQueueSend( xRxCDC, &USB2Queue, 0 );

          /* Release the FIFO */
					portENTER_CRITICAL();
					{
						ulStatus = AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_1 ];
						usbCSR_CLEAR_BIT( &ulStatus, uiCurrentBank );
						AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_1 ] = ulStatus;
					}
					portEXIT_CRITICAL();

					/* Re-enable endpoint 1's interrupts */
					AT91C_BASE_UDP->UDP_IER = AT91C_UDP_EPINT1;
				
					/* Update the current bank in use */
					if( uiCurrentBank == AT91C_UDP_RX_DATA_BK0 ) 
						uiCurrentBank = AT91C_UDP_RX_DATA_BK1;
					else 
						uiCurrentBank = AT91C_UDP_RX_DATA_BK0;

				}
				else 
					break;
			}
		}
	}
}
/*------------------------------------------------------------*/
#if 0
void vUSBSendByte( portCHAR cByte, int timeout )
{
  static xBULKBUFFER EnQueue;
  static char inPacket = 0;  // OSC packet detection eliminates need for tx timeouts

  if (!inPacket && (cByte == SOP))
  { 
    inPacket = 1;
    EnQueue.Count = 0;
    EnQueue.Data[EnQueue.Count++] = cByte;
  }
  else  // in a packet
  {
    EnQueue.Data[EnQueue.Count++] = cByte;

    // if end of packet of buffer full, commit buffer to queue
    if ((cByte == SOP) || (EnQueue.Count == EP_FIFO)) 
    {
      if (cByte == SOP) inPacket = 0;
//      if ( xQueueSend( xTxCDC, &EnQueue, timeout ) != pdPASS )
      while (xQueueSend( xTxCDC, &EnQueue, 0 ) != pdPASS ) 
        ;

      EnQueue.Count = 0;
    }
  }
}
#endif

#if 0
inline int cUSBGetByte( portTickType timeout )
{
  static xBULKBUFFER DeQueue;
  static char QueueValid = 0;
  static unsigned char out;

  // first look for bytes in a buffer we've already gotten
drainQueue:
  if (QueueValid) {
    if (out < DeQueue.Count)
      return DeQueue.Data[out++];  // drain the buffer
    else
      QueueValid = 0;   // buffer was empty ... next check for new one.
  }

  if ( xQueueReceive( xRxCDC, &DeQueue, timeout ) != pdFAIL )
  {
    // got a new message from USB waiting on the queue
    out = 0;    // reset drain index
    QueueValid = 1;   // indicate we'll be working from the just copied message
    goto drainQueue;  // now go back and fetch the first entry from message
  }
  else
    return -1;        // nope, there was really nothing available
}
#endif
/*------------------------------------------------------------*/

static void prvSendZLP( int endpoint )
{
unsigned portLONG ulStatus;

	/* Wait until the FIFO is free - even though we are not going to use it.
	THERE IS NO TIMEOUT HERE! */
	while( AT91C_BASE_UDP->UDP_CSR[ endpoint ] & AT91C_UDP_TXPKTRDY )
	{
		vTaskDelay( usbSHORTEST_DELAY );
	}

	portENTER_CRITICAL();
	{
		/* Cancel any further pending data */
		pxControlTx.ulTotalDataLength = pxControlTx.ulNextCharIndex;

		/* Set the TXPKTRDY bit to cause a transmission with no data. */
		ulStatus = AT91C_BASE_UDP->UDP_CSR[ endpoint ];
		usbCSR_SET_BIT( &ulStatus, AT91C_UDP_TXPKTRDY );
		AT91C_BASE_UDP->UDP_CSR[ endpoint ] = ulStatus;
	}
	portEXIT_CRITICAL();
}
/*------------------------------------------------------------*/

static void prvSendStall( void )
{
	unsigned portLONG ulStatus;

	portENTER_CRITICAL();
	{
		/* Force a stall by simply setting the FORCESTALL bit in the CSR. */
		ulStatus = AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ];
		usbCSR_SET_BIT( &ulStatus, AT91C_UDP_FORCESTALL );
		AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ] = ulStatus;
	}
	portEXIT_CRITICAL();
}
/*------------------------------------------------------------*/

static void prvResetEndPoints( void )
{
unsigned portLONG ulTemp;

	eDriverState = eJUST_RESET;
	ucControlState = 0;

	/* Reset all the end points. */
	AT91C_BASE_UDP->UDP_RSTEP  = usbEND_POINT_RESET_MASK;
	AT91C_BASE_UDP->UDP_RSTEP  = ( unsigned portLONG ) 0x00;

	/* Enable data to be sent and received. */
	AT91C_BASE_UDP->UDP_FADDR = AT91C_UDP_FEN;

	/* Repair the configuration end point. */
	portENTER_CRITICAL();
	{
		ulTemp = AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ];
		usbCSR_SET_BIT( &ulTemp, ( ( unsigned portLONG ) ( AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_CTRL ) ) );
		AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ] = ulTemp;
		AT91C_BASE_UDP->UDP_IER = AT91C_UDP_EPINT0;
	}
	portEXIT_CRITICAL();
	uiCurrentBank = AT91C_UDP_RX_DATA_BK0;
}
/*------------------------------------------------------------*/

static void prvProcessEndPoint0Interrupt( xISRStatus *pxMessage )
{
static xUSB_REQUEST xRequest;
unsigned portLONG ulRxBytes;

	/* Get number of bytes received, if any */
	ulRxBytes = pxMessage->ulCSR0 >> 16;
	ulRxBytes &= usbRX_COUNT_MASK;

	if( pxMessage->ulCSR0 & AT91C_UDP_TXCOMP )
	{
		/* We received a TX complete interrupt.  What we do depends on
		what we sent to get this interrupt. */

		if( eDriverState == eJUST_GOT_CONFIG )
		{
			/* We sent an acknowledgement of a SET_CONFIG request.  We
			are now at the end of the enumeration.
			
			TODO: Config 0 sets unconfigured state, should enter Address state.
			Request for unsupported config should stall. */
			AT91C_BASE_UDP->UDP_GLBSTATE = AT91C_UDP_CONFG;
			
			/* Set up endpoints */
			portENTER_CRITICAL();
			{
				unsigned portLONG ulTemp;

				/* Set endpoint 1 to bulk-out */
				ulTemp = AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_1 ];					
				usbCSR_SET_BIT( &ulTemp, AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT );
				AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_1 ] = ulTemp;		
				AT91C_BASE_UDP->UDP_IER = AT91C_UDP_EPINT1;
				/* Set endpoint 2 to bulk-in */
				ulTemp = AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_2 ];					
				usbCSR_SET_BIT( &ulTemp, AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN );
				AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_2 ] = ulTemp;		
				AT91C_BASE_UDP->UDP_IER = AT91C_UDP_EPINT2;
					/* Set endpoint 3 to interrupt-in, enable it, and enable interrupts */
				ulTemp = AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_3 ];					
				usbCSR_SET_BIT( &ulTemp, AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_INT_IN );
				AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_3 ] = ulTemp;		
				/*AT91F_UDP_EnableIt( AT91C_BASE_UDP, AT91C_UDP_EPINT3 );				 */
			}
			portEXIT_CRITICAL();

			eDriverState = eREADY_TO_SEND;
		}		
		else if( eDriverState == eJUST_GOT_ADDRESS )
		{
			/* We sent an acknowledgement of a SET_ADDRESS request.  Move
			to the addressed state. */
			if( ulReceivedAddress != ( unsigned portLONG ) 0 )
			{			
				AT91C_BASE_UDP->UDP_GLBSTATE = AT91C_UDP_FADDEN;
			}
			else
			{
				AT91C_BASE_UDP->UDP_GLBSTATE = 0;
			}			

			AT91C_BASE_UDP->UDP_FADDR = ( AT91C_UDP_FEN | ulReceivedAddress );		
			eDriverState = eNOTHING;
		}
		else
		{		
			/* The TXCOMP was not for any special type of transmission.  See
			if there is any more data to send. */
			prvSendNextSegment();
		}
	}

	if( pxMessage->ulCSR0 & AT91C_UDP_RX_DATA_BK0 )
	{
		/* Received a control data packet.  May be a 0-length ACK or a data stage. */
		unsigned portCHAR ucBytesToGet;
	 
		/* Got data.  Cancel any outgoing data. */
		pxControlTx.ulNextCharIndex = pxControlTx.ulTotalDataLength;
		
		 /* Determine how many bytes we need to receive. */
		ucBytesToGet = pxControlRx.ulTotalDataLength - pxControlRx.ulNextCharIndex;
		if( ucBytesToGet > ulRxBytes ) 
		{	
			ucBytesToGet = ulRxBytes;
		}

		/* If we're not expecting any data, it's an ack - just quit now. */
		if( !ucBytesToGet )
		{
			 return;
		}

		/* Get the required data and update the index. */
		memcpy( pxControlRx.ucBuffer, pxMessage->ucFifoData, ucBytesToGet );
		pxControlRx.ulNextCharIndex += ucBytesToGet;	
	}

	if( pxMessage->ulCSR0 & AT91C_UDP_RXSETUP )
	{
		/* Received a SETUP packet.  May be followed by data packets. */

		if( ulRxBytes >= usbEXPECTED_NUMBER_OF_BYTES )
		{		 		
			/* Create an xUSB_REQUEST variable from the raw bytes array. */

			xRequest.ucReqType = pxMessage->ucFifoData[ usbREQUEST_TYPE_INDEX ];
			xRequest.ucRequest = pxMessage->ucFifoData[ usbREQUEST_INDEX ];

			xRequest.usValue = pxMessage->ucFifoData[ usbVALUE_HIGH_BYTE ];
			xRequest.usValue <<= 8;
			xRequest.usValue |= pxMessage->ucFifoData[ usbVALUE_LOW_BYTE ];
						
			xRequest.usIndex = pxMessage->ucFifoData[ usbINDEX_HIGH_BYTE ];
			xRequest.usIndex <<= 8;
			xRequest.usIndex |= pxMessage->ucFifoData[ usbINDEX_LOW_BYTE ];
			
			xRequest.usLength = pxMessage->ucFifoData[ usbLENGTH_HIGH_BYTE ];
			xRequest.usLength <<= 8;
			xRequest.usLength |= pxMessage->ucFifoData[ usbLENGTH_LOW_BYTE ];

			pxControlRx.ulNextCharIndex = 0;
			if( ! (xRequest.ucReqType & 0x80) ) /* Host-to-Device transfer, may need to get data first */
			{
				if( xRequest.usLength > usbMAX_CONTROL_MESSAGE_SIZE )
				{	
					/* Too big!  No space for control data, stall and abort. */
					prvSendStall();
					return;
				}

				pxControlRx.ulTotalDataLength = xRequest.usLength;
			}
			else
			{
				/* We're sending the data, don't wait for any. */
				pxControlRx.ulTotalDataLength = 0; 
			}
		}
	}

	/* See if we've got a pending request and all its associated data ready */
	if( ( pxMessage->ulCSR0 & ( AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RXSETUP ) ) 
		&& ( pxControlRx.ulNextCharIndex >= pxControlRx.ulTotalDataLength ) )
	{
		unsigned portCHAR ucRequest;

		/* Manipulate the ucRequestType and the ucRequest parameters to 
		generate a zero based request selection.  This is just done to 
		break up the requests into subsections for clarity.  The 
		alternative would be to have more huge switch statement that would
		be difficult to optimise. */
		ucRequest = ( ( xRequest.ucReqType & 0x60 ) >> 3 );
		ucRequest |= ( xRequest.ucReqType & 0x03 );
			
		switch( ucRequest )
		{
			case usbSTANDARD_DEVICE_REQUEST:	
				/* Standard Device request */
				prvHandleStandardDeviceRequest( &xRequest );
				break;

			case usbSTANDARD_INTERFACE_REQUEST:	
				/* Standard Interface request */
				prvHandleStandardInterfaceRequest( &xRequest );
				break;

			case usbSTANDARD_END_POINT_REQUEST:	
				/* Standard Endpoint request */
				prvHandleStandardEndPointRequest( &xRequest );
				break;

			case usbCLASS_INTERFACE_REQUEST:	
				/* Class Interface request */
				prvHandleClassInterfaceRequest( &xRequest );
				break;

			default:	/* This is not something we want to respond to. */
				prvSendStall();	
		}
	}
}
/*------------------------------------------------------------*/

static void prvGetStandardDeviceDescriptor( xUSB_REQUEST *pxRequest )
{
	/* The type is in the high byte.  Return whatever has been requested. */
	switch( ( pxRequest->usValue & 0xff00 ) >> 8 )
	{
		case usbDESCRIPTOR_TYPE_DEVICE:
			prvSendControlData( ( unsigned portCHAR * ) &pxDeviceDescriptor, pxRequest->usLength, sizeof( pxDeviceDescriptor ), pdTRUE );
			break;

		case usbDESCRIPTOR_TYPE_CONFIGURATION:
			prvSendControlData( ( unsigned portCHAR * ) &( pxConfigDescriptor ), pxRequest->usLength, sizeof( pxConfigDescriptor ), pdTRUE );
			break;

		case usbDESCRIPTOR_TYPE_STRING:

			/* The index to the string descriptor is the lower byte. */
			switch( pxRequest->usValue & 0xff )
			{			
				case usbLANGUAGE_STRING:
					prvSendControlData( ( unsigned portCHAR * ) &pxLanguageStringDescriptor, pxRequest->usLength, sizeof(pxLanguageStringDescriptor), pdTRUE );
					break;

				case usbMANUFACTURER_STRING:
					prvSendControlData( ( unsigned portCHAR * ) &pxManufacturerStringDescriptor, pxRequest->usLength, sizeof( pxManufacturerStringDescriptor ), pdTRUE );
					break;

				case usbPRODUCT_STRING:
					prvSendControlData( ( unsigned portCHAR * ) &pxProductStringDescriptor, pxRequest->usLength, sizeof( pxProductStringDescriptor ), pdTRUE );
					break;

				case usbCONFIGURATION_STRING:
					prvSendControlData( ( unsigned portCHAR * ) &pxConfigurationStringDescriptor, pxRequest->usLength, sizeof( pxConfigurationStringDescriptor ), pdTRUE );
					break;

				case usbINTERFACE_STRING:
					prvSendControlData( ( unsigned portCHAR * ) &pxInterfaceStringDescriptor, pxRequest->usLength, sizeof( pxInterfaceStringDescriptor ), pdTRUE );
					break;

				default:
					prvSendStall();
					break;
			}
			break;

		default:
			prvSendStall();
			break;
	}
}
/*------------------------------------------------------------*/

static void prvHandleStandardDeviceRequest( xUSB_REQUEST *pxRequest )
{
unsigned portSHORT usStatus = 0;

	switch( pxRequest->ucRequest )
	{
		case usbGET_STATUS_REQUEST:
			/* Just send two byte dummy status. */
			prvSendControlData( ( unsigned portCHAR * ) &usStatus, sizeof( usStatus ), sizeof( usStatus ), pdFALSE );
			break;

		case usbGET_DESCRIPTOR_REQUEST:
			/* Send device descriptor */
			prvGetStandardDeviceDescriptor( pxRequest );
			break;

		case usbGET_CONFIGURATION_REQUEST:
			/* Send selected device configuration */
			prvSendControlData( ( unsigned portCHAR * ) &ucUSBConfig, sizeof( ucUSBConfig ), sizeof( ucUSBConfig ), pdFALSE );
			break;

		case usbSET_FEATURE_REQUEST:
			prvSendZLP(usbEND_POINT_0);
			break;

		case usbSET_ADDRESS_REQUEST:			
			/* Get assigned address and send ack, but don't implement new address until we get a TXCOMP */
			prvSendZLP(usbEND_POINT_0);			
			eDriverState = eJUST_GOT_ADDRESS;			
			ulReceivedAddress = ( unsigned portLONG ) pxRequest->usValue;
			break;

		case usbSET_CONFIGURATION_REQUEST:
			/* Ack SET_CONFIGURATION request, but don't implement until TXCOMP */
			ucUSBConfig = ( unsigned portCHAR ) ( pxRequest->usValue & 0xff );
			eDriverState = eJUST_GOT_CONFIG;
			prvSendZLP(usbEND_POINT_0);
			break;

		default:
			/* Any unsupported request results in a STALL response. */
			prvSendStall();
			break;
	}
}
/*------------------------------------------------------------*/

static void prvHandleClassInterfaceRequest( xUSB_REQUEST *pxRequest )
{
	switch( pxRequest->ucRequest )
	{
		case usbSEND_ENCAPSULATED_COMMAND:
			prvSendStall();
			break;

		case usbGET_ENCAPSULATED_RESPONSE:
			prvSendStall();
			break;

		case usbSET_LINE_CODING:
			/* Set line coding - baud rate, data bits, parity, stop bits */
			prvSendZLP(usbEND_POINT_0);
			memcpy( ( void * ) pxLineCoding, pxControlRx.ucBuffer, sizeof( pxLineCoding ) );
			break;

		case usbGET_LINE_CODING:
			/* Get line coding */
			prvSendControlData( (unsigned portCHAR *) &pxLineCoding, pxRequest->usLength, sizeof( pxLineCoding ), pdFALSE );
			break;

		case usbSET_CONTROL_LINE_STATE:
			/* D0: 1=DTR, 0=No DTR,  D1: 1=Activate Carrier, 0=Deactivate carrier (RTS, half-duplex) */
			prvSendZLP(usbEND_POINT_0);
			ucControlState = pxRequest->usValue;
			break;

		default:
			prvSendStall();
			break;
	}
}
/*------------------------------------------------------------*/

static void prvGetStandardInterfaceDescriptor( xUSB_REQUEST *pxRequest )
{
	switch( ( pxRequest->usValue & ( unsigned portSHORT ) 0xff00 ) >> 8 )
	{
		default:
			prvSendStall();
			break;
	}
}
/*-----------------------------------------------------------*/

static void prvHandleStandardInterfaceRequest( xUSB_REQUEST *pxRequest )
{
unsigned portSHORT usStatus = 0;

	switch( pxRequest->ucRequest )
	{
		case usbGET_STATUS_REQUEST:
			/* Send dummy 2 bytes. */
			prvSendControlData( ( unsigned portCHAR * ) &usStatus, sizeof( usStatus ), sizeof( usStatus ), pdFALSE );
			break;

		case usbGET_DESCRIPTOR_REQUEST:
			prvGetStandardInterfaceDescriptor( pxRequest ); 
			break;

		/* This minimal implementation does not respond to these. */
		case usbGET_INTERFACE_REQUEST:
		case usbSET_FEATURE_REQUEST:
		case usbSET_INTERFACE_REQUEST:	

		default:
			prvSendStall();
			break;
	}
}
/*-----------------------------------------------------------*/

static void prvHandleStandardEndPointRequest( xUSB_REQUEST *pxRequest )
{
	switch( pxRequest->ucRequest )
	{
		/* This minimal implementation does not expect to respond to these. */
		case usbGET_STATUS_REQUEST:
		case usbCLEAR_FEATURE_REQUEST: 
		case usbSET_FEATURE_REQUEST:

		default:			
			prvSendStall();
			break;
	}
}
/*-----------------------------------------------------------*/

static void vDetachUSBInterface( void)
{
	/* Setup the PIO for the USB pull up resistor. */

  // MakingThings: PA11, now for Pullup
	/* Start without the pullup - this will get set at the end of this 
	function. */
	#if ( CONTROLLER_VERSION == 90 )
		AT91C_BASE_PIOB->PIO_PER = AT91C_PIO_PB11;
    AT91C_BASE_PIOB->PIO_OER = AT91C_PIO_PB11;
    AT91C_BASE_PIOB->PIO_SODR = AT91C_PIO_PB11;
	#elif ( CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 )
	  AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA11;
	  AT91C_BASE_PIOA->PIO_OER = AT91C_PIO_PA11;
	  /* Disable pull up */
	  AT91C_BASE_PIOA->PIO_CODR = AT91C_PIO_PA11;
  #elif ( CONTROLLER_VERSION == 200 )
	  AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA30;
	  AT91C_BASE_PIOA->PIO_OER = AT91C_PIO_PA30;
	  /* Disable pull up */
	  AT91C_BASE_PIOA->PIO_CODR = AT91C_PIO_PA30;
	#endif
} 
/*-----------------------------------------------------------*/

static void vInitUSBInterface( void )
{
extern void ( vUSB_ISR_Wrapper )( void );

	/* Create the queue used to communicate between the USB ISR and task. */
	xUSBInterruptQueue = xQueueCreate( usbQUEUE_LENGTH + 1, sizeof( xISRStatus * ) );
	
	/* Create the queues used to hold Rx and Tx characters. */
	xRxCDC = xQueueCreate( USB_CDC_QUEUE_SIZE, ( unsigned portCHAR ) sizeof(  struct BulkBufferStruct ) );
	xTxCDC = xQueueCreate( USB_CDC_QUEUE_SIZE + 1, ( unsigned portCHAR ) sizeof(  struct BulkBufferStruct  ) );

	if( (!xUSBInterruptQueue) || (!xRxCDC) || (!xTxCDC) )
	{	
		/* Not enough RAM to create queues!. */
		return;
	}
	
	/* Initialise a few state variables. */
	pxControlTx.ulNextCharIndex = ( unsigned portLONG ) 0;
	pxControlRx.ulNextCharIndex = ( unsigned portLONG ) 0;
	ucUSBConfig = ( unsigned portCHAR ) 0;
	eDriverState = eNOTHING;
	ucControlState = 0;
	uiCurrentBank = AT91C_UDP_RX_DATA_BK0;


	/* HARDWARE SETUP */

	/* Set the PLL USB Divider */
	AT91C_BASE_CKGR->CKGR_PLLR |= AT91C_CKGR_USBDIV_1;

	/* Enables the 48MHz USB clock UDPCK and System Peripheral USB Clock. */
	AT91C_BASE_PMC->PMC_SCER = AT91C_PMC_UDP;
	AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_UDP);

  /* MAKINGTHINGS: ADDITION */
  /* Setup the PIO for USB input */
	#if ( CONTROLLER_VERSION == 90 )
		AT91C_BASE_PIOB->PIO_PER = AT91C_PIO_PB10;
    AT91C_BASE_PIOB->PIO_ODR = AT91C_PIO_PB10;
	#elif ( CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 )
		AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA10;
		AT91C_BASE_PIOA->PIO_ODR = AT91C_PIO_PA10;
  #elif ( CONTROLLER_VERSION == 200 )
		AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA29;
		AT91C_BASE_PIOA->PIO_ODR = AT91C_PIO_PA29;
	#endif


	/* Start without the pullup - this will get set at the end of this 
	function. */
	#if ( CONTROLLER_VERSION == 90 )
		AT91C_BASE_PIOB->PIO_PER = AT91C_PIO_PB11;
    AT91C_BASE_PIOB->PIO_OER = AT91C_PIO_PB11;
    AT91C_BASE_PIOB->PIO_SODR = AT91C_PIO_PB11;
	#elif ( CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 )
		AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA11;
		AT91C_BASE_PIOA->PIO_OER = AT91C_PIO_PA11;
    AT91C_BASE_PIOA->PIO_CODR = AT91C_PIO_PA11;
  #elif ( CONTROLLER_VERSION == 200 )
		AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA30;
		AT91C_BASE_PIOA->PIO_OER = AT91C_PIO_PA30;
    AT91C_BASE_PIOA->PIO_CODR = AT91C_PIO_PA30;
	#endif

	/* When using the USB debugger the peripheral registers do not always get
	set to the correct default values.  To make sure set the relevant registers
	manually here. */
	AT91C_BASE_UDP->UDP_IDR = ( unsigned portLONG ) 0xffffffff;
	AT91C_BASE_UDP->UDP_ICR = ( unsigned portLONG ) 0xffffffff;
	AT91C_BASE_UDP->UDP_CSR[ 0 ] = ( unsigned portLONG ) 0x00;
	AT91C_BASE_UDP->UDP_CSR[ 1 ] = ( unsigned portLONG ) 0x00;
	AT91C_BASE_UDP->UDP_CSR[ 2 ] = ( unsigned portLONG ) 0x00;
	AT91C_BASE_UDP->UDP_CSR[ 3 ] = ( unsigned portLONG ) 0x00;
	AT91C_BASE_UDP->UDP_GLBSTATE = 0;
	AT91C_BASE_UDP->UDP_FADDR = 0;

	/* Enable the transceiver. */
	AT91C_UDP_TRANSCEIVER_ENABLE = 0;

	/* Enable the USB interrupts - other interrupts get enabled as the 
	enumeration process progresses. */
	AT91F_AIC_ConfigureIt( AT91C_ID_UDP, usbINTERRUPT_PRIORITY, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, ( void (*)( void ) ) vUSB_ISR_Wrapper );
	AT91C_BASE_AIC->AIC_IECR = 0x1 << AT91C_ID_UDP;


	/* Wait a short while before making our presence known. */
	vTaskDelay( usbINIT_DELAY );
  
  /* MAKINGTHINGS: ADDITION */
	#if ( CONTROLLER_VERSION == 90 )
    AT91C_BASE_PIOB->PIO_CODR = AT91C_PIO_PB11;
  #elif( CONTROLLER_VERSION == 95 || CONTROLLER_VERSION == 100 )
    AT91C_BASE_PIOA->PIO_SODR = AT91C_PIO_PA11;
  #elif( CONTROLLER_VERSION == 200 )
    AT91C_BASE_PIOA->PIO_SODR = AT91C_PIO_PA30;
  #endif
}
/*-----------------------------------------------------------*/

static void prvSendControlData( unsigned portCHAR *pucData, unsigned portSHORT usRequestedLength, unsigned portLONG ulLengthToSend, portLONG lSendingDescriptor )
{
	if( ( ( unsigned portLONG ) usRequestedLength < ulLengthToSend ) )
	{
		/* Cap the data length to that requested. */
		ulLengthToSend = ( unsigned portSHORT ) usRequestedLength;
	}
	else if( ( ulLengthToSend < ( unsigned portLONG ) usRequestedLength ) && lSendingDescriptor )
	{
		/* We are sending a descriptor.  If the descriptor is an exact 
		multiple of the FIFO length then it will have to be terminated
		with a NULL packet.  Set the state to indicate this if
		necessary. */
		if( ( ulLengthToSend % usbFIFO_LENGTH ) == 0 )
		{
			eDriverState = eSENDING_EVEN_DESCRIPTOR;
		}
	}

	/* Here we assume that the previous message has been sent.  THERE IS NO
	BUFFER OVERFLOW PROTECTION HERE.

	Copy the data to send into the buffer as we cannot send it all at once
	(if it is greater than 8 bytes in length). */
	memcpy( pxControlTx.ucBuffer, pucData, ulLengthToSend );

	/* Reinitialise the buffer index so we start sending from the start of 
	the data. */
	pxControlTx.ulTotalDataLength = ulLengthToSend;
	pxControlTx.ulNextCharIndex = ( unsigned portLONG ) 0;

	/* Send the first 8 bytes now.  The rest will get sent in response to 
	TXCOMP interrupts. */
	prvSendNextSegment();
}
/*-----------------------------------------------------------*/

static void prvSendNextSegment( void )
{
volatile unsigned portLONG ulNextLength, ulStatus, ulLengthLeftToSend;

	/* Is there any data to send? */
	if( pxControlTx.ulTotalDataLength > pxControlTx.ulNextCharIndex )
	{
		ulLengthLeftToSend = pxControlTx.ulTotalDataLength - pxControlTx.ulNextCharIndex;
	
		/* We can only send 8 bytes to the fifo at a time. */
		if( ulLengthLeftToSend > usbFIFO_LENGTH )
		{
			ulNextLength = usbFIFO_LENGTH;
		}
		else
		{
			ulNextLength = ulLengthLeftToSend;
		}

		/* Wait until we can place data in the fifo.  THERE IS NO TIMEOUT
		HERE! */
		while( AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ] & AT91C_UDP_TXPKTRDY )
		{
			vTaskDelay( usbSHORTEST_DELAY );
		}

		/* Write the data to the FIFO. */
		while( ulNextLength > ( unsigned portLONG ) 0 )
		{
			AT91C_BASE_UDP->UDP_FDR[ usbEND_POINT_0 ] = pxControlTx.ucBuffer[ pxControlTx.ulNextCharIndex ];
	
			ulNextLength--;
			pxControlTx.ulNextCharIndex++;
		}
	
		/* Start the transmission. */
		portENTER_CRITICAL();
		{
			ulStatus = AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ];
			usbCSR_SET_BIT( &ulStatus, ( ( unsigned portLONG ) 0x10 ) );
			AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ] = ulStatus;
		}
		portEXIT_CRITICAL();
	}
	else
	{
		/* There is no data to send.  If we were sending a descriptor and the 
		descriptor was an exact multiple of the max packet 
     then we need
		to send a null to terminate the transmission. */
		if( eDriverState == eSENDING_EVEN_DESCRIPTOR )
		{
			prvSendZLP(usbEND_POINT_0);
			eDriverState = eNOTHING;
		}
	}
}

#endif // MAKE_CTRL_USB


