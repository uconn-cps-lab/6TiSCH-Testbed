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
 *  ====================== hct_msg_data.c =============================================
 *  This module contains HCT data message related API.
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */


#include "hct.h"

#include "nhl_api.h"
#include "mac_config.h"
#include "nhl.h"

#if defined (__IAR_SYSTEMS_ICC__) || defined(__TI_COMPILER_VERSION__)
#include <ti/sysbios/family/arm/m3/Hwi.h>
#else
#include <stdlib.h>
#endif
#include "nv_params.h"
#ifdef FEATURE_MAC_SECURITY
#include "mac_tsch_security.h"
#endif // FEATURE_MAC_SECURITY

extern uint16_t numberOfQueuedTxPackets;  //JIRA74

void HCT_tx_request (ulpsmacaddr_t *dest,uint16_t addrMode, BM_BUF_t *pdataBuf);
void HCT_preparePacket(NHL_McpsDataReq_t* nhlMcpsDataReq);

void HCT_MSG_cb(uint16_t hct_msg,BM_BUF_t *ppayload)
{
    NHLDB_TSCH_CMM_cmdmsg_t* cmm_msg;
    cmm_msg = NHLDB_TSCH_CMM_alloc_cmdmsg(2);
    bool status = false;
    
    if (cmm_msg)
    {
        uint32_t bufAddr;
        uint32_t ptrAddr;

        cmm_msg->type = hct_msg;
        cmm_msg->event_type = TASK_EVENT_HCT;
        bufAddr = (uint32_t)ppayload;

        memcpy(cmm_msg->arg,&bufAddr,sizeof(bufAddr));
        ptrAddr = (uint32_t)cmm_msg;

        status = SN_post(nhl_tsch_mailbox, &ptrAddr, BIOS_NO_WAIT,
                NOTIF_TYPE_MAILBOX);
        
        if (status == false)
        {
            NHLDB_TSCH_CMM_free_cmdmsg(cmm_msg);
            ULPSMAC_Dbg.numNhlEventPostError++;
        }
    }
        
    if (status == false && ppayload != NULL)
    {
      BM_free_6tisch(ppayload);
    }
}

void HCT_tx_request (ulpsmacaddr_t *dest,uint16_t addrMode, BM_BUF_t *pdataBuf)
{
    ulpsmacaddr_t ulpsmac_addr_dest;
    NHL_McpsDataReq_t nhlMcpsDataReq;
    NHL_DataReq_t*     nhlDataReq;

    // copy the address (don't swap, it is done in gw)
    memcpy(ulpsmac_addr_dest.u8,dest,sizeof(ulpsmacaddr_t));

    nhlDataReq = &nhlMcpsDataReq.dataReq;
    nhlDataReq->dstAddrMode = ulpsmacaddr_addr_mode(&ulpsmac_addr_dest);
    nhlDataReq->dstAddr = ulpsmac_addr_dest;
    nhlMcpsDataReq.pMsdu = BM_alloc(9);
    if (nhlMcpsDataReq.pMsdu)
    {
        BM_copyfrom(nhlMcpsDataReq.pMsdu,pdataBuf->buf_aligned,pdataBuf->datalen);

        nhlMcpsDataReq.pMsdu->coap_type = pdataBuf->coap_type; // jc

        HCT_preparePacket(&nhlMcpsDataReq);

        NHL_dataReq(&nhlMcpsDataReq,HCT_IF_MAC_data_conf_proc);
    }

     BM_free_6tisch(pdataBuf);
}

#if IS_ROOT
/**************************************************************************************************//**
 *
 * @brief       This function handle data conform message.  it is running in NHL task context.
 *
 * @param[in]   arg -- pointer to data conf argument
 *
 * @return
 *
 ***************************************************************************************************/
void HCT_IF_MAC_data_conf_proc(NHL_McpsDataConf_t *confArg)
{
    BM_BUF_t *pdata1;
    uint8_t *ptemp;
    uint16_t addr_len;
    HCT_PKT_BUF_s Pkt;

    HCT_Hnd->num_data_conf_to_host++;

    pdata1 = BM_alloc(4);
    if (pdata1 == NULL)
    {
        return;
    }

    // start to build the HCT data indication header
    ptemp = BM_getBufPtr(pdata1);

    // set up HW platform and MAC protocol
#ifndef CC1310F128
    *ptemp++ = HW_PLATFORM_CC26XX;
#else
    *ptemp++ = HW_PLATFORM_CC13XX;
#endif
    *ptemp++ = MAC_PROTOCOL_TSCH;

    // set up the data indcication event ID
    *ptemp++ = HCT_MSG_DATA_EVENT_ID_CONFIRM;

    // status
    *ptemp++ = confArg->hdr.status;

    // MSDU handle
    *ptemp++ = confArg->msduHandle;

    // numbackoffs
    *ptemp++ = confArg->numBackoffs;

    // Flow control   JIRA74
    *ptemp++ = (numberOfQueuedTxPackets < 256) ? numberOfQueuedTxPackets : 255;

    // compute data 1 size
    addr_len = ptemp - BM_getBufPtr(pdata1);
    BM_setDataLen(pdata1,addr_len);

    Pkt.pdata1 = pdata1;
    Pkt.pdata2 = NULL;
    Pkt.msg_type = HCT_MSG_TYPE_DATA_TRANSFER;
    Pkt.msg_len  = addr_len;

    // call send to host
    HCT_RX_send(&Pkt);
}

