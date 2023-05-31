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

/*******************************************************************************
*                                            INCLUDES
*******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <pthread.h>
#include <comm.h>
#include <pltfrm_mbx.h>
#include <proj_assert.h>
#include <crc16.h>
#include <util.h>
#include <time.h>

#include "uip_rpl_process.h"
#include "rime/rimeaddr.h"
#include "net/uip.h"
#include "net/packetbuf.h"

#include "mac-dep.h"
#include "bm_api.h"
#include "device.h"
#include "nm.h"

/* -----------------------------------------------------------------------------
*                                         DEFINE
* -----------------------------------------------------------------------------
*/
#define COMM_TX_MAC_INTF_MBX_MAX_MSG_SZ    128
#define COMM_TX_MAC_INTF_MBX_MAX_MSG_CNT   64
#define MAX_NUM_OF_PEND_TX_PACKETS           (12)  //JIRA74
#define EXP_NUM_OF_PEND_TX_PACKETS           (MAX_NUM_OF_PEND_TX_PACKETS - 4)  //JIRA74
#define COMM_PACKET_PACING_INTERVAL_IN_USEC  (500000)  //0.5 seconds, JIRA74
#define MAX_NUM_OF_QED_UIP_PACKETS           (3)
#define EXP_NUM_OF_QED_UIP_PACKETS           (1)
/* -----------------------------------------------------------------------------
*                                         Global Variables
* -----------------------------------------------------------------------------
*/
UINT8 uartName[32] = "/dev/ttyUSB1";
COMM_cntxt_s COMM_cntxt = { {uartName, B460800, -1, } };

pthread_t COMM_TX_threadInfo;
pthread_t COMM_RX_threadInfo;

pltfrm_mbx_t COMM_TX_macIntfMbx;

UINT8 COMM_TX_macIntfMbxMsgBuff[COMM_TX_MAC_INTF_MBX_MAX_MSG_SZ];

UINT8 COMM_RX_msgPyldBuff[COMM_MAX_MSG_PYLD_LEN];
UINT8 COMM_TX_hdrBuff[COMM_MAX_TX_HDR_BUFF_LEN];
UINT8 COMM_TX_getPibMsgPyldBuff[COMM_GET_PIB_MSG_PYLD_LEN];

const ulpsmacaddr_t ulpsmacaddr_broadcast = { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };

COMM_DBG_s Comm_Dbg;
int comm_tx_ready = 0, comm_rx_ready = 0;
uint8_t numberOfPendTxPackets = 0;  //JIRA74
extern void recv_cb(void);
extern void sent_cb(uint8_t status, uint8_t num_tx);

