/*
* Copyright (c) 2010-2014 Texas Instruments Incorporated
*
* All rights reserved not granted herein.
* Limited License. 
*
* Texas Instruments Incorporated grants a world-wide, royalty-free,
* non-exclusive license under copyrights and patents it now or hereafter
* owns or controls to make, have made, use, import, offer to sell and sell ("Utilize")
* this software subject to the terms herein.  With respect to the foregoing patent
*license, such license is granted  solely to the extent that any such patent is necessary
* to Utilize the software alone.  The patent license shall not apply to any combinations which
* include this software, other than combinations with devices manufactured by or for TI (â€œTI Devicesâ€�).
* No hardware patent is licensed hereunder.
*
* Redistributions must preserve existing copyright notices and reproduce this license (including the
* above copyright notice and the disclaimer and (if applicable) source code license limitations below)
* in the documentation and/or other materials provided with the distribution
*
* Redistribution and use in binary form, without modification, are permitted provided that the following
* conditions are met:
*
*       * No reverse engineering, decompilation, or disassembly of this software is permitted with respect to any
*     software provided in binary form.
*       * any redistribution and use are licensed by TI for use only with TI Devices.
*       * Nothing shall obligate TI to provide you with source code for the software licensed and provided to you in object code.
*
* If software source code is provided to you, modification and redistribution of the source code are permitted
* provided that the following conditions are met:
*
*   * any redistribution and use of the source code, including any resulting derivative works, are licensed by
*     TI for use only with TI Devices.
*   * any redistribution and use of any object code compiled from the source code and any resulting derivative
*     works, are licensed by TI for use only with TI Devices.
*
* Neither the name of Texas Instruments Incorporated nor the names of its suppliers may be used to endorse or
* promote products derived from this software without specific prior written permission.
*
* DISCLAIMER.
*
* THIS SOFTWARE IS PROVIDED BY TI AND TIâ€™S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TIâ€™S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *  ====================== mtxm.c =============================================
 *  
 */

#include "mac_config.h"
#include "tsch-acm.h"
#include "tsch-txm.h"
#include "ulpsmacbuf.h"
#include "framer-802154e.h"
#include "tsch_api.h"

#include "mtxm.h"
#include "tsch-acm.h"
#include "tsch-db.h"
#if IS_ROOT

#else
#include "nhldb-tsch-nonpc.h"
#endif

#include "mac_pib_pvt.h"
#ifdef FEATURE_MAC_SECURITY
//#include "mac_tsch_security_pib.h"
#endif
/*---------------------------------------------------------------------------*/
pnmPib_t* mtxm_pnmdb_ptr;

extern void TSCH_TXM_updateDevLinks(uint8_t addrMode, ulpsmacaddr_t *pDstAddr);
extern void TSCH_TXM_freeDevPkts(uint8_t addrMode, ulpsmacaddr_t *pDstAddr);

macTxPacketParam_t macTxPktParam;
frame802154e_t macTxPeekframer;

#if ULPSMAC_L2MH
ulpsmacaddr_t L2DstAddr;
#endif

ie802154e_hie_t ht1,ht2;

/**************************************************************************************************//**
 *
 * @brief       This is MAC TX data done callback function. This function will be
 *              called under the following situations.
 *              1). MAC header generation has error
 *              2). TX packet is succesfully transmitted
 *              3). TX packet transmsion fails (exceed the maximum retry)
 *
 *              Note that the data will be free in nhl_McpsCb.
 *
 *
 * @param[in]   ptr -- the pointer to data buffer
 *              status -- tranmssion status
 *              numBackoffs -- number of retry only when status =0
 *
 *
 * @return
 *
 ***************************************************************************************************/
static void mcps_data_sent_cb(void * ptr, uint16_t status, uint8_t numBackoffs)
{
    tschMcpsDataCnf_t confirm;
    confirm.hdr.status = (uint8_t) status;
    confirm.numBackoffs = numBackoffs;

    if (McpsCb)
    {
        confirm.hdr.event = MAC_MCPS_DATA_CNF;
        McpsCb((tschCbackEvent_t *)(&confirm), (BM_BUF_t*) ptr);
    }
}

/**************************************************************************************************//**
 *
 * @brief       This is TX beacon done callback function. This function will be
 *              called under the following situations.
 *              1). MAC header generation has error
 *              2). TX packet is succesfully transmitted
 *
 *              Note that the data will be free in nhl_McpsCb.
 *
 *
 * @param[in]   ptr -- the pointer to beacon data buffer
 *              status -- tranmssion status
 *              numBackoffs -- number of retry only when status =0
 *
 *
 * @return
 *
 ***************************************************************************************************/
static void mlme_beacon_sent_cb(void * ptr, uint16_t status, uint8_t transmissions)
{
    tschMlmeBeaconConfirm_t confirm;
    confirm.hdr.status = (uint8_t) status;

    if (MlmeCb)
    {
        confirm.hdr.event = MAC_MLME_BCN_CNF;
        MlmeCb((tschCbackEvent_t *)(&confirm), (BM_BUF_t*) ptr);
    }
}

/**************************************************************************************************//**
 *
 * @brief       This is asscoaiation request timeout callback function. This function will be
 *              called when assocaition request is timeout (No assocaition response is received). It
 *              will post the message to NHL task.
 *
 * @param[in]     ptr -- the pointer (not used)
 *
 *
 *
 * @return
 *
 ***************************************************************************************************/
void mlme_assoc_req_timeout_cb(void * ptr)
{
    tschMlmeAssociateConfirm_t assocconf;

    assocconf.hdr.status = UPMAC_STAT_NO_DATA;
    mtxm_pnmdb_ptr->doingAssocReq = 0;

    if (MlmeCb)
    {
        assocconf.hdr.event = MAC_MLME_ASSOCIATE_CNF;
        MlmeCb((tschCbackEvent_t *)(&assocconf), NULL);
    }
}

