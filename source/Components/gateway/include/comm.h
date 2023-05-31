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
#ifndef __COMM_H__
#define __COMM_H__

#include <termios.h>
#include <unistd.h>
#include <project.h>
#include <pltfrm_timer.h>
#include <typedefs.h>
#include <uart.h>

#include "bm_api.h"

#define COMM_MAX_MSG_PYLD_LEN  4096
#define COMM_MAX_TX_HDR_BUFF_LEN  256
#define COMM_GET_PIB_MSG_PYLD_LEN 6

#define COMM_MSG_TYPE_DATA_TRANSFER    0x00
#define COMM_MSG_TYPE_GET_SYSTEM_INFO  0x01
#define COMM_MSG_TYPE_GET_PHY_PIB      0x02
#define COMM_MSG_TYPE_GET_MAC_PIB      0x03
#define COMM_MSG_TYPE_SET_INFO         0x04
#define COMM_MSG_TYPE_SHUTDOWN         0x05
#define COMM_MSG_TYPE_NW_START         0x08
#define COMM_MSG_TYPE_LOAD_SYS_CFG     0x0c
#define COMM_MSG_TYPE_SET_MAC_PIB      0x0d
#define COMM_MSG_TYPE_ATTACH           0x10
#define COMM_MSG_TYPE_DETACH           0x11
#define COMM_MSG_TYPE_SCHEDULE         0x16
#define COMM_MSG_TYPE_NHL_SCHEDULE     0x17

#define COMM_MSG_HDR_LEN        0x4
#define COMM_MSG_HDR_CRC_LEN    0x2
#define COMM_MSG_PYLD_CRC_LEN   0x2

#define ATTACH_INDICATION_EVENT_JOIN_FROM_LEAF                      (0)
#define ATTACH_INDICATION_EVENT_JOIN_FROM_INTERMEDIATE              (1)
#define ATTACH_INDICATION_EVENT_ASSO_REQ_FROM_LEAF                  (2)
#define ATTACH_INDICATION_EVENT_ASSO_REQ_FROM_INTERMEDIATE          (3)

/* HCT_MSG_TYPE_LOAD_SYSTEM_CONFIG */
#define HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_TYPE_SLOT_FRAME_SIZE                 0x0002
#define HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_LEN_SLOT_FRAME_SIZE                  0x0002

#define HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_TYPE_NUMBER_SHARED_SLOT              0x0003
#define HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_LEN_NUMBER_SHARED_SLOT               0x0001

#define HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_TYPE_BEACON_FREQ                     0x0001
#define HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_LEN_BEACON_FREQ                      0x0001

#define HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_TYPE_GATEWAY_ADDRESS                 0x0004
#define HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_LEN_GATEWAY_ADDRESS                  0x0002

#define COMM_MSG_PKT_OFFSET    28

#define COMM_DATA_TRANSFER_REQUEST_MSG      0x1
#define COMM_DATA_TRANSFER_CONFIRM_MSG      0x2
#define COMM_DATA_TRANSFER_INDICATION_MSG   0x3

#define COMM_DATA_TRANSFER_MSG_MODE_FIELD_SIZE   0x2

#define COMM_MAX_NUM_MAC_PIB  0x5
#define COMM_MAX_LENGTH_MAC_PIB     16

#define LMAC_DATA_REQ       0x01
#define GET_SYS_INFO        0x02
#define SET_SYS_INFO        0x03
#define SET_PHYTX_INFO      0x04
#define SET_PHYRX_INFO      0x05
#define RESET_DEVICE        0x06
#define MAC_DATA_REQ        0x07
#define START_NETWORK       0x08
#define MAC_PIB_SET         0x09
#define MAC_PIB_GET         0x0a
#define MAC_PIB_CLEAR       0x0b
#define SET_PHYTX_GAIN      0x0c
#define RX_SCHEDULE         0x0d
#define NHL_SCHEDULE        0x0e
#define INV_SYNC_MSG        0xff

#define MAC_KEY_SOURCE_MAX_LEN      8

typedef struct
{
  UART_cntxt_s uartCntxt;
} COMM_cntxt_s;

typedef struct
{
  UINT8 msgType;
  UINT8 seq: 4;
  UINT8 resv: 2;
  UINT8 rpy: 1;
  UINT8 org: 1;
  UINT8 pyldLenLSB;
  UINT8 pyldLenMSB;
  UINT16 msgHdrCRC16;
  UINT16 msgPyldCRC16;
} COMM_msgHdr_s;

typedef struct
{
  UINT16 numItems;
  UINT32 attrId[COMM_MAX_NUM_MAC_PIB];
  UINT16 attrIdx[COMM_MAX_NUM_MAC_PIB];
  UINT16 attrLen[COMM_MAX_NUM_MAC_PIB];
  UINT16 *attrVal_p[COMM_MAX_NUM_MAC_PIB];
} COMM_setMacPib_s;

