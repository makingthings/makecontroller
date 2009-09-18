

#ifndef WATCHDOG_H
#define WATCHDOG_H

/**
  The Watchdog timer can reset the board in the event that it's not behaving as expected.
  
  \section Usage
  If you don't need the watchdog, simply turn it off by calling
  \code Watchdog::disable() \endcode

  If you want to use it, specify the length of the countdown to enable() and then
  periodically call restart() to restart the countdown.  If the countdown ever gets
  all the way to zero, the board will reset.  

  \b Example
  \code
  Watchdog::enable(2000); // start the countdown

  void MyTask()
  {
    while(true)
    {
      if( everything_is_normal )
        Watchdog::restart();
      // if everything is not normal for long enough,
      // the countdown will expire and the board will reset
    }
  }
  \endcode
*/
class Watchdog
{
public:
  Watchdog() { }
  static void enable( int millis );
  static void restart( );
  static void disable( );
};

#endif // WATCHDOG_H