/**************************************************************************************************//**
 *
 * @brief       This is COMM-STATUS indication callback function.
 *              it will post the message to NHL task.
 *
 *
 * input parameters
 *
 * @param[in]   ptr     -- the pointer (packet buffer)
 *              status  -- status in the call back function to identify the error cause
 *              transmissions -- number of retry
 *
 *
 * @return
 *
 ***************************************************************************************************/
static void mlme_comm_status_indc_cb(void * ptr, uint16_t status, uint8_t transmissions)
{
}

void mlme_join_indication_cb(uint8_t addrMode, ulpsmacaddr_t *pDstAddr, uint8_t event_id)
{
    tschMlmeCommStatusIndication_t indc;

    indc.dstAddrMode = addrMode;
    // add the event ID (using reason  code)
    indc.reason = event_id;

    memcpy(&indc.dstAddr,pDstAddr,sizeof(ulpsmacaddr_t));

    if (MlmeCb)
    {
        indc.hdr.event = MAC_MLME_JOIN_IND;
        MlmeCb((tschCbackEvent_t *)(&indc), NULL);
    }
}
/**************************************************************************************************//**
 *
 * @brief       This is callback function for assocaition request. This function will be called
 *              1). MAC header generation has error
 *              2). TX packet is succesfully transmitted
 *              3). TX packet transmsion fails (exceed the maximum retry)
 *
 *              The data packet is freed. It also posts the message to NHL task.
 *
 *
 * @param[in]   ptr     -- the pointer (packet buffer)
 *              status  -- status in the call back function to identify the error cause
 *              transmissions -- number of retry
 *
 * @return
 *
 ***************************************************************************************************/
static void mlme_assoc_req_sent_cb(void * ptr, uint16_t status, uint8_t transmissions)
{
    // free the TX packet
    if (ptr != NULL)
    {
        BM_free_6tisch((BM_BUF_t *) ptr);
    }

    if(status == UPMAC_STAT_SUCCESS)
    {

        clock_time_t timeout_ct;

        // sent successfully, so start timer
        timeout_ct = ULPSMAC_MS_TO_CTICKS(mtxm_pnmdb_ptr->responseWaitTime);
        Clock_stop(assoc_req_ctimer);
        Clock_setTimeout(assoc_req_ctimer, timeout_ct);
        Clock_start(assoc_req_ctimer);
    }
    else
    {
        tschMlmeAssociateConfirm_t assocconf;

        assocconf.hdr.status = (uint8_t) status;
        mtxm_pnmdb_ptr->doingAssocReq = 0;

        if (MlmeCb)
        {
            assocconf.hdr.event = MAC_MLME_ASSOCIATE_CNF;
            MlmeCb((tschCbackEvent_t *)(&assocconf), NULL);
        }
    }

}

/**************************************************************************************************//**
 *
 * @brief       This is callback function for assocaition response. This function will be called
 *              1). MAC header generation has error
 *              2). TX packet is succesfully transmitted
 *              3). TX packet transmsion fails (exceed the maximum retry)
 *
 *              The data packet is freed. It also posts the message to NHL task.
 *
 * @param[in]   ptr     -- the pointer (packet buffer)
 *              status  -- status in the call back function to identify the error cause
 *              transmissions -- number of retry
 *
 *
 * @return
 *
 ***************************************************************************************************/
static void mlme_assoc_resp_sent_cb(void * ptr, uint16_t status, uint8_t transmissions)
{
    // free the TX packet
    BM_free_6tisch((BM_BUF_t *) ptr);

    mlme_comm_status_indc_cb(NULL, status, transmissions);
    if (status==0)
    {
        // clear the WAIT_CONFIRM bit in the TX link
        TSCH_TXM_updateDevLinks(MTXM_getPktDstAddrMode(),MTXM_getPktDstAddr());

        //send inidcation to Host
        mlme_join_indication_cb(MTXM_getPktDstAddrMode(),MTXM_getPktDstAddr(),
                                MAC_ATTACH_INDICATION_FROM_DIRECT_CONN_NODE);
    }
}

/**************************************************************************************************//**
 *
 * @brief       This is callback function for dis-assocaition request. This function will be called
 *              1). MAC header generation has error
 *              2). TX packet is transmitted
 *
 *              The data packet is freed. It also posts the message (MAC_MLME_DISASSOCIATE_CNF)
 *              to NHL task.
 *
 *
 * @param[in]   ptr     -- the pointer (packet buffer)
 *              status  -- status in the call back function to identify the error cause
 *              transmissions -- number of retry
 *
 * @return
 *
 ***************************************************************************************************/
static void mlme_disassoc_req_sent_cb(void * ptr, uint16_t status, uint8_t transmissions)
{
    tschMlmeDisassociateConfirm_t conf;

    // free the packet
    BM_free_6tisch((BM_BUF_t *) ptr);

    conf.devicePanId    = MTXM_getPktDstPanID();
    conf.deviceAddrMode = MTXM_getPktDstAddrMode();

    if(conf.deviceAddrMode == MAC_ADDR_MODE_EXT) {
        ulpsmacaddr_long_copy((ulpsmacaddr_t *) &conf.deviceAddress, MTXM_getPktDstAddr() );
    } else {
        ulpsmacaddr_short_copy((ulpsmacaddr_t *) &conf.deviceAddress, MTXM_getPktDstAddr() );
    }

#if IS_ROOT
    // free the TX packet and link for this device
    TSCH_TXM_freeDevPkts(conf.deviceAddrMode,&conf.deviceAddress);
#else
    // free all packets and links
    TSCH_prepareDisConnect();
#endif
    conf.hdr.status = (uint8_t) status;

    if (MlmeCb)
    {
        conf.hdr.event = MAC_MLME_DISASSOCIATE_CNF;
        MlmeCb((tschCbackEvent_t *)(&conf), NULL);
    }
}