typedef struct
{
  UINT16 numItems;
  UINT32 attrId[COMM_MAX_NUM_MAC_PIB];
  UINT16 attrIdx[COMM_MAX_NUM_MAC_PIB];
} COMM_getMacPib_s;

typedef struct
{
  UINT16 rsttype;
} COMM_resetSys_s;

typedef struct
{
  BM_BUF_t *buff_p;
  UINT32 msgLen;
} COMM_rxSchedule_s;

typedef struct
{
  UINT8 operation;
  UINT16 slotOffset;
  UINT16 channelOffset;
  UINT16 nodeAddr;
  UINT8 linkOptions;
} COMM_NHLSchedule_s;

/* Common security type */
typedef struct
{
  uint8_t   keySource[MAC_KEY_SOURCE_MAX_LEN];  /* Key source */
  uint8_t   securityLevel;                      /* Security level */
  uint8_t   keyIdMode;                          /* Key identifier mode */
  uint8_t   keyIndex;                           /* Key index */
  uint8_t   frameCounterSuppression;            /* frame cnt suppression */
  uint8_t   frameCounterSize;                   /* frame cnt size */
} tschSec_t;

#define ULPSMACADDR_LONG_SIZE   8    /* long address size */
#define ULPSMACADDR_SHORT_SIZE  2    /* short address size */
#define ULPSMACADDR_PANID_SIZE  2    /* pan id size */

#define ULPSMACADDR_MODE_NULL   0x00
#define ULPSMACADDR_MODE_SHORT  0x02
#define ULPSMACADDR_MODE_LONG   0x03

typedef union
{
  uint8_t u8[ULPSMACADDR_LONG_SIZE];
} ulpsmacaddr_t;

typedef struct
{
  uint8_t panIdSuppressed;     /* frameControlOptions */
  uint8_t iesIncluded;        /* frameControlOptions */
  uint8_t seqNumSuppressed;   /* frameControlOptions */
} tschFrmCtrlOpt_t;

typedef struct
{
  tschFrmCtrlOpt_t fco;
  ulpsmacaddr_t   dstAddr;
  uint16_t        dstPanId;
  uint8_t         srcAddrMode;
  uint8_t         dstAddrMode;
  uint8_t         mdsuHandle;
  uint8_t         txOptions;
  BM_BUF_t *      pIEList;
  uint8_t         power;
} tschDataReq_t;

typedef struct
{
  BM_BUF_t *      pMsdu;      /* buffer pointer to MSDU */
  tschSec_t       sec;        /* Security parameters, not support yet*/
  tschDataReq_t   dataReq;    /* Data request parameters*/
} COMM_dataReq_s;

typedef struct
{
  uint16_t slot_frame_size;
  uint8_t  number_shared_slot;
  uint16_t beacon_freq;
  uint16_t gateway_address;
} COMM_setSysCfg_s;

typedef struct
{
  UINT8 id;
  union
  {
    COMM_setMacPib_s    setMacPib;
    COMM_getMacPib_s    getMacPib;
    COMM_dataReq_s      macDataReq;
    COMM_setSysCfg_s    setSysCfg;
    COMM_resetSys_s     resetSys;
    COMM_rxSchedule_s   rxSched;
    COMM_NHLSchedule_s  NHLSched;
    UINT32 timerId;
  }
  info;
} COMM_TX_msg_s;

typedef struct
{
  UINT32  err_hct_crc;
  UINT32  buffer_full;
} COMM_DBG_s;

extern COMM_DBG_s Comm_Dbg;

// structs byte packed
#pragma pack(1)

typedef struct
{
  // in TLV format
  uint16_t  type_slot_frame_size;
  uint16_t  len_slot_frame_size;
  uint16_t   value_slot_frame_size;
  uint16_t  type_parent_id;
  uint16_t  len_parent_id;
  uint16_t  value_parent_id;
  uint16_t  type_numbber_shared_slots;
  uint16_t  len_numbber_shared_slots;
  uint8_t   value_numbber_shared_slots;
  uint16_t  type_beacon_freq;
  uint16_t  len_beacon_freq;
  uint8_t  value_beacon_freq;
  uint16_t  type_gateway_address;
  uint16_t  len_gateway_address;
  uint16_t  value_gateway_address;
} COMM_setSysCfgMsg_s;

// return to default packing
#pragma pack()

extern int COMM_init(void);
extern int COMM_sendMsg(COMM_TX_msg_s * req_p);
extern void COMM_waitCommThreadReady(void);
extern void COMM_flowControl();
#endif
