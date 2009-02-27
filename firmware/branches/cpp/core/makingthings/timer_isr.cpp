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


#include "FreeRTOS.h"
#include "timer.h"

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

  Timer::Manager* manager = &Timer::manager;

  int status = manager->tc->TC_SR;
  if ( status & AT91C_TC_CPCS )
  {
    manager->servicing = true;

    int jitter;
    manager->count++;
    jitter = manager->tc->TC_CV;

    manager->jitterTotal += jitter;
    if ( jitter > manager->jitterMax )
      manager->jitterMax = jitter;
    if ( jitter > manager->jitterMaxAllDay )
      manager->jitterMaxAllDay = jitter;

    // Run through once to make the callback calls
    Timer* te = manager->first;
    manager->next = NULL;
    manager->previous = NULL;
    manager->nextTime = -1;
    while ( te != NULL )
    {
      manager->next = te->next;
      te->timeCurrent -= manager->tc->TC_RC + manager->tc->TC_CV;
      if ( te->timeCurrent <= 0 )
      {
        if ( te->repeat )
        {
          te->timeCurrent += te->timeInitial;
        }
        else
        {
          // remove it if necessary (do this first!)
          if ( manager->previous == NULL )
            manager->first = manager->next;
          else
            manager->previous->next = manager->next;     
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
        if ( ( manager->previous == NULL && manager->first == te ) ||
             ( manager->previous != NULL && manager->previous->next == te ) )
        {
          if ( manager->nextTime == -1 || te->timeCurrent < manager->nextTime )
            manager->nextTime = te->timeCurrent;
        }
      } 
      else
      {
        manager->previous = te;
      }

      te = manager->next;
    }

    if ( manager->first != NULL )
    {
      // Add in whatever we're at now
      manager->nextTime += manager->tc->TC_CV;
      // Make sure it's not too big
      if ( manager->nextTime > 0xFFFF )
        manager->nextTime = 0xFFFF;
      manager->tc->TC_RC = manager->nextTime;
    }
    else
    {
      manager->tc->TC_CCR = AT91C_TC_CLKDIS;
      manager->running = false;
    }

    jitter = manager->tc->TC_CV;

    manager->servicing = false;
  }

	/* Clear AIC to complete ISR processing */
	AT91C_BASE_AIC->AIC_EOICR = 0;

	/* Do a task switch if needed */
	// portEXIT_SWITCHING_ISR( false );
}

