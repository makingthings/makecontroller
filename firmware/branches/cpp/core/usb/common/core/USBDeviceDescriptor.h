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

/**
 *  \page USBDeviceDescriptor
 *  !!!Purpose
 * 
 *      Class for manipulating USB device descriptors.
 * 
 *  !!!Usage
 * 
 *      -# Test
 */

#ifndef USBDEVICEDESCRIPTOR_H
#define USBDEVICEDESCRIPTOR_H

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/**
    Constants: USB release numbers
        USBDeviceDescriptor_USB2_00 - The device supports USB 2.00.
*/
#define USBDeviceDescriptor_USB2_00         0x0200

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------

#ifdef __ICCARM__          // IAR
#pragma pack(1)            // IAR
#define __attribute__(...) // IAR
#endif                     // IAR

/*
    Type: USBDeviceDescriptor
        USB standard device descriptor structure.

    Variables:
        bLength - Size of this descriptor in bytes.
        bDescriptorType - Descriptor type (<USBGenericDescriptor_DEVICE>).
        bcdUSB - USB specification release number in BCD format.
        bDeviceClass - Device class code.
        bDeviceSubClass - Device subclass code.
        bDeviceProtocol - Device protocol code.
        bMaxPacketSize0 - Maximum packet size of endpoint 0 (in bytes).
        idVendor - Vendor ID.
        idProduct - Product ID.
        bcdDevice - Device release number in BCD format.
        iManufacturer - Index of the manufacturer string descriptor.
        iProduct - Index of the product string descriptor.
        iSerialNumber - Index of the serial number string descriptor.
        bNumConfigurations - Number of possible configurations for the device.
*/
typedef struct {

   unsigned char bLength;           
   unsigned char bDescriptorType;   
   unsigned short bcdUSB;            
   unsigned char bDeviceClass;      
   unsigned char bDeviceSubClass;   
   unsigned char bDeviceProtocol;   
   unsigned char bMaxPacketSize0;   
   unsigned short idVendor;          
   unsigned short idProduct;         
   unsigned short bcdDevice;         
   unsigned char iManufacturer;     
   unsigned char iProduct;          
   unsigned char iSerialNumber;     
   unsigned char bNumConfigurations;

} __attribute__ ((packed)) USBDeviceDescriptor; // GCC

#ifdef __ICCARM__          // IAR
#pragma pack()             // IAR
#endif                     // IAR

#endif //#ifndef USBDEVICEDESCRIPTOR_H

