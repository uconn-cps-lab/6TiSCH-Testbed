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

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>
#include <pltfrm_mbx.h>
#include <pltfrm_timer.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <signal.h>

#include "comm.h"

timerReq_t reqBuff;
timerEntry_t *timerHead_p = NULL;
pltfrmTimerHndl_t timerEntryHndl = 100;
pltfrm_mbx_t timerMbx;

pthread_mutex_t timerMutex;

pthread_t timerThreadInfo;

static struct sigevent pltfrmTimerSev;
static struct itimerspec pltfrmTimerSpec;
static timer_t pltfrmTimerid;

/******************************************************************************
 * FUNCTION NAME:
 *
 * DESCRIPTION:
 *
 * Return Value:
 *
 * Input Parameters:
 *
 * Output Parameters:
 ******************************************************************************/
void pltfrm_timer_del_req_hndlr(pltfrmTimerHndl_t hndl)
{
   //syslog(LOG_INFO, "<%s> hndl<%d> \n", __FUNCTION__, hndl);

   if (hndl > 0)
   {
      timerEntry_t *entry_p = timerHead_p;
      timerEntry_t *prev_p = NULL;

      while (entry_p != NULL)
      {
         if (entry_p->hndl == hndl)
         {
            break;
         }

         prev_p = entry_p;
         entry_p = entry_p->next_p;
      }

      if (entry_p != NULL)
      {
         if (prev_p == NULL)
         {
            timerHead_p = entry_p->next_p;
         }
         else
         {
            prev_p->next_p = entry_p->next_p;
         }

         free(entry_p);
      }
   }
   else
   {
      assert(0);
   }

   return;
}

/******************************************************************************
 * FUNCTION NAME:
 *
 * DESCRIPTION:
 *
 * Return Value:
 *
 * Input Parameters:
 *
 * Output Parameters:
 ******************************************************************************/
void pltfrm_timer_add_req_hndlr(pltfrmTimerHndl_t newHndl,
                                int tmoMsecs,
                                timerCbFunc_t cbFunc_p,
                                void *param1_p,
                                void *param2_p,
                                timerEntry_t *entry_p)
{
   timerEntry_t *newEntry_p;

   // syslog(LOG_INFO, "<%s> hndl<%u>/tmoMsecs<%d>", __FUNCTION__, newHndl, tmoMsecs);

   if (entry_p == NULL)
   {
      if (tmoMsecs <= 0 || cbFunc_p == NULL)
      {
         assert(0);
         return;
      }

      newEntry_p = (timerEntry_t *)malloc(sizeof(timerEntry_t));

      if (newEntry_p == NULL)
      {
         assert(0);
         return;
      }

      newEntry_p->cbFunc_p = cbFunc_p;
      newEntry_p->param1_p = param1_p;
      newEntry_p->param2_p = param2_p;
      newEntry_p->hndl = newHndl;
      newEntry_p->tmoMsecs = tmoMsecs;
      newEntry_p->type = (newHndl == PLTFRM_TIMER_INVALID_HNDL) ? PLTFRM_TIMER_TYPE_PERIODIC : PLTFRM_TIMER_TYPE_ONE_SHOT;
   }
   else
   {
      newEntry_p = entry_p;
   }

   pltfrm_timer_list_add_req_hndlr(newHndl, newEntry_p);

   return;
}
/******************************************************************************
 * FUNCTION NAME:
 *
 * DESCRIPTION:
 *
 * Return Value:
 *
 * Input Parameters:
 *
 * Output Parameters:
 ******************************************************************************/
void pltfrm_timer_req_hndlr(timerReq_t *req_p)
{
   switch (req_p->type)
   {
      case PLTFRM_TIMER_REQ_START:
         pltfrm_timer_add_req_hndlr(req_p->hndl,
                                    req_p->tmoMsecs,
                                    req_p->cbFunc_p,
                                    req_p->param1_p,
                                    req_p->param2_p,
                                    NULL);
         break;

      case PLTFRM_PERIODIC_TIMER_REQ_START:
         pltfrm_timer_add_req_hndlr(PLTFRM_TIMER_INVALID_HNDL,
                                    req_p->tmoMsecs,
                                    req_p->cbFunc_p,
                                    req_p->param1_p,
                                    req_p->param2_p,
                                    NULL);
         break;

      case PLTFRM_TIMER_REQ_STOP:
         pltfrm_timer_del_req_hndlr(req_p->hndl);
         break;

      case PLTFRM_TIMER_REQ_ADD_ENTRY:
         pltfrm_timer_list_add_req_hndlr(req_p->hndl, (timerEntry_t *)req_p->param2_p);
         break;

      case PLTFRM_TIMER_REQ_DEL_ENTRY:
         pltfrm_timer_list_del_req_hndlr(req_p->hndl);
         break;

      case PLTFRM_TIMER_TMO:  //Handled in the main message loop by default
         break;

      default:
         assert(0);
         break;
   }
}
/*****************************************************************************
*  FUNCTION:
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*  RETURNS:
*
*****************************************************************************/
void pltfrm_timer_timeoutHandler(union sigval sv)
{
   timerReq_t tmrReq;

   syslog(LOG_INFO, "<%s> enter", __FUNCTION__);
   tmrReq.type = PLTFRM_TIMER_TMO;
   pltfrm_mbx_send(&timerMbx, (unsigned char *)&tmrReq, PLTFRM_MBX_OPN_NO_WAIT);
   syslog(LOG_INFO, "<%s> exit", __FUNCTION__);
}

