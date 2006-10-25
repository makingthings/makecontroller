/*********************************************************************************

 Copyright 2006 MakingThings

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

#include "timer_internal.h"

#include "AT91SAM7X256.h"
#include "types.h"

extern struct Timer_ Timer;

void Timer_Isr( void ) __attribute__ ((interrupt("IRQ")));
// void Timer_Isr( void ) __attribute__ ((interrupt(naked)));

void Timer_Isr( void )
{
	/* This ISR can cause a context switch.  Therefore a call to the 
	portENTER_SWITCHING_ISR() macro is made.  This must come BEFORE any 
	stack variable declarations. */
	//portENTER_SWITCHING_ISR();

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
    TimerEntry* te = Timer.runningFirst;
    TimerEntry* pe = NULL;
    while ( te != NULL )
    {
      TimerEntry* ne = te->next;
      te->time -= AT91C_BASE_TC0->TC_RC + AT91C_BASE_TC0->TC_CV;
      if ( te->time <= 0 )
      {
        if ( te->callback != NULL )
          (*te->callback)( te->id );
        
        if ( te->repeat )
        {
          te->time += te->timeInitial;
        }
        else
        {
          // remove it if necessary (do this first!)
          if ( pe == NULL )
            Timer.runningFirst = ne;
          else
            pe->next = ne;
               
          // add it to the free queue
          te->next = Timer.freeFirst;
          Timer.freeFirst = te;        
        }
      } 
      else
      {
        pe = te;
      }
      te = ne;
    }

    // Slightly sad thing: new entries created during the callbacks need to get added after this loop
    if ( Timer.newFirst != NULL )
    {
      if ( pe == NULL )
      {  
        Timer.runningFirst = Timer.newFirst; 
      }
      else
      {
        pe->next = Timer.newFirst;
      }
      Timer.newFirst = NULL;
    }

    te = Timer.runningFirst;
    if ( te != NULL )
    {
      // Run through again to determine the next time
      int nextTime = -1;
      while ( te != NULL )
      {
        if ( nextTime == -1 || te->time < nextTime )
          nextTime = te->time;
        te = te->next;
      }
      // Add in whatever we're at now
      nextTime += AT91C_BASE_TC0->TC_CV;
      // Make sure it's not too big
      if ( nextTime > 0xFFFF )
        nextTime = 0xFFFF;
      AT91C_BASE_TC0->TC_RC = nextTime;
    }
    else
    {
      AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKDIS;
      Timer.running = false;
    }

    Timer.servicing = false;
  }

	/* Clear AIC to complete ISR processing */
	AT91C_BASE_AIC->AIC_EOICR = 0;

	/* Do a task switch if needed */
	//portEXIT_SWITCHING_ISR( false );
}

