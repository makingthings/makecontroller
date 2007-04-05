/*******************************************************************************
*
*      Company                   :  CAP GEMINI
*      Service Line              :  Product Devlopment
*      Project                   :  FingerChip
*      Task                      :  
*
*      Soft name                 :  $RCSfile: FC_Error.h,v $
*      Soft version              :  $Revision: 1.1 $
*      Last modified             :  $Date: 2006/07/27 23:24:19 $
*
*      Description               :  Define all the error codes return by the Dll
*
*      Authors                   :  Dauvergne Mickaël
*
*      Platform                  :  Independent 
*      System                    :  Independent 
*
*      Programming language      :  C ANSI
*      Operating System          :  Independent 
*      Date of creation          :  02/14/2001
*
*      Prefix                    :
*      Code size (.o) (KB)       :
*
*      References      :
*
*      History                   :
*                04/26/2001 THM  :  FC_IMAGE_TOO_SMALL is an error now
*                                   and mean that the recipient image is 
*                                   too small.
*                                   FC_TRUNCATED_IMAGE now mean that the
*                                   reconstructed image is too big to fit
*                                   the image buffer and will be truncated.
*
*     Copyright (c) 
*     All rights reserved
*
******************************************************************************/

#define	FC_OK	            	        0	//OK

//ERROR
#define FC_ERROR_MASK	              (short)(0x8000) 
#define FC_CODE_MASK	              (short)(0x7fff)

//Critical Errors
#define FC_ERROR	                  (short)(0x8000)    //Internal error
#define FC_DLL_NOT_INITIALIZED	    (short)(0x8001)
#define FC_NOT_OPEN	                (short)(0x8002) //Device not opened
#define	FC_DRIVER_NOT_FOUND	        (short)(0x8004) //driver may not be installed 
#define FC_DEVICE_NOT_CONNECTED     (short)(0x8008) //Device may be  not connected
#define FC_DRIVER_ERROR	            (short)(0x8010) //Error during driver  acquisition
                                                //or device not plugged
#define FC_BUSY	                    (short)(0x8020) //device busy
#define FC_NOT_ENOUGH_MEMORY	      (short)(0x8040) //can't allocate memory
#define FC_NOT_INITIALIZED	        (short)(0x8080) //parameter not initialized
#define	FC_NO_SLICE_MEMORY	        (short)(0x8100) //no slice memory
#define	FC_NO_IMAGE_MEMORY	        (short)(0x8200) //no image memory

#define FC_SYNCHRO_LOST	            (short)(0x9001) //synchro bytes lost
#define FC_WRONG_SIZE_SLICE	        (short)(0x9002) //wrong slice size 
#ifdef WIN32
#define FC_WRONG_IMAGE_WIDTH	      (short)(0x9004)
#endif

#define FC_NO_BANDWIDTH	            (short)(0xA001) //No bandwidth allocated for usb driver

#define FC_OPERATION_ABORTED        (short)(0xC000) //Operation was aborted

//Not Critical Errors
#define FC_BAD_FINGER	              (short)(0xD001) //finger not swept fine
#define FC_IMAGE_TOO_BIG	          (short)(0xD002) //image buffer too big
#define	FC_IMAGE_TOO_SMALL	        (short)(0xD004) //image is too small
#define FC_HEIGHT_TOO_LITTLE	      (short)(0xD008) //Image height to little
#define FC_TOO_SLOW	                (short)(0xD010) //sweeping was too slow
#define FC_TOO_FAST	                (short)(0xD020) //sweeping was too fast
#define FC_NOT_ENOUGH_GOOD_SLICE    (short)(0xD040)//not enough good slices
#define FC_TIME_OUT	                (short)(0xD080) //timeOut
#define FC_NOT_SUPPORTED            (short)(0xD100) //Parameter or function not 
                                          //supported by the current device

//WARNING
#define FC_DIRECTORY_MISSING	      (short)(0x0001) //directory is missing
#define FC_UNABLE_OPEN_FILE	        (short)(0x0002) //can't open the file
#define FC_UNCONSISTENT_SLICE	      (short)(0x0004) //Slices are not consistent
#define FC_NO_THERMAL	              (short)(0x0008) //Chip warming does not work
#define FC_THRESHOLD_HIGH	          (short)(0x0010) //Finger detection threshold to high
#define FC_HALF_BANDWIDTH	          (short)(0x0020) //only half bandwidth allocated for usb driver
#define FC_TEMPERATURE_NOT_UPTODATE	(short)(0x0040) //Temperature is not uptodate
#define FC_TOO_WARM                 (short)(0x0080) //Temperature too high to warm 
#define FC_TOO_COLD                 (short)(0x0090) //Temperature too low to warm
#define FC_SLOW	                    (short)(0x0900) //Sweeping was a bit slow
#define	FC_IMAGE_TRUNCATED	        (short)(0x0300) //reconstructed image was truncated
#define FC_NO_FINGER	              (short)(0x2000) //No finger detected
#define FC_TRUNCATED_MESSAGE	      (short)(0x4000) //Error message was truncated

