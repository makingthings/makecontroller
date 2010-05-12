

#include "AT91SAM7X256.h"
#include "watchdog.h"

#define WATCHDOG_KEY (0xA5 << 24)

/**
  Enable the watchdog timer.
  Specify the number of milliseconds that the watchdog should wait before 
  resetting.  Remember that you enable() or disable() can only be called
  once until the processor is reset again.

  The maximum countdown length is 16 seconds.
  @param millis The number of milliseconds in which a reset will occur.
*/
void Watchdog::enable( int millis )
{
  int period = (millis * 256) / 1000;
  AT91C_BASE_WDTC->WDTC_WDMR =  AT91C_WDTC_WDRSTEN |        // enable reset on timeout
                                AT91C_WDTC_WDDBGHLT |       // respect debug mode
                                AT91C_WDTC_WDIDLEHLT |      // respect idle mode
                                ( (period << 16 ) & AT91C_WDTC_WDD ) | // delta is as wide as the period, so we can restart anytime
                                (period & AT91C_WDTC_WDV);  // set the period
}

/**
  Restart the watchdog timer countdown.
  Call enable() first, and then call this occassionally to restart 
  the watchdog countdown so that it doesn't reset.
*/
void Watchdog::restart( )
{
  AT91C_BASE_WDTC->WDTC_WDCR = WATCHDOG_KEY | AT91C_WDTC_WDRSTT;
}

/**
  Disable the watchdog timer.
  Turn the watchdog off completely if you don't need it.
*/
void Watchdog::disable( )
{
  AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS;
}


