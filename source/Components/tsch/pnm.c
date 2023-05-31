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
 *  ====================== pnm.c =============================================
 *  
 */
#include "hal_api.h"

#if defined (__IAR_SYSTEMS_ICC__) || defined(__TI_COMPILER_VERSION__)
#include <string.h>
#endif

#include "tsch_os_fun.h"
#include "mac_config.h"
#include "tsch_api.h"
#include "nhl_mgmt.h"
#include "ulpsmacaddr.h"
#include "prmtv802154e.h"

#include "pnm-db.h"
#include "mac_pib.h"
#include "tsch-db.h"
#include "tsch-acm.h"

#include "sys/clock.h"

// Scan and Start
#include "nhl_api.h"

extern void coap_msg_post_scan_timer();
/*---------------------------------------------------------------------------*/

#define SCAN_CHANNELS_LEN 3

CbackFunc_t MlmeCb = NULL;
CbackFunc_t McpsCb = NULL;

static pnmPib_t* pnm_pnmdb_ptr;

uint8_t cur_scan_idx;
static uint8_t cur_scan_type;
uint8_t scan_channels[SCAN_CHANNELS_LEN];
static uint8_t scan_in_progress = UPMAC_FALSE;

static clock_time_t scan_dur;

extern void TSCH_startupMsg(void);

/**************************************************************************************************//**
*
* @brief       This function will initialize the pnm_pnmdb_ptr.
*
*
* @param[in]   pnm_db_ptr -- pointer of PNM_DB data structure
*
*
* @return
*
***************************************************************************************************/
void PNM_init(pnmPib_t* pnm_db_ptr)
{
    pnm_pnmdb_ptr = pnm_db_ptr;
}

/**************************************************************************************************//**
 *
 * @brief       This function is called by a coordinator or PAN coordinator to start or
 *              reconfigure a network.  Before starting a network the device must have set its
 *              short address.  A PAN coordinator sets the short address by setting the attribute
 *              MAC_SHORT_ADDRESS.  A coordinator or device sets the short address through association.
 *              When parameter panCoordinator is TRUE, the MAC automatically sets attributes
 *              MAC_PAN_ID and start the beacon transmission.  If panCoordinator is FALSE, these
 *              parameters are ignored (they would already be set through association).
 *
 *
 * @param[in]   parg -- the pointer to tschMlmeStartReq_t data structure
 *
 *
 * @return      status
 *
 ***************************************************************************************************/
uint16_t TSCH_MlmeStartReq(tschMlmeStartReq_t* arg)
{
    uint16_t res;
    ie802154e_pie_t find_ie;
    uint8_t found;

    if( ulpsmacaddr_short_node == 0)
    {
        return UPMAC_STAT_NO_SHORT_ADDRESS;
    }

    // Need to be less than ULPSMACBUF_DATA_SIZE - MAX_HEADER_SIZE (25 bytes)
    pnm_pnmdb_ptr->ebPayloadIeListLen = BM_copyto(arg->pIE_List, pnm_pnmdb_ptr->ebPayloadIeList);

    find_ie.ie_elem_id = IE_ID_TSCH_SYNCH;
    found = ie802154e_find_in_payloadielist(pnm_pnmdb_ptr->ebPayloadIeList, pnm_pnmdb_ptr->ebPayloadIeListLen, &find_ie);
    if(found)
    {
        pnm_pnmdb_ptr->ebPayloadIeASNOs = (uint8_t) (find_ie.ie_content_ptr - pnm_pnmdb_ptr->ebPayloadIeList);
    }
    else
    {
        /* no ASN field in IE */
        return UPMAC_STAT_INVALID_PARAMETER;
    }

    res = UPMAC_STAT_SUCCESS;
    if(arg->panCoordinator == UPMAC_TRUE)
    {
        ulpsmacaddr_panid_node = arg->panId;

        // set up the PAN ID
        TSCH_MlmeSetReq(TSCH_MAC_PIB_ID_macPANId, &ulpsmacaddr_panid_node);
#ifdef FEATURE_MAC_SECURITY
        //Default Key Source and Default Lookup Id
        {
          uint16_t my_shortaddr;
          TSCH_MlmeGetReq(TSCH_MAC_PIB_ID_macShortAddress, &my_shortaddr);
          MAC_SecurityKeyIdUpate(ulpsmacaddr_panid_node, (uint8_t *)&my_shortaddr, &ulpsmacaddr_long_node);
        }
#endif
        pnm_pnmdb_ptr->is_coord = 1;
        res = ULPSMAC_LOWMAC.set_pan_coord();
    }

    return res;
}

