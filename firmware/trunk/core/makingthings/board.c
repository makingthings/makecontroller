/*
    ChibiOS/RT - Copyright (C) 2006-2007 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
 * This initialization is performed just after reset before BSS and DATA
 * segments initialization.
 */
void hwinit0(void)
{
  #ifndef WATCHDOG_ENABLE
  AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS;
  #endif
  at91sam7_clock_init();
}

/*
 * Late initialization code.
 * This initialization is performed after BSS and DATA segments initialization
 * and before invoking the main() function.
 */
void hwinit1(void)
{
  halInit();
  // peripheral inits - these are here so they're conveniently already
  // done for common usage, but can be removed by conditionalization
  #ifndef NO_SPI_INIT
  spiInit();
  #endif
  
  #ifndef NO_EEPROM_INIT
  eepromInit();
  #endif

  #ifndef NO_PWM_INIT
  pwmInit();
  #endif

  // PIT Initialization.
  AIC_ConfigureIT(AT91C_ID_SYS,
                  AT91C_AIC_SRCTYPE_HIGH_LEVEL | (AT91C_AIC_PRIOR_HIGHEST - 1),
                  SYSIrqHandler);
  AIC_EnableIT(AT91C_ID_SYS);
  AT91C_BASE_PITC->PITC_PIMR = (MCK / 16 / CH_FREQUENCY) - 1;
  AT91C_BASE_PITC->PITC_PIMR |= AT91C_PITC_PITEN | AT91C_PITC_PITIEN;

  chSysInit(); // ChibiOS/RT initialization.

  // would rather not put this below chSysInit() but it relies on the RTOS being setup
  #ifndef NO_AIN_INIT
  analoginInit();
  #endif
}
