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
 *  ====================== tsch-txm.c =============================================
 *  Timeslot TX Manager
 */

#include "hal_api.h"
#include "tsch_os_fun.h"
#include "pwrest/pwrest.h"

#include "tsch-txm.h"
#include "ulpsmac.h"
#include "lib/memb.h"
#include "lib/random.h"

#include "ulpsmacbuf.h"
#include "tsch-acm.h"
#include "framer-802154e.h"
#include "ie802154e.h"

#include "mrxm.h"
#include "mtxm.h"
#include "mac_pib_pvt.h"
#include "nhl_device_table.h"

#include "mac_config.h"
#if !IS_ROOT
#include "coap/harp.h"
extern HARP_self_t HARP_self;
#endif

#if WITH_UIP6
#include "uip_rpl_process.h"
#endif

#if DMM_ENABLE
/* Include DMM module */
#include "dmm_scheduler.h"
#include "dmm_policy_blesp_wsnnode.h"
#endif //DMM_ENABLE

/*---------------------------------------------------------------------------*/
MEMB(qdpkt_memb, TSCH_TXM_queued_packet_t, BM_NUM_BUFFER);

static TSCH_DB_handle_t* txm_db_hnd;

static uint8_t shared_bo_retx;
static int16_t shared_bo_cnt;

BM_BUF_t* usm_buf_ackrecv = NULL;
BM_BUF_t* usm_buf_keepalive = NULL;
BM_BUF_t* usmBufBeaconTx = NULL;
TSCH_TXM_queued_packet_t* bcn_qdpkt_ptr;

// Jiachen
#ifndef HARP_MINIMAL
uint64_t packet_sent_asn_others;
#endif
uint64_t packet_sent_asn_sensors; 
uint64_t packet_sent_asn_ping;



uint8_t rxAckFlag; 
uint16_t numberOfQueuedTxPackets;  //JIRA74

#define TSCH_NUM_TX_OPTIONS         (2)
const uint8_t TSCH_TX_LinkOption[TSCH_NUM_TX_OPTIONS]=
{   TSCH_PIB_LINK_OPTIONS_TX,
    TSCH_PIB_LINK_OPTIONS_SH
};

#ifdef FEATURE_MAC_SECURITY
#include "mac_tsch_security.h"
extern unsigned char *pKeyTx;
extern unsigned char *pKeyRx;
extern uint32_t *pFrameCounter;
extern pnmPib_t* mtxm_pnmdb_ptr;
MACSECURITY_DBG_s MACSECURITY_DBG;
BM_BUF_t cypered_usm_buf;
#endif

extern uint8_t numConLostBeacon;
extern macRxPacketParam_t macRxPktParam;

#define DATA_SYNCH_LOSS    10

/**************************************************************************************************//**
 *
 * @brief       This is post processing function when packet is transmited. In TSCH MAC, beacon
 *              data and MAC command have its own callback functions. It will post message to
 *              NHL to further process
 *
 * @param[in]   sent        -- packet's its own callback function
 *              ptr         -- pointer to packet (beacon, data or command) buffer
 *              status      -- MAc TX transmission status
 *              num_tx      -- number of retry in MAC TX
 *
 * @return
 *
 **************************************************************************************************/

static inline void sent_postproc(LOWMAC_txcb_t sent, void *ptr, int status, int num_tx)
{
  if(sent) {
    sent(ptr, status, num_tx);
  }
}

/**************************************************************************************************//**
 *
 * @brief       This function update the assigned link option. It clears theh WAIT_CONFIRM bit in the
 *              link option.
 *              This function is called by association ACK callback
 *
 * @param[in]   addrMode -- address mode of destination address in pDstAddr
 *              pDstAddr -- pointer of ulpsmacaddr_t
 *
 * @return
 *
 **************************************************************************************************/
void TSCH_TXM_updateDevLinks(uint8_t addrMode, ulpsmacaddr_t *pDstAddr)
{
  uint8_t sf_iter;
  TSCH_DB_slotframe_t* slotframe;
  TSCH_DB_link_t *link,*nextLink;
  uint16_t devShortAddr;

  if (addrMode == ULPSMACADDR_MODE_LONG)
  { 
    devShortAddr = NHL_deviceTableGetShortAddr(pDstAddr);
  }
  else
  {
    devShortAddr = ulpsmacaddr_to_short(pDstAddr);
  }

  for(sf_iter=0; sf_iter<TSCH_DB_MAX_SLOTFRAMES; sf_iter++)
  {
    if (TSCH_DB_SLOTFRAME_IN_USE(txm_db_hnd, sf_iter) == ULPSMAC_FALSE)
    {
        continue;
    }

    slotframe = TSCH_DB_SLOTFRAME(txm_db_hnd, sf_iter);

    link = TSCH_DB_SLOTFRAME_LINK_HEAD(slotframe);

    if (link == NULL)
    {
        continue;
    }

    // tranverse through all links and free TX packets

    while (link)
    {
        // get next link
        nextLink = list_item_next(link);

        if ( (TSCH_DB_LINK_PEERADDR(link) == devShortAddr ) &&
             (TSCH_DB_LINK_OPTIONS(link) &  MAC_LINK_OPTION_TX ) )
        {   // link's peer address match device address
            // link option  is TX

            if (TSCH_DB_LINK_OPTIONS(link) & MAC_LINK_OPTION_WAIT_CONFIRM)
            {   // clear this bit
                TSCH_DB_LINK_OPTIONS(link) &= ~(MAC_LINK_OPTION_WAIT_CONFIRM);
            }
        }

        // go to next link
        link = nextLink;
    }

  }

  return ;
}



/**************************************************************************************************//**
 *
 * @brief       This function returns the corresponding ASN for a link given the ASN of current
 *              timeslot
 *
 * @param[in]   cur_asn -- ASN of current timeslot
 *              link    -- link handle
 *              slotframe  -- slotframe handle
 *
 * @return      ASN of the input link
 *
 ***************************************************************************************************/

uint64_t link_asn(uint64_t cur_asn, TSCH_DB_link_t* link,TSCH_DB_slotframe_t* slotframe)
{
    uint64_t asn;
    uint16_t slot_frame_size;
    uint16_t periodSize=0;

    slot_frame_size =TSCH_DB_SLOTFRAME_SIZE_DIR(slotframe);
    periodSize = slot_frame_size*(link->vals.period);

    asn = (uint64_t) (cur_asn/periodSize*periodSize
       +slot_frame_size*(link->vals.periodOffset)+TSCH_DB_LINK_TIMESLOT(link));

    return asn;
}


/**************************************************************************************************//**
 *
 * @brief       This function is used to find the closest next link in slot frame. When TX packet
 *              is enque, the cur_link is not specified. This function will compute the slot number
 *              from the current ASN. It starts from the head of link and find the closest link
 *
 *              During the retransmission case, cur_link is specified in this function call
 *
 * @param[in]   slotframe_id -- id of slot frame
 *              usm_buf      -- BM_BUF pointer of TX packet (it is used to find the packet type)
 *              cur_link     -- current link (when enque from UMAC, this value is NULL)
 *
 * @return      pointer of next link in the DB. if it is NULL, there is no next link.
 *
 ***************************************************************************************************/
