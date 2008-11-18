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
    Title: USBDeviceQualifierDescriptor

    About: Purpose
        Class for manipulating USB device qualifier descriptors.
*/

#ifndef USBDEVICEQUALIFIERDESCRIPTOR_H
#define USBDEVICEQUALIFIERDESCRIPTOR_H

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------

#ifdef __ICCARM__          // IAR
#pragma pack(1)            // IAR
#define __attribute__(...) // IAR
#endif                     // IAR

/*
    Type: USBDeviceQualifierDescriptor
        Alternate device descriptor indicating the capabilities of the device
        in full-speed, if currently in high-speed; or in high-speed, if it is
        currently in full-speed. Only relevant for devices supporting the
        high-speed mode.

    Variables:
        bLength - Size of the descriptor in bytes.
        bDescriptorType - Descriptor type (<USBDESC_DEVICE_QUALIFIER>).
        bcdUSB - USB specification release number (in BCD format).
        bDeviceClass - Device class code.
        bDeviceSubClass - Device subclass code.
        bDeviceProtocol - Device protocol code.
        bMaxPacketSize0 - Maximum packet size of endpoint 0.
        bNumConfigurations - Number of possible configurations for the device.
        bReserved - Reserved.
*/
typedef struct {

   unsigned char bLength;           
   unsigned char bDescriptorType;   
   unsigned short bcdUSB;            
   unsigned char bDeviceClass;      
   unsigned char bDeviceSubClass;   
   unsigned char bDeviceProtocol;   
   unsigned char bMaxPacketSize0;   
   unsigned char bNumConfigurations;
   unsigned char bReserved;         

} __attribute__ ((packed)) USBDeviceQualifierDescriptor; // GCC

#ifdef __ICCARM__          // IAR
#pragma pack()             // IAR
#endif                     // IAR

#endif //#ifndef USBDEVICEQUALIFIERDESCRIPTOR_H

