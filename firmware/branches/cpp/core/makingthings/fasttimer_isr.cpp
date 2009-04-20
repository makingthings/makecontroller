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
#include "fasttimer.h"

void DisableFIQFromThumb( void )
{
  asm volatile ( "STMDB SP!, {R0}" );   /* Push R0.                 */
  asm volatile ( "MRS   R0, CPSR" );    /* Get CPSR.                */
  asm volatile ( "ORR   R0, R0, #0x40" ); /* Disable FIQ.           */
  asm volatile ( "MSR   CPSR, R0" );    /* Write back modified value.       */
  asm volatile ( "LDMIA SP!, {R0}" );   /* Pop R0.                  */
  asm volatile ( "BX    R14" );       /* Return back to thumb.          */
}
    
void EnableFIQFromThumb( void )
{
  asm volatile ( "STMDB SP!, {R0}" );   /* Push R0.                 */  
  asm volatile ( "MRS   R0, CPSR" );    /* Get CPSR.                */  
  asm volatile ( "BIC   R0, R0, #0x40" ); /* Enable FIQ.              */  
  asm volatile ( "MSR   CPSR, R0" );    /* Write back modified value.       */  
  asm volatile ( "LDMIA SP!, {R0}" );   /* Pop R0.                  */
  asm volatile ( "BX    R14" );       /* Return back to thumb.          */
}

// At the moment, the FastTimer ISR or callbacks, very importantly, can't call any OS stuff since
// the IRQ might happen any old where

void FastTimer_Isr( void ) __attribute__ ((naked));

// Made non-local for debugging
FastTimer* ftimer;
int jitter;
int timeReference;

void FastTimer_Isr( void )
{
  portENTER_FIQ( );
  FastTimer::Manager* manager = &FastTimer::manager;
  int status = manager->tc->TC_SR;
  if ( status & AT91C_TC_CPCS )
  {
    manager->servicing = true;

    //AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKDIS;

    // make sure there's not another IRQ while we're processing
    timeReference = manager->tc->TC_RC;
    manager->tc->TC_RC = 0xFF00;

#ifdef FASTIRQ_MONITOR_IO
    Io_SetTrue( FASTIRQ_MONITOR_IO );
#endif

#ifdef FASTIRQ_STATS
    int startCount = manager->tc->TC_CV;

    // moved outside for debugging
    //int jitter;
    jitter = manager->tc->TC_CV;

    if ( ++manager->count == 1000 )
    {
      // need to not do division here... takes too long
      //manager->jitterTotal = manager->jitterTotal / manager->count;
      manager->jitterTotal = 0;
      manager->jitterMax = 0;
      // need to not do division here... takes too long
      // manager->durationTotal = manager->durationTotal / manager->count;
      manager->durationTotal = 0;
      manager->durationMax = 0;
      manager->count = 1;
    }
    
    manager->jitterTotal += jitter;
        
    if ( jitter > manager->jitterMax )
      manager->jitterMax = jitter;
    if ( jitter > manager->jitterMaxAllDay )
      manager->jitterMaxAllDay = jitter;

#endif

    //FastTimerEntry* ftimer = manager->first;
    // Use this during debuggin
    ftimer = manager->first;
    manager->next = NULL;
    manager->previous = NULL;
    manager->nextTime = 0xFF00;
    int removed = false;
    // timeReference = AT91C_BASE_TC2->TC_CV;

    while ( ftimer != NULL )
    {
      manager->next = ftimer->next;
      ftimer->timeCurrent -= ( timeReference + manager->tc->TC_CV );
      if ( ftimer->timeCurrent <= FASTTIMER_MINCOUNT )
      {
        // Watch out for gross errors
        //if ( ftimer->timeCurrent < -FASTTIMER_MINCOUNT)
        //  ftimer->timeCurrent = -FASTTIMER_MINCOUNT;

        if ( ftimer->repeat )
          ftimer->timeCurrent += ftimer->timeInitial;
        else
        {
          // remove it if necessary (do this first!)
          if ( manager->previous == NULL )
            manager->first = manager->next;
          else
            manager->previous->next = manager->next;  
          removed = true;
        }

        if ( ftimer->callback != NULL )
        {
          // in this callback, the callee is free to add and remove any members of this list
          // which might effect the first, next and previous pointers
          // so don't assume any of those local variables are good anymore
          (*ftimer->callback)( ftimer->id );
        }
      }
      
      // note that this has to be better than this ultimately - since
      // the callback routine can remove the entry without letting us know
      // at all.
      if ( !removed )
      {
        if ( manager->nextTime == -1 || ftimer->timeCurrent < manager->nextTime )
          manager->nextTime = ftimer->timeCurrent;
        manager->previous = ftimer;
      }

      ftimer = manager->next;
    }

    if ( manager->first != NULL )
    {
      // Make sure it's not too big
      if ( manager->nextTime > 0xFF00 )
        manager->nextTime = 0xFF00;
      // Make sure it's not too small
      if ( manager->nextTime < (int)manager->tc->TC_CV + 20 )
        manager->nextTime = manager->tc->TC_CV + 20;
      manager->tc->TC_RC = manager->nextTime;
    }
    else
    {
      manager->tc->TC_CCR = AT91C_TC_CLKDIS;
      manager->running = false;
    }

#ifdef FASTIRQ_STATS    
    int duration = manager->tc->TC_CV - startCount;
    manager->durationTotal += duration;  
    if ( duration > manager->durationMax )
      manager->durationMax = duration;
    if ( duration > manager->durationMaxAllDay )
      manager->durationMaxAllDay = duration;
#endif

#ifdef FASTIRQ_MONITOR_IO    
    Io_SetFalse( FASTIRQ_MONITOR_IO );
#endif

    // AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
    manager->servicing = false;
  }
  portEXIT_FIQ( );
}

