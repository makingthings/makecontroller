/*********************************************************************************

 Copyright 2006-2008 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

/* 
  BASIC INTERRUPT DRIVEN DRIVER FOR MAKE BOARD. 
*/

/* Scheduler includes. */

#include "FreeRTOS.h"

#include "types.h"

#include "timer.h"
#include "timer_internal.h"

#include "io.h"

#include "AT91SAM7X256.h"

extern struct Timer_ Timer;

// At the moment, the Timer ISR or callbacks, very importantly, can't call any OS stuff since
// the IRQ might happen any old where

void Timer_Isr( void ) __attribute__ ((interrupt("IRQ")));
//void Timer_Isr( void ) __attribute__ ((interrupt(naked)));

void Timer_Isr( void )
{
	/* This ISR can cause a context switch.  Therefore a call to the 
	portENTER_SWITCHING_ISR() macro is made.  This must come BEFORE any 
	stack variable declarations. */
	// portENTER_SWITCHING_ISR();

  int status = AT91C_BASE_TC0->TC_SR;
  if ( status & AT91C_TC_CPCS )
  {
    Timer.servicing = true;

    int jitter;
    Timer.count++;
    jitter = AT91C_BASE_TC0->TC_CV;

    Timer.jitterTotal += jitter;
    if ( jitter > Timer.jitterMax )
      Timer.jitterMax = jitter;
    if ( jitter > Timer.jitterMaxAllDay )
      Timer.jitterMaxAllDay = jitter;

    // Run through once to make the callback calls
    TimerEntry* te = Timer.first;
    Timer.next = NULL;
    Timer.previous = NULL;
    Timer.nextTime = -1;
    while ( te != NULL )
    {
      Timer.next = te->next;
      te->timeCurrent -= AT91C_BASE_TC0->TC_RC + AT91C_BASE_TC0->TC_CV;
      if ( te->timeCurrent <= 0 )
      {
        if ( te->repeat )
        {
          te->timeCurrent += te->timeInitial;
        }
        else
        {
          // remove it if necessary (do this first!)
          if ( Timer.previous == NULL )
            Timer.first = Timer.next;
          else
            Timer.previous->next = Timer.next;     
        }

        if ( te->callback != NULL )
        {
          // in this callback, the callee is free to add and remove any members of this list
          // which might effect the first, next and previous pointers
          // so don't assume any of those local variables are good anymore
          (*te->callback)( te->id );
        }

        // Assuming we're still on the list (if we were removed, then re-added, we'd be on the beggining of
        // the list with this task already performed) see whether our time is the next to run
        if ( ( Timer.previous == NULL && Timer.first == te ) ||
             ( Timer.previous != NULL && Timer.previous->next == te ) )
        {
          if ( Timer.nextTime == -1 || te->timeCurrent < Timer.nextTime )
            Timer.nextTime = te->timeCurrent;
        }
      } 
      else
      {
        Timer.previous = te;
      }

      te = Timer.next;
    }

    if ( Timer.first != NULL )
    {
      // Add in whatever we're at now
      Timer.nextTime += AT91C_BASE_TC0->TC_CV;
      // Make sure it's not too big
      if ( Timer.nextTime > 0xFFFF )
        Timer.nextTime = 0xFFFF;
      AT91C_BASE_TC0->TC_RC = Timer.nextTime;
    }
    else
    {
      AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKDIS;
      Timer.running = false;
    }

    jitter = AT91C_BASE_TC0->TC_CV;

    Timer.servicing = false;
  }

	/* Clear AIC to complete ISR processing */
	AT91C_BASE_AIC->AIC_EOICR = 0;

	/* Do a task switch if needed */
	// portEXIT_SWITCHING_ISR( false );
}