/**************************************************************************************************//**
 *
 * @brief       This function handle MAC data indication callback function. It is running in NHL
 *              task context.
 *              It will copy the input data to packetbuf which is used by application.
 *              It will add the Dat ainidcation interface
 *
 * @param       arg     -- the pointer to tschMcpsDataInd_t data structure
 *              buf     -- pointer to data buffer
 *              buf_len -- length of buffer data
 *
 *
 * @return
 *
 ***************************************************************************************************/
void NHL_dataIndCb(NHL_McpsDataInd_t* dataIndArg)
{
    NHL_DataInd_t *pMac;
    uint16_t addr_len;
    BM_BUF_t *pdata1, *pdata2;
    uint8_t *ptemp, *pbase;
    HCT_PKT_BUF_s Pkt;

    HCT_Hnd->num_data_ind_to_host++;

    pdata1 = BM_alloc(10);
    if (pdata1 == NULL)
    {
        BM_free_6tisch(dataIndArg->pMsdu);
        return;
    }

    pdata2 = BM_alloc(11);
    if (pdata2 == NULL)
    {
        BM_free_6tisch(dataIndArg->pMsdu);
        BM_free_6tisch(pdata1);
        return;
    }

    // copy the buffer data to data packet 2
    BM_copyfrom(pdata2,dataIndArg->pMsdu->buf_aligned,dataIndArg->pMsdu->datalen);

    // start to build the HCT data indication header
    pbase = ptemp = BM_getDataPtr(pdata1);

    // set up HW platform and MAC protocol
#ifndef CC1310F128
    *ptemp++ = HW_PLATFORM_CC26XX;
#else
    *ptemp++ = HW_PLATFORM_CC13XX;
#endif
    *ptemp++ = MAC_PROTOCOL_TSCH;

    // set up the data indcication event ID
    *ptemp++ = HCT_MSG_DATA_EVENT_ID_INDICATION;

    // set up status
    *ptemp++ = 0;

    pMac = &(dataIndArg->mac);
    // set up the MAC source and destination mode
    *ptemp++ = ((pMac->dstAddrMode & HCT_MSG_DATA_INDICATION_DST_ADDR_MODE_MASK) <<
                    HCT_MSG_DATA_INDICATION_DST_ADDR_MODE_SHIFT ) |
                ((pMac->srcAddrMode & HCT_MSG_DATA_INDICATION_SRC_ADDR_MODE_MASK) <<
                    HCT_MSG_DATA_INDICATION_SRC_ADDR_MODE_SHIFT );
    addr_len = 8;

    // source address (no swap)
    memcpy(ptemp,pMac->srcAddr.u8,addr_len);
    ptemp += addr_len;

    // destination address (no swap)
    memcpy(ptemp,pMac->dstAddr.u8,addr_len);
    ptemp += addr_len;

    // time stamp
    memcpy(ptemp,&(pMac->timestamp),sizeof(pMac->timestamp));
    ptemp += sizeof(pMac->timestamp);

    // source PAN ID
    memcpy(ptemp,&(pMac->srcPanId),sizeof(pMac->srcPanId));
    ptemp += sizeof(pMac->srcPanId);

    // destination PAN ID
    memcpy(ptemp,&(pMac->dstPanId),sizeof(pMac->dstPanId));
    ptemp += sizeof(pMac->dstPanId);

    // LQI
    *ptemp++ = pMac->mpduLinkQuality;

    // RSSI
    *ptemp++ = pMac->rssi;

    // DSN
    *ptemp++ = pMac->dsn;

    // secuirty level
    *ptemp++ = dataIndArg->sec.securityLevel;

    // keyID mode
    *ptemp++ = dataIndArg->sec.keyIdMode;

    // key Index
    *ptemp++ = dataIndArg->sec.keyIndex;

    // key source
    memcpy(ptemp,dataIndArg->sec.keySource, sizeof(dataIndArg->sec.keySource));
    ptemp += sizeof(dataIndArg->sec.keySource);

    //frame counter suppression
    *ptemp++ = dataIndArg->sec.frameCounterSuppression;

    // frame counter length
    *ptemp++ = dataIndArg->sec.frameCounterSize;

    // IE related
    // IE is not included
    pMac->fco.iesIncluded = 0;
    *ptemp++ = (pMac->fco.iesIncluded)<<2;

    if (pMac->fco.iesIncluded)
    {
        uint16_t ie_len;
        if (pMac->pIEList == NULL)
        {
            ie_len =0;
        }
        else
        {
            ie_len = BM_getDataLen(pMac->pIEList);
        }
        memcpy(ptemp, &ie_len,sizeof(ie_len));
        ptemp += sizeof(ie_len);

        if ((ie_len + sizeof(addr_len) + (ptemp - pbase)) <= BM_BUFFER_SIZE)
        {
           memcpy(ptemp,BM_getDataPtr(pMac->pIEList),ie_len);  //JIRA 44, done
           ptemp += ie_len;
        }
        else
        {
#if DEBUG_MEM_CORR
           volatile int errloop = 0;
           while(1)
           {
              errloop++;
           }
#else
           BM_free_6tisch(dataIndArg->pMsdu);
           BM_free_6tisch(pdata1);
           BM_free_6tisch(pdata2);
           return;
#endif
        }
    }

    // MAC payload length
    addr_len = dataIndArg->pMsdu->datalen;
    memcpy(ptemp, &addr_len, sizeof(addr_len));
    ptemp += sizeof(addr_len);

    // ASN append, only 5 bytes
    uint64_t cur_asn = TSCH_ACM_get_ASN();
    memcpy(ptemp, &cur_asn, 5);
    ptemp += 5;

    // compute data buffer 1 size
    addr_len = ptemp - BM_getDataPtr(pdata1);
    BM_setDataLen(pdata1,addr_len);

    Pkt.pdata1 = pdata1;
    Pkt.pdata2 = pdata2;
    Pkt.msg_type = HCT_MSG_TYPE_DATA_TRANSFER;
    Pkt.msg_len  = addr_len + BM_getDataLen(pdata2);


    // Freeing pMSDU
    BM_free_6tisch(dataIndArg->pMsdu);

    // call send to host
    HCT_RX_send(&Pkt);

}
#endif

