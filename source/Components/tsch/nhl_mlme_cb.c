/*
* Copyright (c) 2015 Texas Instruments Incorporated
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
 *  ====================== nhl_mlme_cb.c =============================================
 *  
 */


//*****************************************************************************
// the includes
//*****************************************************************************
#include <string.h>
#include "nhl.h"
#include "nhl_mlme_cb.h"
#include "nhl_mgmt.h"
#include "nhldb-tsch-cmm.h"
#include "prmtv802154e.h"
#include "tsch_os_fun.h"
#include "tsch_api.h"
#include "ulpsmac.h"

#include "nhl_device_table.h"

#include "mac_config.h"

#ifdef ENABLE_HCT
#include "HCT/hct.h"
#endif


/**************************************************************************************************//**
 *
 * @brief       This function is NHL MLME callback function handler
 *
 *
 * @param[in]   arg     -- the pointer to tschCbackEvent_t data structure
 *              usm_buf     -- pointer to BM data buffer
 *
 * @return
 *
 ***************************************************************************************************/
void nhl_MlmeCb(tschCbackEvent_t *arg, BM_BUF_t* usm_buf)
{
    uint8_t cb_type=MAC_CB_ERR;
    uint8_t msg_type;
    uint8_t arg_size;
    NHLDB_TSCH_CMM_cmdmsg_t* cmm_msg;

    switch (arg->hdr.event)
    {
        case MAC_MLME_ASSOCIATE_IND:
        if (iam_coord)
        {
            msg_type = COMMAND_ASSOCIATE_INDICATION;
            arg_size = sizeof(tschMlmeAssociateIndication_t);
            cb_type |= MAC_CB_POST;
        }
        break;
        case MAC_MLME_ASSOCIATE_CNF:
        msg_type = COMMAND_ASSOCIATE_CONFIRM;
        arg_size = sizeof(tschMlmeAssociateConfirm_t);
        cb_type |= MAC_CB_POST;
        break;
        case MAC_MLME_DISASSOCIATE_IND:
        //if (iam_coord)
        {
            msg_type = COMMAND_DISASSOCIATE_INDICATION;
            arg_size = sizeof(tschMlmeDisassociateIndication_t);
            cb_type |= MAC_CB_POST;
        }
        break;
        case MAC_MLME_DISASSOCIATE_CNF:
        msg_type = COMMAND_DISASSOCIATE_CONFIRM;
        arg_size = sizeof(tschMlmeDisassociateConfirm_t);
        cb_type |= MAC_CB_POST;
        break;
        case MAC_MLME_BEACON_NOTIFY_IND:
        msg_type = COMMAND_BEACON_INDICATION;
        arg_size = sizeof(tschMlmeBeaconNotifyIndication_t);
        cb_type |= MAC_CB_POST;
        break;
        case MAC_MLME_SCAN_CNF:
        msg_type = COMMAND_SCAN_CONFIRM;
        arg_size = sizeof(tschMlmeScanConfirm_t);
        cb_type |= MAC_CB_POST;
        break;
        case MAC_MLME_COMM_STATUS_IND :
        msg_type = COMMAND_COMM_STATUS_INDICATION;
        arg_size = sizeof(tschMlmeCommStatusIndication_t);
        cb_type |= MAC_CB_POST;
        break;
        case MAC_MLME_JOIN_IND:
        msg_type = COMMAND_JOIN_INDICATION;
        arg_size = sizeof(tschMlmeCommStatusIndication_t);
        cb_type |= MAC_CB_POST;
        break;
        case MAC_MLME_BCN_CNF:
         /* Do not free dedicated beacon TX buffer */
        // cb_type |= MAC_CB_FREE_BUF;
            BM_setDataLen(usm_buf,0);
        break;
        case MAC_MLME_RESTART:
        msg_type = COMMAND_RESTART;
        arg_size = sizeof(tschMlmeRestart_t);
        cb_type |= MAC_CB_POST;
        break;
        case MAC_MLME_BEACON_REQUEST:
        msg_type = COMMAND_BEACON_REQUEST;
        arg_size = sizeof(tschMlmeBeaconRequest_t);
        cb_type |= MAC_CB_POST;
        break;
        case MAC_MLME_SHARE_SLOT_MODIFY:
        msg_type = COMMAND_SHARE_SLOT_MODIFICATION;
        arg_size = sizeof(tschMlmeShareSlotModify_t);
        cb_type |= MAC_CB_POST;
        break;
#if ULPSMAC_L2MH
        case MAC_MLME_ASSOCIATE_IND_L2MH:
        msg_type = COMMAND_ASSOCIATE_INDICATION_L2MH;
        arg_size = sizeof(tschTiAssociateIndicationL2mh_t);
        cb_type |= MAC_CB_POST;
        break;
        case MAC_MLME_ASSOCIATE_CNF_L2MH:
        msg_type = COMMAND_ASSOCIATE_CONFIRM_L2MH;
        arg_size = sizeof(tschTiAssociateConfirmL2mh_t);
        cb_type |= MAC_CB_POST;
        break;
        case MAC_MLME_DISASSOCIATE_IND_L2MH:
        msg_type = COMMAND_DISASSOCIATE_INDICATION_L2MH;
        arg_size = sizeof(tschTiDisassociateIndicationL2mh_t);
        cb_type |= MAC_CB_POST;
        break;
        case MAC_MLME_DISASSOCIATE_CNF_L2MH:
        msg_type = COMMAND_DISASSOCIATE_CONFIRM_L2MH;
        arg_size = sizeof(tschTiDisassociateConfirmL2mh_t);
        cb_type |= MAC_CB_POST;
        break;
#endif        
        
        default :
        break;
    }

    if (cb_type & MAC_CB_FREE_BUF)
    {
        if (usm_buf)
        {
            BM_free_6tisch(usm_buf);
        }
    }

    if (cb_type & MAC_CB_POST)
    {
        bool status = false;

        cmm_msg = NHLDB_TSCH_CMM_alloc_cmdmsg(5);

        if (cmm_msg)
        {
            uint32_t ptrAddr;

            if (usm_buf)
            {
                cmm_msg->len = BM_getDataLen((BM_BUF_t *)usm_buf);
                BM_copyto((BM_BUF_t *)usm_buf,cmm_msg->buf);
            }
            cmm_msg->type = msg_type;
            cmm_msg->event_type = TASK_EVENT_MSG;
            memcpy(cmm_msg->arg, arg, arg_size);
            ptrAddr = (uint32_t)cmm_msg;
            status = SN_post(nhl_tsch_mailbox, &ptrAddr,BIOS_NO_WAIT,NOTIF_TYPE_MAILBOX);
            
            if (status == false)
            {
              NHLDB_TSCH_CMM_free_cmdmsg(cmm_msg);
              ULPSMAC_Dbg.numNhlEventPostError++;
            }
        }
    }
}

