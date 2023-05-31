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
 *  ====================== nhldb-tsch-pc.c =============================================
 *  
 */

#include "lib/random.h"

#include "nhl_mgmt.h"
#include "nhldb-tsch-cmm.h"
#include "nhldb-tsch-pc.h"
#include "nhldb-tsch-sched.h"
#include "nhl_device_table.h"
#include "pnm.h"
#include "ie802154e.h"

#include "tsch_os_fun.h"
#include "mac_config.h"

#ifdef ENABLE_HCT
#include "HCT/hct.h"
#endif

/*---------------------------------------------------------------------------*/

#define NUM_BCN_LINKS_IN_ADV                1
#define MAX_NUM_LINKS_IN_ADV                IE_SLOTFRAMELINK_MAX_LINK

#define BCN_LINK_INDEX 0
#define SH_LINK_INDEX  1

#define TIMESLOT_INDEX     0
#define CH_OFFSET_INDEX           1
#define LINK_OPTIONS_INDEX        2
#define PERIOD_INDEX              3
#define PERIOD_OFFSET_INDEX       4

/*---------------------------------------------------------------------------*/
tschMlmeSetSlotframeReq_t
pc_sfs[NHLDB_TSCH_PC_NUM_SLOTFRAMES] =
{
    { 0, 0, 0},
};

static uint16_t adv_links_schedule[MAX_NUM_LINKS_IN_ADV][5];
static uint16_t num_adv_links;
static uint16_t num_sh_links;

#if !(defined(ENABLE_HCT) && defined(GW_SCHED))
static ie_link_info_field_t
pc_links_in_assocresp[NHLDB_TSCH_PC_NUM_SLOTFRAMES][NUM_LINKS_IN_ASSOC] =
{
    {
        {0, 0, MAC_LINK_OPTION_TX | MAC_LINK_OPTION_TK },     // advertisement slot
        {0, 0, MAC_LINK_OPTION_TX},                         // dedicated TX slot
        {0, 0, MAC_LINK_OPTION_RX | MAC_LINK_OPTION_TK},   // dedicated RX slot
    },
};
#endif
static uint8_t pc_link_handle_refcnt[NHLDB_TSCH_PC_NUM_SLOTFRAMES][NHLDB_TSCH_PC_MAX_NUM_LINKS];

/**************************************************************************************************//**
*
* @brief       This function is used to find link handler from pc_link_handle_refcnt
*
*
* @param[in]   slotframeHandle -- slot frame handler
*
* @param[out]  setLinkArg -- pointer of tschMlmeSetLinkReq_t. The LinkHandler field will be updated
*
* @return     TRUE -- operation is success. False: operation fails
***************************************************************************************************/
uint8_t NHLDB_TSCH_find_link_handle(uint8_t slotframeHandle, tschMlmeSetLinkReq_t* setLinkArg)
{
    uint16_t iter;
    uint16_t res;
    PIB_802154E_link_t link;

    for(iter=0; iter<NHLDB_TSCH_PC_MAX_NUM_LINKS; iter++) {

        uint16_t get_size;
        uint32_t key = Task_disable();
        if(pc_link_handle_refcnt[slotframeHandle][iter] == 0) {
            Task_restore(key);
            continue;
        }

        link.slotframe_handle = slotframeHandle;
        link.link_id = iter;

        get_size = sizeof(link);
        res = PNM_get_db(PIB_802154E_LINK, &link, &get_size);
        if(res == STAT802154E_SUCCESS)
        {
           if((link.timeslot == setLinkArg->timeslot) &&
               //(link.channel_offset == setLinkArg->channelOffset) && //Tao: commented out, finding link handle should only compare timeslot
               (link.period == setLinkArg->period) &&
               (link.periodOffset == setLinkArg->periodOffset)
              )
            {
                   Task_restore(key);
                   setLinkArg->linkHandle = iter;
                   return NHL_TRUE;
            }
        }
        Task_restore(key);
    }

    return NHL_FALSE;
}

