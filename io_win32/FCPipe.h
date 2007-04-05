/******************************************************************************
*
*      Company                   :  CIRIEL for ATMEL
*      Service Line              :  Product Devlopment
*      Project                   :  FingerChip
*      Task                      :  
*
*      Soft name                 :  FCPipe.h
*      Soft version              :  1.00 
*
*      Description               :  interface of the CFCPipe class
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

#ifndef _INCLUDE_FINGERCHIP_PIPE_HEADER_
#define _INCLUDE_FINGERCHIP_PIPE_HEADER_

#include "WTypes.h"


class CFCPipe
{
    ULONG m_WriteCount;
    ULONG m_ReadCount;

  public :

    ULONG GetWriteCount();
    void ResetWriteCount();

    ULONG GetReadCount();
    void ResetReadCount();

    short Read(LPVOID pBuffer, ULONG ulBufferSize);
    short Write(LPVOID pBuffer, ULONG ulBufferSize);

    virtual short ReadPipe(LPVOID pBuffer, ULONG ulBufferSize) = 0;
    virtual short WritePipe(LPVOID pBuffer, ULONG ulBufferSize) = 0;

    virtual short FlushOut() = 0;
};

#endif