void HCT_IF_MAC_free_data_req_pkt(HCT_PKT_BUF_s *pPkt)
{
    if (pPkt->pdata1)
    {
        BM_free_6tisch(pPkt->pdata1);
    }

    if (pPkt->pdata2)
    {
        BM_free_6tisch(pPkt->pdata2);
    }

}

/**************************************************************************************************//**
 *
 * @brief       This function handle data request message from application. it is running in
 *              HCT TX task context.
 *
 *
 *
 * @param[in]   pPkt         -- the BM buffer pointer to user data payload
 *
 *
 * @return
 *
 ***************************************************************************************************/
void HCT_IF_MAC_data_req_proc(HCT_PKT_BUF_s *pPkt)
{
    NHL_McpsDataReq_t dataSendArg;
    NHL_DataReq_t* dataReq; /* Data request parameters*/
    NHL_Sec_t *secReq;      /* Security request parameters */
    uint8_t *ptemp, data;
    uint16_t payload_len,len1,len2 ;
    BM_BUF_t *pdata1,*pdata2;
    ulpsmacaddr_t ulpsmac_addr_dest;
    uint8_t *pdst;

    HCT_Hnd->num_data_req_from_host++;

    pdata1 = pPkt->pdata1;
    pdata2 = pPkt->pdata2;

    dataReq = &dataSendArg.dataReq;

    // start to build the HCT data indication header
    ptemp = BM_getDataPtr(pdata1);

    // skip HW platform/MAC protocol
    ptemp +=2;
    // src and dst address  mode
    data = *ptemp++;

    dataReq->srcAddrMode =  (data >>4) & 0x3 ;
    dataReq->dstAddrMode =  (data & 0x3);

    // destination address (always 8 bytes)

    //ulpsmacaddr_long_cmp(&dataReq->dstAddr, (ulpsmacaddr_t *)ptemp);
    ulpsmacaddr_long_copy(&ulpsmac_addr_dest, (ulpsmacaddr_t *)ptemp);
    ptemp += 8;

    //destination PAN ID
    memcpy(&dataReq->dstPanId, ptemp,2);
    //TSCH_MlmeGetReq(TSCH_MAC_PIB_ID_macPANId, &dataReq->dstPanId);
    ptemp +=2;

    // MSDU hand
    dataReq->mdsuHandle = *ptemp++;

    //tx option
    dataReq->txOptions = *ptemp++;

    // TX power
    dataReq->power = *ptemp++;

    secReq = &dataSendArg.sec;

    // secuirty level
    secReq->securityLevel = *ptemp++;

    // key IDmode
    secReq->keyIdMode = *ptemp++;

    // keyIndex
    secReq->keyIndex = *ptemp++;

    // key source
    memcpy(secReq->keySource,ptemp,8);
    ptemp +=8;

    secReq->frameCounterSuppression = *ptemp++;
    secReq->frameCounterSize = *ptemp++;
#if 0
    MAC_MlmeSetSecurityReq(MAC_FRAME_COUNTER_MODE, &secReq->frameCounterSize);
#endif
    //IEinclude, SN and PAN ID supprresion,
    data  = *ptemp++;

    dataReq->fco.iesIncluded = (data >> 2) & 0x01;
    dataReq->fco.seqNumSuppressed = (data >>1) & 0x01;
    dataReq->fco.panIdSuppressed = (data & 0x01);

    // now, assume no IE

    // MAC payload length
    memcpy(&payload_len, ptemp,2);
    ptemp +=2;
    
    static bool ledDebug=0;
    if (payload_len==80)
    {
      if (ulpsmacaddr_long_cmp(&ulpsmac_addr_dest,&ulpsmacaddr_null))
      {
        ledDebug ^= 1;
        HAL_LED_set(2,ledDebug);
      }
    }

    // create the data buffer
    dataSendArg.pMsdu = BM_alloc(5);
    if (dataSendArg.pMsdu == NULL)
    {
        HCT_IF_MAC_free_data_req_pkt(pPkt);
        return;
    }
    // special coap packets
    if(dataReq->txOptions==2)
    {
      dataSendArg.pMsdu->coap_type = 1;
    }
    // compute how many MAC payload in the data1 buffer
    data = (ptemp - BM_getDataPtr(pdata1));

    if (BM_getDataLen(pdata1) >= data )
    {
        len1 = BM_getDataLen(pdata1) - data;
    }
    else
    {
        HCT_Hnd->err_tx_data1_len++;

        BM_free_6tisch(dataSendArg.pMsdu);
        HCT_IF_MAC_free_data_req_pkt(pPkt);
        return;
    }

    // copy the data to USM buffer
    BM_copyfrom(dataSendArg.pMsdu, ptemp, len1);

    if (pdata2)
    {
        HCT_Hnd->data_payload_len_in_two_buf++;
        len2 = BM_getDataLen(pdata2);

        if (payload_len != (len1+ len2))
        {
            HCT_Hnd->err_tx_data2_len++;
            BM_free_6tisch(dataSendArg.pMsdu);
            HCT_IF_MAC_free_data_req_pkt(pPkt);
            return;
        }

        pdst = BM_getDataPtr(dataSendArg.pMsdu) + len1;
        if (len2 <= BM_BUFFER_SIZE - dataSendArg.pMsdu->dataoffset -len1)
        {
           memcpy(pdst,BM_getDataPtr(pdata2),len2);  //JIRA 44, done
        }
        else
        {
#if DEBUG_MEM_CORR
           volatile int errloop = 0;
           while(1)
           {
              errloop++;
           }
#else
           BM_free_6tisch(dataSendArg.pMsdu);
           HCT_IF_MAC_free_data_req_pkt(pPkt);
           return;
#endif
        }
    }
    // set up the pMsdu length
    BM_setDataLen(dataSendArg.pMsdu,payload_len);

    // free the input data
    HCT_IF_MAC_free_data_req_pkt(pPkt);
    HCT_tx_request(&ulpsmac_addr_dest,dataReq->dstAddrMode,dataSendArg.pMsdu);
}

