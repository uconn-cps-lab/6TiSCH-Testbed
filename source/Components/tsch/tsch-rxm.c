/*
* Copyright (c) 2010 - 2014 Texas Instruments Incorporated
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
 *  ====================== tsch-rxm.c =============================================
 *  Timeslot RX Manager
 */

#if defined (__IAR_SYSTEMS_ICC__) || defined(__TI_COMPILER_VERSION__)
#include <string.h>
#endif

#include "hal_api.h"
#include "tsch_os_fun.h"
#include "pwrest/pwrest.h"
#include "tsch-rxm.h"
#include "ulpsmac.h"

#include "tsch-acm.h"
#include "mrxm.h"

#include "framer-802154e.h"
#include "ie802154e.h"

#include "nhl_device_table.h"           //zelin



#if DMM_ENABLE
/* Include DMM module */
#include "dmm_scheduler.h"
#include "dmm_policy_blesp_wsnnode.h"
#endif //DMM_ENABLE
/*---------------------------------------------------------------------------*/
#define TWO_TIMESLOT_LENGTH    (2*TSCH_ACM_RTC_TIMESLOT_LENGTH)
#define SYNC_LOST_THRESHOLD    50

uint8_t numConLostBeacon=0;

/*---------------------------------------------------------------------------*/
TSCH_DB_handle_t* rxm_db_hnd;

static BM_BUF_t* usm_buf_acksend = NULL;
BM_BUF_t* usm_buf_datarecv = NULL;

uint8_t rxRxmFlag = 0;

/**************************************************************************************************//**
 *
 * @brief       This is used to process the received packet in timeslot
 *
 *
 * @param[in]   link -- pointer of current link data structure
 *              recv_usm_buf -- pointer of received buffer
 *              this_slot_start -- timestamp of current time slot start
 *              do_sync -- do sync (timing adjustment)
 *
 *
 * @return     status of operation
 *
 ***************************************************************************************************/
