/*
* Copyright (c) 2006 -2014, Texas Instruments Incorporated
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
* *  Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
* *  Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* *  Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
*  ====================== hct_rx.c =============================================
*  This module contains HCT RX API.
*/

/* ------------------------------------------------------------------------------------------------
*                                          Includes
* ------------------------------------------------------------------------------------------------
*/
#include "hct.h"
#include "crc16.h"

#if defined (__IAR_SYSTEMS_ICC__) || defined(__TI_COMPILER_VERSION__)
#include <ti/sysbios/family/arm/m3/Hwi.h>
#else
#include <stdlib.h>
#endif

/**************************************************************************************************//**
*
* @brief       This function will send the HCT message to Host> inside this function, it will
*              generate the HCT message header, header CRC and payload CRC.
*              Since we don't have linked-list BD buffer, we will pass two BM_BUF to this function
*
*              The fisrt buffer will provide the message related header like data confirm, or data
*              indication parameters.
*
*              The second one will provide the data packet if there is one.
*
* @param[in]   pPkt - packet pointer
*
*
* @return      operation status
***************************************************************************************************/
int8_t HCT_RX_send(HCT_PKT_BUF_s *pPkt)
{
    uint16_t msghdr[(HCT_MSG_HDR_SIZE + HCT_MSG_EXT_HDR_SIZE) >> 1];
    uint16_t    msg_len;
    BM_BUF_t *pdata1,*pdata2;
    uint16_t msg_type_tag;
    uint8_t numBytes;
    uint8_t msgPayload[2*BM_BUFFER_SIZE];
    uint8_t* pMsgPayload = msgPayload;
    
    msg_type_tag = pPkt->msg_type;
    pdata1 = pPkt->pdata1;
    pdata2 = pPkt->pdata2;
    
    if (pdata1 == NULL)
    {  
        HCT_Hnd->num_rx_invalid_para++;
        if (pdata2)
        {
            BM_free_6tisch(pdata2);
        }
        return HCT_STATUS_ILLEGAL_PARAMS;
    }
    
    msg_len = pdata1->datalen;
    memcpy(pMsgPayload,BM_getDataPtr(pdata1),pdata1->datalen);
    pMsgPayload += pdata1->datalen;
    
    if (pdata2)
    {
        msg_len += pdata2->datalen;
        memcpy(pMsgPayload,BM_getDataPtr(pdata2),pdata2->datalen);
        pMsgPayload += pdata2->datalen;
    }
    
    msghdr[HCT_MSG_OFFSET_MSG_TYPE_TAG >> 1] = (msg_type_tag & HCT_MSG_TYPE_MASK) |
        (((msg_type_tag & HCT_MSG_TAG_MASK) & ~HCT_MSG_TAG_ORG_MASK) | HCT_MSG_TAG_ORG_MAC);
    // msg_len + 4 byte hdr size
    msghdr[HCT_MSG_OFFSET_MSG_LENGTH >> 1] = msg_len + HCT_MSG_EXT_HDR_SIZE;
    // compute the header CRC16
    msghdr[(HCT_MSG_HDR_SIZE + HCT_MSG_EXT_HDR_OFFSET_HEADER_CRC16) >> 1] = crc16_getCRC16(INIT_CRC16, (uint8_t *)&msghdr[0], HCT_MSG_HDR_SIZE);
    // compute the payload CRC16
    msghdr[(HCT_MSG_HDR_SIZE + HCT_MSG_EXT_HDR_OFFSET_PAYLOAD_CRC16) >> 1] = crc16_getCRC16(INIT_CRC16, (uint8_t *)&msgPayload, msg_len);
    
    // output packet HCT header
    numBytes = UART_write(HCT_Hnd->uart_hnd,msghdr,HCT_MSG_HDR_SIZE + HCT_MSG_EXT_HDR_SIZE);
    
    if (numBytes == (HCT_MSG_HDR_SIZE + HCT_MSG_EXT_HDR_SIZE))
    {
      // output the data
      numBytes = UART_write(HCT_Hnd->uart_hnd,&msgPayload,msg_len);
      if (numBytes != msg_len){
           HCT_Hnd->numUartWriteError++;
           UART_writeCancel(HCT_Hnd->uart_hnd);
      }
    }
    else
    {
      HCT_Hnd->numUartWriteError++;
      UART_writeCancel(HCT_Hnd->uart_hnd);
    }
    
    BM_free_6tisch(pdata1);
    if (pdata2)
    {
        BM_free_6tisch(pdata2);
    }
    
    HCT_Hnd->num_rx_to_host++;
    
    return HCT_STATUS_SUCCESS;
}
