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
 *  ====================== nhl_task.c =============================================
 *  
 */

#include "tsch_os_fun.h"
#include "mac_config.h"
#include "tsch_api.h"
#include "nhl.h"
#include "nhl_mgmt.h"
#include "nhl_mlme_cb.h"
#include "nhldb-tsch-cmm.h"
#include "nhl_device_table.h"
#include "tsch-acm.h"

#include "nhl_api.h"
#if IS_ROOT
#include "nhldb-tsch-pc.h"
#endif
#if DMM_ENABLE
#include "dmm_scheduler.h"
#include "dmm_policy_blesp_wsnnode.h"
#endif //DMM_ENABLE

#if defined (__IAR_SYSTEMS_ICC__) || defined(__TI_COMPILER_VERSION__)
#include <string.h>
#endif

#ifdef ENABLE_HCT
#include "HCT/hct.h"
#endif

#include "pwrest/pwrest.h"

extern void nhl_MlmeCb(tschCbackEvent_t *,BM_BUF_t*);
extern void nhl_McpsCb(tschCbackEvent_t *,BM_BUF_t*);
static int isDuplicatedMacPacket(NHL_McpsDataInd_t* rxPacktInfo_p);

//*****************************************************************************
// functions
//*****************************************************************************

/***************************************************************************//**
 * @fn          nhl_tsch_task
 *
 * @brief       This is NHL Task to process command/data requests, confirmations
 *              and data indication and other calls.
 *
 * @param[in]   none
 *
 * @return      none
 *
 ******************************************************************************/
