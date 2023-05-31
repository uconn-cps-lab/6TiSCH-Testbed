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
* include this software, other than combinations with devices manufactured by or for TI (“TI Devices”). 
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
* THIS SOFTWARE IS PROVIDED BY TI AND TI’S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TI’S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *  ====================== nhldb-tsch-nonpc.c =============================================
 *  
 */
#include "nhldb-tsch-cmm.h"
#include "nhldb-tsch-nonpc.h"
#include "tsch-db.h"
#include "nhl_mgmt.h"
#include "pnm.h"
#include "mac_pib.h"
#include "mac_config.h"

/*---------------------------------------------------------------------------*/
#define MAX_NUM_SLOTFRAMES               1

#define MAX_NUM_LINKS_IN_ADV             IE_SLOTFRAMELINK_MAX_LINK

/*---------------------------------------------------------------------------*/
#define NHLDB_TSCH_NONPC_MAX_NUM_SLOTFAMES    1
//#define NHLDB_TSCH_NONPC_MAX_NUM_LINKS         32
#define NHLDB_TSCH_NONPC_MAX_NUM_LINKS         46 // jiachen: TSCH_MAX_NUM_LINKS+1
/*---------------------------------------------------------------------------*/
static uint8_t nonpc_num_sfs ;
static tschMlmeSetSlotframeReq_t nonpc_sfs[MAX_NUM_SLOTFRAMES];

static uint8_t nonpc_num_links_in_adv[MAX_NUM_SLOTFRAMES];

static uint8_t num_bcn_links;

static tschMlmeSetLinkReq_t nonpc_links_in_adv[MAX_NUM_SLOTFRAMES][MAX_NUM_LINKS_IN_ADV];

/*---------------------------------------------------------------------------*/
uint8_t nonpc_defadv_idx = ULPSMAC_MAX_NUM_ADVERTISER;

static uint8_t nonpc_num_adv;
static NHLDB_TSCH_NONPC_advEntry_t nonpc_adv_table[ULPSMAC_MAX_NUM_ADVERTISER];
static uint8_t nonpc_link_handle_refcnt[MAX_NUM_SLOTFRAMES][NHLDB_TSCH_NONPC_MAX_NUM_LINKS];

static NHLDB_TSCH_NONPC_advEntry_t* nonpc_astat_list = NULL;

extern uint8_t last_rx_bcn_channel;
extern uint8_t rx_bcn_channel_idx;

// need to group together
//
extern uint8_t has_short_addr;
extern uint8_t has_bcn_tx_link;

extern uint8_t beacon_timer_started;

#if ULPSMAC_L2MH
/*---------------------------------------------------------------------------*/
#define MAX_NUM_ASSOC_L2MH_TABLE 4

/*---------------------------------------------------------------------------*/
static NHLDB_TSCH_NONPC_l2mhent_t
assoc_l2mh_table[MAX_NUM_ASSOC_L2MH_TABLE];
#endif /* ULPSMAC_L2MH */

//#define REMOVE_JP
void NHL_fatalErrorHandler()
{
#if (DEBUG_HALT_ON_ERROR)
   volatile int errorCounter = 0;
   while (1)  //fmfmdebugdebug
   {
      errorCounter ++;
   }
#endif
}
/**************************************************************************************************//**
*
* @brief       TSCH nonPC initialization function
*
*
* @param[in]
*
*
* @param[out]
*
* @return
*
***************************************************************************************************/

void NHLDB_TSCH_NONPC_init(void )
{
    nonpc_defadv_idx = ULPSMAC_MAX_NUM_ADVERTISER;
    nonpc_num_adv    = 0;
    nonpc_num_sfs    = 0;

    has_short_addr = NHL_FALSE;
    has_bcn_tx_link = NHL_FALSE;

    beacon_timer_started = ULPSMAC_FALSE;

    memset(&nonpc_num_links_in_adv[0],0,sizeof(nonpc_num_links_in_adv));
    memset(&nonpc_link_handle_refcnt[0][0],0,sizeof(nonpc_link_handle_refcnt));
    memset(nonpc_adv_table,0,sizeof(nonpc_adv_table));
    nonpc_astat_list = NULL;

#if ULPSMAC_L2MH
    memset(assoc_l2mh_table,0,sizeof(assoc_l2mh_table));
#endif /* ULPSMAC_L2MH */
}

/**************************************************************************************************//**
*
* @brief       get the number of beacon adventry
*
*
* @param[in]
*
*
* @param[out]
*
* @return      number of adventry we have received during channel scan
*
***************************************************************************************************/

inline uint8_t
NHLDB_TSCH_NONPC_num_adventry()
{
    return nonpc_num_adv;
}

/**************************************************************************************************//**
*
* @brief       get  beacon adventry based on index
*
*
* @param[in]   idx -- index of beacon adventry array
*
*
* @param[out]
*
* @return      pointer of beacon adventry
*
***************************************************************************************************/

NHLDB_TSCH_NONPC_advEntry_t*
NHLDB_TSCH_NONPC_get_adventry(uint8_t idx)
{
    if(idx >= ULPSMAC_MAX_NUM_ADVERTISER) {
        return NULL;
    }

    return &nonpc_adv_table[idx];
}

/**************************************************************************************************//**
*
* @brief       find the beacon adventry from received beacons
*
*
* @param[in]   panid -- PAN ID
*              addrMode -- address mode of beacon
*              addr -- pointer of address in beacon
*
*
* @param[out]
*
* @return      pointer of beacon adventry
*
***************************************************************************************************/

NHLDB_TSCH_NONPC_advEntry_t*
NHLDB_TSCH_NONPC_find_adventry(uint16_t panid, uint8_t addrmode, ulpsmacaddr_t* addr)
{
    uint8_t iter;
    NHLDB_TSCH_NONPC_advEntry_t* cur_adv;
    uint8_t is_same;

    for(iter=0; iter<ULPSMAC_MAX_NUM_ADVERTISER; iter++) {
        cur_adv = &nonpc_adv_table[iter];
        is_same = NHL_FALSE;
        if(cur_adv->inUse) {
            if((panid == NHLDB_TSCH_NONPC_PANID_ANY) || (cur_adv->panid == panid)) {
                if(cur_adv->addrmode == addrmode) {
                    if(addrmode == ULPSMACADDR_MODE_LONG) {
                        is_same = ulpsmacaddr_long_cmp(&cur_adv->addr, addr);
                    } else {
                        is_same = ulpsmacaddr_short_cmp(&cur_adv->addr, addr);
                    }
                }
            }
        }

        if(is_same)
        {
            cur_adv->rx_channel = last_rx_bcn_channel;
            return cur_adv;
        }

    }

    return NULL;
}

static uint8_t NHLDB_TSCH_NONPC_getJoinPriority(uint8_t* ie_buf, uint8_t ie_buflen)
{
    ie802154e_pie_t pie;
    uint8_t *pcontent;
    uint16_t remainlen = ie_buflen;
    uint8_t *pbuf = ie_buf;
    uint8_t group_id;
    uint8_t elem_id;
    uint16_t elemlen;
    uint16_t sublen;
    uint16_t mlmelen = 0;

    memset(&pie, 0, sizeof(pie));

    while(remainlen && (elemlen = ie802154e_parse_payloadie(pbuf, remainlen, &pie)))
    {
        pbuf += elemlen;
        remainlen -= elemlen;

        pcontent = pie.ie_content_ptr;
        group_id = pie.ie_group_id;
        mlmelen = pie.ie_content_length;

        if(group_id == IE_GROUP_ID_MLME)
        {
            while(mlmelen && (sublen = ie802154e_parse_payloadsubie(pcontent, mlmelen, &pie)))
            {
                pcontent += sublen;
                mlmelen -= sublen;

                elem_id = pie.ie_elem_id;

                switch(elem_id)
                {
                    case IE_ID_TSCH_SYNCH:
                    {
                        ie_tsch_synch_t synch;

                        ie802154e_parse_tsch_synch(pie.ie_content_ptr, &synch);
#ifdef REMOVE_JP
                        return 0;
#else
                         return synch.macJoinPriority;
#endif
                    }
                    default:
                    break;
                }
            }
        }
    }

    return 0xff;
}