static TSCH_DB_link_t* find_next_link(uint8_t slotframe_id, BM_BUF_t* usm_buf, TSCH_DB_link_t* cur_link)
{
    TSCH_DB_slotframe_t* slotframe;
    TSCH_DB_link_t* link;
    uint8_t idx;
    uint16_t    pkt_type,dst_addr_mode;
    uint16_t devShortAddr;
    ulpsmacaddr_t dstAddr;
    slotframe = TSCH_DB_SLOTFRAME(txm_db_hnd, slotframe_id);
    /* Empty link return */
    if(!TSCH_DB_SLOTFRAME_LINK_HEAD(slotframe))
    {
        return NULL;
    }

    MTXM_peekMacHeader(BM_getBufPtr(usm_buf),&pkt_type,&dst_addr_mode,&dstAddr);

    uint8_t isCoAP = usm_buf->coap_type;
    if ( dst_addr_mode == ULPSMACADDR_MODE_LONG)
    {
        devShortAddr = NHL_deviceTableGetShortAddr(&dstAddr);
    }
    else
    {
        devShortAddr = ulpsmacaddr_to_short(&dstAddr);
    }

    switch(pkt_type)
    {
        case ULPSMACBUF_ATTR_PACKET_TYPE_BEACON:
            pkt_type = TSCH_DB_LINK_TYPE_ADV;
            break;
        default:
            pkt_type = TSCH_DB_LINK_TYPE_NORMAL;
            break;
    }

    
    if(!cur_link)
    {
        /* Find the closest link to current timeslot */
        uint64_t link_head_asn;
        uint64_t cur_asn;

        cur_asn = TSCH_DB_get_mac_asn(txm_db_hnd);
        cur_link = TSCH_DB_SLOTFRAME_LINK_HEAD(slotframe);
        link_head_asn = link_asn(cur_asn, cur_link, slotframe);

       if (link_head_asn <cur_asn)
        {
            TSCH_DB_link_t* next_link;
            uint64_t next_link_asn;

            next_link = TSCH_DB_SLOTFRAME_NEXT_LINK(slotframe, cur_link);

            /* Use link head if current timeslot is ahead of all links*/
            while(next_link != cur_link)
            {
                next_link_asn = link_asn(cur_asn, next_link,slotframe);

                if(next_link_asn > cur_asn)
                {
                    cur_link = next_link;
                    break;
                }
                next_link = TSCH_DB_SLOTFRAME_NEXT_LINK(slotframe, next_link);
            }
        }
    }
    else
    {
        cur_link = TSCH_DB_SLOTFRAME_NEXT_LINK(slotframe, cur_link);
    }
       
    // We first check dedicated link and then check shared link
    // Standard says any link that first comes regardless of dedicated/shared
    // But, shared link can be large back off, so we prefer dedicated link
    
    for (idx=0;idx<TSCH_NUM_TX_OPTIONS;idx++)
    {
        uint8_t linkOption;
        linkOption = TSCH_TX_LinkOption[idx];        
        link = cur_link;
        
        do
        {    
            // jiachen: isolate special coap and other packets
            if(isCoAP)
            {
              if(TSCH_DB_LINK_OPTIONS(link) != TSCH_DB_LINK_OPTIONS_TX_COAP)
              {
                  link = TSCH_DB_SLOTFRAME_NEXT_LINK(slotframe, link);
                  continue;
              }
            } else {
                // skip coap-tx links
                if(TSCH_DB_LINK_OPTIONS(link) == TSCH_DB_LINK_OPTIONS_TX_COAP)
                {
                    link = TSCH_DB_SLOTFRAME_NEXT_LINK(slotframe, link);
                    continue;
                }
            }
            
            if ( (TSCH_DB_LINK_OPTIONS(link) & linkOption ) &&
                 (TSCH_DB_LINK_TYPE(link) == pkt_type) )
            {
                if(TSCH_DB_LINK_PEERADDR(link) == 0xffff)
                {
                    // found a related link
                    return link;
                }
                else
                {
                    if  (( TSCH_DB_LINK_PEERADDR(link) == devShortAddr ) &&
                         ( !(TSCH_DB_LINK_OPTIONS(link) & MAC_LINK_OPTION_WAIT_CONFIRM)) )
                    {
                         // address has been matched
                         // found a related link
                         return link;
                    }
                }
            } // dedicated link

            link = TSCH_DB_SLOTFRAME_NEXT_LINK(slotframe, link);

        } while (link != cur_link);

    }

  return NULL;
}

/**************************************************************************************************//**
 *
 * @brief       This function is used to check if the MAC TX queue is empty.
 *
 * @param[in]
 *
 *
 * @return      true if MAC TX is empty (no data to send)
 *              false if MAC TX is not empty
 *
 ***************************************************************************************************/
static uint8_t is_que_empty(void)
{
  uint8_t sf_iter;

  TSCH_DB_slotframe_t* slotframe;
  TSCH_DB_link_t* link;

  for(sf_iter=0; sf_iter<TSCH_DB_MAX_SLOTFRAMES; sf_iter++)
  {

    if(TSCH_DB_SLOTFRAME_IN_USE(txm_db_hnd, sf_iter) == LOWMAC_TRUE)
    {
      slotframe = TSCH_DB_SLOTFRAME(txm_db_hnd, sf_iter);

      /* Empty link return */
      if(!TSCH_DB_SLOTFRAME_LINK_HEAD(slotframe)) {
        return LOWMAC_TRUE;
      }

      link = TSCH_DB_SLOTFRAME_LINK_HEAD(slotframe);
      while(link) {
        if(TSCH_DB_LINK_PKTQ_LIST_HEAD(link)) {
            return LOWMAC_FALSE;
        }
        link = list_item_next(link);
      } // while

    } // if in use

  } // for sf_iter

  return LOWMAC_TRUE;
}

/**************************************************************************************************//**
 *
 * @brief       This function is used to check the received ACK packet. It will check the following
 *              items
 *              - address mode (sr/destination)
 *              - received DST and sent SRC
 *              - received SRC and sent DST
 *
 * @param[in]  usm_buf_ack --  pointer of received ACK data buffer
 *             usm_buf_data -- pointer of transmitted data buffer
 *
 * @return      1 if all chekings are passed.
 *              0 if there is any error.
 *
 ***************************************************************************************************/
static inline int16_t check_recv_ack(BM_BUF_t* usm_buf_ack, BM_BUF_t* usm_buf_data)
{
  uint16_t res;
  uint8_t ackDstMode,ackSrcMode;
  uint8_t dataSrcMode,dataDstMode;

  ackDstMode  = MRXM_getPktDstAddrMode();
  ackSrcMode  = MRXM_getPktSrcAddrMode();
  dataSrcMode = MTXM_getPktSrcAddrMode();
  dataDstMode = MTXM_getPktDstAddrMode();
  // compare with my address
  if ( ackDstMode != dataSrcMode )
  {
    return 0;
  }

  res = ulpsmacaddr_cmp(ackDstMode,MRXM_getPktDstAddr(),MTXM_getPktSrcAddr());
  if(!res) 
  {
    return 0;
  }

  // compare with intended node address
  if(ackSrcMode != dataDstMode )
  {
    return 0;
  }

  res = ulpsmacaddr_cmp(ackSrcMode,MRXM_getPktSrcAddr(),MTXM_getPktDstAddr());
  if(!res) 
  {
    return 0;
  }

  // compare DSN
  if((MRXM_getPktSeqSuppression() == 0) &&
     (MTXM_getPktSeqSuppression() ==0))
  {
      // seqno is not suppressed (TX and RX SN should match)
      if(MTXM_getPktSeqNo() != MRXM_getPktSeqNo() )
      {
        return 0;
      }
  }

  return 1;
}