void nhl_tsch_task(void)
{
    NHLDB_TSCH_CMM_cmdmsg_t* cmm_msg;
    uint32_t ptrAddr;

#if DMM_ENABLE
    DMM_waitBleOpDone();
#endif //DMM_ENABLE

    TSCH_event_register(MAC_REG_MLME_EVENTS, nhl_MlmeCb);
    TSCH_event_register(MAC_REG_DATA_EVENTS, nhl_McpsCb);
    TSCH_Init();

    while (1) {
        SN_pend(nhl_tsch_mailbox, &ptrAddr,
                BIOS_WAIT_FOREVER,NOTIF_TYPE_MAILBOX);
#if DISABLE_TISCH_WHEN_BLE_CONNECTED && DMM_ENABLE && CONCURRENT_STACKS
        if (bleConnected)
        {
            NHLDB_TSCH_CMM_free_cmdmsg((NHLDB_TSCH_CMM_cmdmsg_t*)ptrAddr);
            continue;
        }
#endif //DMM_ENABLE && !CONCURRENT_STACKS
        
        PWREST_on(PWREST_TYPE_CPU);

        cmm_msg = (NHLDB_TSCH_CMM_cmdmsg_t*)ptrAddr;
        if (cmm_msg->event_type == TASK_EVENT_MSG)
        {
            switch (cmm_msg->type)
            {
                case COMMAND_ASTAT_REQ_PROC:  
                   NHL_astat_req_handler();
                   break;
                case COMMAND_ADJUST_KA_TIMER:  
                   TSCH_TXM_adjust_ka_timer_handler((rtimer_clock_t *)(cmm_msg->arg));
                   break;
                case COMMAND_SEND_KA_PACKET:  
                   TSCH_TXM_send_ka_packet_handler((void *)(cmm_msg->arg));
                   break;
                case COMMAND_TRANSMIT_PACKET:
                TSCH_transmit_packet_handler(cmm_msg->arg);
                break;
                case COMMAND_BEACON_REQUEST:
                beacon_req_proc();
                break;
                case COMMAND_DATA_CONFIRM:
                if (NHL_dataConfCbLocal != NULL)
                {
                    NHL_McpsDataConf_t* confArg;
                    confArg = (NHL_McpsDataConf_t*) cmm_msg->arg;
                    NHL_dataConfCbLocal(confArg);
                }
                break;
                case COMMAND_DATA_INDICATION:
                if (cmm_msg->buf && cmm_msg->len)
                {
                   NHL_McpsDataInd_t* indArg=(NHL_McpsDataInd_t*) cmm_msg->arg;

                   if (!isDuplicatedMacPacket(indArg))
                   {
                      indArg->pMsdu = BM_alloc(12);
                      if (indArg->pMsdu != NULL)
                      {
                          BM_copyfrom(indArg->pMsdu,cmm_msg->buf,cmm_msg->len);
                          indArg->recv_asn = cmm_msg->recv_asn; // Jiachen
                          NHL_dataIndCb(indArg);
                      }
                   }
                 }
                break;
                case COMMAND_ASSOCIATE_INDICATION:
                assoc_indc_proc(cmm_msg->arg,cmm_msg->buf, cmm_msg->len);
                break;
                case COMMAND_ASSOCIATE_RESPONSE_IE:
                assoc_resp_proc(cmm_msg->buf, cmm_msg->len);
                break;
                case COMMAND_DISASSOCIATE_INDICATION:
                disassoc_indc_proc(cmm_msg->arg, cmm_msg->buf,
                                   cmm_msg->len);
#if !IS_ROOT
                TSCH_prepareDisConnect();
                TSCH_restartNode();
#endif
                break;
                case COMMAND_COMM_STATUS_INDICATION:
                comm_status_indc_proc(cmm_msg->arg);
                break;
                case COMMAND_JOIN_INDICATION:
                join_indication_proc(cmm_msg->arg);
                break;
                case COMMAND_SHARE_SLOT_MODIFICATION:
                NHL_modify_share_link();
                break;
#if ULPSMAC_L2MH
                case COMMAND_ASSOCIATE_INDICATION_L2MH:
                assoc_indc_l2mh_proc(cmm_msg->arg, cmm_msg->buf, cmm_msg->len);
                break;
                case COMMAND_DISASSOCIATE_INDICATION_L2MH:
                disassoc_indc_l2mh_proc(cmm_msg->arg,cmm_msg->buf,cmm_msg->len);
                break;
#endif
                case COMMAND_BEACON_INDICATION:
                beacon_notify_indc_proc(cmm_msg->arg, cmm_msg->buf,
                                        cmm_msg->len);
                break;
                case COMMAND_SCAN_CONFIRM:
                scan_conf_proc(cmm_msg->arg);
                break;
#if !IS_ROOT
                case COMMAND_ASSOCIATE_CONFIRM:
                assoc_conf_proc(cmm_msg->arg, cmm_msg->buf, cmm_msg->len);
                break;
                case COMMAND_RESTART:
                TSCH_prepareDisConnect();
                TSCH_restartNode();
                break;
#if ULPSMAC_L2MH
                case COMMAND_ASSOCIATE_CONFIRM_L2MH:
                assoc_conf_l2mh_proc(cmm_msg->arg, cmm_msg->buf, cmm_msg->len);
                break;
                case COMMAND_DISASSOCIATE_CONFIRM_L2MH:
                disassoc_conf_l2mh_proc(cmm_msg->arg,cmm_msg->buf,cmm_msg->len);
                break;
#endif
#endif //!IS_ROOT
                case COMMAND_DISASSOCIATE_CONFIRM:
                disassoc_conf_proc(cmm_msg->arg);
#if IS_ROOT
#else
                // restart  TSCH  MAC
                TSCH_prepareDisConnect();
                TSCH_restartNode();
#endif
                break;
                default:
                break;
            }
        }
#ifdef ENABLE_HCT
        else if (cmm_msg->event_type == TASK_EVENT_HCT)
        {
          HCT_IF_proc_TSCH_Msg(cmm_msg);
        }
#endif
        NHLDB_TSCH_CMM_free_cmdmsg(cmm_msg);
        PWREST_off(PWREST_TYPE_CPU);
    }
}

/***************************************************************************//**
 * @fn          eventPost
 *
 * @brief       This is post event to mailbox to wake up TSCH time slot task.
 *              It is called in RF interrupt context
 *
 * @param[in]   eventID - event type
 *
 * @return
 *
 ******************************************************************************/