#if ULPSMAC_L2MH
/**************************************************************************************************//**
 *
 * @brief       This is callback function for Level 2 Multi Hop assocaition request/response call
 *              back functions.
 *
 *              The data packet is freed. It also posts the message to NHL task.
 *
 *
 * @param[in]   ptr     -- the pointer (packet buffer)
 *              status  -- status in the call back function to identify the error cause
 *              transmissions -- number of retry
 *
 * @return      None
 *
 ***************************************************************************************************/
static void mlme_l2mh_sent_cb(void * ptr, uint16_t status, uint8_t transmissions)
{
    // free the TX packet
    BM_free_6tisch((BM_BUF_t *) ptr);

    mlme_comm_status_indc_cb(NULL, status, transmissions);
}

static void mlme_l2mh_assoc_res_sent_cb(void * ptr, uint16_t status, uint8_t transmissions)
{
    // free the TX packet
    BM_free_6tisch((BM_BUF_t *) ptr);

    mlme_comm_status_indc_cb(NULL, status, transmissions);
    if (status==0)
    {
        // clear the WAIT_CONFIRM bit in the TX link
        TSCH_TXM_updateDevLinks(MTXM_getPktDstAddrMode(),MTXM_getPktDstAddr());

        //send inidcation to Host
        mlme_join_indication_cb(ULPSMACADDR_MODE_LONG,&L2DstAddr,
                                MAC_ATTACH_INDICATION_FROM_INTERMEDIATE_NODE);
    }
}

#endif

/**************************************************************************************************
 *
 * @brief       This function initializes the mtxm_pnmdb_ptr handler. It will point to PNM DB
 *
 *
 * @param[in]
 *
 * @return
 *
 ***************************************************************************************************/
void MTXM_init(pnmPib_t* pnm_db_ptr)
{
    mtxm_pnmdb_ptr = pnm_db_ptr;
    
    
     /* header termination IE 1&2*/      
    ht1.ie_type = IE_TYPE_HEADER;
    ht1.ie_elem_id = IE_ID_LIST_TERM1;
    ht1.ie_content_length = 0;
    ht1.ie_content_ptr = NULL;
    
    ht2.ie_type = IE_TYPE_HEADER;
    ht2.ie_elem_id = IE_ID_LIST_TERM2;
    ht2.ie_content_length = 0;
    ht2.ie_content_ptr = NULL;
}


/**************************************************************************************************//**
 *
 * @brief       This function sends application data to the TSCH MAC for transmission in a MAC
 *              data frame. The TSCH MAC can only buffer a certain number of data request frames.
 *              When the MAC is congested and cannot accept the data request it sends a
 *              TSCH_MCPS_DATA_CNF with status MAC_TRANSACTION_OVERFLOW.  Eventually the TSCH MAC
 *              will become uncongested and send a TSCH_MCPS_DATA_CNF for a buffered request.
 *              At this point the application can attempt another data request.  Using this scheme,
 *              the application can send data whenever it wants but it must queue data to be resent
 *              if it receives an overflow status.
 *
 *
 * @param[in]   parg -- the pointer to tschMcpsDatareq_t data structure
 *
 *
 * @return
 *
 ***************************************************************************************************/
void TSCH_McpsDataReq(tschMcpsDataReq_t* parg)
{
    BM_BUF_t* usm_buf;
    frame802154e_t frame;
    uint16_t res;
    
    memset(&frame,0,sizeof(frame));

    frame.fcf.frame_type = MAC_FRAME_TYPE_DATA;

    res = framer_genFCF(&frame,parg->dataReq.fco.iesIncluded,parg->dataReq.fco.seqNumSuppressed,parg->dataReq.dstPanId,
                       parg->dataReq.dstAddrMode,&parg->dataReq.dstAddr,parg->dataReq.srcAddrMode,1);

#ifdef FEATURE_MAC_SECURITY
    if (parg->sec.keyIdMode == 1 && parg->sec.keyIndex == 0 && parg->dataReq.dstAddrMode == ULPSMACADDR_MODE_SHORT)
     {  //JIRA39
        //use the pairwise key if ready
        int i;
        uint16_t shortAddr = parg->dataReq.dstAddr.u8[0] + 256*parg->dataReq.dstAddr.u8[1];
        for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
        {
           if (shortAddr == macSecurityPib.macDeviceTable[i].shortAddress)
           {
              if (macSecurityPib.macDeviceTable[i].keyUsage & MAC_KEY_USAGE_TX)
              {
                 parg->sec.keyIndex = 0xFF;  //Use the pairwise key if ready
              }
              break;
           }
        }
     }
    res = framer_genSecHdr(&frame, &parg->sec);
#endif
    usm_buf = parg->pMsdu;
    res = framer_802154e_createHdr(usm_buf,&frame);

    if(res == 0)
    {
        mcps_data_sent_cb(usm_buf, UPMAC_STAT_FRAME_TOO_LONG, 0);
        return;
    }
    // deliver to lowmac (MAC TX queue)

    ULPSMAC_LOWMAC.send(usm_buf, mcps_data_sent_cb, usm_buf);
}


/**************************************************************************************************//**
 *
 * @brief       This function is used to generate the beacon or enhanced beacon in TSCH network.
 *              Once this operation is complete, TSCH_MLME_BEACON_CNF will be sent to the
 *              application. The beacon payload IE is set by application.
 *
 *
 * @param[in]    arg -- the pointer to tschMlmeBeaconReq_t
 *
 *
 * @return      None
 *
 ***************************************************************************************************/
