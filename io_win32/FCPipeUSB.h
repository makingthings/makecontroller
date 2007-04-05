/******************************************************************************
*
*      Company                   :  CIRIEL for ATMEL
*      Service Line              :  Product Devlopment
*      Project                   :  FingerChip
*      Task                      :  
*
*      Soft name                 :  FCPipeUSB.h
*      Soft version              :  1.00 
*
*      Description               :  interface of the CFCPipeUSB class
*                  
*      Authors                   :  Pascal E
*
*      Platform                  :  PC
*      System                    :  Win 9X, 2K
*
*      Programming language      :  C ++
*      Operating System          :  Windows
*      Date of creation          :  08/30/2002
*
*      Prefix                    :
*      Code size (.o) (KB)       :
*
*      References                :
*
*      History                   :
*
*     Copyright (c) 
*     All rights reserved
*
******************************************************************************/

#ifndef _INCLUDE_FINGERCHIP_PIPE_USB_HEADER_
#define _INCLUDE_FINGERCHIP_PIPE_USB_HEADER_

#include <windows.h>
#include <setupapi.h>
#include <basetyps.h>

#include "FCPipe.h"

class CFCPipeUSB : public CFCPipe
{
    HANDLE m_hPipeIn;
    HANDLE m_hPipeOut;

    BOOL GetUsbDeviceFileName( LPGUID  pGuid, char **outNameBuf);

    HANDLE OpenUsbDevice(LPGUID  pGuid, char **outNameBuf);
    HANDLE OpenOneDevice (HDEVINFO HardwareDeviceInfo,
                          PSP_INTERFACE_DEVICE_DATA DeviceInfoData,
	                        char **devName);

  public :

    CFCPipeUSB();

    short Open();
    short Close();

	short IsDriverThere();

    virtual short ReadPipe(LPVOID pBuffer, ULONG ulBufferSize);  
    virtual short WritePipe(LPVOID pBuffer, ULONG ulBufferSize);  

    virtual short FlushOut();
	virtual short FlushIn();
};

#endif
