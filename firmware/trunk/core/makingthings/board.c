/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,2011 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.

                                      ---

    A special exception to the GPL can be applied should you wish to distribute
    a combined work that includes ChibiOS/RT, without being obliged to provide
    the source code for any proprietary components. See the file exception.txt
    for full details of how and when the exception can be applied.
*/

#include "core.h"

void kill(void)
{
  AT91C_BASE_RSTC->RSTC_RCR = (AT91C_RSTC_EXTRST | AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST | (0xA5 << 24));
}

/*
 * FIQ Handler weak symbol defined in vectors.s.
 */
void FiqHandler(void);

CH_IRQ_HANDLER(SpuriousHandler)
{
  CH_IRQ_PROLOGUE();
  AT91C_BASE_AIC->AIC_EOICR = 0;
  CH_IRQ_EPILOGUE();
}

void UndHandler(void)
{
#ifdef NO_RESTART_ON_FAILURE
  while(1);
#else
  kill();
#endif
}

void SwiHandler(void)
{
#ifdef NO_RESTART_ON_FAILURE
  while(1);
#else
  kill();
#endif
}

void PrefetchHandler(void)
{
#ifdef NO_RESTART_ON_FAILURE
  while(1);
#else
  kill();
#endif
}

void FiqHandler(void)
{
  while(1);
}

void AbortHandler(void)
{
#ifdef NO_RESTART_ON_FAILURE
  while(1);
#else
  kill();
#endif
}

/**
 * @brief   PAL setup.
 * @details Digital I/O ports static configuration as defined in @p board.h.
 *          This variable is used by the HAL when initializing the PAL driver.
 */
#if HAL_USE_PAL || defined(__DOXYGEN__)
const PALConfig pal_default_config =
{
  {VAL_PIOA_ODSR, VAL_PIOA_OSR, VAL_PIOA_PUSR},
#if (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) || \
    (SAM7_PLATFORM == SAM7X512)
  {VAL_PIOB_ODSR, VAL_PIOB_OSR, VAL_PIOB_PUSR}
#endif
};
#endif

/*
 * SYS IRQ handling here.
 */
static CH_IRQ_HANDLER(SYSIrqHandler) {

  CH_IRQ_PROLOGUE();

  if (AT91C_BASE_PITC->PITC_PISR & AT91C_PITC_PITS) {
    (void) AT91C_BASE_PITC->PITC_PIVR;
    chSysLockFromIsr();
    chSysTimerHandlerI();
    chSysUnlockFromIsr();
  }
  
#if USE_SAM7_DBGU_UART
  if (AT91C_BASE_DBGU->DBGU_CSR & 
    (AT91C_US_RXRDY | AT91C_US_TXRDY | AT91C_US_PARE | AT91C_US_FRAME | AT91C_US_OVRE | AT91C_US_RXBRK)) {
    sd_lld_serve_interrupt(&SD3);
  }
#endif  
  AT91C_BASE_AIC->AIC_EOICR = 0;
  CH_IRQ_EPILOGUE();
}

/*
 * Early initialization code.
 * This initialization must be performed just after stack setup and before
 * any other initialization.
 */
void __early_init(void) {
  
  /* Watchdog disabled.*/
  #ifndef WATCHDOG_ENABLE
  AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS;
  #endif
  at91sam7_clock_init();
}

/*
 * Board-specific initialization code.
 */
void boardInit(void)
{
  /*
   * PIT Initialization.
   */
  AIC_ConfigureIT(AT91C_ID_SYS,
                  AT91C_AIC_SRCTYPE_HIGH_LEVEL | (AT91C_AIC_PRIOR_HIGHEST - 1),
                  SYSIrqHandler);
  AIC_EnableIT(AT91C_ID_SYS);
  AT91C_BASE_PITC->PITC_PIMR = (MCK / 16 / CH_FREQUENCY) - 1;
  AT91C_BASE_PITC->PITC_PIMR |= AT91C_PITC_PITEN | AT91C_PITC_PITIEN;
}