void TSCH_MlmeBeaconReq(tschMlmeBeaconReq_t* arg)
{
    frame802154e_t frame;
    uint16_t res;
    BM_BUF_t *ptxBuf;

    memset(&frame,0,sizeof(frame));
    
    frame.fcf.frame_type = MAC_FRAME_TYPE_BEACON;
    res = framer_genFCF(&frame,1,arg->bsnSuppression,ulpsmacaddr_panid_node,
                       arg->dstAddrMode,&arg->dstAddr,MAC_ADDR_MODE_SHORT,1);
    
    frame.hdrie = (uint8_t*)&ht1;
    frame.hdrie_len = MIN_HEADER_IE_LEN;

    // beacon payload IE
    ptxBuf = arg->pIE_List;
    res = framer_802154e_createHdr(ptxBuf,&frame);

    // beacon frame no payload in current implementation

    if(BM_getDataLen(ptxBuf)!=71&&BM_getDataLen(ptxBuf)!=50){
        for(;;);
    }
    if(res == 0)
    {
        // fail to create header,
        //currently only reason is "not enough head space"
        mlme_beacon_sent_cb(ptxBuf, UPMAC_STAT_FRAME_TOO_LONG, 0);
        return;
    }

    // deliver to lowmac (MAC TX Queue)
    ULPSMAC_LOWMAC.send(ptxBuf, mlme_beacon_sent_cb, ptxBuf);
}

/**************************************************************************************************//**
 *
 * @brief       This function sends an associate request to a coordinator device.  The application
 *              shall attempt to associate only with a suitable PAN from received beacon list
 *              during scan process.
 *              When the associate request is complete the MAC sends a TSCH_MLME_ASSOCIATE_CNF
 *              to the application.
 *
 *
 * @param[in]   parg -- the pointer to tschMlmeAssociateReq_t data structure
 *
 * @return
 *
 ***************************************************************************************************/
void TSCH_MlmeAssociateReq(tschMlmeAssociateReq_t* arg)
{
    frame802154e_t frame;
    uint8_t len;
    BM_BUF_t *usm_buf;
    uint16_t res;
    uint8_t *buf_ptr;

    memset(&frame,0,sizeof(frame));
    
    // get the association request IE
    usm_buf = arg->pIE_List;

    if(mtxm_pnmdb_ptr->doingAssocReq == 1)
    {
        // Currently another request is processing
        mlme_assoc_req_sent_cb(usm_buf, UPMAC_STAT_ERR_BUSY, 0);
        return;
    }
    else
    {   // mark the association in progress
        mtxm_pnmdb_ptr->doingAssocReq = 1;
    }


    len = BM_getDataLen(usm_buf);
    buf_ptr = BM_getDataPtr(usm_buf) + len;

    frame.fcf.frame_type = MAC_FRAME_TYPE_COMMAND;
    res = framer_genFCF(&frame,arg->fco.iesIncluded,0,arg->coordPanId,
                        arg->coordAddrMode,&arg->coordAddress,MAC_ADDR_MODE_EXT,1);
    
    if (arg->fco.iesIncluded)
    {
      frame.hdrie = (uint8_t*)&ht1;
      frame.hdrie_len = MIN_HEADER_IE_LEN;
    }

    buf_ptr[0] = MAC_COMMAND_ASSOC_REQ; //association request
    buf_ptr[1] = arg->capabilityInformation;

    // set data len
    BM_setDataLen(usm_buf,len+2);

    res = framer_802154e_createHdr(usm_buf,&frame);

    if(res == 0)
    {
        // fail to create header,
        //currently only reason is "not enough head space"
        mlme_assoc_req_sent_cb(usm_buf, UPMAC_STAT_FRAME_TOO_LONG, 0);
        return;
    }

    // deliver to lowmac (to MAC TX queue)
    ULPSMAC_LOWMAC.send(usm_buf, mlme_assoc_req_sent_cb, usm_buf);


}

/**************************************************************************************************//**
 *
 * @brief       This function sends an associate response to a device requesting to associate.
 *              This function must be called after receiving a TSCH_MLME_ASSOCIATE_IND. It is
 *              implemented in PAN coordinator.  When the associate response is complete,
 *              the MAC sends a TSCH_MLME_COMM_STATUS_IND to the application to indicate the
 *              success or failure of the operation.
 *
 * @param[in]   parg -- the pointer to tschMlmeAssociateRsp_t data structure
 *
 *
 * @return
 *
 ***************************************************************************************************/

void TSCH_MlmeAssociateResp(tschMlmeAssociateResp_t* arg)
{
    frame802154e_t frame;
    uint8_t * buf_ptr;
    uint8_t len;
    uint16_t res;
    BM_BUF_t *ptxBuf;
    
    memset(&frame,0,sizeof(frame));

    frame.fcf.frame_type = MAC_FRAME_TYPE_COMMAND;
    res = framer_genFCF(&frame,arg->fco.iesIncluded,0,ulpsmacaddr_panid_node,
                        MAC_ADDR_MODE_EXT,&arg->destExtAddr,MAC_ADDR_MODE_EXT,0);

#ifdef FEATURE_MAC_SECURITY
  //Device Table update for security
  MAC_SecurityDeviceTableUpdate(ulpsmacaddr_panid_node, &arg->destShortAddr, &arg->destExtAddr);
#endif

    // get the association response IE
    ptxBuf = arg->pIE_List;

    len = BM_getDataLen(ptxBuf);
    buf_ptr = BM_getDataPtr(ptxBuf) + len;

    // add the MAC association response payload
    // set up the command payload
    buf_ptr[0] = MAC_COMMAND_ASSOC_RESP;
    buf_ptr += 1;
    *buf_ptr++ = arg->destShortAddr;
    *buf_ptr++ = arg->destShortAddr>>8;

    buf_ptr[0] = arg->status;
    buf_ptr += 1;

    // set data len
    BM_setDataLen(ptxBuf,len+4);

    res = framer_802154e_createHdr(ptxBuf,&frame);

    if(res == 0)
    {
        // fail to create header,
        //currently only reason is "not enough head space"
        mlme_assoc_resp_sent_cb(ptxBuf, UPMAC_STAT_FRAME_TOO_LONG, 0);
        return;
    }

    // deliver to lowmac
    ULPSMAC_LOWMAC.send(ptxBuf, mlme_assoc_resp_sent_cb, ptxBuf);

}

