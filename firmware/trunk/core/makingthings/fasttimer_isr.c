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
#include "fasttimer.h"
#include "fasttimer_internal.h"
#include "AT91SAM7X256.h"

void DisableFIQFromThumb( void )
{
	asm volatile ( "STMDB	SP!, {R0}" );		/* Push R0.									*/
	asm volatile ( "MRS		R0, CPSR" );		/* Get CPSR.								*/
	asm volatile ( "ORR		R0, R0, #0x40" );	/* Disable FIQ.						*/
	asm volatile ( "MSR		CPSR, R0" );		/* Write back modified value.				*/
	asm volatile ( "LDMIA	SP!, {R0}" );		/* Pop R0.									*/
	asm volatile ( "BX		R14" );				/* Return back to thumb.					*/
}
		
void EnableFIQFromThumb( void )
{
	asm volatile ( "STMDB	SP!, {R0}" );		/* Push R0.									*/	
	asm volatile ( "MRS		R0, CPSR" );		/* Get CPSR.								*/	
	asm volatile ( "BIC		R0, R0, #0x40" );	/* Enable FIQ.							*/	
	asm volatile ( "MSR		CPSR, R0" );		/* Write back modified value.				*/	
	asm volatile ( "LDMIA	SP!, {R0}" );		/* Pop R0.									*/
	asm volatile ( "BX		R14" );				/* Return back to thumb.					*/
}

extern struct FastTimer_ FastTimer;

// At the moment, the FastTimer ISR or callbacks, very importantly, can't call any OS stuff since
// the IRQ might happen any old where

void FastTimer_Isr( void ) __attribute__ ((naked));

// Made non-local for debugging
FastTimerEntry* te;
int jitter;
int timeReference;

void FastTimer_Isr( void )
{
  portENTER_FIQ( );
  int status = AT91C_BASE_TC2->TC_SR;
  if ( status & AT91C_TC_CPCS )
  {
    FastTimer.servicing = true;

    //AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKDIS;

    // make sure there's not another IRQ while we're processing
    timeReference = AT91C_BASE_TC2->TC_RC;
    AT91C_BASE_TC2->TC_RC = 0xFF00;

#ifdef FASTIRQ_MONITOR_IO
    Io_SetTrue( FASTIRQ_MONITOR_IO );
#endif

#ifdef FASTIRQ_STATS
    int startCount = AT91C_BASE_TC2->TC_CV;

    // moved outside for debugging
    //int jitter;
    jitter = AT91C_BASE_TC2->TC_CV;

    if ( ++FastTimer.count == 1000 )
    {
      // need to not do division here... takes too long
      //FastTimer.jitterTotal = FastTimer.jitterTotal / FastTimer.count;
      FastTimer.jitterTotal = 0;
      FastTimer.jitterMax = 0;
      // need to not do division here... takes too long
      // FastTimer.durationTotal = FastTimer.durationTotal / FastTimer.count;
      FastTimer.durationTotal = 0;
      FastTimer.durationMax = 0;
      FastTimer.count = 1;
    }
    
    FastTimer.jitterTotal += jitter;
        
    if ( jitter > FastTimer.jitterMax )
      FastTimer.jitterMax = jitter;
    if ( jitter > FastTimer.jitterMaxAllDay )
      FastTimer.jitterMaxAllDay = jitter;

#endif

    //FastTimerEntry* te = FastTimer.first;
    // Use this during debuggin
    te = FastTimer.first;

    FastTimer.next = NULL;
    FastTimer.previous = NULL;
    FastTimer.nextTime = 0xFF00;
    int removed = false;
    // timeReference = AT91C_BASE_TC2->TC_CV;

    while ( te != NULL )
    {
      FastTimer.next = te->next;
      te->timeCurrent -= ( timeReference + AT91C_BASE_TC2->TC_CV );
      if ( te->timeCurrent <= FASTTIMER_MINCOUNT )
      {
        // Watch out for gross errors
        //if ( te->timeCurrent < -FASTTIMER_MINCOUNT)
        //  te->timeCurrent = -FASTTIMER_MINCOUNT;

        if ( te->repeat )
        {
          te->timeCurrent += te->timeInitial;
        }
        else
        {
          // remove it if necessary (do this first!)
          if ( FastTimer.previous == NULL )
            FastTimer.first = FastTimer.next;
          else
            FastTimer.previous->next = FastTimer.next;  
          removed = true;
        }

        if ( te->callback != NULL )
        {
          // in this callback, the callee is free to add and remove any members of this list
          // which might effect the first, next and previous pointers
          // so don't assume any of those local variables are good anymore
          (*te->callback)( te->id );
        }
      }
      
      // note that this has to be better than this ultimately - since
      // the callback routine can remove the entry without letting us know
      // at all.
      if ( !removed )
      {
        if ( FastTimer.nextTime == -1 || te->timeCurrent < FastTimer.nextTime )
        {
          FastTimer.nextTime = te->timeCurrent;
        }
        FastTimer.previous = te;
      }

      te = FastTimer.next;
    }

    if ( FastTimer.first != NULL )
    {
      // Make sure it's not too big
      if ( FastTimer.nextTime > 0xFF00 )
        FastTimer.nextTime = 0xFF00;
      // Make sure it's not too small
      if ( FastTimer.nextTime < (int)AT91C_BASE_TC2->TC_CV + 20 )
        FastTimer.nextTime = AT91C_BASE_TC2->TC_CV + 20;
      AT91C_BASE_TC2->TC_RC = FastTimer.nextTime;
    }
    else
    {
      AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKDIS;
      FastTimer.running = false;
    }

#ifdef FASTIRQ_STATS    
    int duration = AT91C_BASE_TC2->TC_CV - startCount;
    FastTimer.durationTotal += duration;  
    if ( duration > FastTimer.durationMax )
      FastTimer.durationMax = duration;
    if ( duration > FastTimer.durationMaxAllDay )
      FastTimer.durationMaxAllDay = duration;
#endif

#ifdef FASTIRQ_MONITOR_IO    
    Io_SetFalse( FASTIRQ_MONITOR_IO );
#endif

    // AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
    FastTimer.servicing = false;
  }
  portEXIT_FIQ( );
}

