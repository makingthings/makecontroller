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

#include <ch.h>
#include <pal.h>
#include "board.h"
#include "at91lib/aic.h"
#include "analogin.h"
#include "spi.h"
#include "pwm.h"

/*
 * FIQ Handler weak symbol defined in vectors.s.
 */
void FiqHandler(void);

static CH_IRQ_HANDLER(SpuriousHandler) {

  CH_IRQ_PROLOGUE();

  AT91C_BASE_AIC->AIC_EOICR = 0;

  CH_IRQ_EPILOGUE();
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
  AT91C_BASE_AIC->AIC_EOICR = 0;

  CH_IRQ_EPILOGUE();
}

void kill(void)
{
  AT91C_BASE_RSTC->RSTC_RCR = ( AT91C_RSTC_EXTRST | AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST | (0xA5 << 24 ) );
}

/*
 * Digital I/O ports static configuration as defined in @p board.h.
 */
static const AT91SAM7XPIOConfig config =
{
  {VAL_PIOA_ODSR, VAL_PIOA_OSR, VAL_PIOA_PUSR},
  {VAL_PIOB_ODSR, VAL_PIOB_OSR, VAL_PIOB_PUSR}
};

/*
 * Early initialization code.
 * This initialization is performed just after reset before BSS and DATA
 * segments initialization.
 */
void hwinit0(void) {
  
  // Flash Memory: 1 wait state, about 50 cycles in a microsecond.
  AT91C_BASE_MC->MC_FMR = (AT91C_MC_FMCN & (50 << 16)) | AT91C_MC_FWS_1FWS;

  #ifndef WATCHDOG_ENABLE
  AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS;
  #endif

  // Enables the main oscillator and waits 56 slow cycles as startup time.
  AT91C_BASE_PMC->PMC_MOR = (AT91C_CKGR_OSCOUNT & (7 << 8)) | AT91C_CKGR_MOSCEN;
  while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MOSCS))
    ;

  // PLL setup: DIV = 14, MUL = 72, PLLCOUNT = 10
  // PLLfreq = 96109714 Hz (rounded)
  AT91C_BASE_PMC->PMC_PLLR = (AT91C_CKGR_DIV & 14) |
                             (AT91C_CKGR_PLLCOUNT & (10 << 8)) |
                             (AT91C_CKGR_MUL & (72 << 16));
  while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_LOCK))
    ;

  // Master clock = PLLfreq / 2 = 48054858 Hz (rounded)
  AT91C_BASE_PMC->PMC_MCKR = AT91C_PMC_CSS_PLL_CLK | AT91C_PMC_PRES_CLK_2;
  while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY))
    ;

  // PIO initialization.
  palInit(&config);
}

/*
 * Late initialization code.
 * This initialization is performed after BSS and DATA segments initialization
 * and before invoking the main() function.
 */
void hwinit1(void) {
  int i;

  // Default AIC setup, the device drivers will modify it as needed.
  AT91C_BASE_AIC->AIC_ICCR = 0xFFFFFFFF;
  AT91C_BASE_AIC->AIC_SVR[0] = (AT91_REG)FiqHandler;
  for (i = 1; i < 31; i++) {
    AT91C_BASE_AIC->AIC_SVR[i] = (AT91_REG)NULL;
    AT91C_BASE_AIC->AIC_EOICR = (AT91_REG)i;
  }
  AT91C_BASE_AIC->AIC_SPU  = (AT91_REG)SpuriousHandler;
  
  
  /*
    peripheral inits - these are here so they're conveniently already
    done for common usage, but can be removed by conditionalization
  */
  #ifndef NO_AIN_INIT
  ainInit();
  #endif
  
  #ifndef NO_SPI_INIT
  spiInit();
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

  #ifndef NO_SERIAL_INIT
  // sdInit();
  // AT91C_BASE_PIOA->PIO_PDR   = AT91C_PA3_RTS0 | AT91C_PA4_CTS0;
  // AT91C_BASE_PIOA->PIO_ASR   = AT91C_PIO_PA3 | AT91C_PIO_PA4;
  // AT91C_BASE_PIOA->PIO_PPUDR = AT91C_PIO_PA3 | AT91C_PIO_PA4;
  #endif

  // ChibiOS/RT initialization.
  chSysInit();
}