/**************************************************************************************************//**
*
* @brief       This function is used to delete link handler
*
*
* @param[in]   slotframeHandle -- slot frame handler
* @param[in]   linkHandle -- link handler
*
* @return
***************************************************************************************************/
void NHLDB_TSCH_delete_link_handle(uint8_t slotframeHandle, uint16_t linkHandle)
{
    uint32_t key = Task_disable();
    if(pc_link_handle_refcnt[slotframeHandle][linkHandle] > 0)
    {
        pc_link_handle_refcnt[slotframeHandle][linkHandle]=0;
    }
    Task_restore(key);
    return ;
}

/**************************************************************************************************//**
*
* @brief       This function is used to assign link handler
*
*
* @param[in]   slotframeHandle -- slot frame handler
*
* @param[out]  setLinkArg -- pointer of tschMlmeSetLinkReq_t. The LinkHandler field will be updated
*
* @return      TRUE -- operation is success. False: operation fails
***************************************************************************************************/
uint8_t NHLDB_TSCH_assign_link_handle(uint8_t slotframeHandle, tschMlmeSetLinkReq_t* setLinkArg)
{
    uint16_t iter;
    uint8_t already_set;

    already_set = NHLDB_TSCH_find_link_handle(slotframeHandle, setLinkArg);
    if(already_set) {
        return ASSIGN_LINK_EXIST;
    }

    // assign new one
    for(iter=0; iter<NHLDB_TSCH_PC_MAX_NUM_LINKS; iter++) {
        uint32_t key = Task_disable();
        if(pc_link_handle_refcnt[slotframeHandle][iter] == 0) {
            setLinkArg->linkHandle = iter;
            pc_link_handle_refcnt[slotframeHandle][iter] = 1;
            Task_restore(key);
            return ASSIGN_NEW_LINK;
        }
        Task_restore(key);
    }

    // no available id slot
    setLinkArg->linkHandle = NHLDB_TSCH_PC_MAX_NUM_LINKS;
    return ASSIGN_NO_LINK;
}


