/******************************************************************************
*
*      Company                   :  CIRIEL for ATMEL
*      Service Line              :  Product Devlopment
*      Project                   :  FingerChip
*      Task                      :  
*
*      Soft name                 :  FCPipeUSB.cpp
*      Soft version              :  1.00
*
*      Description               :  This file contain the definition of the
*                                   CFCPipeUSB class 
*                  
*      Authors                   :  Pascal E
*
*      Platform                  :  PC
*      System                    :  Win 9X, 2K
*
*      Programming language      :  C ++
*      Operating System          :  Windows
*      Date of creation          :  09/16/2002
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

#include "FCPipeUSB.h"

#include <stdlib.h>

#include "GUID829.h"

#include "FC_Error.h"


///////////////////////////////////////////////////////////////////////////////
CFCPipeUSB::CFCPipeUSB()
{
  m_hPipeIn = INVALID_HANDLE_VALUE;
  m_hPipeOut = INVALID_HANDLE_VALUE;
}

///////////////////////////////////////////////////////////////////////////////
short CFCPipeUSB::IsDriverThere()
{
	char *sDeviceName;

	if(! GetUsbDeviceFileName( (LPGUID) &GUID_CLASS_I82930_BULK, &sDeviceName))
		return FC_DRIVER_NOT_FOUND;
	free(sDeviceName);
	return FC_OK;

}

///////////////////////////////////////////////////////////////////////////////
short CFCPipeUSB::Open()
{

  char *sDeviceName;

  char *sPipeNameIn;
  char *sPipeNameOut;

  if(! GetUsbDeviceFileName( (LPGUID) &GUID_CLASS_I82930_BULK, &sDeviceName))
    return FC_DRIVER_NOT_FOUND;

  sPipeNameIn  = (char *) malloc(strlen(sDeviceName) + 10);
  sPipeNameOut = (char *) malloc(strlen(sDeviceName) + 10);
  ::strcpy(sPipeNameIn, sDeviceName);
  ::strcat(sPipeNameIn, "\\PIPE01");

  ::strcpy(sPipeNameOut, sDeviceName);
  ::strcat(sPipeNameOut, "\\PIPE00");
  free(sDeviceName);

  m_hPipeIn = ::CreateFile(sPipeNameIn, 
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ,
						   NULL,
						   OPEN_EXISTING, 
                           0, 
						   NULL);

  m_hPipeOut = ::CreateFile(sPipeNameOut, 
                            GENERIC_WRITE,
                            FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING, 
                            0,
							NULL);


  if(    (m_hPipeIn == INVALID_HANDLE_VALUE)
      || (m_hPipeOut == INVALID_HANDLE_VALUE))
  {
    Close();
	int i =GetLastError();
    return i;
  }

  return FC_OK;
}

///////////////////////////////////////////////////////////////////////////////
short CFCPipeUSB::Close()
{
  if(m_hPipeIn != INVALID_HANDLE_VALUE)
    ::CloseHandle(m_hPipeIn);
  if(m_hPipeOut != INVALID_HANDLE_VALUE)
    ::CloseHandle(m_hPipeOut);

  m_hPipeIn = INVALID_HANDLE_VALUE;
  m_hPipeOut = INVALID_HANDLE_VALUE;;

  return FC_OK;
}

///////////////////////////////////////////////////////////////////////////////
short CFCPipeUSB::ReadPipe(LPVOID pBuffer, ULONG ulBufferSize)
{
	int timeout = 0;
	DWORD oldValue = 0;
  if(m_hPipeIn == INVALID_HANDLE_VALUE)
    return FC_ERROR;

  if(!pBuffer || !ulBufferSize)
    return FC_NOT_INITIALIZED;

  DWORD dwBytesRead;

  DWORD dwBytesToRead = ulBufferSize;
  DWORD dwOffset = 0;
  oldValue = dwBytesToRead;

  do
  {
    if( ! ::ReadFile(m_hPipeIn, ((BYTE *) pBuffer) + dwOffset, dwBytesToRead, &dwBytesRead, NULL))
    {
      DWORD dwErr = ::GetLastError();
      return FC_DRIVER_ERROR;
    }

    dwBytesToRead -= dwBytesRead;
    dwOffset += dwBytesRead;
	if (dwBytesRead == 0)
		return FC_DRIVER_ERROR;
  } while((dwBytesToRead != 0) && (timeout != 5));

  
	if (dwBytesToRead != 0)
		return FC_DRIVER_ERROR;

  return FC_OK;
}

///////////////////////////////////////////////////////////////////////////////
short CFCPipeUSB::WritePipe(LPVOID pBuffer, ULONG ulBufferSize)
{
  if(m_hPipeOut == INVALID_HANDLE_VALUE)
    return FC_ERROR;

  if(!pBuffer || !ulBufferSize)
    return FC_NOT_INITIALIZED;

  DWORD dwBytesWritten;

  if(! ::WriteFile(m_hPipeOut, pBuffer, ulBufferSize, &dwBytesWritten, NULL))
  {
    DWORD dwErr = ::GetLastError();
    return FC_DRIVER_ERROR;
  }

  return FC_OK;
}

///////////////////////////////////////////////////////////////////////////////
short CFCPipeUSB::FlushOut()
{
  if(m_hPipeOut == INVALID_HANDLE_VALUE)
    return FC_ERROR;

  ::FlushFileBuffers(m_hPipeOut);
  return FC_OK;
}

///////////////////////////////////////////////////////////////////////////////
short CFCPipeUSB::FlushIn()
{
  if(m_hPipeIn == INVALID_HANDLE_VALUE)
    return FC_ERROR;

  ::FlushFileBuffers(m_hPipeIn);
  return FC_OK;
}

///////////////////////////////////////////////////////////////////////////////
/*++
Routine Description:

    Given the HardwareDeviceInfo, representing a handle to the plug and
    play information, and deviceInfoData, representing a specific usb device,
    open that device and fill in all the relevant information in the given
    USB_DEVICE_DESCRIPTOR structure.

Arguments:

    HardwareDeviceInfo:  handle to info obtained from Pnp mgr via SetupDiGetClassDevs()
    DeviceInfoData:      ptr to info obtained via SetupDiEnumInterfaceDevice()

Return Value:

    return HANDLE if the open and initialization was successfull,
  else INVLAID_HANDLE_VALUE.

--*/

