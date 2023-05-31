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
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <pltfrm_mbx.h>
#include <syslog.h>
#include <sys/time.h>

int pltfrm_mbx_init(pltfrm_mbx_t *mbx_p, int msgSz, int maxMsgCnt)
{
   if (mbx_p == NULL || msgSz == 0 || maxMsgCnt == 0)
   {
      return -1;
   }

   mbx_p->mbxBuff_p = (unsigned char *)malloc(msgSz * maxMsgCnt);

   if (mbx_p->mbxBuff_p == NULL)
   {
      return -1;
   }

   mbx_p->msgSz = msgSz;
   mbx_p->maxMsgCnt = maxMsgCnt;
   mbx_p->msgCnt = 0;
   mbx_p->inIdx = mbx_p->outIdx = 0;

   pthread_mutex_init(&(mbx_p->mbxMutex), NULL);
   sem_init(&(mbx_p->mbxOutSem), 0, 0);

   return 1;
}

int pltfrm_mbx_send(pltfrm_mbx_t *mbx_p, unsigned char *msgBuff_p, int tmoMsecs)
{
   int rc = -1;

   if (mbx_p != NULL && msgBuff_p != NULL)
   {
      pthread_mutex_lock(&(mbx_p->mbxMutex));

      if (mbx_p->msgCnt >= mbx_p->maxMsgCnt)
      {
         pthread_mutex_unlock(&(mbx_p->mbxMutex));
         printf("Error: msgCnt: %d >= maxMsgCnt: %d\n", mbx_p->msgCnt, mbx_p->maxMsgCnt);
         rc = 0;
      }
      else
      {
         memcpy(mbx_p->mbxBuff_p + mbx_p->inIdx * mbx_p->msgSz,
                msgBuff_p,
                mbx_p->msgSz);
         mbx_p->msgCnt++;
         mbx_p->inIdx++;

         if (mbx_p->inIdx >= mbx_p->maxMsgCnt)
         {
            mbx_p->inIdx = 0;
         }

         pthread_mutex_unlock(&(mbx_p->mbxMutex));
         rc = 1;
      }

      sem_post(&(mbx_p->mbxOutSem));
   }

   return rc;
}

// Multiple writers, single reader
int pltfrm_mbx_pend(pltfrm_mbx_t *mbx_p, unsigned char *msgBuff_p, int tmoMsecs)
{
   int rc = -1;

   if (mbx_p != NULL && msgBuff_p != NULL)
   {
      do
      {
         int savedErrno;

         rc = sem_wait(&(mbx_p->mbxOutSem));
         savedErrno = errno;

         if (rc == 0)
         {
            pthread_mutex_lock(&(mbx_p->mbxMutex));

            assert(mbx_p->msgCnt > 0);

            memcpy(msgBuff_p,
                   (mbx_p->mbxBuff_p + (mbx_p->outIdx * mbx_p->msgSz)),
                   mbx_p->msgSz);

            mbx_p->msgCnt--;
            mbx_p->outIdx++;

            if (mbx_p->outIdx >= mbx_p->maxMsgCnt)
            {
               mbx_p->outIdx = 0;
            }

            pthread_mutex_unlock(&(mbx_p->mbxMutex));
            rc = 1;
         }
         else
         {
            if (rc > 0)
            {
               savedErrno = rc;
               rc = -1;
            }

            switch (savedErrno)
            {
               case ETIMEDOUT:
                  {
                     assert (tmoMsecs != PLTFRM_MBX_OPN_WAIT_FOREVER);
                     rc = 0;
                  }
                  break;

               case EINTR:
                  {
                     // syslog(LOG_INFO, "b <%s> eintr", __FUNCTION__);
                     // Call interrupted by signal handler
                  }
                  break;

               case EINVAL:
               default:
                  assert(0);
                  break;
            }
         }
      }
      while (rc < 0);
   }

   return rc;
}