/**************************************************************************************************//**
 *
 * @brief       This function is callback function of channel scan timeout. It will post message
 *              to NHL task.
 *
 *
 * @param[in]   ptr -- the void pointer
 *
 *
 * @return
 *
 ***************************************************************************************************/
void scan_timer_cb(void* ptr)
{
#if !IS_ROOT
   coap_msg_post_scan_timer();
#endif
}
/*******************************************************************************
* @fn
*
* @brief
*
* @param
*
* @return      none
********************************************************************************/
void TSCH_scan_timer_handler()
{
   uint8_t next_scan_channel;

   cur_scan_idx++;

   if (TSCH_MACConfig.bcn_ch_mode != 0xff)
   {
       if ((scan_channels[cur_scan_idx]!=0) && (cur_scan_idx < SCAN_CHANNELS_LEN))
       {
           next_scan_channel = scan_channels[cur_scan_idx];
       }
       else
       {
           scan_in_progress = UPMAC_FALSE;
       }
   }
   else
   {
       if (cur_scan_idx < HAL_RADIO_NUM_CHANNELS)
       {
           next_scan_channel = scan_channels[0]+cur_scan_idx;
       }
       else
       {
           scan_in_progress = UPMAC_FALSE;
       }
   }

     // no more channel to scan
    if(scan_in_progress == UPMAC_FALSE)
    {

        tschMlmeScanConfirm_t arg;

        HAL_LED_set(1,0);
        HAL_RADIO_receiveOff();
        HAL_LED_set(2,0);

        arg.hdr.status = UPMAC_STAT_SUCCESS;
        arg.scanType = cur_scan_type;


        if (MlmeCb)
        {
            arg.hdr.event = MAC_MLME_SCAN_CNF;
            MlmeCb((tschCbackEvent_t *)(&arg), NULL);
        }
        return;
    }

    if (HAL_RADIO_setChannel(next_scan_channel) == HAL_SUCCESS)
    {
       SN_clockSet(scan_ct, 0, scan_dur);
    }
    else
    {
       ULPSMAC_Dbg.numSetChannelErr++;
       SN_clockSet(scan_ct, 0, 10);
    }
}

/**************************************************************************************************//**
 *
 * @brief       This function initiates an active, or passive on one or more channels.
 *              An active scan sends a beacon request on each channel and then listening for beacons.
 *              A passive scan is a receive-only operation that listens for beacons on each channel.
 *              When a scan operation is complete the MAC sends a TSCH_MLME_SCAN_CNF to the application.
 *
 *
 * @param[in]   parg -- the pointer to tschMlmeScanReq_t data structure
 *
 * @return      none
 *
 ***************************************************************************************************/