HANDLE CFCPipeUSB::OpenOneDevice (HDEVINFO HardwareDeviceInfo,
                                  PSP_INTERFACE_DEVICE_DATA DeviceInfoData,
                                  char **devName)
{
  PSP_INTERFACE_DEVICE_DETAIL_DATA functionClassDeviceData = NULL;
  ULONG                            predictedLength = 0;
  ULONG                            requiredLength = 0;

  HANDLE hOut = INVALID_HANDLE_VALUE;

  //
  // allocate a function class device data structure to receive the
  // goods about this particular device.
  //
  SetupDiGetInterfaceDeviceDetail(HardwareDeviceInfo,
                                  DeviceInfoData,
                                  NULL,  // probing so no output buffer yet
                                  0,     // probing so output buffer length of zero
                                  &requiredLength,
                                  NULL); // not interested in the specific dev-node

  predictedLength = requiredLength;

  functionClassDeviceData = (PSP_INTERFACE_DEVICE_DETAIL_DATA) malloc (predictedLength);
  functionClassDeviceData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);

  //
  // Retrieve the information from Plug and Play.
  //
  if (! SetupDiGetInterfaceDeviceDetail(HardwareDeviceInfo,
                                        DeviceInfoData,
                                        functionClassDeviceData,
                                        predictedLength,
                                        &requiredLength,
                                        NULL))
  {
    free(functionClassDeviceData);
    return INVALID_HANDLE_VALUE;
  }

  *devName = strdup(functionClassDeviceData->DevicePath);
  //strcpy(devName, functionClassDeviceData->DevicePath) ;

  hOut = CreateFile( functionClassDeviceData->DevicePath,
                     GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL,          // no SECURITY_ATTRIBUTES structure
                     OPEN_EXISTING, // No special create flags
                     FILE_ATTRIBUTE_NORMAL,             // No special attributes
                     NULL);         // No template file

  free(functionClassDeviceData);
  return hOut;
}