/***************************************************************************//**
 * @fn          COMM_waitCommThreadReady
 *
 * @brief       The function checks if COMM TX and RX threads are ready
 *
 * @param[in]   none
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_waitCommThreadReady(void)
{
  while (1)
  {
    if  ((comm_tx_ready == 1) && (comm_rx_ready == 1))
    {
      return;
    }
  }
}

/***************************************************************************//**
 * @fn          COMM_buildsendMsg
 *
 * @brief       The function builds the HCT message 
 *
 * @param[in]   hdrbuf_p - pointer to header buffer
 * @param[in]   msgType - message type
 * @param[in]   msgLen - message length
 * @param[in]   bd_p - pointer payload buffer 
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
SINT32 COMM_buildsendMsg(UINT8 *hdrbuf_p, UINT8 msgType, UINT16 msgLen, BM_BUF_t* bd_p)
{
  SINT32 rc;
  UINT8 *payld_p = NULL;
  UINT16 pyldLen = msgLen + COMM_MSG_HDR_CRC_LEN + COMM_MSG_PYLD_CRC_LEN;
    
  if ((hdrbuf_p == NULL) || (bd_p == NULL))
  {
    printf("%s: NULL pointer\n",__FUNCTION__);
    return -1;
  }
  
  payld_p = BM_getDataPtr(bd_p);

  ((COMM_msgHdr_s *) hdrbuf_p)->msgType = msgType;
  ((COMM_msgHdr_s *) hdrbuf_p)->org = 0x1;  // From Host
  ((COMM_msgHdr_s *) hdrbuf_p)->rpy = 0x0;  // Ack not needed
  ((COMM_msgHdr_s *) hdrbuf_p)->resv = 0x0;
  ((COMM_msgHdr_s *) hdrbuf_p)->seq = 0x0;
  ((COMM_msgHdr_s *) hdrbuf_p)->pyldLenLSB = pyldLen & 0xff;
  ((COMM_msgHdr_s *) hdrbuf_p)->pyldLenMSB = (pyldLen >> 8) & 0xff;
  
  // compute header CRC
  ((COMM_msgHdr_s *) hdrbuf_p)->msgHdrCRC16 = crc16_blockChecksum(hdrbuf_p, COMM_MSG_HDR_LEN);
  // compute payload CRC
  ((COMM_msgHdr_s *) hdrbuf_p)->msgPyldCRC16 = crc16_blockChecksum(payld_p, msgLen);
  // write header
  rc = UART_writePort(&COMM_cntxt.uartCntxt, COMM_TX_hdrBuff, sizeof(COMM_msgHdr_s));
  
  if (rc > 0)
  {
    // write payload
    rc = UART_writePort(&COMM_cntxt.uartCntxt, payld_p, msgLen);
  }
  
  
  if (rc <=0)
  {
   printf("%s: UART write error\n",__FUNCTION__);
  }
  
  // Free the buffer
  BM_free(bd_p);
 
  return rc;
}

/***************************************************************************//**
 * @fn          COMM_TX_setMacPibReqHndlr
 *
 * @brief       The function sends set MAC PIB request
 *
 * @param[in]   req_p - pointer to request struct
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_TX_setMacPibReqHndlr(COMM_setMacPib_s * req_p)
{
  UINT8 *buff_p = COMM_TX_hdrBuff;
  UINT16 i, msgLen = 0, numItems;
  BM_BUF_t *bd_p;
  
  if (req_p == NULL)
  {
    printf("%s: NULL pointer\n",__FUNCTION__);
    return;
  }
  
  numItems = req_p->numItems;
  
  if (numItems > COMM_MAX_NUM_MAC_PIB)
  {
    printf("%s: Exceed maximum number of MAC PIB items\n",__FUNCTION__);
    return;
  }
  
  for (i = 0; i < numItems; i++)
  {
    msgLen += (req_p->attrLen[i] + 8);  // 8 bytes for attr ID, Index & Len
  }
  
  bd_p = BM_alloc();
  
  if (bd_p != NULL)
  {
    UINT8 *pyld_p = BM_getDataPtr(bd_p);
    UINT8 *ptr;
    UINT16 attrLen;
    
    ptr = pyld_p;
    
    for (i = 0; i < numItems; i++)
    {
      attrLen = req_p->attrLen[i];
      if (attrLen + 4 + 2 + 2 <= BM_BUFFER_SIZE - (ptr - pyld_p))
      {
        memcpy(ptr, &(req_p->attrId[i]), 4);  // copy attr ID
        ptr += 4;
        memcpy(ptr, &(req_p->attrIdx[i]), 2);  // copy attr Index
        ptr += 2;
        memcpy(ptr, &attrLen, 2);  // copy attr Len
        ptr += 2;
        memcpy(ptr, req_p->attrVal_p[i], attrLen);  // copy attr Value  
        ptr += attrLen;
      }
      else
      {
        printf("%s: buffer overflow\n", __FUNCTION__);
        return;
      }
    }
 
    BM_setDataLen(bd_p, msgLen);
    if (COMM_buildsendMsg(buff_p, COMM_MSG_TYPE_SET_MAC_PIB, msgLen, bd_p) <= 0)
    {
      printf("%s: Set MAC PIB error\n", __FUNCTION__);
    }
  }
  else
  {
    Comm_Dbg.buffer_full++;
    printf("%s: BUFFER full (%d).\n",__FUNCTION__,Comm_Dbg.buffer_full);
  }
  
  return;
}

/***************************************************************************//**
 * @fn          COMM_TX_getMacPibReqHndlr
 *
 * @brief       The function sends get MAC PIB request
 *
 * @param[in]   req_p - pointer to request struct
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_TX_getMacPibReqHndlr(COMM_getMacPib_s * req_p)
{
  UINT8 *buff_p = COMM_TX_hdrBuff;
  UINT16 i, msgLen, numItems;
  BM_BUF_t *bd_p;
  BOOL startKeepAliveTimer = FALSE;
  
  if (req_p == NULL)
  {
    printf("%s: NULL pointer\n",__FUNCTION__);
    return;
  }
  
  numItems = req_p->numItems;
  
  if (numItems > COMM_MAX_NUM_MAC_PIB)
  {
    printf("%s: Exceed maximum number of MAC PIB items\n",__FUNCTION__);
    return;
  }
  
  msgLen = numItems * 6;  // attr ID = 4 bytes, attr Index = 2 bytes
  
  bd_p = BM_alloc();
  
  if (bd_p != NULL)
  {
    UINT8 *pyld_p = BM_getDataPtr(bd_p);
    UINT8 *ptr;
    
    ptr = pyld_p;
    
    for (i = 0; i < numItems; i++)
    {
      memcpy(ptr, &(req_p->attrId[i]), 4);  // copy attr ID
      ptr += 4;
      memcpy(ptr, &(req_p->attrIdx[i]), 2);  // copy attr Index
      ptr += 2;
    }
    
    // set up the BD length
    BM_setDataLen(bd_p, msgLen);
    if (COMM_buildsendMsg(buff_p, COMM_MSG_TYPE_GET_MAC_PIB, msgLen, bd_p) <= 0)
    {
      printf("%s: GET MAC PIB error\n", __FUNCTION__);
    }
  }
  else
  {
    Comm_Dbg.buffer_full++;
    printf("%s: BUFFER full (%d).\n",__FUNCTION__,Comm_Dbg.buffer_full);
  }
  
  return;
}

/***************************************************************************//**
 * @fn          COMM_TX_resetdevi
 *
 * @brief       The function resets the root node
 *
 * @param[in]   req_p - pointer to request struct
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_TX_resetdeviceReqHndlr(COMM_resetSys_s *req_p)
{
  UINT8 *buff_p = COMM_TX_hdrBuff;
  UINT16 msgLen;
  BM_BUF_t *bd_p;
  
  if (req_p == NULL)
  {
    printf("%s: NULL pointer\n",__FUNCTION__);
    return;
  }
  
  msgLen = sizeof(COMM_resetSys_s);
  
  bd_p = BM_alloc();
  
  if (bd_p != NULL)
  {
    UINT8 *pyld_p = BM_getDataPtr(bd_p);
    ((COMM_resetSys_s *)pyld_p)->rsttype = req_p->rsttype;
    
    BM_setDataLen(bd_p, msgLen);
    if (COMM_buildsendMsg(buff_p, COMM_MSG_TYPE_SHUTDOWN, msgLen, bd_p) <= 0)
    {
      printf("%s: Shut down error\n", __FUNCTION__);
    }
  }
  else
  {
    Comm_Dbg.buffer_full++;
    printf("%s: BUFFER full (%d).\n",__FUNCTION__,Comm_Dbg.buffer_full);
  }
  return;
}
/***************************************************************************//**
 * @fn          COMM_getMacPibRpy
 *
 * @brief       The function handles get MAC PIB request
 *
 * @param[in]    COMM_RX_msgPyldBuff_p- pointer to received message
 * @param[in]    msgLen- message length
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_getMacPibRpy(UINT8 * COMM_RX_msgPyldBuff_p, UINT32 msgLen)
{
  
  if (COMM_RX_msgPyldBuff_p == NULL)
  {
    printf("%s: NULL pointer\n",__FUNCTION__);
    return;
  }
  
  SINT32 idx;  
  /*
  * Format :
  * > status : 2 bytes
  * > attribute id : 4 bytes
  * > attribute index : 2 bytes
  * > attribute length : 2 bytes
  * > attribute value : n bytes
  */
  
  if (msgLen >= 10)
  {
    UINT8 *attrVal_p = NULL;
    UINT16 sts, attrIdx, attrValLen, u16AttrVal;
    UINT32 attrId, u32AttrVal;
    msgLen -= 10;
    
    COMM_RX_msgPyldBuff_p = UTIL_readTwoBytes(COMM_RX_msgPyldBuff_p, &sts);
    COMM_RX_msgPyldBuff_p = UTIL_readFourBytes(COMM_RX_msgPyldBuff_p, &attrId);
    COMM_RX_msgPyldBuff_p = UTIL_readTwoBytes(COMM_RX_msgPyldBuff_p, &attrIdx);
    COMM_RX_msgPyldBuff_p = UTIL_readTwoBytes(COMM_RX_msgPyldBuff_p, &attrValLen);
    
    if (msgLen < attrValLen)
    {
      return;
    }
    
    if (attrValLen > 0)
    {
      switch (attrValLen)
      {
      case 2:
        COMM_RX_msgPyldBuff_p = UTIL_readTwoBytes(COMM_RX_msgPyldBuff_p, &u16AttrVal);
        attrVal_p = (UINT8 *)&u16AttrVal;
        break;
      case 4:
        COMM_RX_msgPyldBuff_p = UTIL_readFourBytes(COMM_RX_msgPyldBuff_p, &u32AttrVal);
        attrVal_p = (UINT8 *)&u32AttrVal;
        break;
      default:
        attrVal_p = (UINT8 *)COMM_RX_msgPyldBuff_p;
        break;
      }
    }
    
    UIP_setMacPib(attrId, attrValLen, attrVal_p);
    DEV_getMACPib_Reply_Handler();
  }
}
/***************************************************************************//**
 * @fn          ulpsmacaddr_long_cmp
 *
 * @brief       This function compares two MAC address in long format
 *
 * @param[in]   addr1 - pointer to MAC address 1
 * @param[in]   addr2 - pointer to MAC address 2
 *
 * @param[out]  none
 *
 * @return      compare result
 *
 ******************************************************************************/