void TSCH_MlmeScanReq(tschMlmeScanReq_t* arg)
{
    tschMlmeScanConfirm_t conf_cb_arg;

    if(scan_in_progress == UPMAC_TRUE)
    {
        ULPSMAC_Dbg.scanInProg++;
        conf_cb_arg.hdr.status = UPMAC_STAT_SCAN_IN_PROGRESS;
        conf_cb_arg.scanType = arg->scanType;
        if (MlmeCb)
        {
            conf_cb_arg.hdr.event = MAC_MLME_SCAN_CNF;
            MlmeCb((tschCbackEvent_t *)(&conf_cb_arg), NULL);
        }

        return;
    }

    if(arg->scanType != SCAN_TYPE_PASSIVE)
    {
        ULPSMAC_Dbg.scanInvalidPar++;
        conf_cb_arg.hdr.status = UPMAC_STAT_INVALID_PARAMETER;
        conf_cb_arg.scanType = arg->scanType;
        if (MlmeCb)
        {
            conf_cb_arg.hdr.event = MAC_MLME_SCAN_CNF;
            MlmeCb((tschCbackEvent_t *)(&conf_cb_arg), NULL);
        }
        return;
    }

    scan_dur = ULPSMAC_S_TO_CTICKS(arg->scanDuration);

    memcpy(scan_channels,arg->scanChannels,SCAN_CHANNELS_LEN);

    cur_scan_idx = 0;
    cur_scan_type = arg->scanType;

    scan_in_progress = UPMAC_TRUE;

    if (HAL_RADIO_setChannel(scan_channels[0]) == HAL_SUCCESS)
    {
      HAL_RADIO_receiveOn(TSCH_rxISR, 0);
    }
    else
    {
      ULPSMAC_Dbg.numSetChannelErr++;
      conf_cb_arg.hdr.status = UPMAC_STAT_ERR;
      conf_cb_arg.scanType = arg->scanType;
      if (MlmeCb)
      {
            conf_cb_arg.hdr.event = MAC_MLME_SCAN_CNF;
            MlmeCb((tschCbackEvent_t *)(&conf_cb_arg), NULL);
      }
      return;
    }
      
    HAL_LED_set(1,1);
    HAL_LED_set(2,1);

    SN_clockSet(scan_ct, 0, scan_dur);
}

/**************************************************************************************************//**
 *
 * @brief       This function is used to add, delete or modify a slot frame by device.
 *              After complete this operation, MAC shall return the status of this request.
 *
 * @param[in]   parg -- the pointer to tschMlmeSetSlotframeReq_t data structure
 *
 * @return      status of opertaion
 *
 ***************************************************************************************************/
uint16_t TSCH_MlmeSetSlotframeReq(tschMlmeSetSlotframeReq_t* req_arg)
{
    TSCH_PIB_slotframe_t sf;
    uint16_t size;
    uint16_t res;

    res = UPMAC_STAT_INVALID_PARAMETER;
    switch(req_arg->operation)
    {
        case SET_SLOTFRAME_ADD:
        {
            sf.slotframe_handle = req_arg->slotframeHandle;
            sf.slotframe_size = req_arg->size;
            size = sizeof(TSCH_PIB_slotframe_t);
            res = ULPSMAC_LOWMAC.set_db(TSCH_PIB_SLOTFRAME, &sf, &size, NULL);
            break;
        }
        case SET_SLOTFRAME_DELETE:
        {
            sf.slotframe_handle = req_arg->slotframeHandle;
            size = sizeof(TSCH_PIB_slotframe_t);
            res = ULPSMAC_LOWMAC.delete_db(TSCH_PIB_SLOTFRAME, &sf, &size, NULL);
            break;
        }
        case SET_SLOTFRAME_MODIFY:
        // TODO SCHOI: LOWMAC does not support modify yet.
        break;
        default:
        break;
    }

    return res;

}


/**************************************************************************************************//**
 *
 * @brief       This function is used to add, delete or modify a link in a slot frame by device.
 *              After complete this operation, MAC shall return the status of this request.
 *
 * @param[in]   parg -- the pointer to tschMlmeSetLinkReq_t data structure
 *
 * @return      status of opertaion
 *
 ***************************************************************************************************/