static void
NHLDB_TSCH_NONPC_set_beacon_table_entry(tschPanDesc_t* panDescriptor, NHLDB_TSCH_NONPC_advEntry_t* advEntry)
{
    advEntry->inUse = NHL_TRUE;
    advEntry->panid = panDescriptor->coordPanId;
    advEntry->addrmode = panDescriptor->coordAddrMode;;

    if(advEntry->addrmode == ULPSMACADDR_MODE_LONG)
    {
        ulpsmacaddr_long_copy(&advEntry->addr, &(panDescriptor->coordAddr));
    } else {
        ulpsmacaddr_short_copy(&advEntry->addr, &(panDescriptor->coordAddr));
    }

    advEntry->astat = NHLDB_TSCH_NONPC_ASTAT_NONE;
    advEntry->astat_revoke = NHLDB_TSCH_NONPC_ASTAT_NONE;
    advEntry->astat_next = NULL;
    advEntry->rx_channel = last_rx_bcn_channel;
}
/**************************************************************************************************//**
*
* @brief       find the new entry in the beacon adventry array
*
*
* @param[in]   panDescriptor -- PAN descriptor
*              ieBuf -- IE buffer
*              ieBufLen -- pointer of address in beacon
*	           isBlackListed -- flag to indicate whether in blacklisted parent list
*
* @param[out]
*
* @return      pointer of beacon adventry
*
***************************************************************************************************/
NHLDB_TSCH_NONPC_advEntry_t*
NHLDB_TSCH_NONPC_new_adventry(tschPanDesc_t* panDescriptor,uint8_t* ieBuf,
                              uint8_t ieBufLen, uint8_t isBlackListed)
{

    NHLDB_TSCH_NONPC_advEntry_t* cur_adv;
    uint8_t iter;
    uint8_t joinPriority;
    
    joinPriority = NHLDB_TSCH_NONPC_getJoinPriority(ieBuf,ieBufLen);
    
    for(iter=0; iter<ULPSMAC_MAX_NUM_ADVERTISER; iter++)
    {
      cur_adv = &nonpc_adv_table[iter];
      
      if(!cur_adv->inUse)
      {
        NHLDB_TSCH_NONPC_set_beacon_table_entry(panDescriptor,cur_adv);
        cur_adv->isBlackListed = isBlackListed;
        nonpc_num_adv++;
        return cur_adv;
      }
    }
    
    /*
    Update the beaon table if the received beacon is better than one
    of the stored beacons. Entry 0 is reserved for possible previously
    connected preferred parent.
    */
	if (isBlackListed == NHL_FALSE)
    {
        for(iter=0; iter<ULPSMAC_MAX_NUM_ADVERTISER; iter++)
        {
            cur_adv = &nonpc_adv_table[iter];
            
            if (cur_adv->isBlackListed == NHL_TRUE)
            {
               NHLDB_TSCH_NONPC_set_beacon_table_entry(panDescriptor,cur_adv);
               cur_adv->isBlackListed = NHL_FALSE;
               return cur_adv;
            }
            else if ((joinPriority  < cur_adv->join_prio) ||
                ((joinPriority  == cur_adv->join_prio) 
                 &&(panDescriptor->rssi > cur_adv->rssi)) ||
                    ((joinPriority  == cur_adv->join_prio) 
                     &&(panDescriptor->rssi ==  cur_adv->rssi) &&
                     (panDescriptor->linkQuality > cur_adv->lqi))) 
            {
                NHLDB_TSCH_NONPC_set_beacon_table_entry(panDescriptor,cur_adv);
                return cur_adv;
            }
        }
    }
    
    return NULL;
    
}