/**************************************************************************************************//**
*
* @brief       This function is used to init the NHLDB in Pan Coordinator
*
*
* @param[in]
*
*
* @return
***************************************************************************************************/
void NHLDB_TSCH_PC_init()
{
    uint8_t sf_iter, link_iter;
    uint16_t sf_size_arr[NHLDB_TSCH_PC_NUM_SLOTFRAMES];

    NHLDB_TSCH_SCHED_init(sf_size_arr);

    for(sf_iter=0; sf_iter<NHLDB_TSCH_PC_NUM_SLOTFRAMES; sf_iter++) {
        pc_sfs[sf_iter].size = sf_size_arr[sf_iter];
        for(link_iter=0; link_iter<NHLDB_TSCH_PC_MAX_NUM_LINKS; link_iter++) {
            pc_link_handle_refcnt[sf_iter][link_iter] = 0;
        }
    }

}
/**************************************************************************************************//**
*
* @brief       This function is used to set the default link for PC
*
*
* @param[in]
*
*
* @return
***************************************************************************************************/
uint16_t NHLDB_TSCH_PC_set_def_sflink(void)
{
    uint8_t sf_iter;
    uint16_t res;

    for(sf_iter=0; sf_iter<NHLDB_TSCH_PC_NUM_SLOTFRAMES; sf_iter++) {
        tschMlmeSetSlotframeReq_t* slotframe_mlme_arg;
        uint16_t link_iter;

        /* 2. set default slotframe */
        //slotframe_mlme_arg->slotframe_handle already set
        slotframe_mlme_arg = &pc_sfs[sf_iter];
        pc_sfs[sf_iter].size = TSCH_MACConfig.slotframe_size;
        slotframe_mlme_arg->operation = SET_SLOTFRAME_ADD;
        res = TSCH_MlmeSetSlotframeReq(slotframe_mlme_arg);

        if (res != STAT802154E_SUCCESS)
            return res;

        /* 3. set default links */
        num_sh_links = TSCH_MACConfig.num_shared_slot;
        
        if (num_sh_links == 0)
            num_sh_links = 1;
        else if(num_sh_links > 7)
            num_sh_links = 7;
        
        num_adv_links = NUM_BCN_LINKS_IN_ADV+num_sh_links;

        for(link_iter=0; link_iter<num_adv_links; link_iter++)
        {
            tschMlmeSetLinkReq_t link_arg_inadv;
            tschMlmeSetLinkReq_t  link_arg_mlme;


            memset(&link_arg_inadv,0,sizeof(tschMlmeSetLinkReq_t));

            link_arg_inadv.operation=SET_LINK_ADD;
            link_arg_inadv.slotframeHandle=sf_iter;
            link_arg_inadv.nodeAddr=0xffff;

            if (link_iter < NUM_BCN_LINKS_IN_ADV )
            {
                    link_arg_inadv.linkOptions = MAC_LINK_OPTION_RX | MAC_LINK_OPTION_TK;
                    link_arg_inadv.linkType = MAC_LINK_TYPE_ADV;
                    link_arg_inadv.period = TSCH_MACConfig.beacon_period_sf;
                    NHLDB_TSCH_assign_link_handle(sf_iter, &link_arg_inadv);

                    NHLDB_TSCH_SCHED_find_schedule(sf_iter, NHLDB_TSCH_SCHED_ADV_LINK,
                     &(link_arg_inadv.timeslot), &(link_arg_inadv.channelOffset),
                       &(link_arg_inadv.periodOffset));
            }
            else
            {
                    link_arg_inadv.linkOptions = MAC_LINK_OPTION_SH | MAC_LINK_OPTION_RX;
                    link_arg_inadv.linkType = MAC_LINK_TYPE_NORMAL;
                    link_arg_inadv.period = 1;

                    NHLDB_TSCH_assign_link_handle(sf_iter, &link_arg_inadv);

                    NHLDB_TSCH_SCHED_find_schedule(sf_iter, NHLDB_TSCH_SCHED_SH_LINK,
                     &(link_arg_inadv.timeslot), &(link_arg_inadv.channelOffset),
                       &(link_arg_inadv.periodOffset));
            }

            adv_links_schedule[link_iter][TIMESLOT_INDEX] = link_arg_inadv.timeslot;
            adv_links_schedule[link_iter][CH_OFFSET_INDEX] =link_arg_inadv.channelOffset;
            adv_links_schedule[link_iter][LINK_OPTIONS_INDEX] =link_arg_inadv.linkOptions;

#if TSCH_PER_SCHED
            adv_links_schedule[link_iter][PERIOD_INDEX] = link_arg_inadv.period;
            adv_links_schedule[link_iter][PERIOD_OFFSET_INDEX] = link_arg_inadv.periodOffset;
#else
            adv_links_schedule[link_iter][PERIOD_INDEX] = 1;
            adv_links_schedule[link_iter][PERIOD_OFFSET_INDEX] = 0;
#endif

            /* set Mlme arg */
            memcpy(&link_arg_mlme, &link_arg_inadv, sizeof(tschMlmeSetLinkReq_t));

            if (link_arg_inadv.linkOptions == (MAC_LINK_OPTION_RX | MAC_LINK_OPTION_TK))
            {
                link_arg_mlme.linkOptions = MAC_LINK_OPTION_TX;
            }

            res = TSCH_MlmeSetLinkReq(&link_arg_mlme);

            if (res != STAT802154E_SUCCESS)
                return res;
        }
    }
    return res;
}

/**************************************************************************************************//**
*
* @brief       This function is used to copy the PIB's timeslot tempalate to variable
*
*
* @param[in]   pib_tt -- pointer of PIB_802154E_timeslot_template_t
*
* @param[out]  ie_tt -- pointer of ie_timeslot_template_t.  the content of ie_tt will be updated
*
* @return
***************************************************************************************************/
static void copy_ts_template(ie_timeslot_template_t* ie_tt, PIB_802154E_timeslot_template_t* pib_tt)
{
    ie_tt->timeslot_id = pib_tt->timeslot_id;
    ie_tt->macTsCcaOffset = pib_tt->mac_ts_cca_offset;
    ie_tt->TsCCA = pib_tt->ts_cca;
    ie_tt->TsTxOffset = pib_tt->ts_tx_offset;
    ie_tt->TsRxOffset = pib_tt->ts_rx_offset;
    ie_tt->TsRxAckDelay = pib_tt->ts_rx_ack_delay;
    ie_tt->TsTxAckDelay = pib_tt->ts_tx_ack_delay;
    ie_tt->TsRxWait = pib_tt->ts_rx_wait;
    ie_tt->TsAckWait = pib_tt->ts_ack_wait;
    ie_tt->TsRxTx = pib_tt->ts_rx_tx;
    ie_tt->TsMaxAck = pib_tt->ts_max_ack;
    ie_tt->TsMaxTx = pib_tt->ts_max_tx;
    ie_tt->TsTimeslotLength = pib_tt->ts_timeslot_length;
}

