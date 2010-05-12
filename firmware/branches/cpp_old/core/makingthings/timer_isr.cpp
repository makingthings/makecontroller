/*********************************************************************************

 Copyright 2006-2009 MakingThings

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

void TimerIsr_Wrapper( void ) __attribute__ ((naked));
void Timer_Isr( );

void Timer_Isr( void )
{
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
    Timer* timer = manager->first;
    manager->next = NULL;
    manager->previous = NULL;
    manager->nextTime = -1;
    while ( timer != NULL )
    {
      manager->next = timer->next;
      timer->timeCurrent -= (manager->tc->TC_RC + manager->tc->TC_CV);
      if ( timer->timeCurrent <= 0 )
      {
        if ( timer->repeat )
          timer->timeCurrent += timer->timeInitial;
        else
        {
          // remove it if necessary (do this first!)
          if ( manager->previous == NULL )
            manager->first = manager->next;
          else
            manager->previous->next = manager->next;     
        }

        if ( timer->callback != NULL )
        {
          // in this callback, the callee is free to add and remove any members of this list
          // which might effect the first, next and previous pointers
          // so don't assume any of those local variables are good anymore
          (*timer->callback)( timer->id );
        }

        // Assuming we're still on the list (if we were removed, then re-added, we'd be on the beggining of
        // the list with this task already performed) see whether our time is the next to run
        if ( ( manager->previous == NULL && manager->first == timer ) ||
             ( manager->previous != NULL && manager->previous->next == timer ) )
        {
          if ( manager->nextTime == -1 || timer->timeCurrent < manager->nextTime )
            manager->nextTime = timer->timeCurrent;
        }
      } 
      else
        manager->previous = timer;
      
      timer = manager->next;
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

  AT91C_BASE_AIC->AIC_EOICR = 0; // Clear AIC to complete ISR processing
}

void TimerIsr_Wrapper( void )
{
  /* Save the context of the interrupted task. */
  portSAVE_CONTEXT();

  /* Call the handler to do the work.  This must be a separate
  function to ensure the stack frame is set up correctly. */
  Timer_Isr();

  /* Restore the context of whichever task will execute next. */
  portRESTORE_CONTEXT();
}