static int16_t process_recv_pkt(TSCH_DB_link_t* link, BM_BUF_t* recv_usm_buf, rtimer_clock_t this_slot_start, uint8_t do_resync)
{
    uint16_t mpi_recv_len;
    uint8_t  is_broadcast;
    const ulpsmacaddr_t* rxaddr;
    rtimer_clock_t slotdelta_tick;
    rtimer_clock_t diff;
    uint8_t        slotdelta_phase;
    uint8_t pktType;
    rtimer_clock_t pkt_recv_clock;
    uint16_t rxSrcAddrShort;
    int32_t wait_acksend;
    HAL_Status_t  radio_res;
    frame802154e_t frame;
    uint16_t len;

    memset(&frame,0,sizeof(frame));
    
    pkt_recv_clock = MRXM_getPktTimestamp();

    mpi_recv_len = BM_getDataLen(recv_usm_buf);

    if(!(len = framer_802154e_parseHdr(recv_usm_buf, &frame)))
    {
       return 0;
    }

    is_broadcast = 0;

    rxaddr = MRXM_getPktDstAddr();

    /* PAN ID filtering after join the network */
    if (TSCH_MAC_Pib.panId != 0xFFFF)
    {
        if (TSCH_MAC_Pib.panId != MRXM_getPktDstPanID() )
        {
            return 0;
        }
    }

    switch(MRXM_getPktDstAddrMode())
    {
    case ULPSMACBUF_ATTR_ADDR_MODE_LONG:
      is_broadcast = ulpsmacaddr_long_cmp(rxaddr, &ulpsmacaddr_broadcast);
      if(!ulpsmacaddr_long_cmp(rxaddr, &ulpsmacaddr_long_node) && (!is_broadcast))
      {
        return 0;
      }
      break;
    case ULPSMACBUF_ATTR_ADDR_MODE_SHORT:
      rxSrcAddrShort = ulpsmacaddr_to_short(rxaddr);
      is_broadcast = ulpsmacaddr_short_cmp(rxaddr, &ulpsmacaddr_broadcast);
      if ((rxSrcAddrShort !=ulpsmacaddr_short_node) && (!is_broadcast))
      {
        return 0;
      }
      break;
    default:
      return 0;
    }

    pktType = MRXM_getPktType();
    if (pktType == ULPSMACBUF_ATTR_PACKET_TYPE_TSCH_ADV ||
		pktType == ULPSMACBUF_ATTR_PACKET_TYPE_DATA ||
		pktType == ULPSMACBUF_ATTR_PACKET_TYPE_COMMAND)
    {
        rtimer_clock_t exp_peer_slotstart;
      
        /* Calculate slot delta if timeslot inited */
        slotdelta_phase = TSCH_ACM_SLOTDELTA_INPHASE;
        slotdelta_tick = 0;
        
        exp_peer_slotstart = TSCH_ACM_RTC_TIMESLOT_LENGTH -(TSCH_ACM_RTC_TX_OFFSET +
                               HAL_RADIO_DURATION_TICKS(mpi_recv_len) +
                                 HAL_RADIO_RX_CB_DELAY_TICKS ) + pkt_recv_clock;
        
        /* If exp_peer_slotstart wraps around at the end of 32 bit value
        but this_slot_start does not, simply ignore the adjustment.
        */
        diff = exp_peer_slotstart - this_slot_start;
        if (diff < TWO_TIMESLOT_LENGTH)
        {
          slotdelta_tick = diff % TSCH_ACM_RTC_TIMESLOT_LENGTH;
          if(slotdelta_tick > 0)
          {
            if(slotdelta_tick < (TSCH_ACM_RTC_TIMESLOT_LENGTH/2))
            {
              slotdelta_phase = TSCH_ACM_SLOTDELTA_IAM_EARLY;
            } else
            {
              slotdelta_tick = TSCH_ACM_RTC_TIMESLOT_LENGTH - slotdelta_tick;
              slotdelta_phase = TSCH_ACM_SLOTDELTA_IAM_LATE;
            }
          }
        }

        if( (TSCH_DB_TSCH_SYNC_INITED(rxm_db_hnd) == LOWMAC_TRUE) && (do_resync == LOWMAC_TRUE))
        {
            if( (!link) || (link && (TSCH_DB_LINK_OPTIONS(link) & TSCH_DB_LINK_OPTIONS_TK)) )
            {
                if(slotdelta_phase != TSCH_ACM_SLOTDELTA_INPHASE )
                {
                    TSCH_ACM_set_slotdelta(slotdelta_tick, slotdelta_phase);
                  
                    ULPSMAC_Dbg.avgSyncAdjust = 
                    (ULPSMAC_Dbg.avgSyncAdjust*ULPSMAC_Dbg.numSyncAdjust + 
                      ULPSMAC_TICKS_TO_US(slotdelta_tick))/(ULPSMAC_Dbg.numSyncAdjust+1);            
                     ULPSMAC_Dbg.numSyncAdjust++;
                    
                     if (ULPSMAC_TICKS_TO_US(slotdelta_tick) > ULPSMAC_Dbg.maxSyncAdjust)
                     ULPSMAC_Dbg.maxSyncAdjust = ULPSMAC_TICKS_TO_US(slotdelta_tick);
                }
            }
        }
        

        if((!is_broadcast) &&  MRXM_getPktAck())
        {
            uint8_t ack_len;
            int16_t slotdelta_us;
            ie802154e_hie_t hie;
            ie_acknack_time_correction_t ie_arg;
            uint8_t added_len;
            uint8_t * pData;

            if (HAL_RADIO_setChannel(TSCH_DB_LINK_CHANNEL(link)) == HAL_ERR)
            {
              ULPSMAC_Dbg.numSetChannelErr++;
              return 0;
            }
            
            BM_setDataLen(usm_buf_acksend, 0);

            slotdelta_us = 0;
            if(slotdelta_phase != TSCH_ACM_SLOTDELTA_INPHASE)
            {
              if(slotdelta_phase == TSCH_ACM_SLOTDELTA_IAM_EARLY)
              {
                /* I'm lagging, so peer is ahead */
                slotdelta_us = (-1) * (int16_t)(ULPSMAC_TICKS_TO_US(slotdelta_tick));
              } else if(slotdelta_phase == TSCH_ACM_SLOTDELTA_IAM_LATE)
              {
                /* I'm ahead, so peer is lagging need to catch up */
                slotdelta_us = ULPSMAC_TICKS_TO_US(slotdelta_tick);
              }
            }
            
            hie.ie_type = IE_TYPE_HEADER;
            hie.ie_elem_id = IE_ID_ACKNACK_TIME_CORRECTION;
            hie.ie_content_length = IE_ACKNACK_TIME_CORRECTION_LEN;
            hie.ie_content_ptr = (uint8_t *)&ie_arg;
            
            ie_arg.time_synch = slotdelta_us;
            ie_arg.acknack = 0;
            
            
            ie_arg.rxRssi = 0xfe;
            ie_arg.rxTotal = 0xfffffffe;
#if IS_ROOT
            if(MRXM_getPktSrcAddrMode() == ULPSMACBUF_ATTR_ADDR_MODE_SHORT && MRXM_getPktDstAddrMode() == ULPSMACBUF_ATTR_ADDR_MODE_SHORT)
            {
              uint8_t rxTableIndex;
              uint32_t rxTotalTemp;
              rxSrcAddrShort = ulpsmacaddr_to_short(MRXM_getPktSrcAddr());
              rxTableIndex = (uint8_t)(rxSrcAddrShort);
              rxTotalTemp = NHL_deviceTableGetRxTotalNumber(rxTableIndex);
              if(rxTotalTemp != 0xfffffffe)
              {
                rxTotalTemp++;
                if (rxTotalTemp == 4000000)
                  rxTotalTemp = 0;
                NHL_deviceTableSetRxTotalNumber(rxTableIndex, rxTotalTemp);
                ie_arg.rxRssi = MRXM_getPktRssi();
                ie_arg.rxTotal = rxTotalTemp;
              }
            }
#else
            
            if(MRXM_getPktSrcAddrMode() == ULPSMACBUF_ATTR_ADDR_MODE_SHORT && MRXM_getPktDstAddrMode() == ULPSMACBUF_ATTR_ADDR_MODE_SHORT)
            {
              uint16_t tableIndex;
              uint32_t rxTotalTemp;
              tableIndex = NHL_getTableIndex(ulpsmacaddr_to_short(MRXM_getPktSrcAddr()));
              if(tableIndex != NODE_MAX_NUM)
              {
                  rxTotalTemp = NHL_deviceTableGetRxTotalNumber(tableIndex);
                  if(rxTotalTemp != 0xfffffffe)
                  {
                    rxTotalTemp++;
                    if (rxTotalTemp == 4000000)
                      rxTotalTemp = 0;
                    NHL_deviceTableSetRxTotalNumber(tableIndex, rxTotalTemp);
                    ie_arg.rxRssi = MRXM_getPktRssi();
                    ie_arg.rxTotal = rxTotalTemp;
                  }
              }
            }
#endif
            pData = BM_getDataPtr(usm_buf_acksend);
            
            added_len = ie802154e_add_hdrie(&hie, pData, ULPSMACBUF_DATA_SIZE);
            
            BM_setDataLen(usm_buf_acksend, added_len);
             
            framer_802154e_createAckPkt(usm_buf_acksend,1);

            ack_len = BM_getDataLen(usm_buf_acksend);
            if(ack_len != 0)
            {
              wait_acksend = TSCH_ACM_RTC_TX_ACK_DELAY -
                (RTIMER_NOW() - pkt_recv_clock) - HAL_RADIO_TXRX_CMD_DELAY_TICKS;
              wait_acksend = (wait_acksend > 0)? ULPSMAC_TICKS_TO_US(wait_acksend): 0;
              wait_acksend = RF_getCurrentTime() + wait_acksend*4;

              radio_res = HAL_RADIO_transmit(BM_getBufPtr(usm_buf_acksend),BM_getDataLen(usm_buf_acksend), wait_acksend);
              if (radio_res == HAL_SUCCESS)
              {
                /*10,000 rtimer ticks = 152 ms*/
                eventWait(ACM_EVENT_TX_DATA_DONE,10000, this_slot_start);
              }
            }

            PWREST_updateRFSlotCount(PWREST_RX_UNICAST_PACKET);
            PWREST_updateTxRxBytes(ack_len,mpi_recv_len);
        }
        else
        {
            PWREST_updateRFSlotCount(PWREST_RX_BROADCAST_PACKET);
            PWREST_updateTxRxBytes(0,mpi_recv_len);
        }
    }
    
    if (!framer_802154e_parsePayload(recv_usm_buf, len, &frame))
    {
 	   return 0;
    }

    switch (pktType)
    {
        case ULPSMACBUF_ATTR_PACKET_TYPE_TSCH_ADV:
        {
            /*If not PAN coordinator and initial synchronization not established 
             yet, save beacon synchronization information to DB */
            if((!TSCH_DB_IS_PAN_COORD(rxm_db_hnd)) &&
               (TSCH_DB_TSCH_SYNC_INITED(rxm_db_hnd) != LOWMAC_TRUE))
            {
                   ie802154e_pie_t find_ie;
                   uint8_t found;
                   ie_tsch_synch_t ie_cntnt;

                   find_ie.ie_elem_id = IE_ID_TSCH_SYNCH;
                   found = ie802154e_find_in_payloadielist(BM_getDataPtr(recv_usm_buf),
                                                           BM_getDataLen(recv_usm_buf),
                                                           &find_ie);
                   if(found) 
                   {
                       ie802154e_parse_tsch_synch(find_ie.ie_content_ptr, &ie_cntnt);
                       TSCH_ACM_set_adv_syncinfo(recv_usm_buf, mpi_recv_len, ie_cntnt.macASN);
                   }
               }
            break;
        }
        case ULPSMACBUF_ATTR_PACKET_TYPE_DATA:
        case ULPSMACBUF_ATTR_PACKET_TYPE_COMMAND:
        {
            break;
        }
        case ULPSMACBUF_ATTR_PACKET_TYPE_ACK:
        {
            return 0;
        }
        default:
        {
            return 0;
        }
    }
   
    ULPSMAC_UPMAC.input(recv_usm_buf);

    return BM_getDataLen(recv_usm_buf);
}

