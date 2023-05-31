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
 *  ====================== hct_load_system_config.c =============================================
 *  This module contains HCT data message related API.
 */

#include "hct.h"
#include "tlv.h"

#include "mac_reset.h"
#include "nhl_setup.h"
#include "mac_config.h"

#if defined (__IAR_SYSTEMS_ICC__) || defined(__TI_COMPILER_VERSION__)
#include <ti/sysbios/family/arm/m3/Hwi.h>
#else
#include <stdlib.h>
#endif

TLV_s TLVdata;


uint16_t HCT_IF_proc_TSCH_Msg(NHLDB_TSCH_CMM_cmdmsg_t* cmm_msg)
{
    uint8_t msgID;
    uint32_t bufAddr;
    BM_BUF_t *payload;
    uint16_t status;
    uint8_t *pdata;

    msgID = cmm_msg->type;
    memcpy(&bufAddr,cmm_msg->arg,sizeof(bufAddr));
    payload = (BM_BUF_t *)bufAddr;

    // peek the status
    pdata = BM_getDataPtr(payload);

    memcpy(&status, pdata,2);

    HCT_IF_MAC_post_msg_to_Host(msgID,payload);

    // check if msgID is load system config, we will start scan process
    if (msgID == HCT_MSG_TYPE_LOAD_SYSTEM_CONFIG)
    {
#if IS_ROOT
      NHL_MlmeStartPAN_t startArg;
      
      NHL_setupStartArg(&startArg);
      NHL_startPAN(&startArg,NULL);
#endif
    }
    else if (msgID == HCT_MSG_TYPE_NETWORK_START)
    {
    }
    else if (msgID == HCT_MSG_TYPE_ATTACH)
    {
    }
    else if (msgID == HCT_MSG_TYPE_SHUT_DOWN)
    {
        // Testing direct call
        // delay som to allow the reply to Host
        // sleep some time to allow respose back to Host
        Task_sleep(1*CLOCK_SECOND);

        // issue reset commad
        Board_reset();
    }

    return 0;

}
/**************************************************************************************************//**
 *
 * @brief       This function handle data request message from application. it is running in
 *              HCT TX task context.
 *
 *
 *
 * @param[in]   usm_buf         -- the BM buffer pointer to user data payload
 *
 *
 * @return
 *
 ***************************************************************************************************/

void HCT_IF_load_system_config_req_proc(HCT_PKT_BUF_s *pPkt)
{
    uint8_t *ptemp;
    BM_BUF_t *pdata1;
    uint8_t len,pkt_len,dat_len;
    uint16_t status =0;

    pdata1 = pPkt->pdata1;
    ptemp = BM_getDataPtr(pdata1);
    pkt_len = BM_getDataLen(pdata1);

    if (pPkt->pdata2)
    {
        BM_free_6tisch(pPkt->pdata2);
    }

    // we support the following system config
    //  slotframe size
    //  number of shared slots

    // loop through all TLV data
    //  if there is any error, just quir
    dat_len =0;

    while (pkt_len > dat_len )
    {
        len = TLV_get(ptemp,&TLVdata);

        if (len == 0)
        {   // error case, send reply and quit
            status = 10;
            break;
        }

        switch (TLVdata.type)
        {
            case LOAD_SYSTEM_CONFIG_TLV_TYPE_BEACON_FREQ:
               if (TLVdata.len == sizeof(TSCH_MACConfig.beacon_period_sf))
               {
                  memcpy(&TSCH_MACConfig.beacon_period_sf,TLVdata.value,TLVdata.len);   //JIRA 44 done
               }
               else
               {
#if DEBUG_MEM_CORR
                  volatile int errloop = 0;
                  while(1)
                  {
                     errloop++;
                  }
#endif
               }
            break;
            case LOAD_SYSTEM_CONFIG_TLV_TYPE_SLOT_FRAME_SIZE:
                memcpy(&TSCH_MACConfig.slotframe_size,TLVdata.value,TLVdata.len);
            break;
            case LOAD_SYSTEM_CONFIG_TLV_TYPE_NUMBER_SHARED_SLOT:
                memcpy(&TSCH_MACConfig.num_shared_slot,TLVdata.value,TLVdata.len);
            break;
            case LOAD_SYSTEM_CONFIG_TLV_TYPE_GATEWAY_ADDRESS:
                if (TLVdata.len == sizeof(TSCH_MACConfig.gateway_address))
                {
                   memcpy(&TSCH_MACConfig.gateway_address,TLVdata.value,TLVdata.len);
                }
                else
                {
 #if DEBUG_MEM_CORR
                   volatile int errloop = 0;
                   while(1)
                   {
                      errloop++;
                   }
 #endif
                }
            break;
            default:
                // not supported TLV
                HCT_Hnd->err_unsupported_tlv++;
        }

        // update ptemp
        ptemp += len;

        // update the data len
        dat_len += len;
    }

    // send the LOAD_SYSTEM_CONFIG reply with status
    if (status == 0)
    {
        HCT_Hnd->rx_load_sys_config = 1;
    }

    // try to build load_system_config reply, we will use the same buffer
    BM_reset(pdata1);
    ptemp = BM_getDataPtr(pdata1);

    // status 2  bytes
    memcpy(ptemp,&status,sizeof(status));
    ptemp +=sizeof(status);

    // set up buffer length
    BM_setDataLen(pdata1,2);

    // post this message  to NHL task
    HCT_MSG_cb(HCT_MSG_TYPE_LOAD_SYSTEM_CONFIG,pdata1);
}