int ulpsmacaddr_long_cmp(const ulpsmacaddr_t *addr1, const ulpsmacaddr_t *addr2)
{
  uint8_t i;
  
  for(i = 0; i < ULPSMACADDR_LONG_SIZE; i++)
  {
    if(addr1->u8[i] != addr2->u8[i])
    {
      return 0;
    }
  }
  return 1;
}
/***************************************************************************//**
 * @fn          ulpsmacaddr_short_cmp
 *
 * @brief       This function compares two MAC address in short format
 *
 * @param[in]   addr1 - pointer to MAC address 1
 * @param[in]   addr2 - pointer to MAC address 2
 *
 * @param[out]  none
 *
 * @return      compare result
 *
 ******************************************************************************/
int ulpsmacaddr_short_cmp(const ulpsmacaddr_t *addr1, const ulpsmacaddr_t *addr2)
{
  uint8_t i;
  
  for(i = 0; i < ULPSMACADDR_SHORT_SIZE; i++)
  {
    if(addr1->u8[i] != addr2->u8[i])
    {
      return 0;
    }
  }
  
  return 1;
}
/***************************************************************************//**
 * @fn          COMM_TX_setSysInfoReqHndlr
 *
 * @brief       This function sends system configuration to root node
 *
 * @param[in]   req_p - pointer to system configuration 
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_TX_setSysInfoReqHndlr(COMM_setSysCfg_s * req_p)
{
  UINT8 *buff_p = COMM_TX_hdrBuff;
  UINT16 msgLen;
  BM_BUF_t *bd_p;
  COMM_setSysCfgMsg_s *pMsg;
  
  if (req_p == NULL)
  {
    printf("%s: NULL pointer\n",__FUNCTION__);
    return;
  }
  
  msgLen = sizeof(COMM_setSysCfgMsg_s);

  bd_p = BM_alloc();
  
  if (bd_p != NULL)
  {
    UINT8 *pyld_p = BM_getDataPtr(bd_p);
    
    memset(pyld_p, 0, msgLen);
    
    pMsg = (COMM_setSysCfgMsg_s *)pyld_p;
    
    pMsg->type_slot_frame_size = HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_TYPE_SLOT_FRAME_SIZE;
    pMsg->len_slot_frame_size = HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_LEN_SLOT_FRAME_SIZE;
    
    pMsg->value_slot_frame_size = req_p->slot_frame_size;
    pMsg->type_numbber_shared_slots = HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_TYPE_NUMBER_SHARED_SLOT;
    pMsg->len_numbber_shared_slots = HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_LEN_NUMBER_SHARED_SLOT;
    pMsg->value_numbber_shared_slots = req_p->number_shared_slot;
    
    pMsg->type_beacon_freq = HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_TYPE_BEACON_FREQ;
    pMsg->len_beacon_freq = HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_LEN_BEACON_FREQ;
    pMsg->value_beacon_freq = req_p->beacon_freq;
    
    pMsg->type_gateway_address = HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_TYPE_GATEWAY_ADDRESS;
    pMsg->len_gateway_address = HCT_MSG_REQ_LOAD_SYSTEM_CONFIG_LEN_GATEWAY_ADDRESS;
    pMsg->value_gateway_address = req_p->gateway_address;

    if (COMM_buildsendMsg(buff_p, COMM_MSG_TYPE_LOAD_SYS_CFG, msgLen, bd_p)<0)
    {
    	printf("%s: Load system configuration error.\n",__FUNCTION__);
    }
  }
  else
  {
    Comm_Dbg.buffer_full++;
    printf("%s: BUFFER full (%d).\n",__FUNCTION__,Comm_Dbg.buffer_full);
  } 
}

/***************************************************************************//**
 * @fn          COMM_TX_macDataReqHndlr
 *
 * @brief       This function handles data request
 *
 * @param[in]    req_p- pointer to data request struct
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_TX_macDataReqHndlr(COMM_dataReq_s * req_p)
{
  UINT8 *buff_p = COMM_TX_hdrBuff;
  UINT16 msgLen;
  BM_BUF_t *bd_p;
  uint8_t temp;
  uint16_t copy_len;
  tschDataReq_t *pdataReq;
  
  pdataReq = &(req_p->dataReq);
  
  if (req_p->pMsdu == NULL)
  {
    printf("%s: NULL pointer\n",__FUNCTION__);
    return;
  }
  
  bd_p = BM_alloc();
  
  if (bd_p != NULL)
  {
    UINT8 *pyld_p = BM_getDataPtr(bd_p);
    UINT8 *pbase = pyld_p;
    
    msgLen = BM_BUFFER_SIZE;
    
    memset(pyld_p, 0, msgLen);
    
    // HW platform
    *pyld_p++ = 0;
    // MAC protocol
    *pyld_p++ = 0;
    // address mode
    temp = (pdataReq->dstAddrMode & 0x3) | ( (pdataReq->srcAddrMode & 0x3) << 4);
    *pyld_p++ = temp;
    // destination is always 8 bytes
    copy_len = 8;
    memcpy(pyld_p, &(pdataReq->dstAddr), copy_len);
    pyld_p += copy_len;
    //copy PAN
    memcpy(pyld_p, &(pdataReq->dstPanId), 2);
    pyld_p += 2;
    // msdu
    *pyld_p++ = pdataReq->mdsuHandle;
    // tx option
    *pyld_p++ = pdataReq->txOptions;
    //tx power
    *pyld_p++ = pdataReq->power;
    // security level 
    *pyld_p++ = 0;
    // skip keyIDmode, keyIndex, keySource frame counter supp, frame counter
    pyld_p += 12;
    //no IE, no seq compr, no PANID supp
    *pyld_p++ = 0;
    // mac payload length
    copy_len = BM_getDataLen(req_p->pMsdu);
  
    memcpy(pyld_p, &copy_len, 2);
    pyld_p += 2;
    
    // copy the MAC PSD
    if (copy_len + (pyld_p - pbase) <= BM_BUFFER_SIZE)
    {
      BM_copyto(req_p->pMsdu, pyld_p); 
      pyld_p += copy_len;
    }
    else
    {
      printf("%s: Copy len %d is larger than buffer size\n",__FUNCTION__,copy_len + (pyld_p - pbase));
      BM_free(req_p->pMsdu);
      return;
    }
    
    // free MSDU
    BM_free(req_p->pMsdu);
   
    msgLen = pyld_p - BM_getDataPtr(bd_p);
    BM_setDataLen(bd_p, msgLen);
    
    if (COMM_buildsendMsg(buff_p, COMM_MSG_TYPE_DATA_TRANSFER, msgLen, bd_p) <= 0)
    {
      printf("%s: data request error\n", __FUNCTION__);
    }
    else
    {
       numberOfPendTxPackets++;
    }
  }
  else
  {
    Comm_Dbg.buffer_full++;
    printf("%s: BUFFER full (%d).\n",__FUNCTION__,Comm_Dbg.buffer_full);
  }
 
  return;
}
/***************************************************************************//**
 * @fn          OMM_processRXDataConfirmMsg
 *
 * @brief       This function handles data confirmation
 *
 * @param[in]    COMM_RX_msgPyldBuff_p- pointer to received message
 * @param[in]    msgLen- message length
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_processRXDataConfirmMsg(UINT8 * COMM_RX_msgPyldBuff_p, UINT32 msgLen)
{
  uint8_t *ptemp;
  uint8_t status, msdu_hnd, num_tx;
  
  ptemp = COMM_RX_msgPyldBuff_p;
  // ignore the HW platfrom,MAC protocol, event ID
  ptemp += 3;
  
  status = *ptemp++;
  msdu_hnd = *ptemp++;
  num_tx = *ptemp++;
  numberOfPendTxPackets = *ptemp++; //JIRA74

  sent_cb(status, num_tx);
}

/***************************************************************************//**
 * @fn          COMM_processRXDataIndicationMsg
 *
 * @brief       This function handles data indication 
 *
 * @param[in]    COMM_RX_msgPyldBuff_p- pointer to received message
 * @param[in]    msgLen- message length
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_processRXDataIndicationMsg(UINT8 * COMM_RX_msgPyldBuff_p, UINT32 msgLen)
{
  uint8_t *ptemp;
  uint8_t status;
  uint8_t *ppayload, pkt_len;
  ulpsmacaddr_t ulpsmac_addr_dest, ulpsmac_addr_src;
  uint16_t src_panID, dest_panID;
  uint8_t ie_include, addr_len, dst_is_bcast;
  uint8_t temp, srcAddrMode, destAddrMode;
  rimeaddr_t tmp_rime_addr;
  
  ptemp = COMM_RX_msgPyldBuff_p;
  // ignore the HW platfrom,MAC protocol, event ID
  ptemp += 3;
  status = *ptemp++;
  // source/dest address mode
  temp = *ptemp++;
  srcAddrMode = (temp >> 4) & 0x03;
  destAddrMode = temp & 0x03;
  // get src and destination address
  memcpy(ulpsmac_addr_src.u8, ptemp, ULPSMACADDR_LONG_SIZE);
  ptemp += ULPSMACADDR_LONG_SIZE;
  memcpy(ulpsmac_addr_dest.u8, ptemp, ULPSMACADDR_LONG_SIZE);
  ptemp += ULPSMACADDR_LONG_SIZE;
  // ignore timestamp
  ptemp += 4;
  // get src PAN ID
  memcpy(&src_panID, ptemp, 2);
  ptemp += 2;
  //get dest PAN ID
  memcpy(&dest_panID, ptemp, 2);
  ptemp += 2;
  // ignore LQI,RSSI --> KeySource
  ptemp += 16;
  // get IE include (IE is not inlcude)
  ie_include = *ptemp++;
  // ignore IE length and IE content
  // get MAC paylaod length
  memcpy(&pkt_len, ptemp, 2);
  ptemp += 2;

  //extract ASN and report to NM
  uint64_t cur_asn=0;
  memcpy(&cur_asn, ptemp, 5);
  NM_report_asn(cur_asn);
  ptemp+=5;

  ppayload = ptemp;
  
  
  if (uipPakcets[uipPackets_WriteIdx].buf_len != 0)
  {
    //Packet FIFO overflow
    lowpanTxPacketLossCount++;
    printf("%s: 6lowpan TX overflow %d uipPackets_WriteIdx=%d\n",__FUNCTION__,lowpanTxPacketLossCount,uipPackets_WriteIdx);
  }
  else
  {
    // copy packet to packetbuf
    uipPakcets_lock(); 
    packetbuf_copyfrom(ppayload, pkt_len);
    ptemp+=pkt_len;
    
    memset(tmp_rime_addr.u8, 0x00, sizeof(rimeaddr_t));
    
    if (srcAddrMode == ULPSMACADDR_MODE_LONG)
    {
      addr_len = 8;
    }
    else if (srcAddrMode == ULPSMACADDR_MODE_SHORT)
    {
      addr_len = 2;
    }
    else
    {
      addr_len = 0;
    }
    
    pltfrm_byte_memcpy_swapped(&tmp_rime_addr, RIMEADDR_SIZE, ulpsmac_addr_src.u8, addr_len);
    packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &tmp_rime_addr);
    
    dst_is_bcast = 0;
    
    if ((destAddrMode == ULPSMACADDR_MODE_LONG)
        && ulpsmacaddr_long_cmp(&ulpsmac_addr_dest, &ulpsmacaddr_broadcast))
    {
      dst_is_bcast = 1;
      addr_len = 8;
    }
    else if ((destAddrMode == ULPSMACADDR_MODE_SHORT)
             && ulpsmacaddr_short_cmp(&ulpsmac_addr_dest, &ulpsmacaddr_broadcast))
    {
      dst_is_bcast = 1;
      addr_len = 2;
    }
    
    memset(tmp_rime_addr.u8, 0x00, sizeof(rimeaddr_t));
    
    if (!dst_is_bcast)
    {
      if (destAddrMode == ULPSMACADDR_MODE_LONG)
      {
        addr_len = 8;
      }
      else if (destAddrMode == ULPSMACADDR_MODE_SHORT)
      {
        addr_len = 2;
      }
      else
      {
        addr_len = 0;
      }
      
      memcpy(tmp_rime_addr.u8, ulpsmac_addr_dest.u8, addr_len);
    }

    pltfrm_byte_memcpy_swapped(&tmp_rime_addr, RIMEADDR_SIZE, ulpsmac_addr_dest.u8, addr_len);
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &tmp_rime_addr);
    
    recv_cb();
    uipPakcets_unlock(); 
  }
}

/***************************************************************************//**
 * @fn          COMM_processRXDataTransferMsg
 *
 * @brief       This function checks RX data transfer types
 *
 * @param[in]    COMM_RX_msgPyldBuff_p- pointer to received message
 * @param[in]    msgLen- message length
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_processRXDataTransferMsg(UINT8 * COMM_RX_msgPyldBuff_p,
                                   UINT32 msgLen)
{
  uint8_t *ptemp;
  uint8_t eventID;

  ptemp = COMM_RX_msgPyldBuff_p;  
  // ignore the HW platfrom and MAC protocol
  ptemp += 2;
  // get evnt ID
  eventID = *ptemp++;
  
  if (eventID == 0x01)
  {
    COMM_processRXDataConfirmMsg(COMM_RX_msgPyldBuff_p, msgLen);
  }
  else
  {
    COMM_processRXDataIndicationMsg(COMM_RX_msgPyldBuff_p, msgLen);
  }
  
  return;
}

/***************************************************************************//**
 * @fn          COMM_processRxAttachMsg
 *
 * @brief       This function handles received attach event
 *
 * @param[in]    COMM_RX_msgPyldBuff_p- pointer to received message
 * @param[in]    msgLen- message length
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_processRxAttachMsg(UINT8 * COMM_RX_msgPyldBuff_p, UINT32 msgLen)
{
  uint8_t *ptemp;
  ulpsmacaddr_t ulpsmac_addr_src;
  uint16_t shortAddr;
  time_t ltime; 
  uint16_t eventId;
  
  ltime = time(NULL); 
  ptemp = COMM_RX_msgPyldBuff_p;
  // get eventID (2 bytes)
  memcpy(&eventId, ptemp, 2);
  ptemp += 2;
  // get device EUI
  memcpy(ulpsmac_addr_src.u8, ptemp, 8);
  ptemp += 8;
  // get short address
  memcpy(&shortAddr, ptemp, 2);
  ptemp += 2;
  
  printf("** Association Response ACK is received for node (0x%04x), EUI(0x%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x) at %s", shortAddr,
         ulpsmac_addr_src.u8[7], ulpsmac_addr_src.u8[6], ulpsmac_addr_src.u8[5], ulpsmac_addr_src.u8[4],
         ulpsmac_addr_src.u8[3], ulpsmac_addr_src.u8[2], ulpsmac_addr_src.u8[1], ulpsmac_addr_src.u8[0],
         asctime( localtime(&ltime) ));
  
  //UIP_resetDIOTime();
  //moved to Response Sending so we check the response message (reduce unnecessary DIOs)
  
  return;
}

/***************************************************************************//**
 * @fn          COMM_processRxScheduleMsg
 *
 * @brief       This function handles the static scheduler request from RX thread
 *
 * @param[in]    COMM_RX_msgPyldBuff_p- pointer to scheduler struct
 * @param[in]    msgLen- message length
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_processRxScheduleMsg(UINT8 * COMM_RX_msgPyldBuff_p, UINT32 msgLen)
{
  COMM_TX_msg_s msg;
  uint8_t *ptemp;
  ulpsmacaddr_t ulpsmac_addr_src;
  uint16_t shortAddr, coordAddr, nextHopAddr;
  time_t ltime; 
  uint16_t eventId;
  BM_BUF_t *bd_p;
  
  /* get current cal time */
  ltime = time(NULL); 
  
  ptemp = COMM_RX_msgPyldBuff_p;

  // get device EUI
  memcpy(ulpsmac_addr_src.u8, ptemp, 8);
  ptemp += 8;
  memcpy(&shortAddr, ptemp, 2);
  ptemp += 2;
  memcpy(&coordAddr, ptemp, 2);
  ptemp += 2;
  memcpy(&nextHopAddr, ptemp, 2);
  ptemp += 2;

  printf("-- Association Indication is received for node(0x%04x), EUI(%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x) via parent(0x%04x) at %s", shortAddr,
         ulpsmac_addr_src.u8[7], ulpsmac_addr_src.u8[6], ulpsmac_addr_src.u8[5], ulpsmac_addr_src.u8[4],
         ulpsmac_addr_src.u8[3], ulpsmac_addr_src.u8[2], ulpsmac_addr_src.u8[1], ulpsmac_addr_src.u8[0],
         coordAddr, asctime(localtime(&ltime)));
  
  NM_schedule_request_t request;
  NM_schedule_cell_t cells[100];
  NM_schedule_response_t response={0,100,0,0,0,0,cells};
  request.shortAddr = shortAddr;
  request.coordAddr = coordAddr;
  
  
  for(int i = 0; i < 8; ++i)
  {
    request.eui64[i] = ulpsmac_addr_src.u8[8 - 1 - i];
  }
  
  NM_requestNodeAddress(&request);
  printf("-+-+Assigned node addr: %d\n", request.shortAddr);
  NM_schedule(&request, &response);
  response.parentShortAddr = coordAddr;
  response.childShortAddr = request.shortAddr;
  response.nextHopShortAddr = nextHopAddr;
  
  bd_p = BM_alloc();
  if (bd_p != NULL)
  {
    UINT8 *pyld_p = BM_getDataPtr(bd_p);
    UINT8 *ptr;
    ptr = pyld_p;
    
    memcpy(ptr, &(response.status),1);
    ptr += 1;
    memcpy(ptr, &(response.suggestion),2);
    ptr += 2;
    memcpy(ptr, &(response.num_cells), 1);
    ptr += 1;
    memcpy(ptr, &(response.parentShortAddr), 2);
    ptr += 2;
    memcpy(ptr, &(shortAddr), 2);
    ptr += 2;
    memcpy(ptr, &(response.childShortAddr), 2);
    ptr += 2;
    memcpy(ptr, &(response.nextHopShortAddr), 2);
    ptr += 2;
    
    for (int i = 0; i < response.num_cells; i++)
    {
      memcpy(ptr, &(response.cell_list[i].slotOffset), 2);
      ptr += 2;
      memcpy(ptr, &(response.cell_list[i].channelOffset), 2);
      ptr += 2;
      memcpy(ptr, &(response.cell_list[i].period), 1);
      ptr += 1;
      memcpy(ptr, &(response.cell_list[i].offset), 1);
      ptr += 1;
      memcpy(ptr, &(response.cell_list[i].linkOption), 1);
      ptr += 1;
    }
    
    BM_setDataLen(bd_p, ptr - pyld_p);
    msg.id = RX_SCHEDULE;
    msg.info.rxSched.buff_p = bd_p;
    msg.info.rxSched.msgLen = ptr - pyld_p;
    
    if (COMM_sendMsg(&msg) <= 0)
    {
      printf("%s: Failed to send schedule request\n", __FUNCTION__);
    }
    if(response.status==0){
      //only reset DIO if success
      UIP_resetDIOTime();
    }
  }
  else
  {
    Comm_Dbg.buffer_full++;
    printf("%s: BUFFER full (%d).\n",__FUNCTION__,Comm_Dbg.buffer_full);
  }
}