void nhl_AssociateResponseAckIndication(uint8_t addrMode, ulpsmacaddr_t *pDstAddr, uint8_t eventID)
{
#if IS_ROOT
#ifdef ENABLE_HCT
  uint16_t devShortAddr;
  if (addrMode == ULPSMACADDR_MODE_LONG)
  { 
    devShortAddr = NHL_deviceTableGetShortAddr(pDstAddr);
  }
  else
  {
    devShortAddr = ulpsmacaddr_to_short(pDstAddr);
  }
  
  HCT_IF_MAC_Join_indc_proc(&pDstAddr->u8[0],devShortAddr, eventID);
#endif
#endif
}
/**************************************************************************************************//**
 *
 * @brief       This function is NHL MCPS callback function handler
 *
 *
 * @param[in]   arg     -- the pointer to tschCbackEvent_t data structure
 *              usm_buf     -- pointer to BM data buffer
 *
 * @return
 *
 ***************************************************************************************************/
void nhl_McpsCb(tschCbackEvent_t *arg,BM_BUF_t*usm_buf)
{
    uint8_t cb_type=MAC_CB_ERR;
    uint8_t msg_type;
    uint8_t arg_size;
    NHLDB_TSCH_CMM_cmdmsg_t* cmm_msg;

    switch (arg->hdr.event)
    {
        case MAC_MCPS_DATA_CNF:
        msg_type = COMMAND_DATA_CONFIRM;
        arg_size = sizeof(tschMcpsDataCnf_t);
        cb_type |= MAC_CB_FREE_BUF;
        cb_type |= MAC_CB_POST;
        break;
        case MAC_MCPS_DATA_IND:
        msg_type = COMMAND_DATA_INDICATION;
        arg_size = sizeof(tschMcpsDataInd_t);
        cb_type |= MAC_CB_POST;
        break;
        default :
        break;
    }

    if (cb_type & MAC_CB_POST)
    {
        bool status = false;
        cmm_msg = NHLDB_TSCH_CMM_alloc_cmdmsg(4);

        if (cmm_msg)
        {
            uint32_t ptrAddr;

            if (usm_buf)
            {
                cmm_msg->len = BM_getDataLen((BM_BUF_t *)usm_buf);
                cmm_msg->recv_asn = usm_buf->recv_asn; // Jiachen
                BM_copyto((BM_BUF_t *)usm_buf,cmm_msg->buf);
            }
            cmm_msg->type = msg_type;
            cmm_msg->event_type = TASK_EVENT_MSG;
            memcpy(cmm_msg->arg, arg, arg_size);
            ptrAddr = (uint32_t)cmm_msg;
            status = SN_post(nhl_tsch_mailbox, &ptrAddr,BIOS_NO_WAIT,NOTIF_TYPE_MAILBOX);
            
            if (status == false)
            {
              NHLDB_TSCH_CMM_free_cmdmsg(cmm_msg);
              ULPSMAC_Dbg.numNhlEventPostError++;
            }
        }
    }
    
    if (cb_type & MAC_CB_FREE_BUF)
    {
        if (usm_buf)
        {
            BM_free_6tisch( (BM_BUF_t *)usm_buf);
        }
    }
}