/**************************************************************************************************//**
 *
 * @brief       This function is used to handle the received ACK packet. if ACK packet includes
 *              timing correction IE, it will parse this IE.
 *
 * @param[in]  usm_buf_ack  -- pointer of received ACK packet buffer
 *             usm_buf_data -- pointer of transmitted data packet buffer
 *             cur_link     -- pointer to current link data structure
 *
 * @return      1 if ACK packet is good
 *              0 if there is any error.
 *
 ***************************************************************************************************/
static inline int16_t process_recv_ack(BM_BUF_t* usm_buf_ack, BM_BUF_t* usm_buf_data, TSCH_DB_link_t* cur_link)
{
   ie_acknack_time_correction_t ie_cntnt;
   frame802154e_t frame;
   uint16_t len;
   uint8_t channelIndex = cur_link->channel;
   
   memset(&frame,0,sizeof(frame));
   if(!(len = framer_802154e_parseHdr(usm_buf_ack, &frame)) ||
         (MRXM_getPktType() != ULPSMACBUF_ATTR_PACKET_TYPE_ACK))
   {
      return 0;
   }

   if (!framer_802154e_parsePayload(usm_buf_ack, len, &frame))
   {
	   return 0;
   }

    if(!check_recv_ack(usm_buf_ack, usm_buf_data))
    {
        return 0;
    }

    ie_cntnt.acknack = 0;

    if (MRXM_getPktIEFlag() )
    {
        ulpsmacaddr_t* srcAddr;
        srcAddr = MRXM_getPktSrcAddr();
        if (TSCH_DB_isKeepAliveParent(ulpsmacaddr_to_short(srcAddr)))
        {
            ie802154e_hie_t find_ie;
            uint8_t found;

            find_ie.ie_elem_id = IE_ID_ACKNACK_TIME_CORRECTION;
            found = ie802154e_find_in_hdrielist(MRXM_getHdrIePtr(), MRXM_getHdrIeLen(), &find_ie);

            if(found)
            {
                int16_t        slotdelta_us;
                rtimer_clock_t slotdelta_tick;
                uint8_t        slotdelta_phase;

                ie802154e_parse_acknack_time_correction(find_ie.ie_content_ptr, &ie_cntnt);
               
                slotdelta_phase = TSCH_ACM_SLOTDELTA_INPHASE;
                slotdelta_tick = 0;

                slotdelta_us = ie_cntnt.time_synch;
                if(slotdelta_us > 0)
                {
                    slotdelta_tick = ULPSMAC_US_TO_TICKS((uint16_t)(slotdelta_us + 1));
                    if(slotdelta_tick != 0)
                    {
                        slotdelta_phase = TSCH_ACM_SLOTDELTA_IAM_EARLY;
                    }
                } else if(slotdelta_us < 0)
                {
                    slotdelta_tick = ULPSMAC_US_TO_TICKS((uint16_t) ((-1) * (slotdelta_us -1)));
                    if(slotdelta_tick != 0)
                    {
                        slotdelta_phase = TSCH_ACM_SLOTDELTA_IAM_LATE;
                    }
                }
                
                if(MRXM_getPktDstAddrMode() == ULPSMACBUF_ATTR_ADDR_MODE_SHORT)
                {
                  if(ie_cntnt.rxTotal != 0xfffffffe)
                    ULPSMAC_Dbg.numRxTotal = ie_cntnt.rxTotal;
                  if(ie_cntnt.rxRssi != 0xfe)
                  {
                    if (ULPSMAC_Dbg.rxRssiCount[channelIndex] < 0xff)
                    {
                      ULPSMAC_Dbg.avgRxRssi[channelIndex]=
                        (uint8_t)(((uint16_t)(ULPSMAC_Dbg.avgRxRssi[channelIndex])
                                   *ULPSMAC_Dbg.rxRssiCount[channelIndex]
                                     +ie_cntnt.rxRssi)
                                  /(ULPSMAC_Dbg.rxRssiCount[channelIndex]+1));
                      ULPSMAC_Dbg.rxRssiCount[channelIndex]++;
                    }
                  }
                }
                
                if(slotdelta_phase != TSCH_ACM_SLOTDELTA_INPHASE)
                {
                    TSCH_ACM_set_slotdelta(slotdelta_tick, slotdelta_phase);
                    
                    ULPSMAC_Dbg.avgSyncAdjust = 
                      (ULPSMAC_Dbg.avgSyncAdjust*ULPSMAC_Dbg.numSyncAdjust + 
                       ULPSMAC_TICKS_TO_US(slotdelta_tick))/(ULPSMAC_Dbg.numSyncAdjust+1);            
                    ULPSMAC_Dbg.numSyncAdjust++;
                    
                    if (ULPSMAC_TICKS_TO_US(slotdelta_tick) > ULPSMAC_Dbg.maxSyncAdjust)
                      ULPSMAC_Dbg.maxSyncAdjust = ULPSMAC_TICKS_TO_US(slotdelta_tick);
                }
                
                nhl_ipc_msg_post(COMMAND_ADJUST_KA_TIMER, (uint32_t)slotdelta_tick, 0, 0); //JIRA51
                
            }
        }
    }
    
    
    if (ULPSMAC_Dbg.rssiCount[channelIndex] < 0xff)
    {
      ULPSMAC_Dbg.avgRssi[channelIndex]=
        (uint8_t)(((uint16_t)(ULPSMAC_Dbg.avgRssi[channelIndex])
                   *ULPSMAC_Dbg.rssiCount[channelIndex]
                     +macRxPktParam.rssi)
                  /(ULPSMAC_Dbg.rssiCount[channelIndex]+1));
      ULPSMAC_Dbg.rssiCount[channelIndex]++;
    }

    return 1;
}

  /**************************************************************************************************//**
                           *
* @brief       This function is used to handle TX packet is transmitted. It will call sent_postproc
 *              and free the corresponding element in MAC TX queue. This element is created when
     *              packet is enque.

  *
  * @param[in]  link         -- pointer of current link data structure
     *             qdpkt_ptr    -- pointer of TX MAC queue element
     *             status       -- TX packet Done status
 *
     * @return
     *
***************************************************************************************************/
static void packet_sent(TSCH_DB_link_t* link, TSCH_TXM_queued_packet_t* qdpkt_ptr, int status)
{
    sent_postproc(qdpkt_ptr->sent, qdpkt_ptr->cptr, status, qdpkt_ptr->num_tx);

    if (TSCH_DB_LINK_TYPE(link) != TSCH_DB_LINK_TYPE_ADV)
    {
        memb_free(&qdpkt_memb, qdpkt_ptr);
        numberOfQueuedTxPackets -= (numberOfQueuedTxPackets > 0) ? 1 : 0;//JIRA74
    }
}

/**************************************************************************************************//**
 *
 * @brief       TSCH MAC random number generator. It will use RTC1 timing value.
 *
 * @param[in]
 *
 *
 * @return      16 bit random number
 *
 ***************************************************************************************************/
static inline uint16_t my_rand(void)
{
  return (random_rand() + RTIMER_NOW());
}

/**************************************************************************************************//**
 *
 * @brief       reset the back off parameters for the shared links
 *
 * @param[in]
 *
 *
 * @return
 *
 ***************************************************************************************************/
static void reset_sh_backoff(void)
{
  shared_bo_retx = 0;
  shared_bo_cnt = 1;
}

/**************************************************************************************************//**
 *
 * @brief       increment the retry counts for the shared link
 *
 * @param[in]
 *
 *
 * @return
 *
 ***************************************************************************************************/
