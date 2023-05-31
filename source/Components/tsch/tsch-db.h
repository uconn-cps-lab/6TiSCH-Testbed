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
 *  ====================== tsch-db.h =============================================
 *  
 */

#ifndef __TSCH_DB_H__
#define __TSCH_DB_H__

#include "sys/clock.h"
#include "tsch-pib.h"
#include "lowmac.h"

#include "lib/list.h"
/*---------------------------------------------------------------------------*/
#define TSCH_DB_LINK_OPTIONS_TX        TSCH_PIB_LINK_OPTIONS_TX  // TX
#define TSCH_DB_LINK_OPTIONS_TX_COAP   TSCH_PIB_LINK_OPTIONS_TX_COAP // jiachen: TX for COAP
#define TSCH_DB_LINK_OPTIONS_RX        TSCH_PIB_LINK_OPTIONS_RX  // RX
#define TSCH_DB_LINK_OPTIONS_SH        TSCH_PIB_LINK_OPTIONS_SH  // Shared
#define TSCH_DB_LINK_OPTIONS_TK        TSCH_PIB_LINK_OPTIONS_TK  // Timekeeping


#define TSCH_DB_LINK_OPTIONS_TRX_MASK  0x07  // TX/RX/Shared

#define TSCH_DB_LINK_TYPE_NORMAL      TSCH_PIB_LINK_TYPE_NORMAL   // Normal
#define TSCH_DB_LINK_TYPE_ADV         TSCH_PIB_LINK_TYPE_ADV      // Advertisement

/*---------------------------------------------------------------------------*/
#define TSCH_DB_MAX_CHANNEL_HOPPING       1
#define TSCH_DB_MAX_SLOTFRAMES            1

#define TSCH_DB_MAX_ADV_ENTRIES           4

/*---------------------------------------------------------------------------*/
#define TSCH_DB_ATTRIB_ASN                TSCH_PIB_ASN
#define TSCH_DB_ATTRIB_JOIN_PRIO          TSCH_PIB_JOIN_PRIO
#define TSCH_DB_ATTRIB_MIN_BE             TSCH_PIB_MIN_BE
#define TSCH_DB_ATTRIB_MAX_BE             TSCH_PIB_MAX_BE
#define TSCH_DB_ATTRIB_MAX_FRAME_RETRIES  TSCH_PIB_MAX_FRAME_RETRIES
#define TSCH_DB_ATTRIB_DISCON_TIME        TSCH_PIB_DISCON_TIME

#define TSCH_DB_ATTRIB_CHAN_HOP           TSCH_PIB_CHAN_HOP
#define TSCH_DB_ATTRIB_TIMESLOT           TSCH_PIB_TIMESLOT
#define TSCH_DB_ATTRIB_SLOTFRAME          TSCH_PIB_SLOTFRAME
#define TSCH_DB_ATTRIB_LINK               TSCH_PIB_LINK
#define TSCH_DB_ATTRIB_TI_KEEPALIVE       TSCH_PIB_TI_KEEPALIVE



/*---------------------------------------------------------------------------*/
typedef struct __TSCH_DB_ti_keepalive {
  uint8_t  in_use;
  clock_time_t  ka_ct;
  BM_BUF_t* usm_buf;

  TSCH_PIB_ti_keepalive_t vals;

} TSCH_DB_ti_keepalive_t;

/*---------------------------------------------------------------------------*/
typedef struct __TSCH_DB_adv_entry {

  uint8_t in_use;

  uint8_t       addr_mode;
  ulpsmacaddr_t addr;

  rtimer_clock_t  recv_rtick;
  rtimer_clock_t  recv_clock_fine;
  clock_time_t    recv_clock;

  uint8_t  asn[TSCH_PIB_ASN_LEN];

} TSCH_DB_adv_entry_t;

/*---------------------------------------------------------------------------*/
typedef struct __TSCH_DB_channel_hopping {
  uint8_t  in_use;
  TSCH_PIB_channel_hopping_t vals;
} TSCH_DB_channel_hopping_t;

/*---------------------------------------------------------------------------*/
typedef struct __TSCH_DB_timeslot_template {
  TSCH_PIB_timeslot_template_t vals;
} TSCH_DB_timeslot_template_t;


struct __TSCH_DB_slotframe;