/**************************************************************************************************//**
 *
 * @brief       This function is used by an associated device to notify the coordinator of its
 *              intent to leave the PAN.  It is also used by the coordinator to instruct an
 *              associated device to leave the PAN.  When the disassociate procedure is complete
 *              the MAC sends a TSCH_MLME_DISASSOCIATE_CNF to the application.
 *
 * @param[in]   parg -- the pointer to tschMlmeDisassociateReq_t data structure
 *
 * @return
 *
 ***************************************************************************************************/
void TSCH_MlmeDisassociateReq(tschMlmeDisassociateReq_t* arg)
{
    frame802154e_t frame;
    uint8_t len;
    uint8_t *buf_ptr;
    uint16_t res;
    BM_BUF_t    *usm_buf;

    usm_buf = NULL;
    memset(&frame,0,sizeof(frame));

    frame.fcf.frame_type = MAC_FRAME_TYPE_COMMAND;
    // there is no IE in this message
    res = framer_genFCF(&frame,0,0,ulpsmacaddr_panid_node,
                        arg->deviceAddrMode,&arg->deviceAddress,MAC_ADDR_MODE_EXT,1);
    if (res)
    {
        mlme_disassoc_req_sent_cb(usm_buf, UPMAC_STAT_INVALID_PARAMETER, 0);
        return;
    }

    // create the buffer for data payload
    usm_buf = BM_alloc(14);
    if (usm_buf == NULL)
    {
        mlme_disassoc_req_sent_cb(usm_buf, UPMAC_STAT_ERR, 0);
        return;
    }

    // disassociation request payload
    len = 0;
    buf_ptr = BM_getDataPtr(usm_buf) + len;

    // set up the command payload
    buf_ptr[0] = MAC_COMMAND_DISASSOC_NOT; //disassociation notification
    buf_ptr[1] = arg->disassociateReason;

    // set data len
    BM_setDataLen(usm_buf,len+2);

    res = framer_802154e_createHdr(usm_buf,&frame);

    if(res == 0)
    {
        // fail to create header,
        //currently only reason is "not enough head space"
        mlme_disassoc_req_sent_cb(usm_buf, UPMAC_STAT_FRAME_TOO_LONG, 0);
        return;
    }

    // deliver to lowmac (MAC TX queue)
    ULPSMAC_LOWMAC.send(usm_buf, mlme_disassoc_req_sent_cb, usm_buf);
}


#if ULPSMAC_L2MH

/**************************************************************************************************//**
 *
 * @brief       This function sends a L2MH associate request to its parent in the multi hop network.
 *              If its parent is PAN coordinator (root node), there is no more forward of this message.
 *              When the L2 MH associate request is complete the MAC sends a
 *              TSCH_MLME_L2MH_ASSOCIATE_REQUEST_CNF to the application.
 *
 * @param[in]   parg -- the pointer to tschMlmeL2MHAssociateReq_t data structure
 *
 * @return
 *
 ***************************************************************************************************/

void MTXM_send_assoc_req_l2mh(tschTiAssociateReqL2mh_t* arg, BM_BUF_t* usm_buf)
{
    frame802154e_t frame;
    uint8_t len;
    uint8_t *buf_ptr;
    uint16_t res;
    
    memset(&frame,0,sizeof(frame));

    frame.fcf.frame_type = MAC_FRAME_TYPE_COMMAND;
    res = framer_genFCF(&frame,arg->fco.iesIncluded,0,arg->coordPanId,
                        arg->coordAddrMode,&arg->coordAddress,MAC_ADDR_MODE_SHORT,1);
    if (res)
    {
        mlme_l2mh_sent_cb(usm_buf, UPMAC_STAT_INVALID_PARAMETER, 0);
        return;
    }
    // IE lengths
    //frame.payloadie_len = 0;
    //frame.hdrie_len = 0;


    {
        BM_BUF_t *ptxBuf;

        ptxBuf = usm_buf;

        len = BM_getDataLen(ptxBuf);
        buf_ptr = BM_getDataPtr(ptxBuf)+len;

        // set up the command payload
        buf_ptr[0] = MAC_COMMAND_ASSOC_REQ_TI_L2MH; //association request
        buf_ptr[1] = arg->capabilityInformation;
        ulpsmacaddr_long_copy((ulpsmacaddr_t *) &buf_ptr[2], &arg->deviceExtAddr);
        buf_ptr[10]=arg->requestorShrtAddr & 0xff;
        buf_ptr[11]=(arg->requestorShrtAddr>>8) & 0xff;

        // set data len
        BM_setDataLen(ptxBuf,len+12);

        res = framer_802154e_createHdr(ptxBuf,&frame);
    }

    if(res == 0)
    {
        // fail to create header,
        //currently only reason is "not enough head space"
        mlme_l2mh_sent_cb(usm_buf, UPMAC_STAT_FRAME_TOO_LONG, 0);
        return;
    }

    // deliver to lowmac
    ULPSMAC_LOWMAC.send(usm_buf, mlme_l2mh_sent_cb, usm_buf);
}

/**************************************************************************************************//**
 *
 * @brief       This function sends a L2MH associate response to its child in the multi hop network.
 *              If its child is the requestor for L2MH association request message, there is no
 *              more forward of this message.
 *              When the L2 MH associate response is complete the MAC sends a
 *              TSCH_MLME_L2MH_ASSOCIATE_RESPONSE_CNF to the application.
 *
 *
 * @param[in]   parg -- the pointer to tschMlmeL2MHAssociateRsp_t data structure
 *
 *
 * @return      None
 *
 ***************************************************************************************************/