static int NVM_pib_op_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib, void *nvm_params, uint16_t nvm_param_size)
{
   int retVal = 0;

   if (set_pib)
   {
      if (*pibLen_p == nvm_param_size)
      {
         memcpy((uint8_t *)nvm_params, pibVal, *pibLen_p);
         retVal = 1;
      }
   }
   else
   {
      *pibLen_p = nvm_param_size;
      memcpy(pibVal, (uint8_t *)nvm_params, *pibLen_p);
      retVal = 1;
   }
   return(retVal);
}

static int NVM_pib_pan_id_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.panid), sizeof(nvParams.panid)));
}

static int NVM_pib_bch_chan_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.bcn_chan_0), sizeof(nvParams.bcn_chan_0)));
}

static int NVM_pib_mac_psk_07_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, nvParams.mac_psk, 8));
}

static int NVM_pib_mac_psk_8f_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, nvParams.mac_psk+8, 8));
}
//********************************************
static int NVM_pib_assoc_req_timeout_sec_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.assoc_req_timeout_sec), sizeof(nvParams.assoc_req_timeout_sec)));
}
//============================================
static int NVM_pib_slotframe_size_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.slotframe_size), sizeof(nvParams.slotframe_size)));
}
//============================================
static int NVM_pib_keepAlive_period_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.keepAlive_period), sizeof(nvParams.keepAlive_period)));
}
//============================================
static int NVM_pib_coap_resource_check_time_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.coap_resource_check_time), sizeof(nvParams.coap_resource_check_time)));
}
//============================================
static int NVM_pib_coap_port_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.coap_port), sizeof(nvParams.coap_port)));
}
//============================================
static int NVM_pib_coap_dtls_port_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.coap_dtls_port), sizeof(nvParams.coap_dtls_port)));
}
//============================================
static int NVM_pib_tx_power_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.tx_power), sizeof(nvParams.tx_power)));
}
//============================================
static int NVM_pib_bcn_ch_mode_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.bcn_ch_mode), sizeof(nvParams.bcn_ch_mode)));
}
//============================================
static int NVM_pib_scan_interval_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.scan_interval), sizeof(nvParams.scan_interval)));
}
//============================================
static int NVM_pib_num_shared_slot_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.num_shared_slot), sizeof(nvParams.num_shared_slot)));
}
//============================================
static int NVM_pib_fixed_channel_num_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.fixed_channel_num), sizeof(nvParams.fixed_channel_num)));
}
//============================================
static int NVM_pib_rpl_dio_doublings_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.rpl_dio_doublings), sizeof(nvParams.rpl_dio_doublings)));
}
//============================================
static int NVM_pib_coap_obs_max_non_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.coap_obs_max_non), sizeof(nvParams.coap_obs_max_non)));
}
//============================================
static int NVM_pib_coap_default_response_timeout_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.coap_default_response_timeout), sizeof(nvParams.coap_default_response_timeout)));
}
//============================================
static int NVM_pib_debug_level_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.debug_level), sizeof(nvParams.debug_level)));
}
//============================================
static int NVM_pib_phy_mode_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.phy_mode), sizeof(nvParams.phy_mode)));
}
//============================================
static int NVM_pib_fixed_parent_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   return(NVM_pib_op_handler(pibVal, pibLen_p, set_pib, &(nvParams.fixed_parent), sizeof(nvParams.fixed_parent)));
}