/**************************************************************************************************//**
 *
 * @brief       This is used to init the TSCH LMAC RXM module
 *
 *
 * @param[in]   db_hnd -- pointer of TSCH database
 *
 *
 * @return
 *
 ***************************************************************************************************/
LOWMAC_status_e TSCH_RXM_init(TSCH_DB_handle_t* db_hnd)
{
    rxm_db_hnd = db_hnd;

    usm_buf_acksend = BM_alloc(16);
    usm_buf_datarecv = BM_alloc(17);

    return LOWMAC_STAT_SUCCESS;
}

/**************************************************************************************************//**
 *
 * @brief       This is used to process the receive packet
 *
 *
 * @param[in]   usm_buf -- pointer of data packet buffer
 *
 *
 * @return      status of operation
 *
 ***************************************************************************************************/
LOWMAC_status_e TSCH_RXM_recv_pkt(BM_BUF_t* usm_buf)
{
  // before starting state machine of TSCH
  // just forward the packet to upper MAC
  // after finding the synchronization information
  if((TSCH_DB_TSCH_SYNC_INITED(rxm_db_hnd) == LOWMAC_TRUE) ||
     (TSCH_DB_TSCH_STARTED(rxm_db_hnd) == LOWMAC_TRUE)) {
       /* not inited but called */
       return LOWMAC_STAT_NO_LINK;
     }

  //usm_buf is radio dedicated buffer, do not free
  process_recv_pkt(NULL, usm_buf, 0, LOWMAC_FALSE);

  return LOWMAC_STAT_SUCCESS;
}

