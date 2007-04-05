/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    GUID829.h

Abstract:

 The below GUID is used to generate symbolic links to
  driver instances created from user mode

Environment:

    Kernel & user mode

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1997-1998 Microsoft Corporation.  All Rights Reserved.

Revision History:

    11/18/97 : created

--*/
#ifndef GUID829H_INC
#define GUID829H_INC

#include <initguid.h>


// {E6EF7DCD-1795-4a08-9FBF-AA78423C26F0}
DEFINE_GUID(GUID_CLASS_I82930_BULK, 
0xe6ef7dcd, 0x1795, 0x4a08, 0x9f, 0xbf, 0xaa, 0x78, 0x42, 0x3c, 0x26, 0xf0);


#endif // end, #ifndef GUID829H_INC