void HCT_IF_Shutdown_req_proc(HCT_PKT_BUF_s *pPkt)
{
    uint8_t *ptemp;
    BM_BUF_t *pdata1;
    //uint16 reset_type;
    uint16_t status = 0;

    pdata1 = pPkt->pdata1;
    ptemp = BM_getDataPtr(pdata1);

    if (pPkt->pdata2)
    {
        BM_free_6tisch(pPkt->pdata2);
    }

    // ignore the reset type

    // try to build shutdown reply, we will use the same buffer
    BM_reset(pdata1);
    ptemp = BM_getDataPtr(pdata1);

    // status 2  bytes
    memcpy(ptemp,&status,sizeof(status));
    ptemp +=sizeof(status);

    // set up buffer length
    BM_setDataLen(pdata1,2);

    // post this message  to NHL task
    HCT_MSG_cb(HCT_MSG_TYPE_SHUT_DOWN,pdata1);
}

void HCT_IF_Network_Start_req_proc(HCT_PKT_BUF_s *pPkt)
{
    uint8_t *ptemp;
    BM_BUF_t *pdata1;
    uint16_t status =0;

    pdata1 = pPkt->pdata1;
    ptemp = BM_getDataPtr(pdata1);

    if (pPkt->pdata2)
    {
        BM_free_6tisch(pPkt->pdata2);
    }

    // ignore the PAN ID type

    // try to build the reset reply, we will use the same buffer
    BM_reset(pdata1);
    ptemp = BM_getDataPtr(pdata1);

    // status 2  bytes
    memcpy(ptemp,&status,sizeof(status));
    ptemp +=sizeof(status);

    // set up buffer length
    BM_setDataLen(pdata1,2);

    // post this message  to NHL task
    HCT_MSG_cb(HCT_MSG_TYPE_NETWORK_START,pdata1);
}

void HCT_IF_Attach_request_proc(HCT_PKT_BUF_s *pPkt)
{
    uint8_t *ptemp,msg_len;
    BM_BUF_t *pdata1;
    uint16_t status,parentID;
    uint8_t dstAddr[8];

    pdata1 = pPkt->pdata1;
    ptemp = BM_getDataPtr(pdata1);

    msg_len = BM_getDataLen(pdata1);

    if (pPkt->pdata2)
    {
        BM_free_6tisch(pPkt->pdata2);
    }

    // get EUI address
    memcpy(dstAddr,ptemp,8);

    ptemp +=8;

    // compare address
    if (ulpsmacaddr_long_cmp((ulpsmacaddr_t *)dstAddr,&ulpsmacaddr_long_node))
    {   // addrress does  macth
        status = 0;
    }
    else
    {
      status = 1;
    }

    //  Parent ID
    memcpy(&parentID,ptemp,2);
    ptemp += 2;

    if (status == 0)
    {
        TSCH_MACConfig.restrict_to_node = parentID;
    }

    msg_len -= 10;

    // handle the rmaining parent list
    // try to build the reset reply, we will use the same buffer
    BM_reset(pdata1);
    ptemp = BM_getDataPtr(pdata1);

    // status 2  bytes
    memcpy(ptemp,&status,sizeof(status));
    ptemp +=sizeof(status);

    // set up buffer length
    BM_setDataLen(pdata1,2);

    // post this message  to NHL task
    HCT_MSG_cb(HCT_MSG_TYPE_ATTACH,pdata1);
}
