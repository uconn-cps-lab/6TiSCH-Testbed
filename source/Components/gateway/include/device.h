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
  PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <comm.h>

extern uint8_t *rootTschScheduleBuffer_p;
extern uint16_t rootTschScheduleBufferSize;
extern uint16_t rootTschScheduleLen;

extern void DEV_resetSys(void);
extern void DEV_setSysConfig(COMM_setSysCfg_s *psetSysCfg);
extern void DEV_setSysConfig_Handler(UINT8 *msg, UINT32 msglen);
extern void DEV_getMACPib(UINT32 attrId, UINT16 attrIdx);
extern void DEV_getMACPib_Reply_Handler(void);
extern void DEV_shutDown_Handler(UINT8 *msg, UINT32 msglen);
extern void DEV_setMACPib(UINT32 attrId, UINT16 attrIdx, uint8_t *val, uint16_t len); //fengmo NVM
extern void DEV_NHLScheduleRemoveSlot(uint16_t slotOffset, uint16_t node);
extern void DEV_NHLScheduleAddSlot(uint16_t slotOffset, uint16_t channelOffset, uint8_t linkOptions, uint16_t nodeAddr);
extern int  DEV_NHLScheduleGet(uint16_t startLinkIdx, uint8_t *buf, int bufSize);
#endif