/**************************************************************************************************//**
*
* @brief       This function handles the time slot RX task in LMAC. All RX related IE timing is
*              implemented in polling mode.
*
*
* @param[in]   link             -- pointer to TSCH_DB_link_t data structure
*              this_slot_start  -- start time stamp of this time slot (it is reference point)
*
*
* @return      status of this operaion (SUCCESS)
*
***************************************************************************************************/
LOWMAC_status_e TSCH_RXM_timeslot(TSCH_DB_link_t* link, rtimer_clock_t this_slot_start)
{
    uint16_t len;
    uint8_t eventSt;
    int32_t timeout;
    
    rxRxmFlag = 0;

#if DMM_ENABLE && CONCURRENT_STACKS
       DMMPolicy_updateStackState(DMMPolicy_StackType_WsnNode, DMMPOLICY_STACKSTATE_WSNNODE_RX);
#endif //DMM_ENABLE
    if (HAL_RADIO_setChannel(TSCH_DB_LINK_CHANNEL(link)) == HAL_ERR)
    {
      ULPSMAC_Dbg.numSetChannelErr++;
      return LOWMAC_STAT_ERR;
    }
    
    timeout = TSCH_ACM_RTC_RX_OFFSET-(RTIMER_NOW() - this_slot_start)-
      (HAL_RADIO_RAMPUP_DELAY_TICKS+HAL_RADIO_TXRX_CMD_DELAY_TICKS);
    timeout = (timeout > 0)? ULPSMAC_TICKS_TO_US(timeout):0;
    timeout = RF_getCurrentTime() + timeout*4;
    
    HAL_RADIO_receiveOn(TSCH_rxISR, timeout);
    
    timeout = TSCH_ACM_RTC_RX_OFFSET + TSCH_ACM_RTC_RX_WAIT;
    eventSt = eventWait(ACM_EVENT_RX_SYNC,timeout,this_slot_start);

    if (eventSt == ACM_WAIT_EVT_SUCCESS)
    {
        /* Continue to wait */
        timeout = TSCH_ACM_RTC_RX_OFFSET+ TSCH_ACM_RTC_RX_WAIT + TSCH_ACM_RTC_MAX_TX;
        eventSt = eventWait(ACM_EVENT_RX_DONE_OK,timeout,this_slot_start);
    }
    
    if (rxRxmFlag == 0)
    {
        HAL_RADIO_receiveOff();
        PWREST_updateRFSlotCount(PWREST_RX_NO_PACKET);
#if DMM_ENABLE && CONCURRENT_STACKS
       DMMPolicy_updateStackState(DMMPolicy_StackType_WsnNode, DMMPOLICY_STACKSTATE_WSNNODE_SLEEPING);
#endif //DMM_ENABLE
        return LOQMAC_STAT_RX_NO_DATA;
    }

    HAL_RADIO_receiveOff();

#if IS_ROOT
    len =process_recv_pkt(link, usm_buf_datarecv, this_slot_start, LOWMAC_FALSE);
#else
    len =process_recv_pkt(link, usm_buf_datarecv, this_slot_start, LOWMAC_TRUE);
#endif
    len = len;
#if DMM_ENABLE && CONCURRENT_STACKS
       DMMPolicy_updateStackState(DMMPolicy_StackType_WsnNode, DMMPOLICY_STACKSTATE_WSNNODE_SLEEPING);
#endif //DMM_ENABLE
    rxRxmFlag = 0;

    return LOWMAC_STAT_SUCCESS;
}