void MTXM_send_assoc_resp_l2mh(tschTiAssociateRespL2mh_t* arg, BM_BUF_t* usm_buf)
{
    frame802154e_t frame;
    uint8_t * buf_ptr;
    uint8_t len;
    uint16_t res;
    
    memset(&frame,0,sizeof(frame));

    frame.fcf.frame_type = MAC_FRAME_TYPE_COMMAND;
    res = framer_genFCF(&frame,arg->fco.iesIncluded,0,ulpsmacaddr_panid_node,
                        arg->dstAddrMode,&arg->dstAddr,MAC_ADDR_MODE_SHORT,1);

    {
        BM_BUF_t *ptxBuf;

        ptxBuf = (BM_BUF_t *)usm_buf;

        len = BM_getDataLen(ptxBuf);
        buf_ptr = BM_getDataPtr(ptxBuf) + len;

        // set up the command payload
        buf_ptr[0] = MAC_COMMAND_ASSOC_RESP_TI_L2MH;
        buf_ptr[1] = arg->deviceShortAddr & 0xff;
        buf_ptr[2] = (arg->deviceShortAddr>>8)&0xff;
        buf_ptr[3] = arg->status;
        ulpsmacaddr_long_copy((ulpsmacaddr_t *)&buf_ptr[4], &arg->deviceExtAddr);
        buf_ptr[12] = arg->requestorShrtAddr & 0xff;
        buf_ptr[13] = (arg->requestorShrtAddr>>8)&0xff;

        // save the device EUI for inidcation
        ulpsmacaddr_long_copy((ulpsmacaddr_t *) &L2DstAddr, &arg->deviceExtAddr);

        // set data len
        BM_setDataLen(ptxBuf,len+14);

        res = framer_802154e_createHdr(ptxBuf,&frame);
    }

    if(res == 0) {
        // fail to create header,
        //currently only reason is "not enough head space"
        mlme_l2mh_assoc_res_sent_cb(usm_buf, UPMAC_STAT_FRAME_TOO_LONG, 0);
        return;
    }

    // deliver to lowmac
    ULPSMAC_LOWMAC.send(usm_buf, mlme_l2mh_assoc_res_sent_cb, usm_buf);
}

/**************************************************************************************************//**
 *
 * @brief       This function sends a L2MH dis-associate request to its parent in the multi hop network.
 *              If its parent is PAN coordinator (root node), there is no more forward of this message.
 *              When the L2 MH disassociate request is complete the MAC sends a
 *              TSCH_MLME_L2MH_DISASSOCIATE_REQUEST_CNF to the application.
 *
 *
 * @param[in]   parg -- the pointer to tschTiDisassociateReqL2mh_t data structure
 *
 * @return
 *
 ***************************************************************************************************/

void MTXM_send_disassoc_req_l2mh(tschTiDisassociateReqL2mh_t* arg, BM_BUF_t* usm_buf)
{
    frame802154e_t frame;
    uint8_t len;
    uint8_t *buf_ptr;
    uint16_t res;

    memset(&frame,0,sizeof(frame));
    
    frame.fcf.frame_type = MAC_FRAME_TYPE_COMMAND;
    res = framer_genFCF(&frame,arg->fco.iesIncluded,0,ulpsmacaddr_panid_node,
                        arg->coordAddrMode,&arg->coordAddress,MAC_ADDR_MODE_SHORT,1);
    if (res)
    {
        mlme_l2mh_sent_cb(usm_buf, UPMAC_STAT_INVALID_PARAMETER, 0);
        return ;
    }


    // IE lengths
    //frame.payloadie_len = 0;
    //frame.hdrie_len = 0;

    {
        BM_BUF_t *ptxBuf;

        ptxBuf = (BM_BUF_t *)usm_buf;

        len = BM_getDataLen(ptxBuf);
        buf_ptr = BM_getDataPtr(ptxBuf) + len;

        // set up the command payload
        buf_ptr[0] = MAC_COMMAND_DISASSOC_NOT_TI_L2MH;
        buf_ptr[1] = arg->reason;
        ulpsmacaddr_long_copy((ulpsmacaddr_t *) &buf_ptr[2], &arg->deviceExtAddr);
        buf_ptr[10] = arg->requestorShrtAddr & 0xff;
        buf_ptr[11] = (arg->requestorShrtAddr>>8) & 0xff;

        // set data len
        BM_setDataLen(ptxBuf,len+12);

        res = framer_802154e_createHdr(ptxBuf,&frame);
    }
    if(res == 0) {
        // fail to create header,
        //currently only reason is "not enough head space"
        mlme_l2mh_sent_cb(usm_buf, UPMAC_STAT_FRAME_TOO_LONG, 0);
        return;
    }

    // deliver to lowmac
    ULPSMAC_LOWMAC.send(usm_buf, mlme_l2mh_sent_cb, usm_buf);
}

/**************************************************************************************************//**
 *
 * @brief       This function sends a L2MH dis-associate ack to its parent in the multi hop network.
 *              If its parent is PAN coordinator (root node), there is no more forward of this message.
 *              When the L2 MH disassociate request is complete the MAC sends a
 *              TSCH_MLME_L2MH_DISASSOCIATE_REQUEST_CNF to the application.
 *
 *
 * @param[in]   parg -- the pointer to tschTiDisassociateAckL2mh_t data structure
 *
 * @return      None
 *
 ***************************************************************************************************/