/*---------------------------------------------------------------------------*/
typedef struct __TSCH_DB_link {
  struct __TSCH_DB_link*        next;
  void*           qdpkt_list_list;
  list_t          qdpkt_list;
  uint16_t        channel;
  struct __TSCH_DB_slotframe*   slotframe;
  TSCH_PIB_link_t vals;
} TSCH_DB_link_t;

#define TSCH_DB_LINK_TYPE(link_ptr)         \
        ((link_ptr)->vals.link_type)
#define TSCH_DB_LINK_OPTIONS(link_ptr)      \
        ((link_ptr)->vals.link_option)
#define TSCH_DB_LINK_PEERADDR(link_ptr)     \
        ((link_ptr)->vals.peer_addr)
#define TSCH_DB_LINK_SLOTFRAME_ID(link_ptr) \
        ((link_ptr)->vals.slotframe_handle)
#define TSCH_DB_LINK_CHANNEL_OFFSET(link_ptr) \
        ((link_ptr)->vals.channel_offset)
#define TSCH_DB_LINK_TIMESLOT(link_ptr) \
        ((link_ptr)->vals.timeslot)
#define TSCH_DB_LINK_PERIOD_OFFSET(link_ptr)         \
        ((link_ptr)->vals.periodOffset)
#define TSCH_DB_LINK_KEEPALIVE(link_ptr)      \
        ((link_ptr)->keepalive)


#define TSCH_DB_LINK_CHANNEL(link_ptr) \
        ((link_ptr)->channel)


#define TSCH_DB_LINK_PKTQ_LIST_HEAD(link_ptr) \
        (list_head((link_ptr)->qdpkt_list))
#define TSCH_DB_LINK_PKTQ_LIST_PUSH(link_ptr, qdpkt) \
        (list_push((link_ptr)->qdpkt_list, qdpkt))
#define TSCH_DB_LINK_PKTQ_LIST_ADD(link_ptr, qdpkt) \
        (list_add((link_ptr)->qdpkt_list, qdpkt))
#define TSCH_DB_LINK_PKTQ_LIST_POP(link_ptr) \
        (list_pop((link_ptr)->qdpkt_list))

/*---------------------------------------------------------------------------*/
typedef struct __TSCH_DB_slotframe {

  uint8_t         in_use;
  uint8_t         number_of_links;

  void*           link_list_list;
  list_t          link_list;

  uint8_t         size_msb;

  TSCH_PIB_slotframe_t   vals;

} TSCH_DB_slotframe_t;

#define TSCH_DB_SLOTFRAME_SIZE_DIR(slotframe_ptr)      ((slotframe_ptr)->vals.slotframe_size)
#define TSCH_DB_SLOTFRAME_SIZE_MSB_DIR(slotframe_ptr)  ((slotframe_ptr)->size_msb)
#define TSCH_DB_SLOTFRAME_LINK_HEAD(slotframe_ptr)  (list_head((slotframe_ptr)->link_list))
#define TSCH_DB_SLOTFRAME_NEXT_LINK(slotframe_ptr, cur_link_ptr) \
        ( \
          (list_item_next(cur_link_ptr) == NULL) ? \
          list_head((slotframe_ptr)->link_list) : list_item_next(cur_link_ptr)\
        )


/*---------------------------------------------------------------------------*/
typedef struct __TSCH_DB_handle {

  uint8_t  mac_asn[TSCH_PIB_ASN_LEN];
  uint8_t  join_priority;
  uint16_t mac_disconnect_time;
  uint8_t  num_of_slotframes;
  uint8_t  num_tk_entries;

  uint8_t  is_pan_coord;

  volatile uint8_t  tsch_locked;

  volatile uint8_t  tsch_started;
  volatile uint8_t  tsch_sync_inited;

  TSCH_DB_channel_hopping_t   channel_hopping[TSCH_DB_MAX_CHANNEL_HOPPING];
  TSCH_DB_timeslot_template_t timeslot_template;

  TSCH_DB_slotframe_t         slotframes[TSCH_DB_MAX_SLOTFRAMES];

  TSCH_DB_ti_keepalive_t      keepalive;

  TSCH_DB_adv_entry_t         adv_entries[TSCH_DB_MAX_ADV_ENTRIES];
} TSCH_DB_handle_t;