void eventPost(uint8_t eventID)
{
  EVT_MSG_s msg;
  bool status;

  msg.eventID = eventID;

  status = SN_post(acm_evt_mbox,&msg,BIOS_NO_WAIT,NOTIF_TYPE_MAILBOX);
  if (status == false)
  { 
    ULPSMAC_Dbg.numAcmEventPostError++;
  }
}
/***************************************************************************//**
 * @fn          eventWait
 *
 * @brief       This is wait event for mailbox
 *
 * @param[in]   eventID         -- Event ID
 *              timeoutValue    -- Timeout value
 *              refTine         -- Reference time used
 *
 * @return      none
 *
 ******************************************************************************/
uint8_t eventWait(uint8_t eventID, uint32_t timeoutValue, uint32_t reftime)
{
  uint32_t rtos_tick;
  EVT_MSG_s msg;
  bool status;
  uint32_t key;
  rtimer_clock_t rtime_now;

  // convert the RTC1 ticks to RTOS tick
  if ((timeoutValue == BIOS_NO_WAIT) || (timeoutValue == BIOS_WAIT_FOREVER) )
  {
    rtos_tick = timeoutValue;
  }
  else
  {
    key = Hwi_disable();
    rtime_now = RTIMER_NOW();
    timeoutValue = RTIMER_CLOCK_LT(reftime + timeoutValue, rtime_now) ? 0 :
                  reftime + timeoutValue - rtime_now;
    rtos_tick = (uint32_t)((uint64_t)(timeoutValue* CLOCK_SECOND)
                           /(uint64_t)RTIMER_SECOND);

    Hwi_restore(key);
  }

  status = SN_pend(acm_evt_mbox,&msg,rtos_tick,NOTIF_TYPE_MAILBOX);

  if (status == true)
  {
    if (msg.eventID == eventID)
    {
      return ACM_WAIT_EVT_SUCCESS;
    }
    else
    {
      return ACM_WAIT_EVT_NOT_EXPECTED;
    }
  }
  else
  {
    return ACM_WAIT_EVT_TIMEOUT;
  }
}

/***************************************************************************//**
 * @fn          eventFree
 *
 * @brief       This is free event
 *
 * @param[in]   none
 *
 * @return      none
 *
 ******************************************************************************/
void eventFree(void)
{
  uint16_t numMsg,i;
  EVT_MSG_s msg;
  bool status;

  numMsg = Mailbox_getNumPendingMsgs(acm_evt_mbox);

  for (i=0;i<numMsg;i++)
  {
    status = SN_pend(acm_evt_mbox,&msg,BIOS_NO_WAIT,NOTIF_TYPE_MAILBOX);
    if (status == false)
    {
      ULPSMAC_Dbg.numAcmEventPendError++;
    }
  }
}

/***************************************************************************//**
 * @fn          isDuplicatedMacPacket
 *
 * @brief       check if the received packet is a duplicated MAC packet 
 *              due to ACK reception error
 *
 * @param[in]
 *
 * @return      1: if is duplicated, 0: not duplicated packet
 *
 ******************************************************************************/
static int isDuplicatedMacPacket(NHL_McpsDataInd_t* rxPacktInfo_p)
{
   int retVal = 0;

   if (rxPacktInfo_p != NULL &&
         !rxPacktInfo_p->mac.fco.seqNumSuppressed &&
         rxPacktInfo_p->mac.srcAddrMode == ULPSMACADDR_MODE_SHORT)
   {
      //Do duplicated packet detection only for the short address mode
      uint16_t shortAddr = (rxPacktInfo_p->mac.srcAddr.u8[1]<<8 ) 
        | rxPacktInfo_p->mac.srcAddr.u8[0];
      uint16_t devTableEntryIdx = NHL_getTableIndex(shortAddr);
      if (devTableEntryIdx < NODE_MAX_NUM)
      {
         retVal = (NHL_deviceTableGetMacSeqNumber(devTableEntryIdx) 
                   == rxPacktInfo_p->mac.dsn);
         NHL_deviceTableSetMacSeqNumber(devTableEntryIdx, 
                                        rxPacktInfo_p->mac.dsn);
      }
   }

   return(retVal);
}