#ifdef FEATURE_MAC_SECURITY
//============================================
static uint16_t pair_key_peer;
static uint8_t pairKey07Len;
static uint8_t pairKey07[MAC_SEC_KEY_SIZE/2];

static int NVM_pib_pair_key_addr_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   int retv = 0;
   if (set_pib && *pibLen_p == 2)
   {
      memcpy(&pair_key_peer, pibVal, 2);
      retv = 1;
   }
   return(retv);
}

//============================================
static int NVM_pib_pair_key_07_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   int retv = 0;
   if (pair_key_peer != 0 && set_pib && *pibLen_p == 8)
   {
      pairKey07Len = *pibLen_p;
      memcpy(pairKey07, pibVal, pairKey07Len);
      retv = 1;
   }
   return(retv);
}

//============================================
static int NVM_pib_pair_key_8F_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   int retv = 0;
   if (pairKey07Len == 8 && set_pib && *pibLen_p == 8)
   {
      uint16_t i;

      for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
      {
         if (macSecurityPib.macDeviceTable[i].shortAddress == pair_key_peer)
         {
            memcpy(macSecurityPib.macDeviceTable[i].pairwiseKey, pairKey07, pairKey07Len);
            memcpy(macSecurityPib.macDeviceTable[i].pairwiseKey + pairKey07Len, pibVal, *pibLen_p);
            macSecurityPib.macDeviceTable[i].keyUsage |= (MAC_KEY_USAGE_RX);
            break;
         }
      }
      retv = 1;
   }

   pair_key_peer = 0;
   pairKey07Len = 0;

   return(retv);
}

//============================================
static int NVM_pib_delete_pair_key_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   int retv = 0;
   uint16_t peer_addr;
   uint16_t i;

   if (set_pib && *pibLen_p == 2)
   {
      memcpy(&peer_addr, pibVal, 2);

      for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
      {
         if (macSecurityPib.macDeviceTable[i].shortAddress == peer_addr)
         {
            macSecurityPib.macDeviceTable[i].keyUsage &= ~(MAC_KEY_USAGE_RX | MAC_KEY_USAGE_TX);

            break;
         }
      }
      retv = 1;
   }

   return(retv);
}
//============================================
#endif //FEATURE_MAC_SECURITY
//********************************************

static int NVM_pib_nvm_handler(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   extern int globalRestart;
   if (set_pib)
   {
      NVM_update();
      globalRestart = 1;
   }
   return(1);
}

const struct struct_nvm_pib_table
{
   uint16_t id;
   int (*handler)(uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib);
} nvm_pib_table[] =
{
   {TSCH_MAC_PIB_ID_macPANId,                      NVM_pib_pan_id_handler},
   {TSCH_MAC_PIB_ID_BCN_CHAN,                      NVM_pib_bch_chan_handler},
   {TSCH_MAC_PIB_ID_MAC_PSK_07,                    NVM_pib_mac_psk_07_handler},
   {TSCH_MAC_PIB_ID_MAC_PSK_8F,                    NVM_pib_mac_psk_8f_handler},
   {TSCH_MAC_PIB_ID_NVM,                           NVM_pib_nvm_handler},
   {TSCH_MAC_PIB_ID_ASSOC_REQ_TIMEOUT_SEC,         NVM_pib_assoc_req_timeout_sec_handler},
   {TSCH_MAC_PIB_ID_SLOTFRAME_SIZE,                NVM_pib_slotframe_size_handler},
   {TSCH_MAC_PIB_ID_KEEPALIVE_PERIOD,              NVM_pib_keepAlive_period_handler},
   {TSCH_MAC_PIB_ID_COAP_RESOURCE_CHECK_TIME,      NVM_pib_coap_resource_check_time_handler},
   {TSCH_MAC_PIB_ID_COAP_PORT,                     NVM_pib_coap_port_handler},
   {TSCH_MAC_PIB_ID_COAP_DTLS_PORT,                NVM_pib_coap_dtls_port_handler},
   {TSCH_MAC_PIB_ID_TX_POWER,                      NVM_pib_tx_power_handler},
   {TSCH_MAC_PIB_ID_BCN_CH_MODE,                   NVM_pib_bcn_ch_mode_handler},
   {TSCH_MAC_PIB_ID_SCAN_INTERVAL,                 NVM_pib_scan_interval_handler},
   {TSCH_MAC_PIB_ID_NUM_SHARED_SLOT,               NVM_pib_num_shared_slot_handler},
   {TSCH_MAC_PIB_ID_FIXED_CHANNEL_NUM,             NVM_pib_fixed_channel_num_handler},
   {TSCH_MAC_PIB_ID_RPL_DIO_DOUBLINGS,             NVM_pib_rpl_dio_doublings_handler},
   {TSCH_MAC_PIB_ID_COAP_OBS_MAX_NON,              NVM_pib_coap_obs_max_non_handler},
   {TSCH_MAC_PIB_ID_COAP_DEFAULT_RESPONSE_TIMEOUT, NVM_pib_coap_default_response_timeout_handler},
   {TSCH_MAC_PIB_ID_DEBUG_LEVEL,                   NVM_pib_debug_level_handler},
   {TSCH_MAC_PIB_ID_PHY_MODE,                      NVM_pib_phy_mode_handler},
   {TSCH_MAC_PIB_ID_FIXED_PARENT,                  NVM_pib_fixed_parent_handler},
#ifdef FEATURE_MAC_SECURITY
   {TSCH_MAC_PIB_ID_MAC_PAIR_KEY_ADDR,             NVM_pib_pair_key_addr_handler},
   {TSCH_MAC_PIB_ID_MAC_PAIR_KEY_07,               NVM_pib_pair_key_07_handler},
   {TSCH_MAC_PIB_ID_MAC_PAIR_KEY_8F,               NVM_pib_pair_key_8F_handler},
   {TSCH_MAC_PIB_ID_MAC_DELETE_PAIR_KEY,           NVM_pib_delete_pair_key_handler},
#endif //FEATURE_MAC_SECURITY
};

