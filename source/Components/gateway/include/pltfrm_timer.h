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
#ifndef __PLTFRM_TIMER_H__
#define __PLTFRM_TIMER_H__

#include <sys/time.h>
#include <pltfrm_lib.h>

#define PLTFRM_TIMER_USES_MBX

#define PLTFRM_TIMER_TYPE_ONE_SHOT 0
#define PLTFRM_TIMER_TYPE_PERIODIC 1
#define PLTFRM_TIMER_TYPE_UIP_TIMER 2

#define PLTFRM_TIMER_REQ_START  0
#define PLTFRM_TIMER_REQ_STOP   1
#define PLTFRM_PERIODIC_TIMER_REQ_START  2
#define PLTFRM_TIMER_REQ_ADD_ENTRY       3
#define PLTFRM_TIMER_REQ_DEL_ENTRY       4
#define PLTFRM_TIMER_TMO                 5

#define PLTFRM_TIMER_TASK_COMM_PORT  20000

#define PLTFRM_TIMER_INVALID_HNDL 0x0

typedef void (*timerCbFunc_t)(void *param1_p, void *param2_p);

typedef unsigned int pltfrmTimerHndl_t;

typedef struct _timerEntry_
{
   unsigned char type;
   struct timespec expiry;
   unsigned int tmoMsecs;
   timerCbFunc_t cbFunc_p;
   void *param1_p;
   void *param2_p;
   pltfrmTimerHndl_t hndl;
   struct _timerEntry_ *next_p;
} timerEntry_t;

typedef struct _timerReq_
{
   unsigned char type;
   unsigned int tmoMsecs;
   pltfrmTimerHndl_t hndl;
   timerCbFunc_t cbFunc_p;
   void *param1_p;
   void *param2_p;
} timerReq_t;


extern int pltfrm_TIMER_init(void);

extern timerEntry_t * pltfrm_timer_create_TimerEntry(int tmoMsecs, timerCbFunc_t cbFunc_p, void *param1_p);
extern void pltfrm_timer_list_del_req_hndlr(pltfrmTimerHndl_t hndl);
extern void pltfrm_timer_list_add_req_hndlr(pltfrmTimerHndl_t newHndl, timerEntry_t *entry_p);
extern void pltfrm_timer_list_tmo_hndlr();

extern pltfrmTimerHndl_t pltfrm_timer_entry_add(pltfrmTimerHndl_t hndl,  void *param2_p);
extern int pltfrm_timer_entry_del(pltfrmTimerHndl_t hndl);

#endif