/*---------------------------------------------------------------------------*/
uint8_t NHLDB_TSCH_find_link_handle(uint8_t slotframeHandle, tschMlmeSetLinkReq_t* setLinkArg)
{
    uint16_t iter;
    uint16_t res;
    PIB_802154E_link_t link;

    for(iter=0; iter<NHLDB_TSCH_NONPC_MAX_NUM_LINKS; iter++) {

        uint16_t get_size;
        uint32_t key = Task_disable();
        if(nonpc_link_handle_refcnt[slotframeHandle][iter] == 0) {
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
    if(nonpc_link_handle_refcnt[slotframeHandle][linkHandle] > 0)
    {
        nonpc_link_handle_refcnt[slotframeHandle][linkHandle]=0;
    }
    Task_restore(key);
    return ;
}
/*---------------------------------------------------------------------------*/
uint8_t NHLDB_TSCH_assign_link_handle(uint8_t slotframeHandle, tschMlmeSetLinkReq_t* setLinkArg)
{
    uint16_t iter;
    uint8_t already_set;

    already_set = NHLDB_TSCH_find_link_handle(slotframeHandle, setLinkArg);
    if(already_set)
    {
        return ASSIGN_LINK_EXIST;
    }

    // assign new one
    for(iter=0; iter<NHLDB_TSCH_NONPC_MAX_NUM_LINKS; iter++) {
        uint32_t key = Task_disable();
        if(nonpc_link_handle_refcnt[slotframeHandle][iter] == 0) {
            setLinkArg->linkHandle = iter;
            nonpc_link_handle_refcnt[slotframeHandle][iter] = 1;
            Task_restore(key);
            return  ASSIGN_NEW_LINK;
        }
        Task_restore(key);
    }

    // no available id slot
    setLinkArg->linkHandle = NHLDB_TSCH_NONPC_MAX_NUM_LINKS;
    return ASSIGN_NO_LINK;
}

/*---------------------------------------------------------------------------*/
static uint8_t
remove_link_handle(uint8_t slotframeHandle, tschMlmeSetLinkReq_t* setLinkArg)
{
    uint8_t already_set;

    already_set = NHLDB_TSCH_find_link_handle(slotframeHandle, setLinkArg);
    if(already_set) {
        uint32_t key = Task_disable();
        nonpc_link_handle_refcnt[slotframeHandle][setLinkArg->linkHandle]=0;
        Task_restore(key);
        return NHL_TRUE;
    }
    else
    {
        return NHL_FALSE;
    }
}

/*---------------------------------------------------------------------------*/
static void
copy_tst_to_pib(PIB_802154E_timeslot_template_t* pib_tt, ie_timeslot_template_t* ie_tt)
{
    pib_tt->timeslot_id = ie_tt->timeslot_id;
    pib_tt->mac_ts_cca_offset = ie_tt->macTsCcaOffset;
    pib_tt->ts_cca = ie_tt->TsCCA;
    pib_tt->ts_tx_offset = ie_tt->TsTxOffset;
    pib_tt->ts_rx_offset = ie_tt->TsRxOffset;
    pib_tt->ts_rx_ack_delay = ie_tt->TsRxAckDelay;
    pib_tt->ts_tx_ack_delay = ie_tt->TsTxAckDelay;
    pib_tt->ts_rx_wait = ie_tt->TsRxWait;
    pib_tt->ts_ack_wait = ie_tt->TsAckWait;
    pib_tt->ts_rx_tx = ie_tt->TsRxTx;
    pib_tt->ts_max_ack = ie_tt->TsMaxAck;
    pib_tt->ts_max_tx = ie_tt->TsMaxTx;
    pib_tt->ts_timeslot_length = ie_tt->TsTimeslotLength;
}

/*---------------------------------------------------------------------------*/
static void
copy_tst_to_ie(ie_timeslot_template_t* ie_tt, PIB_802154E_timeslot_template_t* pib_tt)
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

/*---------------------------------------------------------------------------*/
static inline void
setup_advent_link(NHLDB_TSCH_NONPC_advEntry_t* setup_advent)
{
    uint8_t sf_iter, link_iter;

    for(sf_iter=0; sf_iter<setup_advent->num_sfs; sf_iter++)
    {
        for(link_iter=0; link_iter<setup_advent->num_links_in_adv[sf_iter]; link_iter++)
        {
            tschMlmeSetLinkReq_t* setLinkArg = &setup_advent->sf_link_in_adv[sf_iter][link_iter];

            if(NHLDB_TSCH_assign_link_handle(setLinkArg->slotframeHandle, setLinkArg) == ASSIGN_NEW_LINK)
            {
                TSCH_MlmeSetLinkReq(setLinkArg);
                if(setLinkArg->linkOptions & MAC_LINK_OPTION_SH)
                {
                    tschMlmeSetLinkReq_t* setLinkInadv;

                    setLinkInadv = &nonpc_links_in_adv[sf_iter][nonpc_num_links_in_adv[sf_iter]];
                    memcpy(setLinkInadv, setLinkArg, sizeof(tschMlmeSetLinkReq_t));
                    nonpc_num_links_in_adv[sf_iter]++;
                }

                if(setLinkArg->linkOptions & MAC_LINK_OPTION_TK )
                {
                    tschMlmeKeepAliveReq_t ka_arg;
                    ka_arg.dstAddrShort = setLinkArg->nodeAddr;
                    ka_arg.period = TSCH_MACConfig.keepAlive_period;
                    TSCH_MlmeKeepAliveReq(&ka_arg);
                }
            }
        }
    }
}
/*---------------------------------------------------------------------------*/
static inline void
remvoe_advent_link(NHLDB_TSCH_NONPC_advEntry_t* setup_advent)
{
    uint8_t sf_iter, link_iter;

    for(sf_iter=0; sf_iter<setup_advent->num_sfs; sf_iter++)
    {
        for(link_iter=0; link_iter<setup_advent->num_links_in_adv[sf_iter]; link_iter++)
        {
            tschMlmeSetLinkReq_t* setLinkArg = &setup_advent->sf_link_in_adv[sf_iter][link_iter];

            if(remove_link_handle(setLinkArg->slotframeHandle, setLinkArg))
            {
                setLinkArg->operation = SET_LINK_DELETE;
                TSCH_MlmeSetLinkReq(setLinkArg);
            }
        } // for link_iter
    } // for slotframe_iter
}

/*---------------------------------------------------------------------------*/
static void
add_advent_to_astat_list(NHLDB_TSCH_NONPC_advEntry_t* add_advent)
{
    NHLDB_TSCH_NONPC_advEntry_t* prev_advent;
    NHLDB_TSCH_NONPC_advEntry_t* cur_advent;

    add_advent->astat_next = NULL;

    if (add_advent->preferred == 0)
    {
       NHL_fatalErrorHandler();
    }

    if(!nonpc_astat_list) {
        nonpc_astat_list = add_advent;
        return;
    }

    prev_advent = nonpc_astat_list;
    cur_advent = prev_advent->astat_next;
    while(cur_advent) {
        // find end of list
        prev_advent = cur_advent;
        cur_advent = prev_advent->astat_next;
    }

    // add new astat to the end of list
    prev_advent->astat_next = add_advent;
}

/*---------------------------------------------------------------------------*/
static void
remove_advent_from_astat_list(NHLDB_TSCH_NONPC_advEntry_t* rm_advent)
{
    NHLDB_TSCH_NONPC_advEntry_t* prev_advent;
    NHLDB_TSCH_NONPC_advEntry_t* cur_advent;

    if(!nonpc_astat_list)
    {
        return;
    }

    if(nonpc_astat_list == rm_advent)
    {
        nonpc_astat_list = rm_advent->astat_next;
        rm_advent->astat_next = NULL;
        return;
    }

    prev_advent = nonpc_astat_list;
    cur_advent = prev_advent->astat_next;
    while(cur_advent)
    {
        if(cur_advent == rm_advent)
        {
            prev_advent->astat_next = rm_advent->astat_next;
            rm_advent->astat_next = NULL;
            return;
        }

        prev_advent = cur_advent;
        cur_advent = prev_advent->astat_next;
    }

    return;
}

/*---------------------------------------------------------------------------*/
static inline void
astat_change(NHLDB_TSCH_NONPC_advEntry_t* cur_adv, uint8_t astat)
{
    switch(cur_adv->astat)
    {
        case  NHLDB_TSCH_NONPC_ASTAT_NONE:
        cur_adv->astat = astat;
        if(astat == NHLDB_TSCH_NONPC_ASTAT_NEED_AREQ)
        {
            cur_adv->is_retry = 0;
            add_advent_to_astat_list(cur_adv);
            setup_advent_link(cur_adv);
        }
        break;
        case  NHLDB_TSCH_NONPC_ASTAT_NEED_AREQ:
        cur_adv->astat = astat;
        if(astat == NHLDB_TSCH_NONPC_ASTAT_NONE)
        {
            remove_advent_from_astat_list(cur_adv);
        }
        break;
        case  NHLDB_TSCH_NONPC_ASTAT_AREQING:
        cur_adv->astat = astat;
        if(astat == NHLDB_TSCH_NONPC_ASTAT_ACONFED)
        {
            TSCH_MACConfig.beaconControl |= BC_IGNORE_RX_BEACON_MASK; //fengmo: per KILBYIIOT-6, Keep Alive
            remove_advent_from_astat_list(cur_adv);
            cur_adv->is_retry = 0;
        } else if(astat == NHLDB_TSCH_NONPC_ASTAT_NEED_AREQ)
        {
            cur_adv->is_retry = 1;
        }else if(astat == NHLDB_TSCH_NONPC_ASTAT_NONE)
        {
            remove_advent_from_astat_list(cur_adv);
            remvoe_advent_link(cur_adv);
        }
        break;
        case  NHLDB_TSCH_NONPC_ASTAT_ACONFED:
        cur_adv->astat = astat;
        break;
        default:
        break;
    }
}

/**************************************************************************************************//**
*
* @brief       find and mark the prefered beacon adventry. It is based on
*              1. join priority
*              2. RSSI and LQI
*
* @param[in]
*
*
* @param[out]
*
* @return      pointer of beacon adventry
*
***************************************************************************************************/
NHLDB_TSCH_NONPC_advEntry_t*
NHLDB_TSCH_NONPC_mark_prefer_adventry(void)
{
  uint8_t            numGoodAdv;
  uint8_t            iter;
  uint8_t            min_join_prio;
  uint32_t           max_rssi, max_lqi;
  uint8_t            max_rssi_preferred_parent_idx;
  NHLDB_TSCH_NONPC_advEntry_t* cur_adv;

  numGoodAdv = 0; 
  
  /* Drop beacons with low RSSI or high PER */
  for(iter=0; iter<ULPSMAC_MAX_NUM_ADVERTISER; iter++)
  {
    cur_adv = &nonpc_adv_table[iter];
    if (cur_adv->inUse == NHL_TRUE && cur_adv->isBlackListed == NHL_FALSE )
    {
      if (cur_adv->rcvdBeacons<(TSCH_MACConfig.scan_interval*3/4))
          cur_adv->inUse  = NHL_FALSE;
      else
          numGoodAdv++;
    }
  }

  /* If we find at least one advertisement that is not from black list, 
     then disable all the advertisements from black list
  */
   if (numGoodAdv > 0)
   {
      for(iter=0; iter<ULPSMAC_MAX_NUM_ADVERTISER; iter++)
      {
        cur_adv = &nonpc_adv_table[iter];
        
        if (cur_adv->isBlackListed == NHL_TRUE)
        {
          cur_adv->inUse  = NHL_FALSE;
        }
      } 
   }
  
  /* First check join priority then mark preferred advertisers that
  have the same minimum join priority among all advertisers */
  min_join_prio = 0xff;
    
  for(iter=0; iter<ULPSMAC_MAX_NUM_ADVERTISER; iter++) {
    cur_adv = &nonpc_adv_table[iter];
    
    if(cur_adv->inUse && (cur_adv->join_prio < min_join_prio))
    {
      min_join_prio = cur_adv->join_prio;
    }
    
    if(cur_adv->inUse)
    {
      ULPSMAC_Dbg.beaonInfo[iter].shortAddress = ulpsmacaddr_to_short(&cur_adv->addr);
      ULPSMAC_Dbg.beaonInfo[iter].joinPriority = cur_adv->join_prio;
      ULPSMAC_Dbg.beaonInfo[iter].rssi = cur_adv->rssi;
      ULPSMAC_Dbg.beaonInfo[iter].lqi = cur_adv->lqi;
    }
  }
  
  if(min_join_prio == 0xff) {
    return NULL;
  }
  
  /* Second check rssi (and then lqi) then mark default advertisers that
  have the minimum rssi among all preferred advertisers
  The simple tie break for the same RSSI is the first come
  */
  
  max_rssi = 0;
  max_lqi = 0;
  max_rssi_preferred_parent_idx = 0;
  for(iter=0; iter<ULPSMAC_MAX_NUM_ADVERTISER; iter++) {
    uint8_t check_if_default;
    cur_adv = &nonpc_adv_table[iter];
    
    check_if_default = 0;
    // first mark as non-preferred
    cur_adv->preferred = NHL_FALSE;
    if(cur_adv->inUse && (cur_adv->join_prio == min_join_prio)) {
      check_if_default = 1;
    }
    
    if(check_if_default) {
      if(cur_adv->rssi > max_rssi) {
        max_rssi = cur_adv->rssi;
        max_lqi = cur_adv->lqi;
        max_rssi_preferred_parent_idx = iter;
      } else if(cur_adv->rssi == max_rssi) {
        if(cur_adv->lqi > max_lqi) {
          max_lqi = cur_adv->lqi;
          max_rssi_preferred_parent_idx = iter;
        }
      }
    }
  } /* for */
  nonpc_defadv_idx = max_rssi_preferred_parent_idx;
  
  /* To make sure that at leat one advertiser is preferred */
  nonpc_adv_table[nonpc_defadv_idx].preferred = NHL_TRUE;
  
  if (TSCH_MACConfig.bcn_ch_mode != 0xff)
  {
    uint8_t i;
    for (i=0;i<3;i++)
    {
      if (nonpc_adv_table[nonpc_defadv_idx].rx_channel == TSCH_MACConfig.bcn_chan[i])
      {
        rx_bcn_channel_idx = i;
        break;
      }
    }
  }
  
  for(iter=0; iter<ULPSMAC_MAX_NUM_ADVERTISER; iter++)
  {
    
    cur_adv = &nonpc_adv_table[iter];
    if(!cur_adv->inUse)
    {
      continue;
    }
    
    if(cur_adv->preferred == NHL_FALSE)
    {
      NHLDB_TSCH_NONPC_astat_change(cur_adv, NHLDB_TSCH_NONPC_ASTAT_NONE);
    }
  }
  
  return &nonpc_adv_table[nonpc_defadv_idx];
}


/**************************************************************************************************//**
*
* @brief       get the default beacon adventry.
*
* @param[in]
*
*
* @param[out]
*
* @return      pointer of beacon adventry
*
***************************************************************************************************/
NHLDB_TSCH_NONPC_advEntry_t*
NHLDB_TSCH_NONPC_default_adventry(void)
{
    if(nonpc_defadv_idx == ULPSMAC_MAX_NUM_ADVERTISER) {
        return NULL;
    }

    return &nonpc_adv_table[nonpc_defadv_idx];
}

/**************************************************************************************************//**
*
* @brief       get the default beacon adventry index.
*
* @param[in]
*
*
* @param[out]
*
* @return      index of default beacon adventry
*
***************************************************************************************************/
uint8_t NHLDB_TSCH_NONPC_default_adventry_index(void)
{
    return nonpc_defadv_idx;
}

/**************************************************************************************************//**
*
* @brief       get the default beacon adventry asn.
*
* @param[in]
*
*
* @param[out]
*
* @return      asn of default beacon adventry
*
***************************************************************************************************/
uint64_t NHLDB_TSCH_NONPC_default_adventry_asn(uint8_t index)
{
    return nonpc_adv_table[index].asn;
}

/**************************************************************************************************//**
*
* @brief       get the default beacon adventry timestamp.
*
* @param[in]
*
*
* @param[out]
*
* @return      timestamp of default beacon adventry
*
***************************************************************************************************/
uint32_t NHLDB_TSCH_NONPC_default_adventry_timestamp(uint8_t index)
{
    return nonpc_adv_table[index].timestamp;
}



/**************************************************************************************************//**
*
* @brief       get the beacon adventry from assocaition state machine
*
* @param[in]
*
*
* @param[out]
*
* @return      pointer of beacon adventry
*
***************************************************************************************************/
NHLDB_TSCH_NONPC_advEntry_t*
NHLDB_TSCH_NONPC_astat_req_adventry(void)
{
    return nonpc_astat_list;
}

/**************************************************************************************************//**
*
* @brief       change assocaition state machine
*
* @param[in]   adv_entry   -- pointer of beacon adventry
*              astat       -- assocaition state
*
*
* @param[out]
*
* @return
*
***************************************************************************************************/
void
NHLDB_TSCH_NONPC_astat_change(NHLDB_TSCH_NONPC_advEntry_t* adv_entry, uint8_t astat)
{
    if(!adv_entry || !adv_entry->inUse) {
        return;
    }

    astat_change(adv_entry, astat);
}

/**************************************************************************************************//**
*
* @brief       set up prefer adventry
*
* @param[in]
*
*
* @param[out]
*
* @return     status of operation
*
***************************************************************************************************/
uint16_t
NHLDB_TSCH_NONPC_setup_prefer_adventry(void)
{
    uint16_t res;
    NHLDB_TSCH_NONPC_advEntry_t* cur_adv;

    uint16_t size;
    uint8_t join_prio;
    uint32_t timeout_ms;
    uint8_t asn[PIB_802154E_ASN_LEN];
    PIB_802154E_timeslot_template_t pib_tt;
    PIB_802154E_channel_hopping_t   pib_hopping;

    uint8_t sf_iter;
    tschMlmeSetSlotframeReq_t* setSfArg;

    res = STAT802154E_ERR;

    cur_adv = NHLDB_TSCH_NONPC_default_adventry();

    if(!cur_adv)
    {
        return res;
    }
    // save the PAN ID in the beacon to MAC PIB
    ulpsmacaddr_panid_node = cur_adv->panid;
    TSCH_MlmeSetReq(TSCH_MAC_PIB_ID_macPANId, &cur_adv->panid);

    /* Set Join Priority */
    join_prio = cur_adv->join_prio + 1;
    size = sizeof(join_prio);
    res = PNM_set_db(PIB_802154E_JOIN_PRIO, &join_prio, &size);

    if (res != STAT802154E_SUCCESS)
    {
        return res;
    }

    /* Should assoc timeout related to join priority, but.. */
    timeout_ms = TSCH_MACConfig.assoc_req_timeout_sec *1000;
    size = sizeof(timeout_ms);
    res = PNM_set_db(PIB_802154E_RESP_WAITTIME, &timeout_ms, &size);

    if (res != STAT802154E_SUCCESS)
    {
        return res;
    }

    /* Set ASN to one in the last received ADV from default advertiser*/
    PIB_802154E_set_asn(asn, cur_adv->asn);
    size = sizeof(asn);
    res = PNM_set_db(PIB_802154E_ASN, asn, &size);

    if (res != STAT802154E_SUCCESS)
    {
        return res;
    }

    /* Set Timeslot template */
    if(cur_adv->ts_template_len == IE_TIMESLOT_TEMPLATE_LEN_ID_ONLY) {
        size = sizeof(PIB_802154E_timeslot_template_t);
        res = PNM_get_db(PIB_802154E_TIMESLOT, &pib_tt, &size);
        if(pib_tt.timeslot_id != cur_adv->ts_template.timeslot_id) {
            return res;
        }
    } else {
        copy_tst_to_pib(&pib_tt, &cur_adv->ts_template);
        size = sizeof(PIB_802154E_timeslot_template_t);
        res = PNM_set_db(PIB_802154E_TIMESLOT, &pib_tt, &size);
    }

    /* Check channel hopping sequence */
    pib_hopping.hopping_sequence_id = cur_adv->hop_seq_id;
    size = sizeof(PIB_802154E_channel_hopping_t);
    res = PNM_get_db(PIB_802154E_CHAN_HOP, &pib_hopping, &size);
    if(res != STAT802154E_SUCCESS)
        return res;

    /* first setup slotframe of def adv */
    nonpc_num_sfs = cur_adv->num_sfs;
    for(sf_iter=0; sf_iter<cur_adv->num_sfs; sf_iter++)
    {
        setSfArg =  &cur_adv->sf[sf_iter];

        setSfArg->operation = SET_SLOTFRAME_ADD;
        res = TSCH_MlmeSetSlotframeReq(setSfArg);
        //TODO: need to copy slotframe after checking status of Confirm
        memcpy(&nonpc_sfs[sf_iter], setSfArg, sizeof(tschMlmeSetSlotframeReq_t));
    }

    return res;

}

/**************************************************************************************************//**
*
* @brief       update prefer adventry
*
* @param[in]
*
*
* @param[out]
*
* @return
*
***************************************************************************************************/
void
NHLDB_TSCH_NONPC_update_prefer_adventry(void)
{
    NHLDB_TSCH_NONPC_advEntry_t* cur_adv;
    uint8_t join_prio;
    uint16_t size;

    cur_adv = NHLDB_TSCH_NONPC_default_adventry();
    if(!cur_adv) {
        // No default advertiser has been set
        return;
    }

    /* Set Join Priority */
    join_prio = cur_adv->join_prio + 1;
    size = sizeof(join_prio);
    PNM_set_db(PIB_802154E_JOIN_PRIO, &join_prio, &size);

    /* Asssume that the slotframe is the same for all network*/
    //setup_advent_links();
}

/**************************************************************************************************//**
*
* @brief       remove prefer adventry
*
* @param[in]
*
*
* @param[out]
*
* @return
*
***************************************************************************************************/
uint16_t
NHLDB_TSCH_NONPC_remove_prefer_adventry(void)
{
    NHLDB_TSCH_NONPC_advEntry_t* cur_adv;

    cur_adv = NHLDB_TSCH_NONPC_default_adventry();
    if(!cur_adv) {
        return ULPSMAC_TRUE;
    }

    memset(cur_adv,0,sizeof(NHLDB_TSCH_NONPC_advEntry_t));

    return ULPSMAC_TRUE;
}

/**************************************************************************************************//**
*
* @brief       create the beacon adventry IE for intermediate node
*
* @param[in]   ie_buf  -- pointer of data buffer
*              ie_buflen -- number of bytes can be stored in the buffer
*
*
* @param[out]  ie_buf -- stored the created beacon IE
*
* @return     number of bytes in created IE
*
***************************************************************************************************/
uint8_t
NHLDB_TSCH_NONPC_create_advie(uint8_t* ie_buf, uint8_t ie_buflen)
{
    ie802154e_pie_t pie;
    uint8_t* cur_buf_ptr;
    uint8_t  cur_remain_len;
    uint8_t  added_len;
    uint16_t size;
    uint8_t  total_len;
    uint8_t numLinksInBcn;

    cur_buf_ptr = ie_buf;
    cur_remain_len = ie_buflen;
    total_len = 0;

    {
        // 1. Reserve for First IE is Synchronization (MACASN and JoinPriority)
        //    Will be added at last moment once length field in outer descriptor is calcuated
        cur_buf_ptr += (MIN_PAYLOAD_IE_LEN+MIN_PAYLOAD_IE_LEN+IE_TSCH_SYNCH_LEN);
        cur_remain_len -= (MIN_PAYLOAD_IE_LEN+MIN_PAYLOAD_IE_LEN+IE_TSCH_SYNCH_LEN);
        total_len += (MIN_PAYLOAD_IE_LEN+IE_TSCH_SYNCH_LEN);
    }

    {
        ie_tsch_slotframelink_t slotframelinkinfo;
        uint8_t sf_iter;
        uint8_t ie_length = 0;

        slotframelinkinfo.numSlotframes = nonpc_num_sfs;
        ie_length += IE_SLOTFRAME_LINK_INFO_LEN;
        for(sf_iter=0; sf_iter<slotframelinkinfo.numSlotframes; sf_iter++)
        {
            ie_tsch_link_period_t linkPeriod;
            ie_slotframe_info_field_t* slotframeinfo;
            uint8_t link_iter;

            memset(&linkPeriod,0,sizeof(ie_tsch_link_period_t));

#if COMPRESS_SH_LINK_IE
    numLinksInBcn = num_bcn_links + 1;
#else
    numLinksInBcn= nonpc_num_links_in_adv[sf_iter];
#endif

            slotframeinfo = &slotframelinkinfo.slotframe_info_list[sf_iter];

            for(link_iter=0; link_iter<numLinksInBcn; link_iter++)
            {
                ie_link_info_field_t* linkinfo;
                tschMlmeSetLinkReq_t* link_in_adv;

                linkinfo = &slotframeinfo->link_info_list[link_iter];
#if COMPRESS_SH_LINK_IE
                if (link_iter >= 1)
                {
                    uint8_t beaconLinkOffset, index;
                    beaconLinkOffset = 0;

                    /*Find the index for the beacon link*/
                    for (index=0; index<nonpc_num_links_in_adv[sf_iter];index++)
                    {
                        if (nonpc_links_in_adv[sf_iter][index].linkOptions
                            == (MAC_LINK_OPTION_RX|MAC_LINK_OPTION_TK))
                        {
                             beaconLinkOffset = index + (link_iter-1);
                             break;
                        }
                    }

                    if (beaconLinkOffset == 0)
                    {
                        /* Skip beacon link IE if cannot find any beacon links */
                        numLinksInBcn = 1;
                        break;
                    }
                    else
                    {
                        link_in_adv = &nonpc_links_in_adv[sf_iter][beaconLinkOffset];
                    }
                }
                else
                {
                    /* For non pan-coordinator node, 1st link will be share link*/
                    link_in_adv = &nonpc_links_in_adv[sf_iter][link_iter];
                }
#else
                link_in_adv = &nonpc_links_in_adv[sf_iter][link_iter];
#endif
                linkinfo->macTimeslot = link_in_adv->timeslot;
                linkinfo->macChannelOffset = link_in_adv->channelOffset;
                linkinfo->macLinkOption = link_in_adv->linkOptions;
                linkPeriod.period[link_iter]=link_in_adv->period;
                linkPeriod.periodOffset[link_iter]= link_in_adv->periodOffset;
            }

            slotframeinfo->macSlotframeHandle = nonpc_sfs[sf_iter].slotframeHandle;
            slotframeinfo->macSlotframeSize = nonpc_sfs[sf_iter].size;
            slotframeinfo->numLinks = numLinksInBcn;
            ie_length += (IE_SLOTFRAME_INFO_LEN + slotframeinfo->numLinks * IE_LINK_INFO_LEN);

#if TSCH_PER_SCHED
            /*  Link Period IE (TI Proprietary) */
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
#endif

#if COMPRESS_SH_LINK_IE
            /*  Link Compression IE (TI Proprietary) */
            if (nonpc_num_links_in_adv[sf_iter]> (num_bcn_links + 1))
            {
                ie_tsch_link_comp_t shLinkComp;

                shLinkComp.linkType = IE_SH_LINK;
                shLinkComp.startTimeslot = nonpc_links_in_adv[sf_iter][0].timeslot;
                shLinkComp.numContTimeslot = nonpc_num_links_in_adv[sf_iter] - num_bcn_links;

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
        }

         /* Slotframe and link IE */
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

        copy_tst_to_ie(&ie_ts_template, &pib_ts_template);
        pie.ie_type = IE_TYPE_PAYLOAD;
        pie.ie_group_id = IE_GROUP_ID_MLME;
        pie.ie_content_length = 0;
        pie.ie_sub_type = IE_SUBTYPE_SHORT;
        pie.ie_elem_id = IE_ID_TSCH_TIMESLOT;
       // pie.ie_sub_content_length = IE_TIMESLOT_TEMPLATE_LEN_FULL;
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

        /* Default channel hopping */
        hopping.macHoppingSequenceID = 0;

        pie.ie_type = IE_TYPE_PAYLOAD;
        pie.ie_group_id = IE_GROUP_ID_MLME;
        pie.ie_content_length = 0;
        pie.ie_sub_type = IE_SUBTYPE_LONG;
        pie.ie_elem_id = IE_ID_CHANNELHOPPING;
        pie.ie_sub_content_length = IE_HOPPING_TIMING_LEN;
        pie.ie_content_ptr = (uint8_t *) &hopping;
        pie.outer_descriptor = 0;

        added_len = ie802154e_add_payloadie(&pie, cur_buf_ptr, cur_remain_len);
        cur_buf_ptr += added_len;
        cur_remain_len -= added_len;
        total_len += added_len;
    }

    {
        /* Synchronization IE(ASN and JoinPriority) */
        ie_tsch_synch_t synch;

        /* ASN will be updated by the lower MAC TSCH at the time of transmission */
        synch.macASN = 0;
        PNM_get_db(PIB_802154E_JOIN_PRIO, (void *) &(synch.macJoinPriority), &size);

        pie.ie_type = IE_TYPE_PAYLOAD;
        pie.ie_group_id = IE_GROUP_ID_MLME;
        pie.ie_content_length = total_len;
        pie.ie_sub_type = IE_SUBTYPE_SHORT;
        pie.ie_elem_id = IE_ID_TSCH_SYNCH;
        pie.ie_sub_content_length = IE_TSCH_SYNCH_LEN;
        pie.ie_content_ptr = (uint8_t*) &synch;
        pie.outer_descriptor = 1;

        added_len = ie802154e_add_payloadie(&pie, ie_buf, ie_buflen);
    }

    return (ie_buflen - cur_remain_len);
}


/**************************************************************************************************//**
*
* @brief       create the association IE for L2MH
*
* @param[in]   adv_entry -- pointer of NHLDB_TSCH_NONPC_advEntry_t
ie_buf  -- pointer of data buffer
*              ie_buflen -- number of bytes can be stored in the buffer
*
*
* @param[out]  ie_buf -- stored the created beacon IE
*
* @return     number of bytes in created IE
*
***************************************************************************************************/
uint8_t
NHLDB_TSCH_NONPC_create_associe_l2mh(NHLDB_TSCH_NONPC_advEntry_t* adv_entry, uint8_t* ie_buf, uint8_t ie_buflen)
{
    ie802154e_pie_t pie;
    uint8_t* cur_buf_ptr;
    uint8_t  cur_remain_len;
    uint8_t  added_len;
    uint16_t size;

    cur_buf_ptr = ie_buf;
    cur_remain_len = ie_buflen;

    {
        ie_tsch_synch_t synch;

        synch.macASN = 0; // No need
        PNM_get_db(PIB_802154E_JOIN_PRIO, (void *) &(synch.macJoinPriority), &size);

        pie.ie_type = IE_TYPE_PAYLOAD;
        pie.ie_group_id = 0;
        pie.ie_elem_id = IE_ID_TSCH_SYNCH;
        //pie.ie_content_length = sizeof(synch);
        pie.ie_content_ptr = (uint8_t*) &synch;

        added_len = ie802154e_add_payloadie(&pie, cur_buf_ptr, cur_remain_len);

        cur_buf_ptr += added_len;
        cur_remain_len -= added_len;
    }

    // At last, IE is list terminate IE
    added_len = ie802154e_add_payloadie_term(cur_buf_ptr, cur_remain_len);

    cur_buf_ptr += added_len;
    cur_remain_len -= added_len;

    return (ie_buflen - cur_remain_len);
}

/**************************************************************************************************//**
*
* @brief      setup the beacon adventry
*
* @param[in]   adv_entry -- pointer of NHLDB_TSCH_NONPC_advEntry_t
*              pan_desc -- pointer of PAN descriptor
*              ie_buf  -- pointer of data buffer
*              ie_buflen -- number of bytes can be stored in the buffer
*
*
* @param[out]
*
* @return
*
***************************************************************************************************/
void
NHLDB_TSCH_NONPC_setup_adventry(NHLDB_TSCH_NONPC_advEntry_t* adv_entry,
                                tschPanDesc_t* pan_desc, uint8_t* ie_buf, uint8_t ie_buflen)
{
    ie802154e_pie_t pie;
    uint8_t *pcontent;
    uint16_t remainlen = ie_buflen;
    uint8_t *pbuf = ie_buf;
    uint8_t group_id;
    uint8_t elem_id;
    uint16_t elemlen;
    uint16_t sublen;
    uint16_t mlmelen = 0;
    ie_tsch_link_period_t linkPeriodIE;
    ie_tsch_link_comp_t linkCompIE;
    tschMlmeSetLinkReq_t   setLinkArgCopy;
    uint8_t shLinkCount = 0;
    
    memset(&linkPeriodIE,0,sizeof(ie_tsch_link_period_t));
    memset(&linkCompIE,0,sizeof(ie_tsch_link_comp_t));
    memset(&pie, 0, sizeof(pie));

    adv_entry->rssi = (pan_desc->rssi+adv_entry->rssi*adv_entry->rcvdBeacons)/(adv_entry->rcvdBeacons+1);
    adv_entry->lqi = (pan_desc->linkQuality+adv_entry->lqi*adv_entry->rcvdBeacons)/(adv_entry->rcvdBeacons+1);
    adv_entry->rcvdBeacons += 1;
    adv_entry->timestamp = pan_desc->timestamp; 
    
    while(remainlen && (elemlen = ie802154e_parse_payloadie(pbuf, remainlen, &pie)))
    {
        pbuf += elemlen;
        remainlen -= elemlen;

        pcontent = pie.ie_content_ptr;
        group_id = pie.ie_group_id;
        mlmelen = pie.ie_content_length;

        if(group_id == IE_GROUP_ID_MLME)
        {
            while(mlmelen && (sublen = ie802154e_parse_payloadsubie(pcontent, mlmelen, &pie)))
            {
                pcontent += sublen;
                mlmelen -= sublen;

                elem_id = pie.ie_elem_id;

                switch(elem_id)
                {
                    case IE_ID_TSCH_SYNCH:
                    {
                        ie_tsch_synch_t synch;

                        ie802154e_parse_tsch_synch(pie.ie_content_ptr, &synch);
#ifdef REMOVE_JP
                         adv_entry->join_prio = 0;
#else
                        adv_entry->join_prio = synch.macJoinPriority;
#endif
                        adv_entry->asn = synch.macASN;
                    }
                    break;
                    case IE_ID_TSCH_SLOTFRAMELINK:
                    {
                        ie_tsch_slotframelink_t    slotframelinkinfo;
                        uint8_t sf_iter;

                        ie802154e_parse_tsch_slotframelink(pie.ie_content_ptr, &slotframelinkinfo);

                        adv_entry->num_sfs = slotframelinkinfo.numSlotframes;
                        if(adv_entry->num_sfs > NHLDB_TSCH_NONPC_MAX_NUM_SLOTFRAMES_IN_ADV)
                        {
                            adv_entry->num_sfs = NHLDB_TSCH_NONPC_MAX_NUM_SLOTFRAMES_IN_ADV;
                        }

                        for(sf_iter=0; sf_iter< adv_entry->num_sfs; sf_iter++)
                        {
                            ie_slotframe_info_field_t* slotframeinfo;
                            tschMlmeSetSlotframeReq_t* setSfArg;
                            uint8_t link_iter;

                            slotframeinfo = &slotframelinkinfo.slotframe_info_list[sf_iter];
                            setSfArg = &adv_entry->sf[sf_iter];
                            setSfArg->slotframeHandle = slotframeinfo->macSlotframeHandle;
                            setSfArg->size = slotframeinfo->macSlotframeSize;

#if COMPRESS_SH_LINK_IE
                            if (linkCompIE.numContTimeslot >0)
                            {
                                adv_entry->num_links_in_adv[sf_iter] =
                                    slotframeinfo->numLinks
                                        + linkCompIE.numContTimeslot - 1;
                            }
                            else
                            {
                                adv_entry->num_links_in_adv[sf_iter] =
                                    slotframeinfo->numLinks;
                            }
#else
                            adv_entry->num_links_in_adv[sf_iter] = slotframeinfo->numLinks;
#endif
                            if(adv_entry->num_links_in_adv[sf_iter] > NHLDB_TSCH_NONPC_MAX_NUM_LINKS_IN_ADV) {
                                adv_entry->num_links_in_adv[sf_iter] = NHLDB_TSCH_NONPC_MAX_NUM_LINKS_IN_ADV;
                            }

                            for(link_iter=0; link_iter<adv_entry->num_links_in_adv[sf_iter]; link_iter++)
                            {

                                tschMlmeSetLinkReq_t*     setLinkArg;
                                ie_link_info_field_t* linkinfo;

                                setLinkArg = &adv_entry->sf_link_in_adv[sf_iter][link_iter];
                                linkinfo = &slotframeinfo[sf_iter].link_info_list[link_iter];

                                if ((shLinkCount == 0)
                                    || (linkinfo->macLinkOption == (MAC_LINK_OPTION_RX|MAC_LINK_OPTION_TK)))
                                {
                                    setLinkArg->operation = SET_LINK_ADD;
                                    setLinkArg->slotframeHandle = slotframeinfo->macSlotframeHandle;
                                    setLinkArg->timeslot = linkinfo->macTimeslot;
                                    setLinkArg->channelOffset = linkinfo->macChannelOffset;
                                    setLinkArg->linkOptions = linkinfo->macLinkOption;

#if TSCH_PER_SCHED
                                    setLinkArg->period = linkPeriodIE.period[link_iter];
                                    setLinkArg->periodOffset = linkPeriodIE.periodOffset[link_iter];
#else
                                    setLinkArg->period = 1;
                                    setLinkArg->periodOffset = 0;
#endif
                                    if(linkinfo->macLinkOption == (MAC_LINK_OPTION_RX|MAC_LINK_OPTION_TK))
                                        setLinkArg->linkType = MAC_LINK_TYPE_ADV;
                                    else
                                        setLinkArg->linkType = MAC_LINK_TYPE_NORMAL;

                                    if (setLinkArg->linkOptions & MAC_LINK_OPTION_SH)
                                    {
                                        setLinkArg->nodeAddr = 0xffff;

                                        if ((linkCompIE.linkType == IE_SH_LINK) && (shLinkCount == 0))
                                        {
                                            shLinkCount++;
                                            memcpy(&setLinkArgCopy,setLinkArg,sizeof(tschMlmeSetLinkReq_t));
                                        }
                                    }
                                    else
                                    {
                                        setLinkArg->nodeAddr = ulpsmacaddr_to_short(&adv_entry->addr);
                                    }
                                }
                                else if (shLinkCount < linkCompIE.numContTimeslot)
                                {
                                      memcpy(setLinkArg,&setLinkArgCopy,sizeof(tschMlmeSetLinkReq_t));
                                      setLinkArg->timeslot = linkCompIE.startTimeslot + shLinkCount;
                                      shLinkCount++;
                                }
                            } // for each link in a slotframe
                        } // for each slotframe in slotframelink
                    }
                    break;
                    case IE_ID_TSCH_TIMESLOT:
                    {
                        ie_timeslot_template_t ie_tt;

                        ie802154e_parse_timeslot_template(&pie, &ie_tt);

                        adv_entry->ts_template_len = pie.ie_sub_content_length;
                        memcpy(&adv_entry->ts_template, &ie_tt, sizeof(ie_tt));
                    }
                    break;
                    case IE_ID_EB_FILTER:
                    {

                    }
                    break;
                    case IE_ID_CHANNELHOPPING:
                    {
                        ie_hopping_t ie_hop;

                        ie802154e_parse_hopping_timing(&pie, &ie_hop);
                        adv_entry->hop_seq_id = ie_hop.macHoppingSequenceID;
                    }
                    break;
                    case IE_ID_TSCH_LINK_PERIOD:
                    {
                        ie802154e_parse_link_period(&pie, sublen, &linkPeriodIE);
                    }
                    break;
                    case IE_ID_TSCH_COMPRESS_LINK:
                    {
                        ie802154e_parse_link_compression(&pie, &linkCompIE);
                    }
                    break;
                    default:
                    break;
                }
            }
        }
        else if(group_id == IE_GROUP_ID_ESDU ||
                (group_id >= IE_GROUP_ID_UNMG_L && group_id <= IE_GROUP_ID_UNMG_H))
        {

        }
        else if(group_id == IE_GROUP_ID_TERM)
            break;
        else
            break;
    }
}


/**************************************************************************************************//**
*
* @brief      update the beacon adventry
*
* @param[in]   adv_entry -- pointer of NHLDB_TSCH_NONPC_advEntry_t
*              pan_desc -- pointer of PAN descriptor
*              ie_buf  -- pointer of data buffer
*              ie_buflen -- number of bytes can be stored in the buffer
*
*
* @param[out]
*
* @return
*
***************************************************************************************************/
void
NHLDB_TSCH_NONPC_update_adventry(NHLDB_TSCH_NONPC_advEntry_t* adv_entry,
                                 tschPanDesc_t* pan_desc, uint8_t* ie_buf, uint8_t ie_buflen)
{
    adv_entry->rssi = pan_desc->rssi;
    adv_entry->lqi = pan_desc->linkQuality;
}


/**************************************************************************************************//**
*
* @brief      parse the association response IE
*
* @param[in]   adv_entry -- pointer of NHLDB_TSCH_NONPC_advEntry_t
*              ie_buf  -- pointer of data buffer
*              ie_buflen -- number of bytes can be stored in the buffer
*
*
* @param[out]
*
* @return
*
***************************************************************************************************/
uint16_t
NHLDB_TSCH_NONPC_parse_assocrespsuggestion(uint8_t* ie_buf, uint8_t ie_buflen)
{
    ie802154e_pie_t pie;
    uint8_t suggest_parent;
    uint16_t remainlen = ie_buflen;
    uint8_t *pcontent;
    uint8_t *pbuf = ie_buf;
    uint8_t group_id;
    uint8_t elem_id;
    uint16_t elemlen;
    uint16_t sublen;
    uint16_t mlmelen = 0;
    while(remainlen && (elemlen = ie802154e_parse_payloadie(pbuf, remainlen, &pie)))
    {
        pbuf += elemlen;
        remainlen -= elemlen;

        pcontent = pie.ie_content_ptr;
        group_id = pie.ie_group_id;
        mlmelen = pie.ie_content_length;

        if(group_id == IE_GROUP_ID_MLME)
        {
            while(mlmelen && (sublen = ie802154e_parse_payloadsubie(pcontent, mlmelen, &pie)))
            {
                pcontent += sublen;
                mlmelen -= sublen;

                elem_id = pie.ie_elem_id;

                switch(elem_id)
                {
                    case IE_ID_TSCH_SUGGEST_PARENT:
                    {
			suggest_parent = (pie.ie_content_ptr[0]<<8)+pie.ie_content_ptr[1];
                    }
                    break;
                    default:
                    break;
                }
            }
        }
    }

    return suggest_parent;
}

/**************************************************************************************************//**
*
* @brief      parse the association response IE
*
* @param[in]   adv_entry -- pointer of NHLDB_TSCH_NONPC_advEntry_t
*              ie_buf  -- pointer of data buffer
*              ie_buflen -- number of bytes can be stored in the buffer
*
*
* @param[out]
*
* @return
*
***************************************************************************************************/
uint8_t
NHLDB_TSCH_NONPC_parse_assocrespie(NHLDB_TSCH_NONPC_advEntry_t* adv_entry, uint8_t* ie_buf, uint8_t ie_buflen)
{
    ie802154e_pie_t pie;
    ie_tsch_link_period_t linkPeriod;
    uint8_t got_adv_tx_link;
    uint16_t remainlen = ie_buflen;
    uint8_t *pcontent;
    uint8_t *pbuf = ie_buf;
    uint8_t group_id;
    uint8_t elem_id;
    uint16_t elemlen;
    uint16_t sublen;
    uint16_t mlmelen = 0;

    //adv_entry->assoc_stat = NHLDB_TSCH_NONPC_ASSOC_CONFIRM;
    got_adv_tx_link = 0;
    num_bcn_links = 0;

    memset(&linkPeriod,0,sizeof(ie_tsch_link_period_t));

    while(remainlen && (elemlen = ie802154e_parse_payloadie(pbuf, remainlen, &pie)))
    {
        pbuf += elemlen;
        remainlen -= elemlen;

        pcontent = pie.ie_content_ptr;
        group_id = pie.ie_group_id;
        mlmelen = pie.ie_content_length;

        if(group_id == IE_GROUP_ID_MLME)
        {
            while(mlmelen && (sublen = ie802154e_parse_payloadsubie(pcontent, mlmelen, &pie)))
            {
                pcontent += sublen;
                mlmelen -= sublen;

                elem_id = pie.ie_elem_id;

                switch(elem_id)
                {
                    case IE_ID_TSCH_LINK_PERIOD:
                    {
                        ie802154e_parse_link_period(&pie, sublen, &linkPeriod);
                    }
                    break;
                    case IE_ID_TSCH_SLOTFRAMELINK:
                    {
                        ie_tsch_slotframelink_t    slotframelinkinfo;
                        uint8_t sf_iter,link_iter;;

                        ie802154e_parse_tsch_slotframelink(pie.ie_content_ptr, &slotframelinkinfo);

                        for(sf_iter=0; sf_iter<slotframelinkinfo.numSlotframes; sf_iter++)
                        {
                            ie_slotframe_info_field_t* slotframeinfo;

                            slotframeinfo = &slotframelinkinfo.slotframe_info_list[sf_iter];

                            for(link_iter=0; link_iter<slotframeinfo->numLinks; link_iter++)
                            {
                                ie_link_info_field_t*      linkinfo;
                                tschMlmeSetLinkReq_t setLinkArg;

                                linkinfo = &slotframeinfo->link_info_list[link_iter];

                                setLinkArg.slotframeHandle = slotframeinfo->macSlotframeHandle;
                                setLinkArg.timeslot = linkinfo->macTimeslot;
                                setLinkArg.channelOffset = linkinfo->macChannelOffset;
                                setLinkArg.linkOptions = linkinfo->macLinkOption;
#if TSCH_PER_SCHED
                                setLinkArg.period = linkPeriod.period[link_iter];
                                setLinkArg.periodOffset = linkPeriod.periodOffset[link_iter];
#else
                                setLinkArg.period = 1;
                                setLinkArg.periodOffset = 0;
#endif
                                if(setLinkArg.linkOptions == (MAC_LINK_OPTION_TX | MAC_LINK_OPTION_TK))
                                {
                                    got_adv_tx_link = 1;
                                    setLinkArg.nodeAddr = 0xffff;
                                    setLinkArg.linkType = MAC_LINK_TYPE_ADV;
                                }
                                else
                                {
                                    setLinkArg.nodeAddr = ulpsmacaddr_to_short(&adv_entry->addr);
                                    setLinkArg.linkType = MAC_LINK_TYPE_NORMAL;
                                }

                                if(NHLDB_TSCH_assign_link_handle(setLinkArg.slotframeHandle, &setLinkArg) == ASSIGN_NEW_LINK)
                                {
                                    setLinkArg.operation = SET_LINK_ADD;
                                    TSCH_MlmeSetLinkReq(&setLinkArg);
                                }

                                if ( setLinkArg.linkType == MAC_LINK_TYPE_ADV)
                                {
                                    tschMlmeSetLinkReq_t* setLinkInadv;

                                    setLinkInadv = &nonpc_links_in_adv[sf_iter][nonpc_num_links_in_adv[sf_iter]];
                                    setLinkArg.linkOptions = MAC_LINK_OPTION_RX | MAC_LINK_OPTION_TK;
                                    memcpy(setLinkInadv, &setLinkArg, sizeof(tschMlmeSetLinkReq_t));
                                    nonpc_num_links_in_adv[sf_iter]++;
                                    num_bcn_links++;
                                }
                            } // for each link
                        } // for each slotframe

                    }
                    break;
                    default:
                    break;
                }
            }
        }
    }

    return got_adv_tx_link;
}

/**************************************************************************************************//**
*
* @brief      Crate association request IE
*
* @param[in]   period --  TX period if it app is periodic
*              numFragPackets  -- Number of fragmented packets from 6lowpan
*              req_usm_buf -- IE buffer
*
*
* @param[out]
*
* @return
*
***************************************************************************************************/

uint8_t NHLDB_TSCH_NONPC_create_assocreq_ie(uint8_t period, uint8_t numFragPackets,BM_BUF_t * req_usm_buf)
{
    BM_BUF_t *pBuf;
    uint8_t dataLen;
    ie802154e_pie_t pie;
    uint8_t* cur_buf_ptr;
    uint8_t  cur_remain_len;
    uint8_t  added_len;
    uint8_t ie_length;

    ie_tsch_periodic_scheduler_t perSched;

    pBuf = req_usm_buf;
    dataLen = BM_getDataLen(pBuf);

    perSched.period = period;
    perSched.numFragPackets = numFragPackets;
    perSched.numHops = 1;

    ie_length = sizeof(ie_tsch_periodic_scheduler_t);

    pie.ie_content_length = MIN_PAYLOAD_IE_LEN+ie_length;
    pie.ie_group_id = IE_GROUP_ID_MLME;
    pie.ie_type = IE_TYPE_PAYLOAD;
    pie.ie_sub_content_length = ie_length;
    pie.ie_elem_id = IE_ID_TSCH_PERIODIC_SCHED;
    pie.ie_sub_type = IE_SUBTYPE_SHORT;
    pie.ie_content_ptr = (uint8_t*) &perSched;
    pie.outer_descriptor = 1;

    dataLen = BM_getDataLen(pBuf);
    cur_buf_ptr = BM_getDataPtr(pBuf)+ dataLen;
    cur_remain_len = ULPSMACBUF_DATA_SIZE - dataLen;

    added_len = ie802154e_add_payloadie(&pie, cur_buf_ptr, cur_remain_len);

    dataLen +=added_len;
    cur_buf_ptr += added_len;
    cur_remain_len -= added_len;

    // At last, IE is list terminate IE
    added_len = ie802154e_add_payloadie_term(cur_buf_ptr, cur_remain_len);

    BM_setDataLen(pBuf, dataLen + added_len);

    return NHL_TRUE;
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
     tschMlmeSetLinkReq_t* link;
     uint8_t i,j;

    status = UPMAC_STAT_ERR;

    /* No need to modify*/
    if (nonpc_num_links_in_adv[0] == (num_bcn_links+1))
      return status;

    pBuf = BM_alloc(23);

    if (pBuf == NULL)
    {
        return status;
    }

    for (i=0;i<MAX_NUM_SLOTFRAMES;i++)
    {
        for (j=0;j<(nonpc_num_links_in_adv[i]-1);j++)
        {
            link = &nonpc_links_in_adv[i][j];

            if (j == 0)
            {
                link->period = TSCH_MACConfig.sh_period_sf;
            }
            else
            {
                memset(link,0,sizeof(tschMlmeSetLinkReq_t));
            }
        }
       /* The last one is beacon link*/
       link = &nonpc_links_in_adv[i][j];
       memcpy(&nonpc_links_in_adv[i][1], link, sizeof(tschMlmeSetLinkReq_t));
       memset(link,0,sizeof(tschMlmeSetLinkReq_t));

       nonpc_num_links_in_adv[i] = num_bcn_links+1;
    }

    pData = BM_getBufPtr(pBuf);

    parsed_len = NHLDB_TSCH_NONPC_create_advie(pData, BM_BUFFER_SIZE);

    BM_setDataLen(pBuf,parsed_len);

    status = PNM_modify_ebPayloadIeList(pBuf);

    BM_free_6tisch(pBuf);

    return status;
}

#if ULPSMAC_L2MH
/**************************************************************************************************//**
*
* @brief      parse L2MH associate IE
*
* @param[in]   assocShortAddr -- short address of L2MH association request
*               ie_buf  -- pointer of data buffer
*              ie_buflen -- number of bytes can be stored in the buffer
*
*
* @param[out]
*
* @return
*
***************************************************************************************************/
void
NHLDB_TSCH_NONPC_parse_assocrespl2mh_ie(uint16_t assocShortAddr, uint8_t* ie_buf, uint8_t ie_buflen)
{
  ie802154e_pie_t pie;
  ie_tsch_link_period_t linkPeriod;
  
  
  uint16_t remainlen = ie_buflen;
  uint8_t *pcontent;
  uint8_t *pbuf = ie_buf;
  uint8_t group_id;
  uint8_t elem_id;
  uint16_t elemlen;
  uint16_t sublen;
  uint16_t mlmelen = 0;
  
  memset(&linkPeriod,0,sizeof(ie_tsch_link_period_t));
  
  while(remainlen && (elemlen = ie802154e_parse_payloadie(pbuf, remainlen, &pie)))
  {
    pbuf += elemlen;
    remainlen -= elemlen;
    
    pcontent = pie.ie_content_ptr;
    group_id = pie.ie_group_id;
    mlmelen = pie.ie_content_length;
    
    if(group_id == IE_GROUP_ID_MLME)
    {
      while(mlmelen && (sublen = ie802154e_parse_payloadsubie(pcontent, mlmelen, &pie)))
      {
        pcontent += sublen;
        mlmelen -= sublen;
        
        elem_id = pie.ie_elem_id;
        
        switch(elem_id)
        {
        case IE_ID_TSCH_LINK_PERIOD:
          {
            ie802154e_parse_link_period(&pie, sublen, &linkPeriod);
          }
          break;
        case IE_ID_TSCH_SLOTFRAMELINK:
          {
            ie_tsch_slotframelink_t    slotframelinkinfo;
            ie_slotframe_info_field_t* slotframeinfo;
            ie_link_info_field_t*      linkinfo;
            uint8_t sf_iter,link_iter;;
            
            ie802154e_parse_tsch_slotframelink(pie.ie_content_ptr, &slotframelinkinfo);
            
            slotframeinfo = slotframelinkinfo.slotframe_info_list;
            
            for(sf_iter=0; sf_iter<slotframelinkinfo.numSlotframes; sf_iter++)
            {
              linkinfo = slotframeinfo[sf_iter].link_info_list;
              
              for(link_iter=0; link_iter<slotframeinfo->numLinks; link_iter++)
              {
                tschMlmeSetLinkReq_t setLinkArg;
                uint8_t linkType = 0xff;
                uint8_t status;
                
                if ((linkinfo[link_iter].macLinkOption == MAC_LINK_OPTION_TX) ||
                    (linkinfo[link_iter].macLinkOption == (MAC_LINK_OPTION_RX | MAC_LINK_OPTION_TK)))
                {
                  linkType = MAC_LINK_TYPE_NORMAL;
                }
                
                if((linkinfo[link_iter].macLinkOption & (MAC_LINK_OPTION_TX | MAC_LINK_OPTION_RX)) &&
                   (linkType == MAC_LINK_TYPE_NORMAL))
                {
                  setLinkArg.slotframeHandle = slotframeinfo[sf_iter].macSlotframeHandle;
                  setLinkArg.timeslot = linkinfo[link_iter].macTimeslot;
                  setLinkArg.channelOffset = linkinfo[link_iter].macChannelOffset;
                  setLinkArg.linkType = MAC_LINK_TYPE_NORMAL;
#if TSCH_PER_SCHED
                  setLinkArg.period = linkPeriod.period[link_iter];
                  setLinkArg.periodOffset = linkPeriod.periodOffset[link_iter];
#else
                  setLinkArg.period = 1;
                  setLinkArg.periodOffset = 0;
#endif
                  
                  if(linkinfo[link_iter].macLinkOption & MAC_LINK_OPTION_TX)
                  {
                    setLinkArg.linkOptions = MAC_LINK_OPTION_RX;
                  } else if(linkinfo[link_iter].macLinkOption & MAC_LINK_OPTION_RX)
                  {
                    setLinkArg.linkOptions = MAC_LINK_OPTION_TX | MAC_LINK_OPTION_WAIT_CONFIRM ;
                  }
                  
                  setLinkArg.nodeAddr = assocShortAddr;
                  
                  status = NHLDB_TSCH_assign_link_handle(setLinkArg.slotframeHandle, &setLinkArg);
                  
                  if(status == ASSIGN_NEW_LINK)
                  {
                    setLinkArg.operation = SET_LINK_ADD;
                    TSCH_MlmeSetLinkReq(&setLinkArg);
                  }
                  else if (status == ASSIGN_LINK_EXIST)
                  {
                    setLinkArg.operation = SET_LINK_MODIFY;
                    TSCH_MlmeSetLinkReq(&setLinkArg);
                  }
                }
              } // for each link
            } // for each slotframe
          }
          break;
          default:
          break;
        }
      }
    }
  }
}

/**************************************************************************************************//**
*
* @brief      add the l2MH assocaition request to table
*
* @param[in]   deviceExtAddr -- pointer of address (originating device request)
*              requestorShrtAddr -- short address of forward the association request
*              prevAddrMode  -- address mode of reverse direction address
*              prevAddr      -- pointer of reverse direction address
*
* @param[out]
*
* @return     status of operation
*
***************************************************************************************************/
uint8_t
NHLDB_TSCH_NONPC_add_l2mhent(const ulpsmacaddr_t* deviceExtAddr,
                             const uint16_t requestorShrtAddr,
                             const uint8_t prevAddrMode, const ulpsmacaddr_t* prevAddr)
{
    uint8_t table_iter;

    for(table_iter=0; table_iter<MAX_NUM_ASSOC_L2MH_TABLE; table_iter++) {

        if(assoc_l2mh_table[table_iter].inUse == NHL_FALSE) {
            NHLDB_TSCH_NONPC_l2mhent_t* cur_table;

            cur_table = &assoc_l2mh_table[table_iter];

            cur_table->inUse = NHL_TRUE;
            cur_table->prevNodeAddrMode = prevAddrMode;
            if(prevAddrMode == ULPSMACADDR_MODE_LONG) {
                ulpsmacaddr_long_copy(&cur_table->prevNodeAddr, prevAddr);
            } else {
                ulpsmacaddr_short_copy(&cur_table->prevNodeAddr, prevAddr);
            }
            ulpsmacaddr_long_copy(&cur_table->deviceExtAddr, deviceExtAddr);
            cur_table->requestorShortAddr = requestorShrtAddr;

            return NHL_TRUE;
        }
    }

    return NHL_FALSE;
}

/**************************************************************************************************//**
*
* @brief      remove the l2MH assocaition request to table
*
* @param[in]   cur_table -- pointer of entry in the table
*
*
* @param[out]
*
* @return    status of operation
*
***************************************************************************************************/
uint8_t
NHLDB_TSCH_NONPC_rem_l2mhent(NHLDB_TSCH_NONPC_l2mhent_t* cur_table)
{
    if(cur_table->inUse == NHL_TRUE) {
        cur_table->inUse = NHL_FALSE;
        return NHL_TRUE;
    }

    return NHL_FALSE;
}

/**************************************************************************************************//**
*
* @brief      find the entry in the l2MH assocaition request table
*
* @param[in]   deviceExtAddr -- pointer of device address
*              requestorShrtAddr -- short address of request (forwarding the original association
*                                   request)
*
*
* @param[out]
*
* @return    pointer of entry in the l2MH assocaition request table
*
***************************************************************************************************/
NHLDB_TSCH_NONPC_l2mhent_t*
NHLDB_TSCH_NONPC_find_l2mhent(const ulpsmacaddr_t* deviceExtAddr,
                              const uint16_t requestorShrtAddr)
{
    uint8_t table_iter;
    for(table_iter=0; table_iter<MAX_NUM_ASSOC_L2MH_TABLE; table_iter++) {
        NHLDB_TSCH_NONPC_l2mhent_t* cur_table;

        cur_table = &assoc_l2mh_table[table_iter];
        if(ulpsmacaddr_long_cmp(&cur_table->deviceExtAddr, deviceExtAddr) &&
           (cur_table->requestorShortAddr == requestorShrtAddr)) {
               return cur_table;
           } // if same device addr

    }

    return NULL;
}
#endif /* ULPSMAC_L2MH */