/***************************************************************************//**
 * @fn          COMM_TX_rxScheduleHndlr
 *
 * @brief       This function sends the static schedule to the root node
 *
 * @param[in]    rxShed_p - pointer to scheduler struct
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_TX_rxScheduleHndlr(COMM_rxSchedule_s *rxShed_p)
{
  int rc=-1;
  rc = COMM_buildsendMsg(COMM_TX_hdrBuff, 
                         COMM_MSG_TYPE_SCHEDULE, 
                         rxShed_p->msgLen, rxShed_p->buff_p);
  
  if (rc <=0 )
  {
     printf("%s: Failed to send static schedule.\n",__FUNCTION__);
  }
}

/***************************************************************************//**
 * @fn          COMM_TX_NHLScheduleHndlr
 *
 * @brief       This function handles dynamic schedule change
 *
 * @param[in]    NHLSched_p - pointer to scheduler struct
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_TX_NHLScheduleHndlr(COMM_NHLSchedule_s *NHLSched_p)
{
  UINT8 *buff_p = COMM_TX_hdrBuff;
  UINT16 msgLen = 8;
  BM_BUF_t *bd_p;
  
  bd_p = BM_alloc();
  
  if (bd_p != NULL)
  {
    UINT8 *pyld_p = BM_getDataPtr(bd_p);
    UINT8 *ptr;
    
    ptr = pyld_p;
    memcpy(ptr, &(NHLSched_p->operation), 1);
    ptr += 1;
    memcpy(ptr, &(NHLSched_p->slotOffset), 2);
    ptr += 2;
    memcpy(ptr, &(NHLSched_p->channelOffset), 2);
    ptr += 2;
    memcpy(ptr, &(NHLSched_p->linkOptions), 1);
    ptr += 1;
    memcpy(ptr, &(NHLSched_p->nodeAddr), 2);
    ptr += 2;
    
    BM_setDataLen(bd_p, msgLen);
    
    if (COMM_buildsendMsg(buff_p, COMM_MSG_TYPE_NHL_SCHEDULE, msgLen, bd_p) <=0)
    {
      printf("%s: Failed to send dynamic schedule message.\n",__FUNCTION__);
    }
  }
  else
  {
    Comm_Dbg.buffer_full++;
    printf("%s: BUFFER full (%d).\n",__FUNCTION__,Comm_Dbg.buffer_full);
  }
  
  return;
}

/***************************************************************************//**
 * @fn          COMM_processRcvdMsg
 *
 * @brief       This function checks the received message type and calls the 
 *              corresponding handler function.
 *
 * @param[in]    msgHdr_p - pointer to message handler
 * @param[in]    COMM_RX_msgPyldBuff_p - pointer to received message 
 * @param[in]    msgLen- message length
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void COMM_processRcvdMsg(COMM_msgHdr_s * msgHdr_p,
                         UINT8 * COMM_RX_msgPyldBuff_p, UINT32 msgLen)
{
  switch (msgHdr_p->msgType)
  {
  case COMM_MSG_TYPE_GET_MAC_PIB:
    COMM_getMacPibRpy(COMM_RX_msgPyldBuff_p, msgLen);
    break;
  case COMM_MSG_TYPE_DATA_TRANSFER:
    COMM_processRXDataTransferMsg(COMM_RX_msgPyldBuff_p, msgLen);
    break;
  case COMM_MSG_TYPE_LOAD_SYS_CFG:
    DEV_setSysConfig_Handler(COMM_RX_msgPyldBuff_p, msgLen);
    break;
  case COMM_MSG_TYPE_SHUTDOWN:
    DEV_shutDown_Handler(COMM_RX_msgPyldBuff_p, msgLen);
    break;
  case COMM_MSG_TYPE_ATTACH:
    COMM_processRxAttachMsg(COMM_RX_msgPyldBuff_p, msgLen);
    break;
  case COMM_MSG_TYPE_SCHEDULE:
    COMM_processRxScheduleMsg(COMM_RX_msgPyldBuff_p, msgLen);
    break;
  default:
    break;
  }
}

/***************************************************************************//**
 * @fn          COMM_rxThreadEntryFn
 *
 * @brief       COMM RX Thread Function 
 *
* @param[in]    param_p - initial function parameters
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void *COMM_rxThreadEntryFn(void *param_p)
{
  UART_flush(&COMM_cntxt.uartCntxt);
  comm_rx_ready = 1;
  
  while (1)
  {
    SINT32 rc, wc;
    COMM_msgHdr_s msgHdr;
    
    rc = UART_readPort(&COMM_cntxt.uartCntxt, (UINT8 *) &msgHdr,
                       sizeof(msgHdr));
     
    if (rc < 0)
    {
      UINT8 count = 0;
      sleep(1);
      printf("UART read error during message header.\n");
     
      while(UART_connectPort(&COMM_cntxt.uartCntxt) < 0) 
      {
        count++;
        if (count > 10)
        {
          printf("Open UART Failed!\n");
          proj_assert(0);
        }
        printf("Retrying to open UART port\n");
        sleep(10);
      }
      printf("UART reopen succesfully.\n");
    }
    else if (rc == sizeof(msgHdr))
    {
      UINT32 msgLen;

      if (msgHdr.msgHdrCRC16 == crc16_blockChecksum(&msgHdr, sizeof(msgHdr) - 4))
      {
        msgLen = msgHdr.pyldLenLSB + (msgHdr.pyldLenMSB << 8);
      
        // Msg Length field includes header CRC and payload CRC
        if (msgLen > (COMM_MSG_HDR_CRC_LEN + COMM_MSG_PYLD_CRC_LEN) 
            && msgLen < COMM_MAX_MSG_PYLD_LEN)
        {
          msgLen -= (COMM_MSG_HDR_CRC_LEN + COMM_MSG_PYLD_CRC_LEN);
       
          rc = UART_readPort(&COMM_cntxt.uartCntxt,COMM_RX_msgPyldBuff, msgLen);
          if (rc < 0)
          {
            UINT8 count = 0;
            sleep(1);
            printf("UART read error during message payload\n");
            while(UART_connectPort(&COMM_cntxt.uartCntxt) < 0) 
            {
              count++;
              if (count > 10)
              {
                printf("Open UART Failed!\n");
                proj_assert(0);
              }
              printf("Retrying to open UART port\n");
              sleep(10);
            }
            printf("UART reopen succesfully.\n");
          }
          else if (rc == msgLen)
          {
            if (msgHdr.msgPyldCRC16 == 0x0
                || msgHdr.msgPyldCRC16 == crc16_blockChecksum(COMM_RX_msgPyldBuff, msgLen))
            {
              COMM_processRcvdMsg(&msgHdr, COMM_RX_msgPyldBuff, msgLen);        
            }
            else
            {
              Comm_Dbg.err_hct_crc++;
              UINT16 * ptr = (UINT16*)&msgHdr;
              printf("%s: HCT payload has bad CRC (%d) %04x,%04x,%04x,%04x!\n", __FUNCTION__, Comm_Dbg.err_hct_crc,*ptr,*(ptr+1),*(ptr+2),*(ptr+3));
            }
          }
        }
      }
      else
      {
        Comm_Dbg.err_hct_crc++;
        UINT16 * ptr = (UINT16*)&msgHdr;
        printf("HCT header has bad CRC (%d) %04x,%04x,%04x,%04x\n",Comm_Dbg.err_hct_crc,*ptr,*(ptr+1),*(ptr+2),*(ptr+3));
        while(UART_connectPort(&COMM_cntxt.uartCntxt) < 0) 
        {
          printf("Retrying to open UART port\n");
          sleep(10);
        }
      }
    }
    else
    {
      /*Read Timeout*/
    }
  }
  
  return NULL;
}

