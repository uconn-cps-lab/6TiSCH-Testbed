/*******************************************************************************
 *  Copyright (c) 2017 Texas Instruments Inc.
 *  All Rights Reserved This program is the confidential and proprietary
 *  product of Texas Instruments Inc.  Any Unauthorized use, reproduction or
 *  transfer of this program is strictly prohibited.
 *******************************************************************************
  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
 *******************************************************************************
 * FILE PURPOSE:
 *
 *******************************************************************************
 * DESCRIPTION:
 *
 *******************************************************************************
 * HISTORY:
 *
 ******************************************************************************/
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include "pltfrm_lib.h"
#include "pltfrm_timer.h"

void pltfrm_debug_hex(int b) {}

void pltfrm_debug_dec(long b) {}

void pltfrm_debug_flush() {}

//*-------------------------------------------------*//

void pltfrm_byte_memcpy_swapped(void* destination, int dest_len, void* source, int src_len)
{
   int i;
   memset(destination, 0, dest_len); //init to zeros.

   for (i = 0; i < src_len; i++)
   {
      ((char*)destination)[i] = ((char*)source)[(src_len - 1) - i];
   }
}

//*-------------------------------------------------*//
int pltfrm_timer_isActive(pltfrm_timer_t* timer)
{
   struct timespec currTime;

   clock_gettime(CLOCK_MONOTONIC, &currTime);
   timerEntry_t *entry = timer->handle;
   return !(
             currTime.tv_sec >= entry->expiry.tv_sec
             ||
             (currTime.tv_sec == entry->expiry.tv_sec
              &&
              currTime.tv_nsec >= entry->expiry.tv_nsec)
          );
}

void pltfrm_timer_init(pltfrm_timer_t* timer)
{
   timer->handle = NULL;
   timer->func = NULL;
   timer->params = NULL;
}

void clock_FuncPtr(void *param1_p, void *param2_p)
{
   pltfrm_timer_t *t;

   // call uip platform function
   t = (pltfrm_timer_t*) (param2_p);
   t->func(param1_p);
}

void pltfrm_timer_create(pltfrm_timer_t* timer, pltfrm_timer_function_t func, void* func_params, unsigned int timeout)
{
   timerEntry_t *newEntry_p;

   timer->func = func;
   timer->params = func_params;

   newEntry_p = pltfrm_timer_create_TimerEntry(timeout, clock_FuncPtr, func_params);
   timer->handle = newEntry_p;

   // update the param2
   newEntry_p->param2_p = timer;

}


bool pltfrm_timer_isCreated(pltfrm_timer_t* timer)
{
   return (((timer)->handle) != NULL);
}

void pltfrm_timer_start(pltfrm_timer_t* timer)
{
   timerEntry_t *Entry_p;

   Entry_p = (timerEntry_t *)timer->handle;
   pltfrm_timer_entry_add(Entry_p->hndl, Entry_p);

}

void pltfrm_timer_stop(pltfrm_timer_t* timer)
{
   timerEntry_t *Entry_p;

   Entry_p = (timerEntry_t *)timer->handle;
   pltfrm_timer_entry_del(Entry_p->hndl);

}

void pltfrm_timer_restart(pltfrm_timer_t* timer, unsigned int timeout)
{
   timerEntry_t *Entry_p;

   Entry_p = (timerEntry_t *)timer->handle;

   pltfrm_timer_stop(timer);

   // update the time out value
   Entry_p->tmoMsecs = timeout;

   pltfrm_timer_start(timer);
}

void pltfrm_timer_delete(pltfrm_timer_t* timer)
{
	if (timer != NULL && timer->handle != NULL)  //fengmo fix
	{
   free(timer->handle);
	   timer->handle = NULL;
	}
}
