/*----------------------------------------------------------------------------
*         ATMEL Microcontroller Software Support  -  ROUSSET  -
*----------------------------------------------------------------------------
* The software is delivered "AS IS" without warranty or condition of any
* kind, either express, implied or statutory. This includes without
* limitation any warranty or condition with respect to merchantability or
* fitness for any particular purpose, or against the infringements of
* intellectual property rights of others.
*----------------------------------------------------------------------------
* File Name           : Board.h
* Object              : AT91SAM7X Evaluation Board Features Definition File.
*
* Creation            : JG   20/Jun/2005
*----------------------------------------------------------------------------
*/
#ifndef Board_h
#define Board_h

#include "AT91SAM7X256.h"
//#include "ioat91sam7x256.h"

/* MAKE BOARD USES PB4, rather than RB18 for PHY power signal */
/* HAS LED ON RB12 */
#define MAKE_BOARD 1


/*-------------------------------*/
/* SAM7Board Memories Definition */
/*-------------------------------*/
// The AT91SAM7X128 embeds a 32-Kbyte SRAM bank, and 128K-Byte Flash

#define  FLASH_PAGE_NB		256
#define  FLASH_PAGE_SIZE	128

/*--------------*/
/* Master Clock */
/*--------------*/

#define EXT_OC          18432000   // Exetrnal ocilator MAINCK
#define MCK             47923200   // MCK (PLLRC div by 2)
#define MCKKHz          (MCK/1000) //

#endif /* Board_h */