uint16_t TSCH_MlmeSetLinkReq(tschMlmeSetLinkReq_t* req_arg)
{
    TSCH_PIB_link_t link;
    uint16_t size;
    uint16_t res;

    res = UPMAC_STAT_INVALID_PARAMETER;

    link.link_id =  req_arg->linkHandle;
    link.slotframe_handle = req_arg->slotframeHandle;

    link.timeslot = req_arg->timeslot;
    link.channel_offset = req_arg->channelOffset;
    link.link_option = req_arg->linkOptions;
    link.link_type = req_arg->linkType;
    link.peer_addr = req_arg->nodeAddr;
    link.period = req_arg->period;
    link.periodOffset = req_arg->periodOffset;

    size = sizeof(TSCH_PIB_link_t);

    switch(req_arg->operation)
    {
        case SET_LINK_ADD:
        {
            res = ULPSMAC_LOWMAC.set_db(TSCH_PIB_LINK, &link, &size, NULL);
            break;
        }
        case SET_LINK_DELETE:
        {
            res = ULPSMAC_LOWMAC.delete_db(TSCH_PIB_LINK, &link, &size, NULL);
            break;
        }
        case SET_LINK_MODIFY:
        {
            res = ULPSMAC_LOWMAC.modify_db(TSCH_PIB_LINK, &link, &size, NULL);
            break;
        }
        default:
        break;
    }

    return res;
}


/**************************************************************************************************//**
 *
 * @brief       This direct execute function will put the MAC into or out of the TSCH mode.
 *              After setting the TSCH mode, MAC shall return the status of this request.
 *
 * @param[in]   parg -- the pointer to tschMlmeTschModeReq_t data structure
 *
 * @return      MAC success or
 *              MAC_NO_SYNC when fails
 *
 ***************************************************************************************************/
uint16_t TSCH_MlmeTschModeReq(tschMlmeTschModeReq_t* arg)
{
    uint16_t res;

    if(arg->mode == TSCH_MODE_ON)
    {
        res = ULPSMAC_LOWMAC.on();
    }
    else
    {
        res = ULPSMAC_LOWMAC.off();
    }

    return res;
}


/**************************************************************************************************//**
 *
 * @brief       This function sets up keepalive period for specified device.
 *
 * @param[in]   parg -- the pointer to tschMlmeKeepAliveReq_t
 *
 * @return      MAC success or
 *              Error when fails
 *
 ***************************************************************************************************/
uint16_t TSCH_MlmeKeepAliveReq(tschMlmeKeepAliveReq_t* arg)
{
    TSCH_PIB_ti_keepalive_t set_ka;
    uint16_t size;

    if(arg->dstAddrShort == 0xffff) {
        return UPMAC_STAT_INVALID_PARAMETER;
    }

    set_ka.dst_addr_short = arg->dstAddrShort;
    set_ka.period = arg->period;

    size = sizeof(TSCH_PIB_ti_keepalive_t);
    return ULPSMAC_LOWMAC.set_db(TSCH_PIB_TI_KEEPALIVE, &set_ka, &size, NULL);
}

/**************************************************************************************************//**
 *
 * @brief       This function gets PNM PIB from data base
 *
 * @param[in]   attrb -- PIB ID of PNM attribute
 *
 * @param[out]  in_value -- void pointer for returned PIB value
 *              in_size -- pointer of returned PIB size
 *
 *
 * @return      MAC success or
 *              Erorr when fails
 *
 ***************************************************************************************************/
uint16_t PNM_get_db(uint16_t attrb, void* in_value, uint16_t* in_size)
{
    uint16_t res = UPMAC_STAT_SUCCESS;

    switch(attrb)
    {
    case PIB_802154E_RESP_WAITTIME:
        *((uint32_t *)in_value) = pnm_pnmdb_ptr->responseWaitTime;
        *in_size = sizeof(pnm_pnmdb_ptr->responseWaitTime);
        break;
        case PIB_802154E_EB_IE_LIST:
        *((void **) in_value) = pnm_pnmdb_ptr->ebPayloadIeList;
        *in_size = pnm_pnmdb_ptr->ebPayloadIeListLen;
        break;
        default:
        res = ULPSMAC_LOWMAC.get_db(attrb, in_value, in_size, NULL);
        break;
    }

    return res;

}