static void inc_sh_backoff(void)
{
    if(shared_bo_retx == 0)
    {
        shared_bo_retx = TSCH_MAC_Pib.minBe;
    }
    else
    {
        shared_bo_retx += 1;
    }

    if(shared_bo_retx > TSCH_MAC_Pib.maxBe)
    {
        shared_bo_retx = TSCH_MAC_Pib.maxBe;
    }

  /* exponential backoff */
  shared_bo_cnt = my_rand() % ((1 << shared_bo_retx) -1);

}


/**************************************************************************************************//**
 *
 * @brief       init the LMAC TXM module. It also init the MAC TX queue
 *
 *
 * @param[in]   db_hnd -- pointer of TSCH_DB_handle which includes all TSCH related parameters.
 *
 *
 * @return      status of operation
 *
 ***************************************************************************************************/
LOWMAC_status_e TSCH_TXM_init(TSCH_DB_handle_t* db_hnd)
{
  txm_db_hnd = db_hnd;

  memb_init(&qdpkt_memb);
  numberOfQueuedTxPackets = 0;//JIRA74

  usm_buf_ackrecv = BM_alloc(20);
  usm_buf_keepalive  = BM_alloc(21);
  usmBufBeaconTx = BM_alloc(22);

  bcn_qdpkt_ptr = memb_alloc(&qdpkt_memb);

  reset_sh_backoff();

  return LOWMAC_STAT_SUCCESS;
}


/**************************************************************************************************//**
 *
 * @brief       enque the MAC TX packet to MAC TX queue. It is called by MAC API
 *
 *
 * @param[in]   usm_buf -- pointer of MAC frame buffer
 *              sent_callback -- callback functiron for this MAC frame
 *              ptr     -- pointer of data buffer
 *
 *
 * @return      status of operation
 *
 ***************************************************************************************************/
LOWMAC_status_e TSCH_TXM_que_pkt(BM_BUF_t* usm_buf, LOWMAC_txcb_t sent_callback, void* ptr)
{

  uint8_t sf_iter;

  TSCH_DB_link_t*           link;
  TSCH_TXM_queued_packet_t* qdpkt_ptr;
  uint32_t key;

  link = NULL;

  for(sf_iter=0; sf_iter<TSCH_DB_MAX_SLOTFRAMES; sf_iter++) {
    if(TSCH_DB_SLOTFRAME_IN_USE(txm_db_hnd, sf_iter) == LOWMAC_TRUE) {

      link = find_next_link(sf_iter, usm_buf, NULL);
      if(link) {
        break;
      }
    }
  }

  if(!link)
  {
    // there is no link, drop this packet
    sent_postproc(sent_callback, ptr, LOWMAC_STAT_NO_LINK, 0);
    return LOWMAC_STAT_NO_LINK;
  }

  if (TSCH_DB_LINK_TYPE(link) == TSCH_DB_LINK_TYPE_ADV)
  {
      qdpkt_ptr = bcn_qdpkt_ptr;
  }
  else
  {
      key = Task_disable();
      qdpkt_ptr = memb_alloc(&qdpkt_memb);
      numberOfQueuedTxPackets++;//JIRA74
      Task_restore(key);
  }


  if(!qdpkt_ptr)
  {
    // record error
    ULPSMAC_Dbg.numTxQueueFull++;
    sent_postproc(sent_callback, ptr, LOWMAC_STAT_NO_QUEUE, 0);
    return LOWMAC_STAT_NO_QUEUE;
  }


  qdpkt_ptr->next = NULL;
  qdpkt_ptr->buf = (struct ulpsmacbuf  *)usm_buf;
  qdpkt_ptr->sent = sent_callback;
  qdpkt_ptr->cptr = ptr;
  qdpkt_ptr->num_tx = 0;

  key = Task_disable();
  TSCH_DB_LINK_PKTQ_LIST_ADD(link, qdpkt_ptr);

  Task_restore(key);

  return LOWMAC_STAT_SUCCESS;
}
/**************************************************************************************************//**
 *
 * @brief
 *
 *
 * @param[in]   usm_buf -- pointer of MAC frame buffer
 *              sent_callback -- callback functiron for this MAC frame
 *              ptr     -- pointer of data buffer
 *
 *
 * @return
 *
 ***************************************************************************************************/
#define min(a,b) (((a) < (b)) ? (a) : (b))
void TSCH_TXM_que_pkt_thread_safe(BM_BUF_t* usm_buf, LOWMAC_txcb_t sent_callback, void* ptr)
{
   bool status = nhl_ipc_msg_post(COMMAND_TRANSMIT_PACKET, (uint32_t)usm_buf, (uint32_t)sent_callback, (uint32_t)ptr);

   if (status == false)
   {
      sent_postproc(sent_callback, ptr, LOWMAC_STAT_NO_QUEUE, 0);
   }
}

/**************************************************************************************************//**
 *
 * @brief       check if we can tranmit the packet based on specified link and link type and packet type
 *
 *
 * @param[in]   link -- pointer of TSCH link data structure
                link_type - actually is linkOption 
 *
 *
 * @return      TRUE, TSCH can transmit the packet in this link
 *              FALSE, TSCH can't transmit the packet
 ***************************************************************************************************/

uint8_t TSCH_TXM_can_send(TSCH_DB_link_t* link, uint8_t link_type)
{
  
  if(link_type == TSCH_DB_LINK_OPTIONS_TX) {
    if(TSCH_DB_LINK_PKTQ_LIST_HEAD(link)) {
   
      return LOWMAC_TRUE;
    }
  } else if(link_type == TSCH_DB_LINK_OPTIONS_SH) {
    if( ((--shared_bo_cnt) < 0) && TSCH_DB_LINK_PKTQ_LIST_HEAD(link)) {
       return LOWMAC_TRUE;
    }
  }

  return LOWMAC_FALSE;
}

/**************************************************************************************************//**
 *
 * @brief       This is the TSCH TX processing function in timeslot.
 *
 *
 * @param[in]   cur_link -- pointer of TSCH link data strureuct
 *              this_slot_start -- time stamp of current time slot starting time
 *              cur_asn     -- ASN number of current time slot
 *
 *
 * @return      TRUE, operation is sucsess
 *              FALSE, operation is failure
 ***************************************************************************************************/