/*---------------------------------------------------------------------------*/
#define TSCH_DB_IS_PAN_COORD(hnd_ptr)  \
        ((hnd_ptr)->is_pan_coord)

#define TSCH_DB_NUM_OF_SLOTFRAMES(hnd_ptr)    \
        ((hnd_ptr)->num_of_slotframes)

#define TSCH_DB_LOCKED(hnd_ptr) \
        ((hnd_ptr)->tsch_locked)
#define TSCH_DB_LOCK_GET(hnd_ptr) \
        ((hnd_ptr)->tsch_locked++)
#define TSCH_DB_LOCK_RELEASE(hnd_ptr) \
        ((hnd_ptr)->tsch_locked--)

#define TSCH_DB_TSCH_STARTED(hnd_ptr) \
        ((hnd_ptr)->tsch_started)
#define TSCH_DB_TSCH_SYNC_INITED(hnd_ptr) \
        ((hnd_ptr)->tsch_sync_inited)

static inline void
TSCH_DB_set_mac_asn(TSCH_DB_handle_t* hnd_ptr, uint64_t asn)
{
  TSCH_PIB_set_asn(hnd_ptr->mac_asn, asn);
}

static inline uint64_t
TSCH_DB_get_mac_asn(TSCH_DB_handle_t* hnd_ptr)
{
  return TSCH_PIB_get_asn(hnd_ptr->mac_asn);
}

static inline void
TSCH_DB_inc_mac_asn(TSCH_DB_handle_t* hnd_ptr)
{
  uint8_t* asn;

  asn = hnd_ptr->mac_asn;

  if(++asn[0] == 0x00) {
    if(++asn[1] == 0x00) {
      if(++asn[2] == 0x00) {
        if(++asn[3] == 0x00) {
          ++asn[4];
        }
      }
    }
  }
}

uint8_t
TSCH_DB_asn_div(uint64_t asn, uint16_t divisor, uint8_t divisor_msb, uint64_t* quo_out, uint16_t* rem_out);
uint8_t
TSCH_DB_asn_mod(uint64_t asn, uint16_t divisor, uint8_t divisor_msb, uint16_t* rem_out);

#define TSCH_DB_CHHOP_SEQ_LEN(hnd_ptr, id) \
        ((hnd_ptr)->channel_hopping[(id)].vals.hopping_sequence_length)
#define TSCH_DB_CHHOP_SEQ_LEN_MSB(hnd_ptr, id) \
        ((hnd_ptr)->channel_hopping[(id)].seq_len_msb)
#define TSCH_DB_CHHOP_SEQ_LIST(hnd_ptr, id, list_index) \
        ((hnd_ptr)->channel_hopping[(id)].vals.hopping_sequence_list[list_index])

#define TSCH_DB_TIMESLOT_TS_LEN(hnd_ptr)           \
        ((hnd_ptr)->timeslot_template.vals.ts_timeslot_length)
#define TSCH_DB_TIMESLOT_TX_OFFSET(hnd_ptr)     \
        ((hnd_ptr)->timeslot_template.vals.ts_tx_offset)
#define TSCH_DB_TIMESLOT_RX_ACK_DELAY(hnd_ptr)  \
        ((hnd_ptr)->timeslot_template.vals.ts_rx_ack_delay)
#define TSCH_DB_TIMESLOT_TX_ACK_DELAY(hnd_ptr)  \
        ((hnd_ptr)->timeslot_template.vals.ts_tx_ack_delay)
#define TSCH_DB_TIMESLOT_ACK_WAIT(hnd_ptr)      \
        ((hnd_ptr)->timeslot_template.vals.ts_ack_wait)
#define TSCH_DB_TIMESLOT_RX_OFFSET(hnd_ptr)     \
        ((hnd_ptr)->timeslot_template.vals.ts_rx_offset)
#define TSCH_DB_TIMESLOT_RX_WAIT(hnd_ptr)       \
        ((hnd_ptr)->timeslot_template.vals.ts_rx_wait)
#define TSCH_DB_TIMESLOT_MAX_TX(hnd_ptr)       \
        ((hnd_ptr)->timeslot_template.vals.ts_max_tx)
#define TSCH_DB_TIMESLOT_MAX_ACK(hnd_ptr)       \
        ((hnd_ptr)->timeslot_template.vals.ts_max_ack)