const uint16_t nvm_pib_table_len = sizeof(nvm_pib_table)/sizeof(nvm_pib_table[0]);

static int NVM_pib_op(uint32_t pibID, uint8_t *pibVal, uint16_t *pibLen_p, uint8_t set_pib)
{
   int retVal = 0;
   int i;

   for (i = 0; i < nvm_pib_table_len; i++)
   {
      if (pibID == nvm_pib_table[i].id)
      {
         if (nvm_pib_table[i].handler)
         {
            nvm_pib_table[i].handler(pibVal, pibLen_p, set_pib);
         }
         retVal = 1;
         break;
      }
   }
   return(retVal);
}

static int NVM_pib_get(uint32_t pibID, uint8_t *pibVal, uint16_t *pibLen_p)
{
   return(NVM_pib_op(pibID, pibVal, pibLen_p, 0));
}

static int NVM_pib_set(uint32_t pibID, uint8_t *pibVal, uint16_t *pibLen_p)
{
   return(NVM_pib_op(pibID, pibVal, pibLen_p, 1));
}

void HCT_IF_MAC_get_pib_proc(HCT_PKT_BUF_s *pPkt)
{
    uint8_t *ptemp, *pbase;
    uint16_t payload_len ;
    BM_BUF_t *pdata1;
    uint32_t pibID;
    uint16_t pibLen,status;
    uint8_t  pibVal[100];

    HCT_Hnd->num_get_pib_request++;

    pdata1 = pPkt->pdata1;

    ptemp = BM_getDataPtr(pdata1);

    if (pPkt->pdata2)
    {
        BM_free_6tisch(pPkt->pdata2);
    }

    // get attribute ID
    memcpy(&pibID,ptemp,sizeof (pibID));
    status = 0;

    if (pibID  == TSCH_MAC_PIB_ID_rootTschSchedule)
    {
       uint16_t pibIdx;
       memcpy(&pibIdx, ptemp + 4, 2);
       pibLen = TSCH_DB_getSchedule(pibIdx, pibVal, sizeof(pibVal));
    }
    else if (!NVM_pib_get(pibID, pibVal, &pibLen))
    {
       // ingore index
       pibLen = TSCH_MlmeGetReqSize(pibID);

       if (pibLen)
       {   // get pib value
           NHL_getPIB(pibID,pibVal);
       }
       else
       {
           status = 2; // invalid PIB
       }
    }

    // try to build get pib reply, we will use the same buffer
    BM_reset(pdata1);
    pbase = ptemp = BM_getDataPtr(pdata1);

    // status 2  bytes
    memcpy(ptemp,&status,sizeof(status));
    ptemp +=sizeof(status);

    // attribute ID (4 bytes)
    memcpy(ptemp,&pibID,sizeof(pibID));
    ptemp += sizeof (pibID);

    // attribute index always 0 (two bytes)
    *ptemp++ = 0;
    *ptemp++ = 0;

    // set length
    memcpy(ptemp, &pibLen,sizeof(pibLen));
    ptemp += sizeof(pibLen);

    // attribute value
    if (pibLen <= BM_BUFFER_SIZE - (ptemp-pbase))
    {
       memcpy(ptemp,pibVal,pibLen);   //JIRA 44, done
       ptemp +=pibLen;
    }
    else
    {
#if DEBUG_MEM_CORRs
       volatile int errloop = 0;
       while(1)
       {
          errloop++;
       }
#else
        BM_free_6tisch(pdata1);
        return;
#endif
    }

    //msg length
    payload_len = ptemp - BM_getDataPtr(pdata1);

    // set up buffer length
    BM_setDataLen(pdata1,payload_len);

    // post this message  to NHL task
    HCT_MSG_cb(HCT_MSG_TYPE_GET_MAC_PIB,pdata1);
}