/******************************************************************************
 * FUNCTION NAME:
 *
 * DESCRIPTION:
 *
 * Return Value:
 *
 * Input Parameters:
 *
 * Output Parameters:
 ******************************************************************************/
void *pltfrm_timer_task_entry_fn(void *param_p)
{
   int rc;

   syslog(LOG_INFO, "<%s> started \n", __FUNCTION__);

   memset(&pltfrmTimerSev, 0, sizeof(pltfrmTimerSev));
   memset(&pltfrmTimerSpec, 0, sizeof(pltfrmTimerSpec));
   pltfrmTimerSev.sigev_notify = SIGEV_THREAD;
   pltfrmTimerSev.sigev_notify_function = pltfrm_timer_timeoutHandler;
   pltfrmTimerSev.sigev_value.sival_ptr = &pltfrmTimerid;

   if (timer_create(CLOCK_MONOTONIC, &pltfrmTimerSev, &pltfrmTimerid) != 0)
   {
      pltfrmTimerid = 0;
      syslog(LOG_INFO, "<%s> timer create failed", __FUNCTION__);
      assert(0);
   }

   do
   {
      memset(&reqBuff, 0, sizeof(reqBuff));
      rc = pltfrm_mbx_pend(&timerMbx,
                           (unsigned char *)&reqBuff,
                           PLTFRM_MBX_OPN_WAIT_FOREVER);
      // syslog(LOG_INFO, "<%s> mbx_pend() rc<%d>", __FUNCTION__, rc);

      pltfrm_timer_list_tmo_hndlr();

      if (rc < 0)
      {
         assert(0);
         continue;
      }

      if (rc > 0)
      {
         pltfrm_timer_req_hndlr(&reqBuff);
      }
   }
   while (1);

   return 0;
}

/******************************************************************************
 * FUNCTION NAME:
 *
 * DESCRIPTION:
 *
 * Return Value:
 *
 * Input Parameters:
 *
 * Output Parameters:
 ******************************************************************************/
int pltfrm_TIMER_init(void)
{
   struct sched_param param;
   int policy;
   int primin;
   int rc;

   pthread_mutex_init(&timerMutex, NULL);
   pltfrm_mbx_init(&timerMbx, sizeof(timerReq_t), 32);

   // Start Timer thread
   rc = pthread_create(&timerThreadInfo, NULL, pltfrm_timer_task_entry_fn, NULL);

   if (rc == 0)
   {
      rc = pthread_getschedparam(timerThreadInfo, &policy, &param);
   }

   if (rc == 0)
   {
      primin = sched_get_priority_max(policy);
      rc = pthread_setschedprio(timerThreadInfo, primin);
   }

   if (rc != 0)
   {
      rc = -1;
   }
   else
   {
      rc = 1;
   }

   return rc;
}

/******************************************************************************
 * FUNCTION NAME:
 *
 * DESCRIPTION:
 *
 * Return Value:
 *
 * Input Parameters:
 *
 * Output Parameters:
 ******************************************************************************/
timerEntry_t * pltfrm_timer_create_TimerEntry(int tmoMsecs,
      timerCbFunc_t cbFunc_p,
      void *param1_p)

{
   timerEntry_t *newEntry_p;
   pltfrmTimerHndl_t newHndl;

   // get handle
   pthread_mutex_lock(&timerMutex);

   newHndl = timerEntryHndl ++;

   if (timerEntryHndl == 0)
   {
      timerEntryHndl = 1;
   }

   pthread_mutex_unlock(&timerMutex);

   //syslog(LOG_INFO, "<%s> hndl<%u>/tmoMsecs<%d>", __FUNCTION__, newHndl, tmoMsecs);

   if (tmoMsecs == 0 || cbFunc_p == NULL)
   {
      assert(0);
      return NULL;
   }

   newEntry_p = (timerEntry_t *)malloc(sizeof(timerEntry_t));

   if (newEntry_p == NULL)
   {
      assert(0);
      return NULL;
   }

   newEntry_p->cbFunc_p = cbFunc_p;
   newEntry_p->param1_p = param1_p;

   newEntry_p->hndl = newHndl;
   newEntry_p->tmoMsecs = tmoMsecs;
   newEntry_p->type = PLTFRM_TIMER_TYPE_UIP_TIMER;
   newEntry_p->next_p = NULL;

   // save the timer entry by caller
   // newEntry_p->param2_p = newEntry_p;

   return newEntry_p;
}
/******************************************************************************
 * FUNCTION NAME:
 *
 * DESCRIPTION:
 *
 * Return Value:
 *
 * Input Parameters:
 *
 * Output Parameters:
 ******************************************************************************/
