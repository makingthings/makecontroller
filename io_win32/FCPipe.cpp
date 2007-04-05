/******************************************************************************
*
*      Company                   :  CIRIEL for ATMEL
*      Service Line              :  Product Devlopment
*      Project                   :  FingerChip
*      Task                      :  
*
*      Soft name                 :  FCPipe.cpp
*      Soft version              :  1.00
*
*      Description               :  This file contain the definition of the
*                                   virtual CFCPipe class 
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

#include "stdafx.h"

#include "FCPipe.h"

#include "FC_Error.h"

#define SIZE_OF_DATA_SEND 250
#define BUFMAX 256
#define SIZEOFBUF 64


///////////////////////////////////////////////////////////////////////////////
ULONG CFCPipe::GetWriteCount()
{
  return m_WriteCount;
}

///////////////////////////////////////////////////////////////////////////////
void CFCPipe::ResetWriteCount()
{
  m_WriteCount = 0;
}

///////////////////////////////////////////////////////////////////////////////
ULONG CFCPipe::GetReadCount()
{
  return m_ReadCount;
}

///////////////////////////////////////////////////////////////////////////////
void CFCPipe::ResetReadCount()
{
  m_ReadCount = 0;
}

///////////////////////////////////////////////////////////////////////////////
short CFCPipe::Read(LPVOID pBuffer, ULONG ulBufferSize)
{
  short ret = ReadPipe(pBuffer, ulBufferSize);

  if(ret == FC_OK)
    m_ReadCount += ulBufferSize;

  return ret;
};

///////////////////////////////////////////////////////////////////////////////
short CFCPipe::Write(LPVOID pBuffer, ULONG ulBufferSize)
{
  short ret = WritePipe(pBuffer, ulBufferSize);

  if(ret == FC_OK)
    m_WriteCount += ulBufferSize;

  return ret;
};


