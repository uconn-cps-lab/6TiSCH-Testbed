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
#include <util.h>
#include <execinfo.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <proj_assert.h>

#define STACK_TRACE_BUFF_LEN  256 // Can store upto 64 4 byte pointers
static UINT8 UTIL_stackTraceBuff[STACK_TRACE_BUFF_LEN];

/***************************************************************************//**
 * @fn          UTIL_genStackTrace
 *
 * @brief       Generate stack trace
 *
 * @param[in]   None
 *
 * @param[out]  None
 *
 * @return      None
 *
 ******************************************************************************/
void UTIL_genStackTrace(void)
{
   SINT8 **funcNames_pp;
   SINT32 cnt, ttyFlag = isatty(STDOUT_FILENO);

   cnt = backtrace((void **)UTIL_stackTraceBuff, STACK_TRACE_BUFF_LEN / (sizeof(void *)));

   if (cnt > 0)
   {
      funcNames_pp = backtrace_symbols((void * const *)UTIL_stackTraceBuff, cnt);

      if (funcNames_pp != NULL)
      {
         SINT32 idx;
         for (idx = 1; idx < cnt; idx++)
         {
            if (ttyFlag)
            {
               printf("%s - %u : %s\n", __FUNCTION__, idx, funcNames_pp[idx]);
            }

         }
         free (funcNames_pp);
      }
   }

   return;
}

/***************************************************************************//**
 * @fn          UTIL_readTwoBytes
 *
 * @brief       Read two bytes value
 *
 * @param[in]   buff_p - Start memory address before read
 *
 * @param[out]  val_p - Read Value
 *
 * @return      Start memory address after read
 *
 ******************************************************************************/
UINT8 *UTIL_readTwoBytes(UINT8 *buff_p, UINT16 *val_p)
{
  proj_assert(buff_p != NULL && val_p != NULL);
  (*val_p) = buff_p[1];
  (*val_p) = ((*val_p) << 8) & 0xff00;
  (*val_p) |= buff_p[0];
  return (buff_p + sizeof(UINT16));
}

/***************************************************************************//**
 * @fn          UTIL_readFourBytes
 *
 * @brief       Read four bytes value
 *
 * @param[in]   buff_p - Start memory address before read
 *
 * @param[out]  val_p - Read Value
 *
 * @return      Start memory address after read
 *
 ******************************************************************************/
UINT8 *UTIL_readFourBytes(UINT8 *buff_p, UINT32 *val_p)
{
  proj_assert(buff_p != NULL && val_p != NULL);
  (*val_p) = buff_p[3];
  (*val_p) = ((*val_p) << 8) & 0xff00;
  (*val_p) |= buff_p[2];
  (*val_p) = ((*val_p) << 8) & 0xffff00;
  (*val_p) |= buff_p[1];
  (*val_p) = ((*val_p) << 8) & 0xffffff00;
  (*val_p) |= buff_p[0];
  return (buff_p + sizeof(UINT32));
}