void pltfrm_timer_list_del_req_hndlr(pltfrmTimerHndl_t hndl)
{
   //syslog(LOG_INFO, "<%s> hndl<%d> \n", __FUNCTION__, hndl);

   if (hndl > 0)
   {
      timerEntry_t *entry_p = timerHead_p;
      timerEntry_t *prev_p = NULL;

      while (entry_p != NULL)
      {
         if (entry_p->hndl == hndl)
         {
            break;
         }

         prev_p = entry_p;
         entry_p = entry_p->next_p;
      }

      if (entry_p != NULL)
      {
         if (prev_p == NULL)
         {
            timerHead_p = entry_p->next_p;
         }
         else
         {
            prev_p->next_p = entry_p->next_p;
         }

         // don't free this object
      }
   }
   else
   {
      assert(0);
   }

   return;
}

/******************************************************************************
 * FUNCTION NAME:
 *
 * DESCRIPTION:
 *
 * Return Value:
 *
 * Input Parameters:
 *
 * Output Parameters:
 ******************************************************************************/
void pltfrm_timer_list_add_req_hndlr(pltfrmTimerHndl_t newHndl, timerEntry_t *newEntry_p)
{
   struct timespec timeSpec;
   int deltaSecs, deltaNSecs;
   int tmoMsecs = newEntry_p->tmoMsecs;

   //syslog(LOG_INFO, "<%s> hndl<%u>/tmoMsecs<%d>", __FUNCTION__, newHndl, tmoMsecs);

   newEntry_p->next_p = NULL;

   clock_gettime(CLOCK_MONOTONIC, &timeSpec);

   if (tmoMsecs >= 1000)
   {
      deltaSecs = tmoMsecs / 1000;
      deltaNSecs = (tmoMsecs - deltaSecs * 1000) * 1000000;
   }
   else
   {
      deltaSecs = 0;
      deltaNSecs = tmoMsecs * 1000000;
   }

   newEntry_p->expiry = timeSpec;
   newEntry_p->expiry.tv_sec += deltaSecs;
   newEntry_p->expiry.tv_nsec += deltaNSecs;

   if (newEntry_p->expiry.tv_nsec >= 1000000000)
   {
      newEntry_p->expiry.tv_nsec -= 1000000000;
      newEntry_p->expiry.tv_sec ++;
   }

   if (timerHead_p == NULL)
   {
      timerHead_p = newEntry_p;
   }
   else
   {
      timerEntry_t *entry_p = timerHead_p;
      timerEntry_t *prev_p = NULL;

      do
      {
         deltaSecs = newEntry_p->expiry.tv_sec - entry_p->expiry.tv_sec;
         deltaNSecs = newEntry_p->expiry.tv_nsec - entry_p->expiry.tv_nsec;

         if (deltaSecs < 0 || ((deltaSecs == 0) && (deltaNSecs <= 0)))
         {
            break;
         }
         else
         {
            prev_p = entry_p;
            entry_p = entry_p->next_p;
         }
      }
      while (entry_p != NULL);

      if (entry_p == NULL)
      {
         prev_p->next_p = newEntry_p;
      }
      else
      {
         if (prev_p == NULL)
         {
            timerHead_p = newEntry_p;
         }
         else
         {
            prev_p->next_p = newEntry_p;
         }

         newEntry_p->next_p = entry_p;
      }
   }

   return;
}
/******************************************************************************
 * FUNCTION NAME:
 *
 * DESCRIPTION:
 *
 * Return Value:
 *
 * Input Parameters:
 *
 * Output Parameters:
 ******************************************************************************/