///////////////////////////////////////////////////////////////////////////////
/*++
Routine Description:

   Do the required PnP things in order to find
   the next available proper device in the system at this time.

Arguments:

    pGuid:      ptr to GUID registered by the driver itself
    outNameBuf: the generated name for this device

Return Value:

    return HANDLE if the open and initialization was successful,
  else INVLAID_HANDLE_VALUE.
--*/

HANDLE CFCPipeUSB::OpenUsbDevice(LPGUID  pGuid, char **outNameBuf)
{
  HANDLE hOut = INVALID_HANDLE_VALUE;

  ULONG                    NumberDevices;
  HDEVINFO                 hardwareDeviceInfo;
  SP_INTERFACE_DEVICE_DATA deviceInfoData;
  ULONG                    i;
  BOOLEAN                  done;

  //
  // Open a handle to the plug and play dev node.
  // SetupDiGetClassDevs() returns a device information set that contains info on all
  // installed devices of a specified class.
  //
  hardwareDeviceInfo = SetupDiGetClassDevs (
                         pGuid,
                         NULL,            // Define no enumerator (global)
                         NULL,            // Define no
                         (DIGCF_PRESENT | // Only Devices present
                         DIGCF_INTERFACEDEVICE)); // Function class devices.

  //
  // Take a wild guess at the number of devices we have;
  // Be prepared to realloc and retry if there are more than we guessed
  //
  NumberDevices = 4;
  done = FALSE;
  deviceInfoData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

  i=0;
  while(!done) 
  {
    NumberDevices *= 2;

    for(; i < NumberDevices; i++) 
    {
      // SetupDiEnumDeviceInterfaces() returns information about device interfaces
      // exposed by one or more devices. Each call returns information about one interface;
      // the routine can be called repeatedly to get information about several interfaces
      // exposed by one or more devices.
      if(SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                      0, // We don't care about specific PDOs
                                      pGuid,
                                      i,
                                      &deviceInfoData)) 
      {
        hOut = OpenOneDevice (hardwareDeviceInfo, &deviceInfoData, outNameBuf);
        if(hOut != INVALID_HANDLE_VALUE) 
        {
          done = TRUE;
          break;
        }
      } 
      else 
      {
        if(ERROR_NO_MORE_ITEMS == GetLastError()) 
        {
           done = TRUE;
           break;
        }
      }
    }
  }

  // SetupDiDestroyDeviceInfoList() destroys a device information set
  // and frees all associated memory.
  SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);

  return hOut;
}

////////////////////////////////////////////////////////////////////////
/*++
Routine Description:

    Given a ptr to a driver-registered GUID, give us a string with the device name
    that can be used in a CreateFile() call.
    Actually briefly opens and closes the device and sets outBuf if successfull;
    returns FALSE if not

Arguments:

    pGuid:      ptr to GUID registered by the driver itself
    outNameBuf: the generated zero-terminated name for this device

Return Value:

    TRUE on success else FALSE

--*/
BOOL CFCPipeUSB::GetUsbDeviceFileName(LPGUID  pGuid, char **outNameBuf)
{
  HANDLE hDev = OpenUsbDevice(pGuid, outNameBuf);

  if(hDev != INVALID_HANDLE_VALUE)
  {
    CloseHandle(hDev);
    return TRUE;
  }
  return FALSE;
}