BM_BUF_t *        usm_buf_dbg;
LOWMAC_status_e TSCH_TXM_timeslot(TSCH_DB_link_t* cur_link, rtimer_clock_t this_slot_start, uint64_t cur_asn)
{
    TSCH_TXM_queued_packet_t* qdpkt_ptr;
    BM_BUF_t *        usm_buf;
    
    HAL_Status_t  radio_res;

    uint8_t  got_ack;
    uint8_t  is_broadcast;
    int16_t  mac_tx_res;
    int16_t  recv_len;
    int32_t timeout;
    uint8_t  txDataDstMode;
    uint8_t eventSt;
    uint8_t linkOptions;
    uint16_t destAddrShort;

    static uint8_t numDataNoAck = 0;
    
    qdpkt_ptr = TSCH_DB_LINK_PKTQ_LIST_POP(cur_link);
    qdpkt_ptr->next = NULL;
    usm_buf = (BM_BUF_t *)qdpkt_ptr->buf;
    
    if (usm_buf == NULL)
    {
      return LOWMAC_STAT_ERR;
    }    
    
    // parse and save the TX packet MAC header
    MTXM_saveMacHeaderInfo(BM_getBufPtr(usm_buf));

    if( MTXM_getPktType() ==  ULPSMACBUF_ATTR_PACKET_TYPE_BEACON)
    {
        uint8_t* buf_asn_ptr = MTXM_getBeaconASNPtr(BM_getDataPtr(usm_buf));
        ie802154e_set_asn(buf_asn_ptr, cur_asn);
#if WITH_UIP6
        uint8_t jp =  uip_rpl_get_rank();
        *(buf_asn_ptr+5) = jp;
#endif
    }

#ifdef FEATURE_MAC_SECURITY
    {
      uint8_t authlen;
      unsigned char *pAData, *pMData;
      uint8_t aDataLen, mDataLen;
      uint8_t open_payload_len;
      uint8_t *pFrameCounterExt;
      uint32_t *pFrameCounter;
      uint8_t FrameCounterExt;
      uint32_t FrameCounter;
      uint8_t *paux_hdr;
      uint8_t securityLevel;
      uint8_t frmCntSize;
      uint8_t nonce[13];
      
      securityLevel = MTXM_getPktSecurityLevel();
      frmCntSize = MTXM_getPktFrmCntSize();
      
      if(MTXM_getPktSecurityEnable() && securityLevel)
      {
        //copy usm_buf to global_buf
        //set usm_buf pointing to global_buf
        memcpy(&cypered_usm_buf, usm_buf, sizeof(BM_BUF_t));
        usm_buf = &cypered_usm_buf;
        
        destAddrShort = ulpsmacaddr_to_short(MTXM_getPktDstAddr());

        if((macOutgoingFrameSecurity(destAddrShort, (frame802154e_t *)MTXM_getFrameInfo(), &pKeyTx, &pFrameCounterExt, &pFrameCounter)) != MAC_SUCCESS)
        {
          MACSECURITY_DBG.encrypt_security_error++;
          packet_sent(cur_link, qdpkt_ptr, LOWMAC_STAT_ERR);
          return LOWMAC_STAT_ERR;
        }
        
        // replace frame counter by cur_asn
        if(!MTXM_getPktFrmCntSuppression())
        {
          paux_hdr = BM_getBufPtr(usm_buf) + MTXM_getAuxHdrOffset();
          if(frmCntSize)
          {
            if (cur_asn == 0xFFFFFFFFFF)
            {
              MACSECURITY_DBG.encrypt_counter_error++;
              packet_sent(cur_link, qdpkt_ptr, LOWMAC_STAT_ERR);
              return (LOWMAC_STAT_ERR);
            }
            *(paux_hdr+1) = (cur_asn >>  0)&0xFF;
            *(paux_hdr+2) = (cur_asn >>  8)&0xFF;
            *(paux_hdr+3) = (cur_asn >> 16)&0xFF;
            *(paux_hdr+4) = (cur_asn >> 24)&0xFF;
            *(paux_hdr+5) = (cur_asn >> 32)&0xFF;
            FrameCounterExt = (cur_asn >> 32)&0xFF;
            FrameCounter = (cur_asn >>  0)&0xFFFFFFFF;
          }
          else
          {
            FrameCounterExt = 0;
            FrameCounter = *pFrameCounter;
            
            if (FrameCounter == MAC_MAX_FRAME_COUNTER)
            {
              MACSECURITY_DBG.encrypt_counter_error++;
              packet_sent(cur_link, qdpkt_ptr, LOWMAC_STAT_ERR);
              return (LOWMAC_STAT_ERR);
            }
            
            *(paux_hdr + 1) = (FrameCounter >>  0) & 0xFF;
            *(paux_hdr + 2) = (FrameCounter >>  8) & 0xFF;
            *(paux_hdr + 3) = (FrameCounter >> 16) & 0xFF;
            *(paux_hdr + 4) = (FrameCounter >> 24) & 0xFF;
            
            (*pFrameCounter)++;
          }
        }
        
        authlen = securityLevel & 3;
        authlen = (authlen * 4 + ((authlen + 1) >> 2) * 4);
        
        if(MTXM_getPktType() == MAC_FRAME_TYPE_COMMAND)
        {
          open_payload_len = 1;
        }
        else
        {
          open_payload_len = 0;
        }
        
        pAData = BM_getBufPtr(usm_buf);
        aDataLen = MTXM_getPktHdrLen() + MTXM_getPayloadIeLen() + open_payload_len;
        pMData = pAData + aDataLen;
        mDataLen = BM_getDataLen(usm_buf) + BM_getDataOffset(usm_buf) - aDataLen;
        
        if(aDataLen + mDataLen + authlen + 2 > 127)
        {
          MACSECURITY_DBG.encrypt_framesize_error++;
          packet_sent(cur_link, qdpkt_ptr, LOWMAC_STAT_ERR);
          return LOWMAC_STAT_ERR;
        }
        
        memcpy(nonce, ulpsmacaddr_long_node.u8, 8);
        {
       	  uint32_t frameCounter = FrameCounter;
       	  int i;
          for (i = 11; i >= 8; i--)
          {
            nonce[i] = frameCounter & 0xFF;
            frameCounter >>= 8;
          }
        }
        
        if (frmCntSize)
        {
          nonce[12] = FrameCounterExt;
        }
        else
        {
          nonce[12] = securityLevel;
        }
        
        if((macCcmStarTransform(pKeyTx,authlen,nonce,pAData,aDataLen,pMData,mDataLen )) != MAC_SUCCESS)
        {
          MACSECURITY_DBG.encrypt_transform_error++;
          packet_sent(cur_link, qdpkt_ptr, LOWMAC_STAT_ERR);
          return LOWMAC_STAT_ERR;
        }
        
        BM_setDataLen(usm_buf, BM_getDataLen(usm_buf) + authlen);
      }
    }
#endif

   /* Check if it is broadcast packet */
   is_broadcast = 0;
   txDataDstMode = MTXM_getPktDstAddrMode();

   if((txDataDstMode == ULPSMACBUF_ATTR_ADDR_MODE_LONG))
   {
      if(ulpsmacaddr_long_cmp(MTXM_getPktDstAddr(), &ulpsmacaddr_broadcast))
      {
         is_broadcast = 1;
      }
   }
   else if((txDataDstMode == ULPSMACBUF_ATTR_ADDR_MODE_SHORT))
   {
      if(ulpsmacaddr_short_cmp(MTXM_getPktDstAddr(), &ulpsmacaddr_broadcast))
      {
         is_broadcast = 1;
      }
   } 
   
    radio_res = HAL_SUCCESS;
    rxAckFlag = 1;
    if( (MTXM_getPktType() != ULPSMACBUF_ATTR_PACKET_TYPE_BEACON) || !(TSCH_MACConfig.beaconControl & BC_DISABLE_TX_BEACON_MASK))
    {
#if DMM_ENABLE && CONCURRENT_STACKS
       DMMPolicy_updateStackState(DMMPolicy_StackType_WsnNode, DMMPOLICY_STACKSTATE_WSNNODE_TX);
#endif //DMM_ENABLE
       radio_res = HAL_RADIO_setChannel(TSCH_DB_LINK_CHANNEL(cur_link));
       if (radio_res == HAL_SUCCESS)
       {
          timeout = TSCH_ACM_RTC_TX_OFFSET - (RTIMER_NOW() - this_slot_start)
              - HAL_RADIO_TXRX_CMD_DELAY_TICKS;
          timeout = (timeout > 0)? ULPSMAC_TICKS_TO_US(timeout):0;
          timeout = RF_getCurrentTime() + timeout *4;
          radio_res = HAL_RADIO_transmit(BM_getBufPtr(usm_buf),
                                     BM_getDataLen(usm_buf), timeout);
       }
       else
       {
          ULPSMAC_Dbg.numSetChannelErr++;
       }
    
       if (radio_res == HAL_SUCCESS)
       {
          eventWait(ACM_EVENT_TX_DATA_DONE,10000, this_slot_start);
       }
       else
       {
           packet_sent(cur_link, qdpkt_ptr, LOWMAC_STAT_TX_COLLISION);
       }
       
       got_ack = 0;
       recv_len = 0;
       if((radio_res == HAL_SUCCESS) && (!is_broadcast) && MTXM_getPktAck() )
       {
          timeout = TSCH_ACM_RTC_RX_ACK_DELAY - (RTIMER_NOW() - HAL_txEndTime) -
                HAL_RADIO_TXRX_CMD_DELAY_TICKS;
          timeout = (timeout > 0)? ULPSMAC_TICKS_TO_US(timeout):0;
          timeout = RF_getCurrentTime() + timeout*4;
                
          HAL_RADIO_receiveOn(TSCH_rxISR, timeout);

          rxAckFlag = 0;
          timeout = TSCH_ACM_RTC_RX_ACK_DELAY + TSCH_ACM_RTC_ACK_WAIT;
          eventSt = eventWait(ACM_EVENT_RX_SYNC, timeout, HAL_txEndTime);

          if (eventSt == ACM_WAIT_EVT_SUCCESS)
          {   // continue to wait
             timeout = TSCH_ACM_RTC_RX_ACK_DELAY + TSCH_ACM_RTC_ACK_WAIT + TSCH_ACM_RTC_MAX_ACK;
             eventSt = eventWait(ACM_EVENT_RX_DONE_OK,timeout, HAL_txEndTime);
          }

          HAL_RADIO_receiveOff();
       }
#if DMM_ENABLE && CONCURRENT_STACKS
       DMMPolicy_updateStackState(DMMPolicy_StackType_WsnNode, DMMPOLICY_STACKSTATE_WSNNODE_SLEEPING);
#endif //DMM_ENABLE
    }

    if (rxAckFlag == 0)
    {
       mac_tx_res = LOWMAC_STAT_TX_NOACK;
    }
    else
    {
       rxAckFlag = 0;
       recv_len = process_recv_ack(usm_buf_ackrecv, usm_buf, cur_link);
       got_ack = (recv_len > 0) ? 1 : 0;
    }

    qdpkt_ptr->num_tx++;

    if(is_broadcast)
    {
        PWREST_updateTxRxBytes(BM_getDataLen(usm_buf),0);
        
        /* broadcast, no backoff at all */
        if(radio_res == HAL_SUCCESS)
        {
            if( (TSCH_DB_LINK_OPTIONS(cur_link) & TSCH_DB_LINK_OPTIONS_SH) || is_que_empty())
            {
                reset_sh_backoff();
            }
            packet_sent(cur_link, qdpkt_ptr, LOWMAC_STAT_SUCCESS);
        }
        else
        {
            if(TSCH_DB_LINK_OPTIONS(cur_link) & TSCH_DB_LINK_OPTIONS_SH)
            {
                inc_sh_backoff();
            }
            packet_sent(cur_link, qdpkt_ptr, LOWMAC_STAT_TX_COLLISION);
        }
    }
    else
    {
        PWREST_updateRFSlotCount(PWREST_TX_UNICAST_PACKET);
        PWREST_updateTxRxBytes(BM_getDataLen(usm_buf),recv_len);
        
        /* Unicast. Check transmission status. */
        mac_tx_res = LOWMAC_STAT_SUCCESS;

        if (MTXM_getPktAck())
        {
            /* unicast with ack request */
            if(got_ack != 1)
            {
                if(radio_res == HAL_SUCCESS)
                {
                    mac_tx_res = LOWMAC_STAT_TX_NOACK;
                }
                else
                {
                    mac_tx_res = LOWMAC_STAT_TX_COLLISION;
                }
            }
        }
        else
        {
            /* unicast, but no ack request*/
            if(radio_res != HAL_SUCCESS)
            {
                mac_tx_res = LOWMAC_STAT_TX_COLLISION;
            }
        }
        
        destAddrShort = ulpsmacaddr_to_short(MTXM_getPktDstAddr());
        linkOptions = TSCH_DB_LINK_OPTIONS(cur_link) & TSCH_DB_LINK_OPTIONS_TRX_MASK;
        
        /* 
        1. Update the TX statistics only when TX to parent
        2. Update the TX  statistics only in dedicated TX link
        */
        if ((destAddrShort == ULPSMAC_Dbg.parent) && 
            (linkOptions == TSCH_DB_LINK_OPTIONS_TX))
        {
          uint8_t len = BM_getDataLen(usm_buf);
          uint8_t* pBuf =  BM_getBufPtr(usm_buf);
          uint8_t channelIndex = cur_link->channel;
          
          ULPSMAC_Dbg.numTxTotal++;
          ULPSMAC_Dbg.numTxLengthTotal += len;
          
          if (mac_tx_res != LOWMAC_STAT_SUCCESS)
          {
              ULPSMAC_Dbg.numTxNoAck++;           
              if (qdpkt_ptr->num_tx > TSCH_MAC_Pib.maxFrameRetries)
              {
                  ULPSMAC_Dbg.numTxFail++;
              } else {
                  
              }                  
          }
          
          /* reset TX counter to avid ovreflow*/
          if (ULPSMAC_Dbg.numTxTotal == 4000000)
          {
              ULPSMAC_Dbg.numTxTotal = MIN_ETX_COUNT;
              ULPSMAC_Dbg.numTxNoAck = 0;
              ULPSMAC_Dbg.numTxFail = 0;
          }
          
          if(ULPSMAC_Dbg.numTxLengthTotal >= 400000000)
          {
              ULPSMAC_Dbg.numTxLengthTotal = ULPSMAC_Dbg.numTxLengthTotal - 400000000;
          }
          
          if (ULPSMAC_Dbg.numTxTotalPerCh[channelIndex] < 0xff)
          {
              ULPSMAC_Dbg.numTxTotalPerCh[channelIndex]++;
              if (mac_tx_res != LOWMAC_STAT_SUCCESS)
              {
                  ULPSMAC_Dbg.numTxNoAckPerCh[channelIndex]++;
              }
          }
        }

        if(mac_tx_res == LOWMAC_STAT_SUCCESS)
        {
          usm_buf_dbg = usm_buf;
          /* successful TX: shutoff the backoff */
          /* 7.5.1.4.3: */
          if( (TSCH_DB_LINK_OPTIONS(cur_link) & TSCH_DB_LINK_OPTIONS_SH) || is_que_empty())
          {
            reset_sh_backoff();
          }
          
          if (!(TSCH_DB_LINK_OPTIONS(cur_link) & TSCH_DB_LINK_OPTIONS_SH))
          {
            numDataNoAck = 0;
          }
          // Jiachen: log the sent asn of its coap sensing packet
          if((usm_buf->owner == 30) && // buf created by itself, see sicslowpan_tx_request of uip_rpl_process.c
             (destAddrShort == ULPSMAC_Dbg.parent)  &&  // to parent
             (linkOptions == TSCH_DB_LINK_OPTIONS_TX)) // unicast 
          { 
            if(usm_buf->coap_type == BM_COAP_DATA_SENSORS)
            {
              packet_sent_asn_sensors = cur_asn;
            }
//            if(!usm_buf->coap_type)
//            {
//              packet_sent_asn_others = cur_asn;
//            }
            if(usm_buf->coap_type == BM_COAP_DATA_PING)
            {
              packet_sent_asn_ping = cur_asn;
            }
            

          }

          packet_sent(cur_link, qdpkt_ptr, LOWMAC_STAT_SUCCESS);
          
        }
        else if(qdpkt_ptr->num_tx > TSCH_MAC_Pib.maxFrameRetries )
        {
          /* tried max time broadcast is always one time try is the max*/
          
          if(TSCH_DB_LINK_OPTIONS(cur_link) & TSCH_DB_LINK_OPTIONS_SH)
          {
            reset_sh_backoff();
          }
          else
          {
            numDataNoAck++;
            if (numDataNoAck >= DATA_SYNCH_LOSS)
            {
              if (MlmeCb)
              {
                tschMlmeRestart_t restart;
                restart.hdr.event =  MAC_MLME_RESTART;
                MlmeCb((tschCbackEvent_t *)(&restart),NULL);
              }
              numDataNoAck = 0;
              ULPSMAC_Dbg.numSyncLost++;
            }
          }
          
          packet_sent(cur_link, qdpkt_ptr, mac_tx_res);
        }
        else
        {
          TSCH_DB_link_t* next_link;
          
          next_link = find_next_link(TSCH_DB_LINK_SLOTFRAME_ID(cur_link), usm_buf, cur_link);

          TSCH_DB_LINK_PKTQ_LIST_PUSH(next_link, qdpkt_ptr);
          
          if(TSCH_DB_LINK_OPTIONS(cur_link) & TSCH_DB_LINK_OPTIONS_SH)
          {
            inc_sh_backoff();
          }
          }
    } // if !is_broadcast
    
    return LOWMAC_STAT_SUCCESS;
}