void pltfrm_timer_list_tmo_hndlr()
{
   struct timespec timeSpec;
   struct timespec tmo;

   while (timerHead_p != NULL)
   {
      int deltaSecs, deltaNSecs;

      clock_gettime(CLOCK_MONOTONIC, &timeSpec);

      deltaSecs = timerHead_p->expiry.tv_sec - timeSpec.tv_sec;
      deltaNSecs = timerHead_p->expiry.tv_nsec - timeSpec.tv_nsec;

      //syslog(LOG_INFO, "<%s> deltaSecs<%d>/deltaNsecs<%d> \n", __FUNCTION__, deltaSecs, deltaNSecs);

      if (deltaSecs > 0 || (deltaSecs == 0 && deltaNSecs > 0))
      {
         if (deltaNSecs < 0)
         {
            deltaSecs --;
            deltaNSecs += 1000000000;
         }

         tmo.tv_sec = deltaSecs;
         tmo.tv_nsec = deltaNSecs;
         break;
      }
      else
      {
         timerEntry_t *expEntry_p = timerHead_p;
         timerHead_p = timerHead_p->next_p;

         if (expEntry_p->cbFunc_p != NULL)
         {
            // syslog(LOG_INFO, "<%s> Timer expired ... (hndl<%u>/tmp<%u>\n",
            //        __FUNCTION__, expEntry_p->hndl, expEntry_p->tmoMsecs);
            (*expEntry_p->cbFunc_p)(expEntry_p->param1_p, expEntry_p->param2_p);
         }

         // else
         //    printf("\n <%s> cbFunc_p NULL ... \n", __FUNCTION__);

         if (expEntry_p->type == PLTFRM_TIMER_TYPE_PERIODIC)
         {
            // syslog(LOG_INFO, "<%s> Periodic Timer expired ... \n", __FUNCTION__);
            pltfrm_timer_add_req_hndlr(PLTFRM_TIMER_INVALID_HNDL,
                                       0, NULL,
                                       NULL, NULL,
                                       expEntry_p);
         }
         else if (expEntry_p->type == PLTFRM_TIMER_TYPE_ONE_SHOT)
         {
            free(expEntry_p);
         }
         else
         {
            // UIP timer, don't free the timer entry object
            //free(expEntry_p);
         }
      }
   }

   if (timerHead_p != NULL)
   {
      if (pltfrmTimerid != 0)
      {
         pltfrmTimerSpec.it_value = tmo;

         if (timer_settime(pltfrmTimerid, 0, &pltfrmTimerSpec, NULL) != 0)  //If the timer is already running it will be updated with the new time
         {
            syslog(LOG_INFO, "<%s> settime() failed", __FUNCTION__);
         }
         else
         {
            syslog(LOG_INFO, "<%s> timer started: %d sec", __FUNCTION__, (int)(pltfrmTimerSpec.it_value.tv_sec));
         }
      }
   }

   return;
}
/******************************************************************************
 * FUNCTION NAME:
 *
 * DESCRIPTION:
 *
 * Return Value:
 *
 * Input Parameters:
 *
 * Output Parameters:
 ******************************************************************************/
pltfrmTimerHndl_t pltfrm_timer_entry_add(pltfrmTimerHndl_t hndl,  void *param2_p)
{
   timerReq_t tmrReq;

   tmrReq.type = PLTFRM_TIMER_REQ_ADD_ENTRY;

   tmrReq.hndl = hndl;

   tmrReq.param2_p = param2_p;

   //syslog(LOG_INFO, "<%s> hndl<%u> ", __FUNCTION__, tmrReq.hndl);

   if (pltfrm_mbx_send(&timerMbx, (unsigned char *)&tmrReq, PLTFRM_MBX_OPN_NO_WAIT) <= 0)
   {
      return (pltfrmTimerHndl_t)PLTFRM_TIMER_INVALID_HNDL;
   }

   return tmrReq.hndl;
}
/******************************************************************************
 * FUNCTION NAME:
 *
 * DESCRIPTION:
 *
 * Return Value:
 *
 * Input Parameters:
 *
 * Output Parameters:
 ******************************************************************************/
int pltfrm_timer_entry_del(pltfrmTimerHndl_t hndl)

{
   int rc = 0;
   timerReq_t tmrReq;

   //syslog(LOG_INFO, "<%s> hndl<%d> \n", __FUNCTION__, hndl);

   if (hndl == PLTFRM_TIMER_INVALID_HNDL)
   {
      assert(0);
      rc = -1;
   }
   else
   {
      tmrReq.type = PLTFRM_TIMER_REQ_DEL_ENTRY;
      tmrReq.hndl = hndl;

      if (pltfrm_mbx_send(&timerMbx, (unsigned char *)&tmrReq, PLTFRM_MBX_OPN_NO_WAIT) <= 0)
      {
         rc = -1;
      }
   }

   return rc;
}

/******************************************************************************
 * FUNCTION NAME:
 *
 * DESCRIPTION:
 *
 * Return Value:
 *
 * Input Parameters:
 *
 * Output Parameters:
 ******************************************************************************/
// tick in ms???
uint32_t pltfrm_getTimeStamp(void)
{
   struct timeval currTime;
   uint32_t ts, ts1;

   gettimeofday(&currTime, NULL);

   ts = currTime.tv_sec * 1000;
   ts1 = currTime.tv_usec / 1000;

   ts = ts + ts1;

   syslog(LOG_INFO, "<%s> tick (%x) in ms", __FUNCTION__, ts);

   return ts;
}