/**************************************************************************************************//**
*
* @brief       This function is used to create the beacon IE
*
*
* @param[in]   buf -- pointer of buffer to store the beacon IE
*              bufLen -- number of bytes in the buffer
*
* @param[out]  buf -- pointer of buffer
*
* @return      number of bytes in the buffer after beacon IE is created
***************************************************************************************************/
uint8_t NHLDB_TSCH_PC_create_advie(uint8_t* buf, uint8_t bufLen)
{
    ie802154e_pie_t pie;
    uint8_t* cur_buf_ptr;
    uint8_t  cur_remain_len;
    uint8_t  added_len;
    uint16_t size;
    uint8_t  total_len;
    uint8_t numLinksInBcn;

    cur_buf_ptr = buf;
    cur_remain_len = bufLen;

    total_len = 0;

#if COMPRESS_SH_LINK_IE
    numLinksInBcn = NUM_BCN_LINKS_IN_ADV + 1;
#else
    numLinksInBcn= num_adv_links;
#endif

    {
        // 1. Reserve for First IE is Synchronization (MACASN and JoinPriority)
        //    Will be added at last moment once length field in outer descriptor is calcuated
        cur_buf_ptr += (MIN_PAYLOAD_IE_LEN+MIN_PAYLOAD_IE_LEN+IE_TSCH_SYNCH_LEN);
        cur_remain_len -= (MIN_PAYLOAD_IE_LEN+MIN_PAYLOAD_IE_LEN+IE_TSCH_SYNCH_LEN);
        total_len += (MIN_PAYLOAD_IE_LEN+IE_TSCH_SYNCH_LEN);
    }

#if TSCH_PER_SCHED
    /*  Link Period IE (TI Proprietary) */
    {
        ie_tsch_link_period_t linkPeriod;
        uint8_t i;

        memset(&linkPeriod,0,sizeof(ie_tsch_link_period_t));

        for(i=0;i<numLinksInBcn;i++)
        {
            linkPeriod.period[i]=adv_links_schedule[i][PERIOD_INDEX];
            linkPeriod.periodOffset[i]= adv_links_schedule[i][PERIOD_OFFSET_INDEX];
        }

        pie.ie_type = IE_TYPE_PAYLOAD;
        pie.ie_group_id = IE_GROUP_ID_MLME;
        pie.ie_content_length = 0;
        pie.ie_sub_type = IE_SUBTYPE_SHORT;
        pie.ie_elem_id = IE_ID_TSCH_LINK_PERIOD;
        pie.ie_sub_content_length = 2*numLinksInBcn;
        pie.ie_content_ptr = (uint8_t*) &linkPeriod;
        pie.outer_descriptor = 0;

        added_len = ie802154e_add_payloadie(&pie, cur_buf_ptr, cur_remain_len);

        cur_buf_ptr += added_len;
        cur_remain_len -= added_len;
        total_len += added_len;
    }
#endif

#if COMPRESS_SH_LINK_IE
    /*  Link Compression IE (TI Proprietary) */
    if (num_adv_links > (NUM_BCN_LINKS_IN_ADV + 1))
    {
        ie_tsch_link_comp_t shLinkComp;

        shLinkComp.linkType = IE_SH_LINK;
        shLinkComp.startTimeslot = adv_links_schedule[NUM_BCN_LINKS_IN_ADV][TIMESLOT_INDEX];
        shLinkComp.numContTimeslot = num_sh_links;

        pie.ie_type = IE_TYPE_PAYLOAD;
        pie.ie_group_id = IE_GROUP_ID_MLME;
        pie.ie_content_length = 0;
        pie.ie_sub_type = IE_SUBTYPE_SHORT;
        pie.ie_elem_id = IE_ID_TSCH_COMPRESS_LINK;
        pie.ie_sub_content_length = sizeof(ie_tsch_link_comp_t);
        pie.ie_content_ptr = (uint8_t*) &shLinkComp;
        pie.outer_descriptor = 0;

        added_len = ie802154e_add_payloadie(&pie, cur_buf_ptr, cur_remain_len);

        cur_buf_ptr += added_len;
        cur_remain_len -= added_len;
        total_len += added_len;
    }
#endif
    {
        /* Slotframe and link IE */
        ie_tsch_slotframelink_t slotframelinkinfo;
        uint8_t sf_iter;
        uint8_t ie_length = 0;

        slotframelinkinfo.numSlotframes = NHLDB_TSCH_PC_NUM_SLOTFRAMES;

        ie_length += IE_SLOTFRAME_LINK_INFO_LEN;

        for(sf_iter=0; sf_iter<slotframelinkinfo.numSlotframes; sf_iter++)
        {
            ie_slotframe_info_field_t* slotframeinfo;
            uint8_t link_iter;

            slotframeinfo = &slotframelinkinfo.slotframe_info_list[sf_iter];

            slotframeinfo->macSlotframeHandle = pc_sfs[sf_iter].slotframeHandle;
            slotframeinfo->macSlotframeSize = pc_sfs[sf_iter].size;
            slotframeinfo->numLinks = numLinksInBcn;

            ie_length += (IE_SLOTFRAME_INFO_LEN + slotframeinfo->numLinks * IE_LINK_INFO_LEN);

            for(link_iter=0; link_iter<slotframeinfo->numLinks; link_iter++)
            {
                ie_link_info_field_t* linkinfo;

                linkinfo = &slotframeinfo->link_info_list[link_iter];
                linkinfo->macTimeslot = adv_links_schedule[link_iter][TIMESLOT_INDEX];
                linkinfo->macChannelOffset = adv_links_schedule[link_iter][CH_OFFSET_INDEX];
                linkinfo->macLinkOption =  adv_links_schedule[link_iter][LINK_OPTIONS_INDEX];
            }
        }

        pie.ie_type = IE_TYPE_PAYLOAD;
        pie.ie_group_id = IE_GROUP_ID_MLME;
        pie.ie_content_length = 0;
        pie.ie_sub_type = IE_SUBTYPE_SHORT;
        pie.ie_elem_id = IE_ID_TSCH_SLOTFRAMELINK;
        pie.ie_sub_content_length = ie_length;
        pie.ie_content_ptr = (uint8_t *) &slotframelinkinfo;
        pie.outer_descriptor = 0;

        added_len = ie802154e_add_payloadie(&pie, cur_buf_ptr, cur_remain_len);

        cur_buf_ptr += added_len;
        cur_remain_len -= added_len;
        total_len += added_len;
    }
    {
        /* Timeslot format IE */
        ie_timeslot_template_t ie_ts_template;
        PIB_802154E_timeslot_template_t pib_ts_template;

        size = sizeof(PIB_802154E_timeslot_template_t);
        PNM_get_db(PIB_802154E_TIMESLOT, (void *) &pib_ts_template, &size);

        copy_ts_template(&ie_ts_template, &pib_ts_template);

        pie.ie_type = IE_TYPE_PAYLOAD;
        pie.ie_group_id = IE_GROUP_ID_MLME;
        pie.ie_content_length = 0;
        pie.ie_sub_type = IE_SUBTYPE_SHORT;
        pie.ie_elem_id = IE_ID_TSCH_TIMESLOT;
        pie.ie_sub_content_length = IE_TIMESLOT_TEMPLATE_LEN_ID_ONLY;
        pie.ie_content_ptr = (uint8_t*) &ie_ts_template;
        pie.outer_descriptor = 0;

        added_len = ie802154e_add_payloadie(&pie, cur_buf_ptr, cur_remain_len);
        cur_buf_ptr += added_len;
        cur_remain_len -= added_len;
        total_len += added_len;
    }
    {
        /* Channel Hopping IE */
        ie_hopping_t hopping;

        // Just use the default one
        hopping.macHoppingSequenceID = 0;

        pie.ie_content_length = sizeof(hopping.macHoppingSequenceID);
        pie.ie_content_ptr = (uint8_t *) &hopping;

        pie.ie_type = IE_TYPE_PAYLOAD;
        pie.ie_group_id = IE_GROUP_ID_MLME;
        pie.ie_content_length = 0;
        pie.ie_sub_type = IE_SUBTYPE_LONG;
        pie.ie_elem_id = IE_ID_CHANNELHOPPING;
        pie.ie_sub_content_length = IE_HOPPING_TIMING_LEN;
        pie.ie_content_ptr = (uint8_t*) &hopping;
        pie.outer_descriptor = 0;

        added_len = ie802154e_add_payloadie(&pie, cur_buf_ptr, cur_remain_len);
        cur_buf_ptr += added_len;
        cur_remain_len -= added_len;
        total_len += added_len;
    }
    {
        /* Synchronization IE(ASN and JoinPriority) */
        ie_tsch_synch_t synch;

        synch.macASN = 0; // ASN will be updated by the lower MAC TSCH at the time of transmission
        PNM_get_db(PIB_802154E_JOIN_PRIO, (void *) &(synch.macJoinPriority), &size);

        pie.ie_type = IE_TYPE_PAYLOAD;
        pie.ie_group_id = IE_GROUP_ID_MLME;
        pie.ie_content_length = total_len;
        pie.ie_sub_type = IE_SUBTYPE_SHORT;
        pie.ie_elem_id = IE_ID_TSCH_SYNCH;
        pie.ie_sub_content_length = IE_TSCH_SYNCH_LEN;
        pie.ie_content_ptr = (uint8_t*) &synch;
        pie.outer_descriptor = 1;

        added_len = ie802154e_add_payloadie(&pie, buf, bufLen);
    }

    return (bufLen - cur_remain_len);
}