/***************************************************************************//**
 * @fn          COMM_txThreadEntryFn
 *
 * @brief       COMM TX Thread Function 
 *
* @param[in]    param_p - initial function parameters
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void *COMM_txThreadEntryFn(void *param_p)
{
  SINT32 rc;
  
  comm_tx_ready = 1;
  
  while (1)
  {
    rc = pltfrm_mbx_pend(&COMM_TX_macIntfMbx, COMM_TX_macIntfMbxMsgBuff,
                         PLTFRM_MBX_OPN_WAIT_FOREVER);
    
    if (rc)
    {
      COMM_TX_msg_s *reqMsg_p =
        (COMM_TX_msg_s *) COMM_TX_macIntfMbxMsgBuff;
      
      switch (reqMsg_p->id)
      {
      case MAC_PIB_SET:
        {
          COMM_TX_setMacPibReqHndlr(&(reqMsg_p->info.setMacPib));
        }
        break;
      case MAC_PIB_GET:
        {
          COMM_TX_getMacPibReqHndlr(&(reqMsg_p->info.getMacPib));
        }
        break;
      case MAC_DATA_REQ:
        {
          COMM_TX_macDataReqHndlr(&(reqMsg_p->info.macDataReq));
        }
        break;
      case SET_SYS_INFO:
        {
          COMM_TX_setSysInfoReqHndlr(&(reqMsg_p->info.setSysCfg));
        }
        break;
      case RESET_DEVICE:
        {
          COMM_TX_resetdeviceReqHndlr(&(reqMsg_p->info.resetSys));
        }
        break;
      case RX_SCHEDULE:
        {
          COMM_TX_rxScheduleHndlr(&(reqMsg_p->info.rxSched));
        }
        break;
      case NHL_SCHEDULE:
        {
          COMM_TX_NHLScheduleHndlr(&(reqMsg_p->info.NHLSched));
        }
        break;  
      default:
        break;
      }
    }
    else
    {
       printf("%s: Pending error in COMM TX Thread\n",__FUNCTION__);
    }
  }
  
  return NULL;
}

/***************************************************************************//**
 * @fn          COMM_init
 *
 * @brief       This function creates COMM TX and RX threads
 *
* @param[in]    none
 *
 * @param[out]  none
 *
 * @return      Init Status
 *
 ******************************************************************************/