#define TSCH_DB_SLOTFRAME(hnd_ptr, id)        \
        (&(hnd_ptr)->slotframes[(id)])
#define TSCH_DB_SLOTFRAME_IN_USE(hnd_ptr, id) \
        ((hnd_ptr)->slotframes[(id)].in_use)
#define TSCH_DB_SLOTFRAME_SIZE(hnd_ptr, id)   \
        ((hnd_ptr)->slotframes[(id)].vals.slotframe_size)
#define TSCH_DB_SLOTFRAME_SIZE_MSB(hnd_ptr, id)   \
        ((hnd_ptr)->slotframes[(id)].size_msb)

#define TSCH_DB_ADV_ENTRY(hnd_ptr, id)        \
        (&(hnd_ptr)->adv_entries[(id)])
#define TSCH_DB_ADV_ENTRY_IN_USE(hnd_ptr, id) \
        ((hnd_ptr)->adv_entries[(id)].in_use)
#define TSCH_DB_ADV_ENTRY_ADDR_MODE(hnd_ptr, id) \
        ((hnd_ptr)->adv_entries[(id)].addr_mode)
#define TSCH_DB_ADV_ENTRY_ADDR(hnd_ptr, id) \
        (&(hnd_ptr)->adv_entries[(id)].addr)
#define TSCH_DB_ADV_ENTRY_RECV_RTICK(hnd_ptr, id) \
        ((hnd_ptr)->adv_entries[(id)].recv_rtick)
#define TSCH_DB_ADV_ENTRY_RECV_CLOCK(hnd_ptr, id) \
        ((hnd_ptr)->adv_entries[(id)].recv_clock)
#define TSCH_DB_ADV_ENTRY_RECV_CLOCK_FINE(hnd_ptr, id) \
        ((hnd_ptr)->adv_entries[(id)].recv_clock_fine)

static inline void
TSCH_DB_adv_entry_asn_set(TSCH_DB_handle_t* hnd_ptr, uint8_t id, uint64_t asn)
{
  TSCH_PIB_set_asn(hnd_ptr->adv_entries[id].asn, asn);
}

static inline uint64_t
TSCH_DB_adv_entry_asn_get(TSCH_DB_handle_t* hnd_ptr, uint8_t id)
{
  return TSCH_PIB_get_asn(hnd_ptr->adv_entries[id].asn);
}

#define TSCH_DB_NUM_TK_ENTRIES(hnd_ptr) \
        ((hnd_ptr)->num_tk_entries)

/*---------------------------------------------------------------------------*/
TSCH_DB_handle_t*
TSCH_DB_init(void);
/*---------------------------------------------------------------------------*/
LOWMAC_status_e
TSCH_DB_set_pan_coord(void);
/*---------------------------------------------------------------------------*/
LOWMAC_status_e
TSCH_DB_get_db(uint16_t attrib_id, void* value, uint16_t* in_size);
/*---------------------------------------------------------------------------*/
LOWMAC_status_e
TSCH_DB_set_db(uint16_t attrib_id, void* value, uint16_t* in_size);
/*---------------------------------------------------------------------------*/
LOWMAC_status_e
TSCH_DB_delete_db(uint16_t attrib_id, void* in_val, uint16_t* in_size);
/*---------------------------------------------------------------------------*/
LOWMAC_status_e
TSCH_DB_modify_db(uint16_t attrib_id, void* in_val, uint16_t* in_size, LOWMAC_dbcb_t db_callback);
/*---------------------------------------------------------------------------*/
TSCH_DB_link_t*
TSCH_DB_find_match_link(TSCH_DB_slotframe_t* slotframe, uint64_t asn, uint16_t timeslot);
/*---------------------------------------------------------------------------*/
uint64_t
TSCH_DB_find_next_link(TSCH_DB_slotframe_t* slotframe, uint64_t curASN, 
                       uint8_t* nextLinkOption);
/*---------------------------------------------------------------------------*/
bool TSCH_DB_isKeepAliveParent(uint16_t ackDestAddr);
void del_link_entry(TSCH_DB_slotframe_t* sf_db, TSCH_DB_link_t* link_db);
int TSCH_DB_getSchedule(uint16_t startLinkIdx, uint8_t *buf, int buf_size);
#endif /* __TSCH_DB_H__ */