void MTXM_send_disassoc_ack_l2mh(tschTiDisassociateAckL2mh_t* arg,BM_BUF_t* usm_buf)
{
    frame802154e_t frame;
    uint8_t * buf_ptr;
    uint8_t len;
    uint16_t res;

    memset(&frame,0,sizeof(frame));
    frame.fcf.frame_type = MAC_FRAME_TYPE_COMMAND;
    res = framer_genFCF(&frame,arg->fco.iesIncluded,0,ulpsmacaddr_panid_node,
                        arg->dstAddrMode,&arg->dstAddr,MAC_ADDR_MODE_SHORT,1);
    if (res)
    {
        mlme_l2mh_sent_cb(usm_buf, UPMAC_STAT_INVALID_PARAMETER, 0);
        return ;
    }

    // Create Association response command message to the buffer
    //ulpsmacbuf_set_datalen(usm_buf,0);

    {
        BM_BUF_t *ptxBuf;

        ptxBuf = (BM_BUF_t *)usm_buf;

        len = BM_getDataLen(ptxBuf);
        buf_ptr = BM_getDataPtr(ptxBuf) + len;

        // set up the command payload
        buf_ptr[0] = MAC_COMMAND_DISASSOC_ACK_TI_L2MH;
        buf_ptr[1] = arg->status;
        ulpsmacaddr_long_copy((ulpsmacaddr_t *)&buf_ptr[2], &arg->deviceExtAddr);
        buf_ptr[10] = arg->requestorShrtAddr & 0xff;
        buf_ptr[11] = (arg->requestorShrtAddr>>8) & 0xff;

        // set data len
        BM_setDataLen(ptxBuf,len+12);

        res = framer_802154e_createHdr(ptxBuf,&frame);
    }

    if(res == 0) {
        // fail to create header,
        //currently only reason is "not enough head space"
        mlme_l2mh_sent_cb(usm_buf, UPMAC_STAT_FRAME_TOO_LONG, 0);
        return;
    }

    // deliver to lowmac
    ULPSMAC_LOWMAC.send(usm_buf, mlme_l2mh_sent_cb, usm_buf);
}
#endif

/**************************************************************************************************//**
*
* @brief       This function will peek the MAC buffer and save the header info
*              for later use
*
* @param[in]   pMacBuf - the pointer to the start of MAC header
*
* @return
*
*
**************************************************************************************************/
void MTXM_peekMacHeader(uint8_t *pMacBuf,uint16_t *pkt_type,uint16_t *dst_addr_mode,ulpsmacaddr_t *pDstAddr)
{
    uint8_t len;
    //frame802154e_t framer;

    len = ULPSMACBUF_DATA_SIZE;

    len = frame802154e_parse_header(pMacBuf, len, &macTxPeekframer);

    // we only need the packet type , destination address mode and destination address
    *pkt_type      = macTxPeekframer.fcf.frame_type;
    *dst_addr_mode = macTxPeekframer.fcf.dest_addr_mode;

    ulpsmacaddr_long_copy(pDstAddr, (ulpsmacaddr_t *)&(macTxPeekframer.dest_addr));
}

/**************************************************************************************************//**
*
* @brief       This function will parse the MAC buffer and save the header info
*              for later use
*
* @param[in]   pMacBuf - the pointer to the start of MAC header
*
* @return
*
*
**************************************************************************************************/
void MTXM_saveMacHeaderInfo(uint8_t *pMacBuf)
{
    uint8_t len;

    // set up the MAC length for worst case

    len = ULPSMACBUF_DATA_SIZE;

    len = frame802154e_parse_header(pMacBuf, len, &(macTxPktParam.txFramer));

    // save the MAC header length
    macTxPktParam.macHdrLen = len;
    extern BM_BUF_t* usmBufBeaconTx;
    if(len==0&&pMacBuf==BM_getBufPtr(usmBufBeaconTx)){
      for(;;);
    }

    // save the src and dst in address format
    ulpsmacaddr_long_copy(&(macTxPktParam.dstAddr), (ulpsmacaddr_t *)&(macTxPktParam.txFramer.dest_addr));
    ulpsmacaddr_long_copy(&(macTxPktParam.srcAddr), (ulpsmacaddr_t *)&(macTxPktParam.txFramer.src_addr));
}

/**************************************************************************************************//**
*
* @brief       This function will return destination address pointer of 802.15.4 MAC packet.
*              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
*
* @param[in]
*
*
* @return      destination address pointer to 802.15.4e MAC packet
*
***************************************************************************************************/
ulpsmacaddr_t *  MTXM_getPktDstAddr(void)
{

    return &(macTxPktParam.dstAddr);

}

/**************************************************************************************************//**
*
* @brief       This function will return source address pointer of 802.15.4 MAC packet ..
*              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
*
*
* @param[in]
*
*
* @return      source address pointer to 802.15.4e MAC packet
*
***************************************************************************************************/
ulpsmacaddr_t *  MTXM_getPktSrcAddr(void)
{

    return &(macTxPktParam.srcAddr);

}

/**************************************************************************************************//**
*
* @brief       This function will return packet type of 802.15.4 MAC packet.
*              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
*
* @param[in]
*
*
* @return      type of 802.15.4e MAC packet
*
***************************************************************************************************/
uint8_t MTXM_getPktType(void)
{

    return (macTxPktParam.txFramer.fcf.frame_type);

}

/**************************************************************************************************//**
*
* @brief       This function will return destination pan id of TX 802.15.4 MAC packet
*              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
*
* @param[in]
*
*
* @return      DST PANID  of 802.15.4e MAC packet
*
***************************************************************************************************/
uint16_t MTXM_getPktDstPanID(void){

    return (macTxPktParam.txFramer.dest_pid);

}

/**************************************************************************************************//**
*
* @brief       This function will return destination address mode of TX 802.15.4 MAC packet.
*              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
*
* @param[in]
*
*
* @return      DST address mode  of 802.15.4e MAC packet
*
***************************************************************************************************/
uint8_t MTXM_getPktDstAddrMode(void)
{

    return (macTxPktParam.txFramer.fcf.dest_addr_mode);

}

/**************************************************************************************************//**
*
* @brief       This function will return source address mode of TX 802.15.4 MAC packet.
*              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
*
* @param[in]
*
*
* @return      SRC address mode  of 802.15.4e MAC packet
*
***************************************************************************************************/
uint8_t MTXM_getPktSrcAddrMode(void)
{

    return (macTxPktParam.txFramer.fcf.src_addr_mode);

}