void HCT_IF_Schedule_resp_proc(HCT_PKT_BUF_s *pPkt)
{
    bool status = false;
    NHLDB_TSCH_CMM_cmdmsg_t* cmdMsg = NHLDB_TSCH_CMM_alloc_cmdmsg(1);
    
    if (cmdMsg)
    {
      uint32_t ptrAddr = (uint32_t)cmdMsg;
      cmdMsg->type = COMMAND_ASSOCIATE_RESPONSE_IE;
      cmdMsg->event_type = TASK_EVENT_MSG;
      
      if (pPkt->pdata1)
      {
        cmdMsg->len = BM_getDataLen(pPkt->pdata1);
        BM_copyto(pPkt->pdata1,cmdMsg->buf);
      }
                     
      status = SN_post(nhl_tsch_mailbox, &ptrAddr,BIOS_NO_WAIT,NOTIF_TYPE_MAILBOX);
      
      if (status == false)
      {
         NHLDB_TSCH_CMM_free_cmdmsg(cmdMsg);
         ULPSMAC_Dbg.numNhlEventPostError++;
      }
    }
    
    BM_free_6tisch(pPkt->pdata1);
}

void HCT_IF_NHL_Schedule_proc(HCT_PKT_BUF_s *pPkt)
{
    BM_BUF_t *pdata1;

    pdata1 = pPkt->pdata1;
    uint8_t* ptemp = BM_getDataPtr(pdata1);

    uint8_t operation;
    uint16_t slotOffset;
    uint16_t channelOffset;
    uint8_t linkOptions;
    uint16_t nodeAddr;
    
    memcpy(&operation, ptemp, 1);
    ptemp += 1;
    memcpy(&slotOffset, ptemp, 2);
    ptemp += 2;
    memcpy(&channelOffset, ptemp, 2);
    ptemp += 2;
    memcpy(&linkOptions, ptemp, 1);
    ptemp += 1;
    memcpy(&nodeAddr, ptemp, 2);
    ptemp += 2;
    
    if(operation == 1){
        NHL_scheduleAddSlot(slotOffset, channelOffset, linkOptions, nodeAddr);
    }else if(operation == 2){
        NHL_scheduleRemoveSlot(slotOffset, nodeAddr);
    }

    if (pPkt->pdata1)
    {
        BM_free_6tisch(pPkt->pdata1);
    }

    if (pPkt->pdata2)
    {
        BM_free_6tisch(pPkt->pdata2);
    }
}

void HCT_IF_MAC_set_pib_proc(HCT_PKT_BUF_s *pPkt)
{
    uint8_t *ptemp;
    uint16_t payload_len ;
    BM_BUF_t *pdata1;
    uint32_t pibID;
    uint16_t pibLen,status=0;
    uint8_t  pibVal[8];

    HCT_Hnd->num_set_pib_request++;

    pdata1 = pPkt->pdata1;

    ptemp = BM_getDataPtr(pdata1);

    if (pPkt->pdata2)
    {
        BM_free_6tisch(pPkt->pdata2);
    }

    // get attribute ID
    memcpy(&pibID,ptemp,sizeof (pibID));
    ptemp += sizeof (pibID);

    // ignore index
    ptemp +=2;

    // attribute length
    memcpy(&pibLen,ptemp,sizeof(pibLen));
    ptemp += sizeof(pibLen);
    if (!NVM_pib_set(pibID, ptemp, &pibLen))
    {
       if (pibLen >8)
       {
           status = 3;
       }
       else
       {
           // copy the pib value
           memcpy(pibVal,ptemp,pibLen);

           pibLen = TSCH_MlmeGetReqSize(pibID);

           if (pibLen)
           {   // get pib value
               status =  NHL_setPIB(pibID,pibVal);
           }
           else
           {
               status = 2; // invalid PIB
           }
       }
    }

    // try to build get pib reply, we will use the same buffer
    BM_reset(pdata1);
    ptemp = BM_getDataPtr(pdata1);

    // status 2  bytes
    memcpy(ptemp,&status,sizeof(status));
    ptemp +=sizeof(status);

    //msg length
    payload_len = ptemp - BM_getDataPtr(pdata1);

    // set up buffer length
    BM_setDataLen(pdata1,payload_len);

    // post this message  to NHL task
    HCT_MSG_cb(HCT_MSG_TYPE_SET_MAC_PIB,pdata1);
}


void HCT_IF_MAC_post_msg_to_Host(uint8_t msgID,BM_BUF_t *payload)
{
    HCT_PKT_BUF_s Pkt;
    Pkt.pdata1 = payload;
    Pkt.pdata2 = NULL;
    Pkt.msg_type = msgID;

    HCT_RX_send(&Pkt);

}

void HCT_IF_MAC_Join_indc_proc(uint8_t* pEUI, uint16_t short_addr, uint8_t eventId)
{
    BM_BUF_t *pdata1;
    uint8_t *ptemp;
    HCT_PKT_BUF_s Pkt;
    uint8_t buf_len;
    uint16_t hct_eventId = eventId;

    pdata1 = BM_alloc(6);
    if (pdata1 == NULL)
    {
        return;
    }

    // start to build the HCT data indication header
    ptemp = BM_getDataPtr(pdata1);

    // set up the attach indication event ID
    memcpy(ptemp,&hct_eventId,2);
    ptemp += 2;

    // set up EUI
    memcpy(ptemp,pEUI,8);
    ptemp += 8;

    // save the short address
    memcpy(ptemp,&short_addr,2);
    ptemp += 2;

    // MAC payload length
    buf_len = ptemp - BM_getDataPtr(pdata1);

    // set buffer len
    BM_setDataLen(pdata1,buf_len);

    Pkt.pdata1 = pdata1;
    Pkt.pdata2 = NULL;
    Pkt.msg_type = HCT_MSG_TYPE_ATTACH;
    Pkt.msg_len  = buf_len;

    // call send to host
    HCT_RX_send(&Pkt);
}

