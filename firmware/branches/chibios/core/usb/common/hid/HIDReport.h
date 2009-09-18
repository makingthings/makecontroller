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
    Title: HIDReport

    About: Purpose
        Definitions used when declaring an HID report descriptor.

    About: Usage
        Use the definitions provided here when declaring a report descriptor,
        which shall be an unsigned char array.
*/

#ifndef HIDREPORT_H
#define HIDREPORT_H

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: Main items
        HIDReport_INPUT - Input item.
        HIDReport_OUPUT - Output item.
        HIDReport_FEATURE - Feature item.
        HIDReport_COLLECTION - Collection item.
        HIDReport_ENDCOLLECTION - End of collection item.
*/
#define HIDReport_INPUT                         0x80
#define HIDReport_OUTPUT                        0x90
#define HIDReport_FEATURE                       0xB0
#define HIDReport_COLLECTION                    0xA0
#define HIDReport_ENDCOLLECTION                 0xC0
                                                
/*
    Constants: Input, output and feature items
        HIDReport_CONSTANT - The report value is constant (vs. variable).
        HIDReport_VARIABLE - Data reported is a variable (vs. array).
        HIDReport_RELATIVE - Data is relative (vs. absolute).
        HIDReport_WRAP - Value rolls over when it reach a maximum/minimum.
        HIDReport_NONLINEAR - Indicates that the data reported has been
            processed and is no longuer linear with the original measurements.
        HIDReport_NOPREFERRED - Device has no preferred state to which it
            automatically returns.
        HIDReport_NULLSTATE - Device has a null state, in which it does not
            report meaningful information.
        HIDReport_VOLATILE - Indicates data can change without the host
            intervention.
        HIDReport_BUFFEREDBYTES - Indicates the device produces a fixed-length
            stream of bytes.
*/
#define HIDReport_CONSTANT                      (1 << 0)
#define HIDReport_VARIABLE                      (1 << 1)
#define HIDReport_RELATIVE                      (1 << 2)
#define HIDReport_WRAP                          (1 << 3)
#define HIDReport_NONLINEAR                     (1 << 4)
#define HIDReport_NOPREFERRED                   (1 << 5)
#define HIDReport_NULLSTATE                     (1 << 6)
#define HIDReport_VOLATILE                      (1 << 7)
#define HIDReport_BUFFEREDBYTES                 (1 << 8)

/*
    Constants: Collection items
        HIDReport_COLLECTION_PHYSICAL - Physical collection.
        HIDReport_COLLECTION_APPLICATION - Application collection.
        HIDReport_COLLECTION_LOGICAL - Logical collection.
        HIDReport_COLLECTION_REPORT - Report collection.
        HIDReport_COLLECTION_NAMEDARRAY - Named array collection.
        HIDReport_COLLECTION_USAGESWITCH - Usage switch collection.
        HIDReport_COLLECTION_USAGEMODIFIER - Usage modifier collection
*/
#define HIDReport_COLLECTION_PHYSICAL           0x00
#define HIDReport_COLLECTION_APPLICATION        0x01
#define HIDReport_COLLECTION_LOGICAL            0x02
#define HIDReport_COLLECTION_REPORT             0x03
#define HIDReport_COLLECTION_NAMEDARRAY         0x04
#define HIDReport_COLLECTION_USAGESWITCH        0x05
#define HIDReport_COLLECTION_USAGEMODIFIER      0x06

/*
    Constants: Global items
        HIDReport_GLOBAL_USAGEPAGE - Current usage page.
        HIDReport_GLOBAL_LOGICALMINIMUM - Minimum value that a variable or array
            item will report.
        HIDReport_GLOBAL_LOGICALMAXIMUM - Maximum value that a variable or array
            item will report.
        HIDReport_GLOBAL_PHYSICALMINIMUM - Minimum value for the physical extent
            of a variable item.
        HIDReport_GLOBAL_PHYSICALMAXIMUM - Maximum value for the physical extent
            of a variable item.
        HIDReport_GLOBAL_UNITEXPONENT - Value of the unit exponent in base 10.
        HIDReport_GLOBAL_UNIT - Unit values.
        HIDReport_GLOBAL_REPORTSIZE - Size of the report fields in bits.
        HIDReport_GLOBAL_REPORTID - Specifies the report ID.
        HIDReport_GLOBAL_REPORTCOUNT - Number of data fields for an item.
        HIDReport_GLOBAL_PUSH - Places a copy of the global item state table on
            the stack.
        HIDReport_GLOBAL_POP - Replaces the item state table with the top
            structure from the stack.
*/
#define HIDReport_GLOBAL_USAGEPAGE              0x04
#define HIDReport_GLOBAL_LOGICALMINIMUM         0x14
#define HIDReport_GLOBAL_LOGICALMAXIMUM         0x24
#define HIDReport_GLOBAL_PHYSICALMINIMUM        0x34
#define HIDReport_GLOBAL_PHYSICALMAXIMUM        0x44
#define HIDReport_GLOBAL_UNITEXPONENT           0x54
#define HIDReport_GLOBAL_UNIT                   0x64
#define HIDReport_GLOBAL_REPORTSIZE             0x74
#define HIDReport_GLOBAL_REPORTID               0x84
#define HIDReport_GLOBAL_REPORTCOUNT            0x94
#define HIDReport_GLOBAL_PUSH                   0xA4
#define HIDReport_GLOBAL_POP                    0xB4

/*
    Constants: Local items
        HIDReport_LOCAL_USAGE - Suggested usage for an item or collection.
        HIDReport_LOCAL_USAGEMINIMUM - Defines the starting usage associated
            with an array or bitmap.
        HIDReport_LOCAL_USAGEMAXIMUM - Defines the ending usage associated with
            an array or bitmap.
        HIDReport_LOCAL_DESIGNATORINDEX - Determines the body part used for a
            control.
        HIDReport_LOCAL_DESIGNATORMINIMUM - Defines the index of the starting
            designator associated with an array or bitmap.
        HIDReport_LOCAL_DESIGNATORMAXIMUM - Defines the index of the ending
            designator associated with an array or bitmap.
        HIDReport_LOCAL_STRINGINDEX - String index for a string descriptor.
        HIDReport_LOCAL_STRINGMINIMUM - Specifies the first string index when
            assigning a group of sequential strings to controls in an array or
            bitmap.
        HIDReport_LOCAL_STRINGMAXIMUM - Specifies the last string index when
            assigning a group of sequential strings to controls in an array or
            bitmap.
        HIDReport_LOCAL_DELIMITER - Defines the beginning or end of a set of
            local items.
*/
#define HIDReport_LOCAL_USAGE                   0x08
#define HIDReport_LOCAL_USAGEMINIMUM            0x18
#define HIDReport_LOCAL_USAGEMAXIMUM            0x28
#define HIDReport_LOCAL_DESIGNATORINDEX         0x38
#define HIDReport_LOCAL_DESIGNATORMINIMUM       0x48
#define HIDReport_LOCAL_DESIGNATORMAXIMUM       0x58
#define HIDReport_LOCAL_STRINGINDEX             0x78
#define HIDReport_LOCAL_STRINGMINIMUM           0x88
#define HIDReport_LOCAL_STRINGMAXIMUM           0x98
#define HIDReport_LOCAL_DELIMITER               0xA8

#endif //#ifndef HIDREPORT_H