/**************************************************************************************************//**
 *
 * @brief       This is the callback function when keepalive packet is sent
 *
 *
 * @param[in]   ptr -- pointer of data packet buffer
 *              status -- status of operation
 *              transmissions     -- number of retry for this packet
 *
 *
 * @return
 *
 ***************************************************************************************************/
static void keepalive_sent_cb(void* ptr, uint16_t status, uint8_t transmissions)
{
  /* Always restart KA timer regarless tx status.
     TXM will handle sync lost detection.*/
  
    uint32_t time = (TSCH_MACConfig.keepAlive_period*CLOCK_SECOND/1000);
    if (status != LOWMAC_STAT_SUCCESS)
    {
      SN_clockSet(ka_ctimer, 0, time/2);
    }
    
    SN_clockRestart(ka_ctimer);
}

/**************************************************************************************************//**
 *
 * @brief
 *
 *
 * @param[in]
 *
 *
 * @return
 *
 ***************************************************************************************************/
bool nhl_ipc_msg_post(uint8_t msgType, uint32_t v1, uint32_t v2, uint32_t v3)
{
   NHLDB_TSCH_CMM_cmdmsg_t* cmm_msg;
   cmm_msg = NHLDB_TSCH_CMM_alloc_cmdmsg(3);
   bool status = false;

   if (cmm_msg)
   {
      uint8_t *arg = cmm_msg->arg;
      cmm_msg->type = msgType;
      cmm_msg->event_type = TASK_EVENT_MSG;
      memcpy(arg, &v1, sizeof(uint32_t));
      arg += sizeof(uint32_t);
      memcpy(arg, &v2, sizeof(uint32_t));
      arg += sizeof(uint32_t);
      memcpy(arg, &v3, sizeof(uint32_t));

      uint32_t ptrAddr = (uint32_t)cmm_msg;
      status = SN_post(nhl_tsch_mailbox, &ptrAddr, BIOS_NO_WAIT, NOTIF_TYPE_MAILBOX);
      
      if (status == false)
      {
        ULPSMAC_Dbg. numNhlEventPostError++;
        NHLDB_TSCH_CMM_free_cmdmsg(cmm_msg);
      }
   }

   return(status);
}

