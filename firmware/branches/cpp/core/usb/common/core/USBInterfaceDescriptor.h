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
    Title: USBInterfaceDescriptor

    About: Purpose
        Definition of a class for manipulating USB interface descriptors.
*/

#ifndef USBINTERFACEDESCRIPTOR_H
#define USBINTERFACEDESCRIPTOR_H

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------

#ifdef __ICCARM__          // IAR
#pragma pack(1)            // IAR
#define __attribute__(...) // IAR
#endif                     // IAR

/*
    Type: USBInterfaceDescriptor
        USB standard interface descriptor structure.

    Variables:
        bLength - Size of the descriptor in bytes.
        bDescriptorType - Descriptor type (<USBDESC_INTERFACE>).
        bInterfaceNumber - Number of the interface in its configuration.
        bAlternateSetting - Value to select this alternate interface setting.
        bNumEndpoints - Number of endpoints used by the inteface (excluding
                        endpoint 0).
        bInterfaceClass - Interface class code.
        bInterfaceSubClass - Interface subclass code.
        bInterfaceProtocol - Interface protocol code.
        iInterface - Index of the interface string descriptor.
*/
typedef struct {

   unsigned char bLength;           
   unsigned char bDescriptorType;   
   unsigned char bInterfaceNumber;  
   unsigned char bAlternateSetting; 
   unsigned char bNumEndpoints;     
   unsigned char bInterfaceClass;   
   unsigned char bInterfaceSubClass;
   unsigned char bInterfaceProtocol;
   unsigned char iInterface;        
                                    
} __attribute__ ((packed)) USBInterfaceDescriptor; // GCC

#ifdef __ICCARM__          // IAR
#pragma pack()             // IAR
#endif                     // IAR

#endif //#ifndef USBINTERFACEDESCRIPTOR_H