/**************************************************************************************************//**
 *
 * @brief       This function sets PNM PIB from data base
 *
 * @param[in]   attrb -- PIB ID of PNM attribute
 *              in_value -- void pointer for returned PIB value
 *              in_size -- pointer of returned PIB size *
 *
 * @return      MAC success or
 *              Erorr when fails
 *
 ***************************************************************************************************/
uint16_t PNM_set_db(uint16_t attrb, void* in_value, uint16_t* in_size)
{
    uint16_t res = UPMAC_STAT_SUCCESS;

    switch(attrb)
    {
        case PIB_802154E_RESP_WAITTIME:
        pnm_pnmdb_ptr->responseWaitTime = *(uint32_t *)in_value;
        *in_size = sizeof(pnm_pnmdb_ptr->responseWaitTime);
        break;
        case PIB_802154E_EB_IE_LIST:
        if((in_value != NULL) && (*in_size > 0) && (*in_size <= ULPSMACBUF_DATA_SIZE)) {
            ie802154e_pie_t find_ie;
            uint8_t found;

            pnm_pnmdb_ptr->ebPayloadIeListLen = *in_size;
            memcpy(pnm_pnmdb_ptr->ebPayloadIeList, in_value, *in_size);

            find_ie.ie_elem_id = IE_ID_TSCH_SYNCH;
            found = ie802154e_find_in_payloadielist(pnm_pnmdb_ptr->ebPayloadIeList, pnm_pnmdb_ptr->ebPayloadIeListLen, &find_ie);
            if(found) {
                pnm_pnmdb_ptr->ebPayloadIeASNOs = (uint8_t) (find_ie.ie_content_ptr - pnm_pnmdb_ptr->ebPayloadIeList);
            } else {
                /* no ASN field in IE */
                res= UPMAC_STAT_INVALID_PARAMETER;
            }
        } else {
            res = UPMAC_STAT_INVALID_PARAMETER;
        }
        break;
        default:
        res = ULPSMAC_LOWMAC.set_db(attrb, in_value, in_size, NULL);
        break;
    }

    return res;

}


/**************************************************************************************************//**
 *
 * @brief       This function is called to modify beacon payload IE.
 *
 *
 * @param[in]   pbuf -- pointer of beacon payload IE
 *
 *
 * @return      status
 *
 ***************************************************************************************************/
uint16_t PNM_modify_ebPayloadIeList(BM_BUF_t* pbuf)
{
    ie802154e_pie_t find_ie;
    uint8_t found;

    // Need to be less than ULPSMACBUF_DATA_SIZE - MAX_HEADER_SIZE (25 bytes)
    pnm_pnmdb_ptr->ebPayloadIeListLen = BM_copyto(pbuf, pnm_pnmdb_ptr->ebPayloadIeList);

    find_ie.ie_elem_id = IE_ID_TSCH_SYNCH;
    found = ie802154e_find_in_payloadielist(pnm_pnmdb_ptr->ebPayloadIeList, pnm_pnmdb_ptr->ebPayloadIeListLen, &find_ie);
    if(found)
    {
        pnm_pnmdb_ptr->ebPayloadIeASNOs = (uint8_t) (find_ie.ie_content_ptr - pnm_pnmdb_ptr->ebPayloadIeList);
    }
    else
    {
        /* no ASN field in IE */
        return UPMAC_STAT_INVALID_PARAMETER;
    }

    return UPMAC_STAT_SUCCESS;
}

/**************************************************************************************************//**
 *
 * @brief       This function is callback registration function
 *
 *
 * @param[in]   reg_event     -- event ID
 *              pCbFunc      -- pointer of callback function
 *
 *
 * @return
 *
 ***************************************************************************************************/
void TSCH_event_register(uint8_t reg_event, CbackFunc_t pCbFunc)
{
    if (reg_event == MAC_REG_MLME_EVENTS)
    {
        MlmeCb = pCbFunc;
    }
    else if (reg_event == MAC_REG_DATA_EVENTS)
    {
        McpsCb = pCbFunc;
    }
}