int COMM_init(void)
{
  int rc;
  
  printf("=== Waiting for port %s \n", COMM_cntxt.uartCntxt.commDevName_p);
  
  while(UART_connectPort(&COMM_cntxt.uartCntxt) < 0) 
  {
    printf("Retrying to open UART port\n");
    sleep(10);
  }
  
  if (pltfrm_mbx_init(&COMM_TX_macIntfMbx, COMM_TX_MAC_INTF_MBX_MAX_MSG_SZ,
                      COMM_TX_MAC_INTF_MBX_MAX_MSG_CNT) < 0)
  {
    printf("%s: Error initializing mailbox \n",__FUNCTION__);
    proj_assert(0);
  }
  
  rc = pthread_create(&COMM_TX_threadInfo, NULL, COMM_txThreadEntryFn, NULL);
  
  if (rc != 0)
  {
    printf("%s: Error creating COMM TX Thread \n",__FUNCTION__);
    proj_assert(0);
  }
  
  rc = pthread_create(&COMM_RX_threadInfo, NULL, COMM_rxThreadEntryFn, NULL);
  
  if (rc != 0)
  {
    printf("%s: Error creating COMM RX Thread \n",__FUNCTION__);
    proj_assert(0);
  }
  
  return 1;
}

/***************************************************************************//**
 * @fn          COMM_sendMsg
 *
 * @brief       This function sends message to COMM TX Thread
 *
* @param[in]    req_p - pointer to message
 *
 * @param[out]  none
 *
 * @return      send message status
 *
 ******************************************************************************/
