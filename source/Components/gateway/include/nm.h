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

#ifndef __NM_H__
#define __NM_H__
#include "net/uip.h"

#define NM_PORT 40000
typedef struct
{
   uint16_t shortAddr;
   uint16_t coordAddr;
   uint8_t eui64[8];
} NM_schedule_request_t;

typedef struct
{
   uint16_t slotOffset;
   uint16_t channelOffset;
   uint8_t period;
   uint8_t offset;
   uint8_t linkOption;
} NM_schedule_cell_t;

typedef struct
{
   uint8_t status;
   uint8_t num_cells;
   uint16_t suggestion;
   uint16_t parentShortAddr;
   uint16_t childShortAddr;
   uint16_t nextHopShortAddr;
   NM_schedule_cell_t *cell_list;
} NM_schedule_response_t;

extern int nm_sock;
int NM_init();
uint16_t NM_requestGatewayAddress();
void NM_requestNodeAddress(NM_schedule_request_t* req);
void NM_schedule(NM_schedule_request_t* req, NM_schedule_response_t* res);
void NM_report_dao(uip_ipaddr_t* transit_addr, uip_ipaddr_t* target_addr, uint8_t* target_flag, uint16_t lifetime);
void NM_report_dio();
void NM_report_asn(uint64_t);
void NM_set_minimal_config(COMM_setSysCfg_s*config);
#endif  //__UI_H__