void HCT_IF_MAC_Schedule_req_proc(uint8_t *pEUI, uint16_t childShortAddr, 
                                  uint16_t parentShortAddr,
                                  uint16_t nextHopShortAddr)
{
    BM_BUF_t *pdata1;
    uint8_t *ptemp;
    HCT_PKT_BUF_s Pkt;
    uint8_t buf_len;

    pdata1 = BM_alloc(7);
    if (pdata1 == NULL)
    {
        return;
    }

    ptemp = BM_getDataPtr(pdata1);
    memcpy(ptemp,pEUI,8);
    ptemp += 8;
    memcpy(ptemp,&childShortAddr,2);
    ptemp += 2;
    memcpy(ptemp,&parentShortAddr,2);
    ptemp += 2;
    memcpy(ptemp,&nextHopShortAddr,2);
    ptemp += 2;

    // MAC payload length
    buf_len = ptemp - BM_getDataPtr(pdata1);

    // set buffer len
    BM_setDataLen(pdata1,buf_len);

    Pkt.pdata1 = pdata1;
    Pkt.pdata2 = NULL;
    Pkt.msg_type = HCT_MSG_TYPE_SCHEDULE;
    Pkt.msg_len  = buf_len;

    // call send to host
    HCT_RX_send(&Pkt);
}
/***************************************************************************//**
 * @fn             HCT_preparePacketHCT
 *
 * @brief       HCT prepares the necessary information for packet
 *              to transmittied. The data packet is in packetbuf. We need to
 *              copy to TSCH BM buffer.
 *
 *
 * @param[in]   nhlMcpsDataReq  -- Structure of the McpsDataReq
 *
 * @param[out]
 *
 * @return
 *
 ******************************************************************************/
void HCT_preparePacket(NHL_McpsDataReq_t* nhlMcpsDataReq)
{
    NHL_DataReq_t*      nhlDataReq;
    NHL_Sec_t*   secReq;
    uint8_t destIsBcast;
    ulpsmacaddr_t ulpsmacDestAddr;
    memset(ulpsmacDestAddr.u8,0,sizeof(ulpsmacaddr_t));

    // Assign DataReq ptr
    nhlDataReq = &nhlMcpsDataReq->dataReq;

    nhlDataReq->txOptions = 1;
    if (nhlDataReq->dstAddrMode == ULPSMACADDR_MODE_NULL)
    {
        destIsBcast = 1;
    } else
    {
        destIsBcast = 0;
    }

    if (destIsBcast)
    {
        nhlDataReq->dstAddrMode = ULPSMACADDR_MODE_SHORT;
        nhlDataReq->txOptions = 0;
        ulpsmacaddr_copy(nhlDataReq->dstAddrMode,&nhlDataReq->dstAddr,
                         &ulpsmacaddr_broadcast);
    }
    
    if (ulpsmacaddr_short_node == ulpsmacaddr_null_short)
    {
        // short address is not assigned yet.
        nhlDataReq->srcAddrMode = ULPSMACADDR_MODE_LONG;
    }
    else
    {
        nhlDataReq->srcAddrMode = ULPSMACADDR_MODE_SHORT;
    }

    NHL_getPIB(TSCH_MAC_PIB_ID_macPANId, &nhlDataReq->dstPanId);

    //data_req->txOptions = txOptions;
    //nhlDataReq->fco.panIdSuppressed = 1;
    nhlDataReq->fco.iesIncluded = 0;
    nhlDataReq->pIEList = NULL;
    nhlDataReq->fco.seqNumSuppressed = 0;
    //nhlDataReq->sendMultipurpose = 0;
    //nhlDataReq->dstPanId = panID;

    secReq = &nhlMcpsDataReq->sec;
#ifdef FEATURE_MAC_SECURITY
    if (((nhlDataReq->dstAddrMode == ULPSMACADDR_MODE_LONG)
        && !ulpsmacaddr_long_cmp(&nhlDataReq->dstAddr,&ulpsmacaddr_broadcast)
        && !ulpsmacaddr_long_cmp(&nhlDataReq->dstAddr,&ulpsmacaddr_null)) ||
        ((nhlDataReq->dstAddrMode == ULPSMACADDR_MODE_SHORT)
        && !ulpsmacaddr_short_cmp(&nhlDataReq->dstAddr,&ulpsmacaddr_broadcast)
        && !ulpsmacaddr_short_cmp(&nhlDataReq->dstAddr,&ulpsmacaddr_null)))
    {
    	NHL_setMacSecurityParameters(secReq);
    }
    else
    {
      secReq->securityLevel = MAC_SEC_LEVEL_NONE;
    }
#else
    secReq->securityLevel = MAC_SEC_LEVEL_NONE;
#endif
}