int COMM_sendMsg(COMM_TX_msg_s * req_p)
{
  int rc;
  
  if (req_p != NULL)
  {
    rc = pltfrm_mbx_send(&COMM_TX_macIntfMbx,
                       (UINT8 *) req_p, PLTFRM_MBX_OPN_NO_WAIT);
  }
  else
  {
    printf("%s: NULL pointer\n",__FUNCTION__);
    rc = -1;
  }
  return rc;
}
/***************************************************************************//**
 * @fn          COMM_flowControl
 *
 * @brief       This function blocks the host thread for certain amount of time if
 *              there are too many TX packets in the root node yet to be sent.
 *
* @param[in]
 *
 * @param[out]
 *
 * @return
 *
 ******************************************************************************/
void COMM_flowControl()  //JIRA74
{
   if (numberOfPendTxPackets >= MAX_NUM_OF_PEND_TX_PACKETS ||
         uipBuffer_count() >= MAX_NUM_OF_QED_UIP_PACKETS)
   {
      while (numberOfPendTxPackets > EXP_NUM_OF_PEND_TX_PACKETS ||
            uipBuffer_count() > EXP_NUM_OF_QED_UIP_PACKETS)
      {
         //printf("******UART flow control: %d, %d\n", numberOfPendTxPackets, uipBuffer_count());
         usleep(COMM_PACKET_PACING_INTERVAL_IN_USEC);
      }
      //printf("******UART flow control done\n");
   }
}