/**************************************************************************************************//**
*
* @brief       This function will return ack flag of TX 802.15.4 MAC packet.
*              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
*
* @param[in]
*
* @return      ACK flag  of 802.15.4e MAC packet
*
***************************************************************************************************/
uint8_t MTXM_getPktAck(void)
{

    return (macTxPktParam.txFramer.fcf.ack_required);

}

/**************************************************************************************************//**
*
* @brief       This function will return seq suppression flag of TX 802.15.4 MAC packet.
*              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
*
* @param[in]
*
* @return      seq suppression flag  of 802.15.4e MAC packet
*
***************************************************************************************************/
uint8_t MTXM_getPktSeqSuppression(void)
{

    return (macTxPktParam.txFramer.fcf.seq_num_suppress);

}

/**************************************************************************************************//**
*
* @brief       This function will return seq number  of TX 802.15.4 MAC packet.
*              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
*

* @param[in]
*
* None
*
* output parameters
*
* None.
*
* @return      seq number  of 802.15.4e MAC packet
*
***************************************************************************************************/
uint8_t MTXM_getPktSeqNo(void)
{

    return (macTxPktParam.txFramer.seq);

}

/**************************************************************************************************//**
*
* @brief       This function will return ASN pointer of  MAC beacon packet.
*              It is only ok for beacon packet
*
* @param       pMacPayload - the pointer to the start of MAC payload
*
* @return       pointer address of ASN location in the MAC beacon frame
*
***************************************************************************************************/
uint8_t * MTXM_getBeaconASNPtr(uint8_t *pMacPayload)
{
    return (uint8_t * )( ((uint32_t)(pMacPayload)) + macTxPktParam.macHdrLen + mtxm_pnmdb_ptr->ebPayloadIeASNOs);

}

#ifdef FEATURE_MAC_SECURITY
/**************************************************************************************************//**
 *
 * @brief       This function will return security enable flag  of TX 802.15.4 MAC packet.
 *              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
 *
 * @param[in]
 *
 * @return      security enable flag  of 802.15.4e MAC packet
 *
 ***************************************************************************************************/
uint8_t MTXM_getPktSecurityEnable(void)
{

  return (macTxPktParam.txFramer.fcf.security_enabled);

}

/**************************************************************************************************//**
 *
 * @brief       This function will return security level  of TX 802.15.4 MAC packet.
 *              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
 *
 * @param[in]
 *
 * @return      security level  of 802.15.4e MAC packet
 *
 ***************************************************************************************************/
uint8_t MTXM_getPktSecurityLevel(void)
{

  return (macTxPktParam.txFramer.aux_hdr.security_control.security_level);

}

/**************************************************************************************************//**
 *
 * @brief       This function will return frame info pointer  of TX 802.15.4 MAC packet.
 *              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
 *
 * @param[in]
 *
 * @return      frame info pointer  of 802.15.4e MAC packet
 *
 ***************************************************************************************************/
frame802154e_t *MTXM_getFrameInfo(void)
{

  return (&macTxPktParam.txFramer);

}

/**************************************************************************************************//**
 *
 * @brief       This function will return frame counter suppression flag  of TX 802.15.4 MAC packet.
 *              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
 *
 *
 * @param[in]
 *
 * @return      frame counter suppressioin flag  of 802.15.4e MAC packet
 *
 ***************************************************************************************************/
uint8_t MTXM_getPktFrmCntSuppression(void)
{

  return (macTxPktParam.txFramer.aux_hdr.security_control.frame_counter_suppression);

}

/**************************************************************************************************//**
 *
 * @brief       This function will return frame counter size  of TX 802.15.4 MAC packet.
 *              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
 *
 *
 * @param[in]
 *
 *
 * @return      frame counter size  of 802.15.4e MAC packet
 *
 ***************************************************************************************************/
uint8_t MTXM_getPktFrmCntSize(void)
{

  return (macTxPktParam.txFramer.aux_hdr.security_control.frame_counter_size);

}

/**************************************************************************************************//**
 *
 * @brief       This function will return Auxiliary Header location  of TX 802.15.4 MAC packet.
 *              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
 *
 * @param[in]
 *
 *
 * @return      auxiliary header location of 802.15.4e MAC packet
 *
 ***************************************************************************************************/
uint8_t MTXM_getAuxHdrOffset(void)
{

  return (macTxPktParam.txFramer.aux_hdr_offset);

}

/**************************************************************************************************//**
 *
 * @brief       This function will return Payload IE length of TX 802.15.4 MAC packet.
 *              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
 *
 * @param[in]
 *
 * @return      payload ie length of 802.15.4e MAC packet
 *
 ***************************************************************************************************/
uint8_t MTXM_getPayloadIeLen(void)
{

  return (macTxPktParam.txFramer.payloadie_len);

}

/**************************************************************************************************//**
 *
 * @brief       This function will return Payload Pointer of TX 802.15.4 MAC packet.
 *              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
 *
 * @param[in]
 *
 * @return      payload pointer of 802.15.4e MAC packet
 *
 ***************************************************************************************************/
uint8_t* MTXM_getPayloadPtr(void)
{

  return (macTxPktParam.txFramer.payload);

}

/**************************************************************************************************//**
 *
 * @brief       This function will return Payload length of TX 802.15.4 MAC packet.
 *              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
 *
 * @param[in]
 *
 * @return      payload length of 802.15.4e MAC packet
 *
 ***************************************************************************************************/
uint8_t MTXM_getPayloadLen(void)
{

  return (macTxPktParam.txFramer.payload_len);

}

/**************************************************************************************************//**
 *
 * @brief       This function will return Header length of TX 802.15.4 MAC packet.
 *              Note: this function must be called after MTXM_saveMacHeaderInfo per TX packet
 *
 * @param[in]
 *
 * @return      header length of 802.15.4e MAC packet
 *
 ***************************************************************************************************/
uint8_t MTXM_getPktHdrLen(void)
{

  return (macTxPktParam.macHdrLen);

}

#endif