/**************************************************************************************************//**
*
* @brief       This function sends an request to create an assoication response.
*
*
* @param[in]   parentShrtAddr -- parent short address of the joining child node
*              childShrtAddr -- child node short address
*              nextHopShortAddr -- Short address of the closest forwarding node 
*                                  for multihop case
*
* @param[out]  none
*
* @return      Request Status
***************************************************************************************************/
void NHLDB_TSCH_PC_request_assocresp_ie(uint16_t parentShrtAddr, 
                                        uint16_t childShrtAddr,
                                        uint16_t nextHopShortAddr)
{
#ifdef GW_SCHED
    ulpsmacaddr_t longAddr = NHL_deviceTableGetLongAddr(childShrtAddr);
    HCT_IF_MAC_Schedule_req_proc(&longAddr.u8[0], childShrtAddr,
                                 parentShrtAddr, nextHopShortAddr);
#else
#error "Need to add scheduler"
#endif
}

/**************************************************************************************************//**
*
* @brief       This function is used to modify beacon payload IE
*
*
* @param[in]
*
* @param[out]
*
* @return     SUCCESS/ERROR
***************************************************************************************************/
uint16_t NHLDB_TSCH_mofify_adv(void)
{
     BM_BUF_t *pBuf;
     uint8_t parsed_len,*pData;
     uint16_t status;
     uint8_t i;
     uint8_t startIndex;

    status = UPMAC_STAT_ERR;

    if (num_adv_links == (NUM_BCN_LINKS_IN_ADV+1))
      return status;

    startIndex=NUM_BCN_LINKS_IN_ADV;

    for (i= startIndex; i< num_adv_links;i++)
    {
        if (i==startIndex)
        {
            adv_links_schedule[i][PERIOD_INDEX] = TSCH_MACConfig.sh_period_sf;
        }
        else
        {
            adv_links_schedule[i][TIMESLOT_INDEX] = 0;
            adv_links_schedule[i][CH_OFFSET_INDEX] =0;
            adv_links_schedule[i][LINK_OPTIONS_INDEX] =0;
            adv_links_schedule[i][PERIOD_INDEX] = 0;
            adv_links_schedule[i][PERIOD_OFFSET_INDEX] = 0;
        }
    }

    num_adv_links = NUM_BCN_LINKS_IN_ADV+1;

    pBuf = BM_alloc(13);

    if (pBuf == NULL)
    {
        return status;
    }

    pData = BM_getBufPtr(pBuf);

    parsed_len = NHLDB_TSCH_PC_create_advie(pData, BM_BUFFER_SIZE);

    BM_setDataLen(pBuf,parsed_len);

    status = PNM_modify_ebPayloadIeList(pBuf);

    BM_free_6tisch(pBuf);

    return status;
}