/**************************************************************************************************//**
 *
 * @brief
 *
 *
 * @param[in]   ptr -- pointer of data packet buffer
 *
 *
 * @return
 *
 ***************************************************************************************************/
void TSCH_TXM_send_keepalive_pkt(void* ptr)
{
   nhl_ipc_msg_post(COMMAND_SEND_KA_PACKET, (uint32_t)ptr, 0, 0);
}

/**************************************************************************************************//**
 *
 * @brief       This is used to transmit the keepalive message
 *
 *
 * @param[in]   ptr -- pointer of data packet buffer
 *
 *
 * @return
 *
 ***************************************************************************************************/
void TSCH_TXM_send_ka_packet_handler(void *ptr)
{
  TSCH_DB_ti_keepalive_t *db_keepalive = *(TSCH_DB_ti_keepalive_t **)ptr;
  ulpsmacaddr_t dst_ulpsmacaddr_short;
  uint8_t len;

  usm_buf_keepalive->datalen = 0;
  usm_buf_keepalive->dataoffset  = 0;
  db_keepalive->usm_buf = usm_buf_keepalive;
  
  ulpsmacaddr_from_short(&dst_ulpsmacaddr_short, db_keepalive->vals.dst_addr_short);
  
  len = framer_802154e_createKaPkt(db_keepalive->usm_buf, &dst_ulpsmacaddr_short);
  
  if(len == 0)
  {
    keepalive_sent_cb(db_keepalive, LOWMAC_STAT_FRAME_TOO_LONG, 0);
    return;
  }
  
  TSCH_TXM_que_pkt(db_keepalive->usm_buf, keepalive_sent_cb, db_keepalive);
  return;
}
/**************************************************************************************************//**
 *
 * @brief
 *
 * @param[in]
 *
 * @return
 *
 ***************************************************************************************************/
void TSCH_TXM_adjust_ka_timer_handler(rtimer_clock_t *slotdelta_tick_p)
{
    if (ka_ctimer)
    {
      uint32_t time = (TSCH_MACConfig.keepAlive_period*CLOCK_SECOND/1000);

      if (ULPSMAC_TICKS_TO_US(*slotdelta_tick_p) > 400)
      {
        SN_clockSet(ka_ctimer, 0, time/2);
      }
      else
      {
        SN_clockSet(ka_ctimer, 0, time);
      }
       SN_clockRestart(ka_ctimer);
    }
}

/**************************************************************************************************//**
 *
 * @brief       This function deletes all the share link in TSCH DB
 *
 *
 * @param[in]   slotframeHandle -- pointer of the slotframe handle of the deleted link
 * @param[in]   linkHandle -- pointer of the link handle of the deleted link
 *
 *
 * @return
 *
 ***************************************************************************************************/
LOWMAC_status_e TSCH_TXM_delete_share_link(uint8_t* slotframeHandle, uint16_t * linkHandle)
{
    TSCH_DB_slotframe_t* slotframe = NULL;
    TSCH_DB_link_t* cur_link = NULL;
    TSCH_DB_link_t* next_link = NULL;

    TSCH_PIB_link_t*     in_link = NULL;
 
    uint8_t sfIter;
    uint16_t size;
    LOWMAC_status_e status;

    status = LOWMAC_STAT_ERR;

    for (sfIter = 0; sfIter < TSCH_DB_MAX_SLOTFRAMES; sfIter++)
    {
        slotframe = TSCH_DB_SLOTFRAME(txm_db_hnd, sfIter);

        cur_link = TSCH_DB_SLOTFRAME_LINK_HEAD(slotframe);

        while (cur_link)
        {
            next_link = list_item_next(cur_link);

            if (cur_link->vals.link_option == (MAC_LINK_OPTION_SH | MAC_LINK_OPTION_RX))
            {
                if (cur_link->vals.timeslot ==1)
                {
                    cur_link->vals.period = TSCH_MACConfig.sh_period_sf;
                }
                else
                {
                    /* BZ 15201 */
                    /* delink from the timeslot 1 to avoid shared queue removal */
                    cur_link->qdpkt_list = &(cur_link->qdpkt_list_list);
                    in_link = &(cur_link->vals);
                    *linkHandle = in_link->link_id;
                    *slotframeHandle = in_link->slotframe_handle;
                    size = sizeof(TSCH_PIB_link_t);
                    status = TSCH_DB_delete_db(TSCH_DB_ATTRIB_LINK,in_link, &size);

                    return status;
                }
            }

            cur_link = next_link;
        }
    }

    return status;
}

/**************************************************************************************************//**
 *
 * @brief       This function deletes all the dedicated links with the specific
 *              link peer address in TSCH DB.
 *
 * @param[in]   linkPeerAddr -- link peer address
 * @param[out]   slotframeHandle -- slotframe handle
 * @param[out]   linkHandle -- link handle 
 *
 * @return
 *
 ***************************************************************************************************/
LOWMAC_status_e TSCH_TXM_delete_dedicated_link(uint16_t linkPeerAddr,uint8_t* slotframeHandle, uint16_t * linkHandle)
{
    TSCH_DB_slotframe_t* slotframe = NULL;
    TSCH_DB_link_t* cur_link = NULL;
    TSCH_DB_link_t* next_link = NULL;

    TSCH_PIB_link_t*     in_link = NULL;

    uint8_t sfIter;
    uint16_t size;
    LOWMAC_status_e status;

    status = LOWMAC_STAT_ERR;

    for (sfIter = 0; sfIter < TSCH_DB_MAX_SLOTFRAMES; sfIter++)
    {
        slotframe = TSCH_DB_SLOTFRAME(txm_db_hnd, sfIter);

        cur_link = TSCH_DB_SLOTFRAME_LINK_HEAD(slotframe);

        while (cur_link)
        {
            next_link = list_item_next(cur_link);

            if(TSCH_DB_LINK_PEERADDR(cur_link) == linkPeerAddr)
            {
                in_link = &(cur_link->vals);
                size = sizeof(TSCH_PIB_link_t);
                *linkHandle = in_link->link_id;
                *slotframeHandle = in_link->slotframe_handle;
                status = TSCH_DB_delete_db(TSCH_DB_ATTRIB_LINK,in_link, &size);

                return status;
            }

            cur_link = next_link;
        }
    }

    return status;
}

/**************************************************************************************************//**
 *
 * @brief       This function free TX packets in MAC TX queue for specified device. It will
 *              tranverse all links in slot frame. This function is called by disassocation process.
 *
 * @param[in]   addrMode -- address mode of destination address in pDstAddr
 *              pDstAddr -- pointer of ulpsmacaddr_t
 *
 * @return
 *
 **************************************************************************************************/
void TSCH_TXM_freeDevPkts(uint8_t addrMode, ulpsmacaddr_t *pDstAddr)
{
  uint8_t sf_iter;
  TSCH_DB_slotframe_t* slotframe;
  TSCH_DB_link_t *link,*nextLink;
  //TSCH_TXM_queued_packet_t* qdpkt_ptr;
  //BM_BUF_t *usm_buf;
  tschMlmeSetLinkReq_t setLinkArg;
  uint16_t devShortAddr;

  if (addrMode == ULPSMACADDR_MODE_LONG)
  { // from device tabl to find the short address
    devShortAddr = NHL_deviceTableGetShortAddr(pDstAddr);
  }
  else
  {
    devShortAddr = ulpsmacaddr_to_short(pDstAddr);
  }

  for(sf_iter=0; sf_iter<TSCH_DB_MAX_SLOTFRAMES; sf_iter++)
  {
    if (TSCH_DB_SLOTFRAME_IN_USE(txm_db_hnd, sf_iter) == ULPSMAC_FALSE)
    {
        continue;
    }

    slotframe = TSCH_DB_SLOTFRAME(txm_db_hnd, sf_iter);

    link = TSCH_DB_SLOTFRAME_LINK_HEAD(slotframe);

    if (link == NULL)
    {
        continue;
    }

    // tranverse through all links and free TX packets

    while (link)
    {
        // get next link
        nextLink = list_item_next(link);

        if (TSCH_DB_LINK_PEERADDR(link) == devShortAddr )
        {   // link's peer address match device address
            // free this link
            setLinkArg.slotframeHandle = link->vals.slotframe_handle;
            setLinkArg.linkHandle = link->vals.link_id;

            setLinkArg.operation = SET_LINK_DELETE;

            TSCH_MlmeSetLinkReq(&setLinkArg);
        }

        // go to next link
        link = nextLink;
    }

  }

  return ;
}
/**************************************************************************************************//**
 *
 * @brief       This function free all TX packets in a MAC TX queue of a specific link
 *
 * @param[in] the link
 *
 * @return
 *
 ***************************************************************************************************/
void TXM_freeQueuedTxPackets(TSCH_DB_link_t* db_link)
{
   BM_BUF_t *usm_buf;
   TSCH_TXM_queued_packet_t* qdpkt_ptr = TSCH_DB_LINK_PKTQ_LIST_POP(db_link);
   while(qdpkt_ptr)
   {
      if (qdpkt_ptr != bcn_qdpkt_ptr)
      {
         qdpkt_ptr->next = NULL;
         usm_buf = (BM_BUF_t *)qdpkt_ptr->buf;

         if ((usm_buf != usm_buf_keepalive) && ((usm_buf != usmBufBeaconTx)))
         {
            BM_free_6tisch(usm_buf);
         }

         memb_free(&qdpkt_memb, qdpkt_ptr);
         numberOfQueuedTxPackets -= (numberOfQueuedTxPackets > 0) ? 1 : 0;//JIRA74
      }

      // next packet in this link
      qdpkt_ptr = TSCH_DB_LINK_PKTQ_LIST_POP(db_link);
  }
}

/**************************************************************************************************//**
 *
 * @brief       This function free all TX packets in MAC TX queue. It will tranverse all links in
 *              slot frame. This function is called by disassocation process.
 *
 * @param[in]
 *
 * @return
 *
 ***************************************************************************************************/
void TSCH_TXM_freePkts(void)
{
  uint8_t sf_iter;
  TSCH_DB_slotframe_t* slotframe;
  TSCH_DB_link_t *link,*nextLink;
  tschMlmeSetLinkReq_t setLinkArg;

  uint32_t key = Task_disable();

  for(sf_iter=0; sf_iter<TSCH_DB_MAX_SLOTFRAMES; sf_iter++)
  {
    if (TSCH_DB_SLOTFRAME_IN_USE(txm_db_hnd, sf_iter) == ULPSMAC_FALSE)
    {
        continue;
    }

    slotframe = TSCH_DB_SLOTFRAME(txm_db_hnd, sf_iter);

    link = TSCH_DB_SLOTFRAME_LINK_HEAD(slotframe);

    if (link == NULL)
    {
        continue;
    }

    // traverse through all links and free TX packets

    while (link)
    {
        // next link
        nextLink = list_item_next(link);
        //TXM_freeQueuedTxPackets(link);

        // free this link
        setLinkArg.slotframeHandle = link->vals.slotframe_handle;
        setLinkArg.linkHandle = link->vals.link_id;

        setLinkArg.operation = SET_LINK_DELETE;

        TSCH_MlmeSetLinkReq(&setLinkArg);

        // go to next link
        link = nextLink;
    }
  }

   reset_sh_backoff();
   Task_restore(key);
  return ;
}               
