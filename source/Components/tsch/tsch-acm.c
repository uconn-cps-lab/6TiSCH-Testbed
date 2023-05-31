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
 *  ====================== tsch-acm.c =============================================
 *  Timeslot Access Manager
 */

#include <prcm.h>
#include "pwrest/pwrest.h"
#include "hal_api.h"
#include "tsch_os_fun.h"
#include "lib/random.h"
#include "rtimer.h"

#include "uip_rpl_process.h"

#include "ulpsmac.h"
#include "ulpsmacbuf.h"
#include "mac_pib_pvt.h"
#include "tsch-acm.h"
#include "tsch-txm.h"
#include "tsch-rxm.h"
#include "nhldb-tsch-sched.h"
#include "mrxm.h"
#include "nhl_mgmt.h"

#include "mac_config.h"
#include "led.h"

#if !IS_ROOT
#include "nhldb-tsch-nonpc.h"
#endif

#if DMM_ENABLE
#include "dmm_scheduler.h"
#include "dmm_policy_blesp_wsnnode.h"
#endif //DMM_ENABLE
/*---------------------------------------------------------------------------*/
rtimer_clock_t rtc_timeslot_length = 0;
rtimer_clock_t rtc_tx_offset;
rtimer_clock_t rtc_rx_offset;
rtimer_clock_t rtc_rx_wait;
rtimer_clock_t rtc_rx_ack_delay;
rtimer_clock_t rtc_tx_ack_delay;
rtimer_clock_t rtc_ack_wait;
rtimer_clock_t rtc_max_tx;
rtimer_clock_t rtc_max_ack;

uint16_t us_actual_timeslot_length;

uint16_t cur_timeslot_id;
uint64_t nextBeaconPostTimeASN;

unsigned long tsch_next_slot_start_time;
unsigned long TIME_N_MS_TICKS = ULPSMAC_US_TO_TICKS(3000);  //3 msec to transmit the beacon

extern uint64_t NHLDB_TSCH_NONPC_default_adventry_asn(uint8_t index);
extern uint32_t NHLDB_TSCH_NONPC_default_adventry_timestamp(uint8_t index);
/*---------------------------------------------------------------------------*/
static TSCH_DB_handle_t* acm_db_hnd;
static struct rtimer acm_rt;

static volatile rtimer_clock_t acm_slotdelta = 0;
static volatile uint8_t acm_slotdelta_phase = TSCH_ACM_SLOTDELTA_INPHASE;

/*Receive channel for the next beacon*/
static uint8_t next_rx_bcn_channel;
/* Channel index*/
uint8_t rx_bcn_channel_idx;

/*---------------------------------------------------------------------------*/
typedef struct __sf_ts_offset {
    uint8_t in_use;
    uint8_t valid;
    uint16_t sf_size;
    uint16_t cur_offset;
} sf_ts_offset_t;

sf_ts_offset_t sf_ts_offsets[TSCH_DB_MAX_SLOTFRAMES];

/* BCN 0, SHS, 1, TX 2, RX 3*/
TSCH_hopping_info_t link_hopping_info[4];

uint8_t last_rx_bcn_channel;

extern TSCH_DB_handle_t* rxm_db_hnd;
extern BM_BUF_t* usm_buf_datarecv;
extern BM_BUF_t* usm_buf_ackrecv;
extern uint8_t rxRxmFlag;
extern uint8_t rxAckFlag;
extern uint8_t cur_scan_idx;

bool rxWakeUpAdjustedFlag = false;

#if REDUCE_TIMESLOT_LENGTH
#define RADIO_RX_EARLY_WAKEUP (700)
#else
#define RADIO_RX_EARLY_WAKEUP (240)
#endif

#define BUFF_SIZE       5
const uint8_t mac_hdr_buf[BUFF_SIZE]={0x41,0xCC,0x01,0xcd,0xab};

/*---------------------------------------------------------------------------*/
static void calc_channel_link(uint64_t cur_asn, TSCH_DB_link_t* cur_link)
{
    uint16_t chlist_index;
    uint16_t channel;
    const uint8_t default_chhop_id = 0;

    chlist_index = (cur_asn + TSCH_DB_LINK_CHANNEL_OFFSET(cur_link))
         % TSCH_DB_CHHOP_SEQ_LEN(acm_db_hnd, default_chhop_id);

    channel = TSCH_DB_CHHOP_SEQ_LIST(acm_db_hnd, default_chhop_id, chlist_index);

    if (TSCH_MACConfig.fixed_channel_num)
        channel = TSCH_MACConfig.fixed_channel_num;

    TSCH_DB_LINK_CHANNEL(cur_link) = channel;
}

static inline void init_ts_offset(void) {
    uint8_t iter;

    for (iter = 0; iter < TSCH_DB_MAX_SLOTFRAMES; iter++) {
        sf_ts_offsets[iter].in_use = LOWMAC_FALSE;
        sf_ts_offsets[iter].valid = LOWMAC_FALSE;
        sf_ts_offsets[iter].sf_size = 0;
        sf_ts_offsets[iter].cur_offset = 0;
    }
}

/*---------------------------------------------------------------------------*/
static inline void inc_ts_offset(uint16_t numTS) {
    uint8_t iter;

    for (iter = 0; iter < TSCH_DB_MAX_SLOTFRAMES; iter++) {

        sf_ts_offset_t* cur_ent;

        cur_ent = &sf_ts_offsets[iter];
        if ((cur_ent->in_use == LOWMAC_TRUE)
            && (cur_ent->valid == LOWMAC_TRUE)) {
                cur_ent->cur_offset +=numTS;
                cur_ent->cur_offset = cur_ent->cur_offset % cur_ent->sf_size;
            }
    }

}

/*---------------------------------------------------------------------------*/
static inline uint16_t get_ts_offset(uint8_t sf_idx) {
    return sf_ts_offsets[sf_idx].cur_offset;
}

/*---------------------------------------------------------------------------*/
static inline uint8_t check_ts_offset(uint8_t sf_idx) {
    sf_ts_offset_t* cur_ent;
    uint8_t res;

    cur_ent = &sf_ts_offsets[sf_idx];
    if ((TSCH_DB_SLOTFRAME_IN_USE(acm_db_hnd, sf_idx)!= cur_ent->in_use) ||
        (TSCH_DB_SLOTFRAME_SIZE(acm_db_hnd, sf_idx) != cur_ent->sf_size) ) {

            cur_ent->in_use = TSCH_DB_SLOTFRAME_IN_USE(acm_db_hnd, sf_idx);
            if(cur_ent->in_use == LOWMAC_TRUE) {

                if(cur_ent->sf_size != TSCH_DB_SLOTFRAME_SIZE(acm_db_hnd, sf_idx)) {
                    cur_ent->valid = LOWMAC_FALSE;
                }
            }
        }

    res = LOWMAC_FALSE;
    if ((cur_ent->in_use == LOWMAC_TRUE) && (cur_ent->valid == LOWMAC_TRUE)) {
        res = LOWMAC_TRUE;
    }

    return res;
}

/*---------------------------------------------------------------------------*/
static inline void update_ts_offset(uint64_t cur_asn) {
    uint8_t iter;
    sf_ts_offset_t* cur_ent;

    for (iter = 0; iter < TSCH_DB_MAX_SLOTFRAMES; iter++) {
        cur_ent = &sf_ts_offsets[iter];

        if ((cur_ent->in_use == LOWMAC_TRUE)
            && (cur_ent->valid == LOWMAC_FALSE)) {
                cur_ent->sf_size = TSCH_DB_SLOTFRAME_SIZE(acm_db_hnd, iter);
                TSCH_DB_asn_mod(cur_asn, cur_ent->sf_size, TSCH_DB_SLOTFRAME_SIZE_MSB(acm_db_hnd, iter),
                                &cur_ent->cur_offset);

                cur_ent->valid = LOWMAC_TRUE;
            }
    }

}

#if !IS_ROOT
/**************************************************************************************************//**
 *
 * @brief       This function is to prepare the leaf  (intermediate) node to disconnect from
 *              PAN network. It frees all packets in MAC TX queue and put TSCH ACM into
 *              not-start and not-init state
 *
 *
 * @param[in]
 *
 * @return
 *
 ***************************************************************************************************/

void TSCH_prepareDisConnect(void)
{
    // free all packets in MAC TX queue
    TSCH_TXM_freePkts();

    // free all allocated links

    // stop ACM process, (not start, not init)
    TSCH_DB_TSCH_SYNC_INITED(acm_db_hnd) =LOWMAC_FALSE;
    TSCH_DB_TSCH_STARTED(acm_db_hnd) = LOWMAC_FALSE;

}
#endif

/**************************************************************************************************//**
 *
 * @brief       This function is to process TX link
 *
 *
 * @param[in]   slot_start -- slot start time stamp
 *              cur_san -- ASN of current slot
 *              cur_sf  -- pointer of current slot frame
 *              cur_link -- pointer of current TX link
 *
 * @return     status of operation
 *
 ***************************************************************************************************/
static inline uint8_t process_link_tx(rtimer_clock_t slot_start,
                                      uint64_t cur_asn, TSCH_DB_slotframe_t* cur_sf, TSCH_DB_link_t* cur_link)
{

    if (TSCH_TXM_can_send(cur_link, TSCH_DB_LINK_OPTIONS_TX) != LOWMAC_TRUE) {
        PWREST_updateRFSlotCount(PWREST_TX_NO_PACKET);
        return LOWMAC_FALSE;
    }
    
    PWREST_updateRFSlotCount(PWREST_TX_SLOT);

    calc_channel_link(cur_asn, cur_link);

    if ((TSCH_DB_LINK_TYPE(cur_link) == TSCH_DB_LINK_TYPE_ADV) && (TSCH_MACConfig.bcn_ch_mode != 0xff) )
    {
        TSCH_DB_LINK_CHANNEL(cur_link) = next_tx_bcn_channel;
    }

    if ((TSCH_DB_LINK_TYPE(cur_link) == TSCH_DB_LINK_TYPE_ADV)) {
        link_hopping_info[0].channel = TSCH_DB_LINK_CHANNEL(cur_link);
        link_hopping_info[0].timeslot = get_ts_offset(0);
        link_hopping_info[0].asn = cur_asn;
    } else if ((TSCH_DB_LINK_TYPE(cur_link) == TSCH_PIB_LINK_TYPE_NORMAL)) {
        link_hopping_info[2].channel = TSCH_DB_LINK_CHANNEL(cur_link);
        link_hopping_info[2].timeslot = get_ts_offset(0);
        link_hopping_info[2].asn = cur_asn;
    }

    TSCH_TXM_timeslot(cur_link, slot_start, cur_asn);

    return LOWMAC_TRUE;
}

/**************************************************************************************************//**
 *
 * @brief       This function is to process RX link
 *
 *
 * @param[in]   slot_start -- slot start time stamp
 *              cur_san -- ASN of current slot
 *              cur_sf  -- pointer of current slot frame
 *              cur_link -- pointer of current RX link
 *
 * @return     status of operation
 *
 ***************************************************************************************************/
static inline uint8_t process_link_rx(rtimer_clock_t slot_start,
                                      uint64_t cur_asn, TSCH_DB_slotframe_t* cur_sf, TSCH_DB_link_t* cur_link)
{
    PWREST_updateRFSlotCount(PWREST_RX_SLOT);
    
    calc_channel_link(cur_asn, cur_link);

    if ((TSCH_DB_LINK_TYPE(cur_link) == TSCH_DB_LINK_TYPE_ADV) && (TSCH_MACConfig.bcn_ch_mode != 0xff) )
    {
        TSCH_DB_LINK_CHANNEL(cur_link) = next_rx_bcn_channel;
    }

    link_hopping_info[3].channel = TSCH_DB_LINK_CHANNEL(cur_link);
    link_hopping_info[3].timeslot = get_ts_offset(0);
    link_hopping_info[3].asn = cur_asn;

    TSCH_RXM_timeslot(cur_link, slot_start);

    return LOWMAC_TRUE;
}

/**************************************************************************************************//**
 *
 * @brief       This function is to process Shared  link
 *
 *
 * @param[in]   slot_start -- slot start time stamp
 *              cur_san -- ASN of current slot
 *              cur_sf  -- pointer of current slot frame
 *              cur_link -- pointer of current shared link
 *
 * @return     status of operation
 *
 ***************************************************************************************************/
static inline uint8_t process_link_sh(rtimer_clock_t slot_start,
                                      uint64_t cur_asn, TSCH_DB_slotframe_t* cur_sf, TSCH_DB_link_t* cur_link)
{
  calc_channel_link(cur_asn, cur_link);
  
  link_hopping_info[1].channel = TSCH_DB_LINK_CHANNEL(cur_link);
  link_hopping_info[1].timeslot = get_ts_offset(0);
  link_hopping_info[1].asn = cur_asn;
  
  if (TSCH_TXM_can_send(cur_link, TSCH_DB_LINK_OPTIONS_SH) == LOWMAC_TRUE) 
  {
//#if REDUCE_TIMESLOT_LENGTH      
#if !ENABLE_HCT
    uint32_t timeout, curTime;
    curTime = RTIMER_NOW();
    timeout = ULPSMAC_US_TO_TICKS(RADIO_RX_EARLY_WAKEUP);
    eventWait(ACM_EVENT_DUMMY_EVENT,timeout,curTime);
#endif
    
    PWREST_updateRFSlotCount(PWREST_TX_SLOT);
    TSCH_TXM_timeslot(cur_link, slot_start, cur_asn);
  } else {
    PWREST_updateRFSlotCount(PWREST_RX_SLOT);
    TSCH_RXM_timeslot(cur_link, slot_start);
  }
  
  return LOWMAC_TRUE;
}

/**************************************************************************************************//**
 *
 * @brief       This function is to compute the next timeslot start time
 *
 *
 * @param[in]   slot_start -- slot start time stamp of current time slot
 *
 *
 * @return     start time of next timeslot
 *
 ***************************************************************************************************/
static rtimer_clock_t next_timeslot_start(rtimer_clock_t slot_start, 
                                uint64_t next_asn,uint8_t next_link_option) 
{  
  rtimer_clock_t cur_rtimer;
  uint64_t cur_asn;
  
  /* Calculate time of our next slot */
  slot_start += (rtc_timeslot_length);
  
  if ((acm_slotdelta_phase != TSCH_ACM_SLOTDELTA_INPHASE) 
      && (acm_slotdelta != 0)) 
  {
    rtimer_clock_t adjustments;
    
    if (TSCH_DB_NUM_TK_ENTRIES(acm_db_hnd) > 0) 
    {
      adjustments = acm_slotdelta / TSCH_DB_NUM_TK_ENTRIES(acm_db_hnd);
    } else {
      adjustments = acm_slotdelta;
    }
    
    if (acm_slotdelta_phase == TSCH_ACM_SLOTDELTA_IAM_EARLY) 
    {
      slot_start += adjustments; 
    } else if (acm_slotdelta_phase == TSCH_ACM_SLOTDELTA_IAM_LATE) 
    {
      slot_start -= adjustments;
    }
    
    acm_slotdelta_phase = TSCH_ACM_SLOTDELTA_INPHASE;
    acm_slotdelta = 0;
  }
  
   cur_asn = TSCH_DB_get_mac_asn(acm_db_hnd);
  
#if TSCH_SKIP_TS_WO_LINK
  if (next_asn != 0)
  {
    uint64_t asn_diff;
    
    if (next_asn > nextBeaconPostTimeASN)
    {
      next_asn = nextBeaconPostTimeASN;
    }
    
    asn_diff = next_asn - cur_asn;
    
    if (asn_diff > 1)
    {
      cur_asn += (asn_diff-1);
      TSCH_DB_set_mac_asn(acm_db_hnd, cur_asn);
      inc_ts_offset((asn_diff-1));
      slot_start += ((asn_diff-1)*rtc_timeslot_length);
    }
  }
#endif
  
#if !ENABLE_HCT
  if ((next_link_option & TSCH_DB_LINK_OPTIONS_RX) == TSCH_DB_LINK_OPTIONS_RX)
  {
    slot_start = slot_start - ULPSMAC_US_TO_TICKS(RADIO_RX_EARLY_WAKEUP);
    rxWakeUpAdjustedFlag = true;
  }
#endif
  
  // Add some delay to current time in case wakeup time is too close
  cur_rtimer= RTIMER_NOW()+6;
  if (RTIMER_CLOCK_LT(slot_start, cur_rtimer))
  {
    uint32_t elap_rtick;
    uint64_t elap_num_timeslots;
    
    if(cur_rtimer  >= slot_start)
      elap_rtick = cur_rtimer  - slot_start;
    else
      elap_rtick = (unsigned int)(0xFFFFFFFF)-slot_start+cur_rtimer +1;
    
    elap_num_timeslots = (elap_rtick / rtc_timeslot_length)+1;
    
    //update ASN
    cur_asn += elap_num_timeslots;
    TSCH_DB_set_mac_asn(acm_db_hnd, cur_asn);
    inc_ts_offset(elap_num_timeslots);
    slot_start += (elap_num_timeslots*rtc_timeslot_length);
  }
  
//#if REDUCE_TIMESLOT_LENGTH
//  if (((next_link_option & TSCH_DB_LINK_OPTIONS_RX) == TSCH_DB_LINK_OPTIONS_RX) &&
//      ((slot_start - RTIMER_NOW()) > HAL_RADIO_RAMPUP_DELAY_TICKS))
//  {
//      slot_start -= ULPSMAC_US_TO_TICKS(RADIO_RX_EARLY_WAKEUP);
//      wakeUpAdjustedFlag = true;
//      HAL_RADIO_allowStandby();
//  }
//#endif
  
  return slot_start;
}

/*---------------------------------------------------------------------------*/

uint8_t timeslot_cb(struct rtimer* rt_ptr, void* ptr) {
    SN_post(timeslot_sem, NULL,  NULL, NOTIF_TYPE_SEMAPHORE);
    return 0;
}

void TSCH_startupMsg(void)
{
    uint8_t *pdata,len;
    BM_BUF_t* usm_buf;

    usm_buf = BM_alloc(19);
    pdata = BM_getDataPtr(usm_buf);
    // copy the fake MAC FC
    memcpy(pdata,mac_hdr_buf,BUFF_SIZE);
    pdata += BUFF_SIZE;
    // copy the Config Sender Addr (destination)
    memcpy(pdata,&config_SenderAddr,8);
    pdata += 8;
    // copy device EUI (SRC in the FC)
    memcpy(pdata,&ulpsmacaddr_long_node, 8);
    pdata +=8;
    len = pdata - BM_getDataPtr(usm_buf);
    BM_setDataLen(usm_buf,len);
    pdata = BM_getDataPtr(usm_buf);

    HAL_RADIO_setChannel(TSCH_MACConfig.bcn_chan[0]);
    HAL_RADIO_transmit(pdata,len, RF_getCurrentTime() + 5000*4);

    BM_free_6tisch(usm_buf);
}

/**************************************************************************************************//**
*
* @brief       Ths function controls the timeslot access of TSCH
*
* @param[in]   none
*
* @return      none
*
***************************************************************************************************/
Task_Handle time_slot_task_handle;
void timeslot_task(void)
{
  rtimer_clock_t  slot_start;   
  uint8_t sf_iter, processed;
  uint64_t cur_asn;
  TSCH_DB_slotframe_t* cur_sf;
  TSCH_DB_link_t* cur_link;
  uint32_t beaconPeriodInTimeSlot;

  uint64_t next_asn;
  uint8_t next_link_option;
  
  static uint32_t timeslot_timeout_cticks; 
  bool pend_status;
  
  time_slot_task_handle = Task_self();  //???
  
  rtimer_init();
#if DMM_ENABLE
  DMM_BleOperations((CONCURRENT_STACKS ? 0 : BLE_OPS_BEACONING) | (BLE_LOCALIZATION_BEFORE_TISCH ? BLE_OPS_SCANNING : 0));
#endif //DMM_ENABLE

  //power reset clock manager
  PRCMPeripheralRunEnable(PRCM_PERIPH_TRNG);
  PRCMLoadSet();
  while(!PRCMLoadGet()){}
  random_init(0);
  
  BM_init();
  TSCH_PIB_init();
  
#ifdef FEATURE_MAC_SECURITY
  MAC_SecurityInit();
#endif
  
  NHL_getPIB(TSCH_MAC_PIB_ID_DEV_EUI,ulpsmacaddr_long_node.u8);
  ulpsmacaddr_short_node = 0;
  
#if DMM_ENABLE && CONCURRENT_STACKS
  DMMPolicy_updateStackState(DMMPolicy_StackType_WsnNode, DMMPOLICY_STACKSTATE_WSNNODE_TX);
#endif //DMM_ENABLE
#if CC13XX_DEVICES
  HAL_RADIO_init(HAL_FreqBand_915MHZ, HAL_PhyMode_PRO50kbpsFSK);
#elif CC26XX_DEVICES
  HAL_RADIO_init(HAL_FreqBand_2400MHZ, HAL_PhyMode_IEEE250kbpsDSSS);
#endif
  
  //HAL_RADIO_setTxPower(0);
  
  TSCH_startupMsg();
#if DMM_ENABLE && CONCURRENT_STACKS
  DMMPolicy_updateStackState(DMMPolicy_StackType_WsnNode, DMMPOLICY_STACKSTATE_WSNNODE_SLEEPING);
#endif //DMM_ENABLE
  timeslot_timeout_cticks = BIOS_WAIT_FOREVER;
  while (1)
  {
#if DMM_ENABLE && CONCURRENT_STACKS
    if (restartBleBeaconReq)
    {
        restartBleBeaconReq = 0;
        RemoteDisplay_postSlowBleBeaconEvent();
    }

    if (bleReqRadioAccess)
    {
        bleReqRadioAccess = 0;
        Semaphore_post(sema_DmmCanSendBleRfCmd);
    }
#endif //DMM_ENABLE
    pend_status = SN_pend(timeslot_sem, NULL,timeslot_timeout_cticks, 
                          NOTIF_TYPE_SEMAPHORE);    
    eventFree();
#if DISABLE_TISCH_WHEN_BLE_CONNECTED && DMM_ENABLE && CONCURRENT_STACKS
    if (bleConnected)
    {
        continue;
    }
#endif //DMM_ENABLE && !CONCURRENT_STACKS
    next_asn= 0;
    next_link_option = 0;
 
    if (TSCH_DB_TSCH_STARTED(acm_db_hnd) == LOWMAC_FALSE)
    {   
      continue;
    }
    
    TSCH_DB_inc_mac_asn(acm_db_hnd);
    inc_ts_offset(1);
    
    cur_asn = TSCH_DB_get_mac_asn(acm_db_hnd);
    
    beaconPeriodInTimeSlot = (uint32_t)TSCH_MACConfig.beacon_period_sf
      *(uint32_t)TSCH_DB_SLOTFRAME_SIZE(acm_db_hnd, 0);
    nextBeaconPostTimeASN = beaconPeriodInTimeSlot
      *(cur_asn/beaconPeriodInTimeSlot+1)-1;
    
    if (cur_asn == nextBeaconPostTimeASN)
    {
#if !DTLS_LEDS_ON
      HAL_LED_set(1,1);
#endif
    }
    
    if (cur_asn == (TSCH_DB_SLOTFRAME_SIZE(acm_db_hnd, 0)
                    *TSCH_MACConfig.active_share_slot_time+1))
    {
      if (MlmeCb)
      {
        tschMlmeShareSlotModify_t cb;
        cb.hdr.event =  MAC_MLME_SHARE_SLOT_MODIFY;
        MlmeCb((tschCbackEvent_t *)(&cb),NULL);
      }
    }
    
    slot_start = RTIMER_TIME(&acm_rt);

//#if REDUCE_TIMESLOT_LENGTH
//    if (wakeUpAdjustedFlag == true)
//    {
//      slot_start =  slot_start + ULPSMAC_US_TO_TICKS(RADIO_RX_EARLY_WAKEUP);
//      wakeUpAdjustedFlag = false;
//    }
//#endif
   
#if !ENABLE_HCT
    if (rxWakeUpAdjustedFlag == true)
    {
      slot_start =  slot_start + ULPSMAC_US_TO_TICKS(RADIO_RX_EARLY_WAKEUP);
      rxWakeUpAdjustedFlag = false;
    }
#endif

    if (pend_status == true)
    {
      if (!TSCH_DB_LOCKED(acm_db_hnd))
      {
        for (sf_iter = 0; sf_iter < TSCH_DB_MAX_SLOTFRAMES; sf_iter++)
        {
          if (check_ts_offset(sf_iter) == LOWMAC_FALSE)
          {
            continue;
          }
          
          cur_timeslot_id = get_ts_offset(sf_iter);
          cur_sf = TSCH_DB_SLOTFRAME(acm_db_hnd, sf_iter);
          cur_link = TSCH_DB_find_match_link(cur_sf,cur_asn, cur_timeslot_id);
          
#if TSCH_SKIP_TS_WO_LINK
          next_asn = TSCH_DB_find_next_link(cur_sf,cur_asn,&next_link_option);
#endif
          
          if (!cur_link)
          {
            continue;
          }
                              
          if (((TSCH_DB_LINK_TYPE(cur_link) == TSCH_DB_LINK_TYPE_ADV)) &&
              ((cur_asn-cur_timeslot_id-TSCH_DB_LINK_PERIOD_OFFSET(cur_link)
                * TSCH_DB_SLOTFRAME_SIZE_DIR(cur_sf))
               % beaconPeriodInTimeSlot!= 0))
          {
            continue;
          }
          
          processed = NHL_FALSE;
          switch (TSCH_DB_LINK_OPTIONS(cur_link) & TSCH_DB_LINK_OPTIONS_TRX_MASK)
          {
          case TSCH_DB_LINK_OPTIONS_TX:
            processed = process_link_tx(slot_start, cur_asn, cur_sf,cur_link);
            break;
          case (TSCH_DB_LINK_OPTIONS_RX) :
            if ((TSCH_MACConfig.beaconControl & BC_IGNORE_RX_BEACON_MASK) && (TSCH_DB_LINK_TYPE(cur_link) == TSCH_DB_LINK_TYPE_ADV))
            {
              processed = NHL_TRUE;
              del_link_entry(cur_sf, cur_link);
            }
            else
            {
              processed = process_link_rx(slot_start, cur_asn, cur_sf,cur_link);
            }
            break;
          case (TSCH_DB_LINK_OPTIONS_SH | TSCH_DB_LINK_OPTIONS_RX):
            processed = process_link_sh(slot_start, cur_asn, cur_sf,cur_link);
            break;
            
          default:
            break;
          }
          
          if (processed == NHL_TRUE)
          {
            break;
          }
        } // for each slotframe
      }
    }
    else
    {
       ULPSMAC_Dbg.numTimerMiss++;
    }
    
    update_ts_offset(cur_asn);
    slot_start = next_timeslot_start(slot_start,next_asn,next_link_option);
    
    ULPSMAC_Dbg.curTimeSlotEndTime = RTIMER_NOW();
    ULPSMAC_Dbg.nextTimeSlotStartTime = slot_start;
    tsch_next_slot_start_time = slot_start;

    rtimer_set(&acm_rt, slot_start, 1, (void (*)(struct rtimer *, void *)) timeslot_cb, NULL);
        
    // Convert to ms first to avoid possible overflow
    uint32_t timeslot_timeout_ms = (uint32_t)((uint64_t)(slot_start - RTIMER_NOW())*1000)/RTIMER_SECOND;
    timeslot_timeout_cticks = (uint32_t)((uint64_t)(timeslot_timeout_ms*CLOCK_SECOND)/1000);
    // Add 1ms delay to timeout 
    timeslot_timeout_cticks += CLOCK_SECOND/1000;
    
    if (cur_asn == nextBeaconPostTimeASN)
    {
      if (beacon_timer_started == ULPSMAC_TRUE)
      {
        if (MlmeCb)
        {
          tschMlmeBeaconRequest_t bcnReq;
          bcnReq.hdr.event =  MAC_MLME_BEACON_REQUEST;
          MlmeCb((tschCbackEvent_t *)(&bcnReq),NULL);
        }
      }
      
      if (TSCH_MACConfig.bcn_ch_mode != 0xff)
      {
        rx_bcn_channel_idx = (rx_bcn_channel_idx+1)%(TSCH_MACConfig.bcn_ch_mode);
        next_rx_bcn_channel = TSCH_MACConfig.bcn_chan[rx_bcn_channel_idx];
      }
#if !DTLS_LEDS_ON
      HAL_LED_set(1,0);
#endif
    }
  }
}

/**************************************************************************************************//**
 *
 * @brief       This function is to init TSCH ACM module
 *
 *
 * @param[in]   db_hnd -- pointer of TSCH_DB_handle_t
 *
 *
 * @return     status of operation
 *
 ***************************************************************************************************/
LOWMAC_status_e TSCH_ACM_init(TSCH_DB_handle_t* db_hnd) {

    acm_db_hnd = db_hnd;

    init_ts_offset();
    return LOWMAC_STAT_SUCCESS;
}

/**************************************************************************************************//**
 *
 * @brief       This function is to read time slot IE from TSCH DB. The read values are stored in
 *              global variables. It will be called for each time slot
 *
 *
 * @param[in]
 *
 *
 * @return
 *
 ***************************************************************************************************/
void TSCH_ACM_readTimeSlotIE(void)
{
    TSCH_PIB_timeslot_template_t timeslotIE;
    uint16_t ie_size;

    TSCH_DB_get_db(TSCH_DB_ATTRIB_TIMESLOT, &timeslotIE, &ie_size);
    rtc_timeslot_length =  ULPSMAC_US_TO_TICKS(timeslotIE.ts_timeslot_length);
    rtc_tx_offset = ULPSMAC_US_TO_TICKS(timeslotIE.ts_tx_offset);
    rtc_rx_offset = ULPSMAC_US_TO_TICKS(timeslotIE.ts_rx_offset);
    rtc_rx_wait = ULPSMAC_US_TO_TICKS(timeslotIE.ts_rx_wait);
    rtc_rx_ack_delay = ULPSMAC_US_TO_TICKS(timeslotIE.ts_rx_ack_delay);
    rtc_tx_ack_delay = ULPSMAC_US_TO_TICKS(timeslotIE.ts_tx_ack_delay);
    rtc_ack_wait = ULPSMAC_US_TO_TICKS(timeslotIE.ts_ack_wait);
    rtc_max_tx = ULPSMAC_US_TO_TICKS(timeslotIE.ts_max_tx);
    rtc_max_ack = ULPSMAC_US_TO_TICKS(timeslotIE.ts_max_ack);

    us_actual_timeslot_length = timeslotIE.ts_timeslot_length;
}
/**************************************************************************************************//**
 *
 * @brief       This function is to init TSCH ACM sync
 *
 *
 * @param[in]   recv_rtick -- received rtick value
 *              recv_clock -- received time stamp of packet
 *              recv_clock_fine -- fine resolution of rtick
 *              asn_given   : ASN value
 *
 * @return     status of operation
 *
 ***************************************************************************************************/
static LOWMAC_status_e init_sync(rtimer_clock_t recv_rtick,
                                 clock_time_t recv_clock, rtimer_clock_t recv_clock_fine,
                                 uint64_t asn_given)
{
    rtimer_clock_t slot_start;

    /* Only one synchronization allowed */
    if (TSCH_DB_TSCH_SYNC_INITED(acm_db_hnd)) {
        return LOWMAC_STAT_SUCCESS;
    }

    TSCH_DB_TSCH_SYNC_INITED(acm_db_hnd) = LOWMAC_TRUE;

    // Assumed that at the time of sync init, timeslot_template has set by NHL
    // But, we provide default timeslot_template for the case where NHL wants
    // to use default one without modification to that.

    TSCH_ACM_readTimeSlotIE();

    if (TSCH_DB_IS_PAN_COORD(acm_db_hnd)) {

        // start as soon as possible
        //Log_info1("recv_tick %d",recv_rtick);
        slot_start = recv_rtick + (rtc_timeslot_length / 2);

        // but, give 16 more slots for possible delay of here
        asn_given += 16;
        slot_start += (rtc_timeslot_length << 4);
        //Log_info1("slot_start %d",slot_start);

    } else
    {
        rtimer_clock_t now_clock;

        uint32_t elap_rtick;
        uint32_t abs_rtick;
        uint32_t elap_num_timeslots;
        uint8_t elap_num_slotframes;

        // check all current time
        now_clock = RTIMER_NOW();

        // convert packet reception time to timeslot start time
        recv_clock -= rtc_tx_offset;

        // calculate elapsed rticks after start of timeslot
        elap_rtick = now_clock - recv_clock;

        // find start time of current slot
        elap_num_timeslots = (elap_rtick / rtc_timeslot_length);
        abs_rtick = ((uint32_t) recv_clock)
            + (elap_num_timeslots * rtc_timeslot_length);

        asn_given += elap_num_timeslots;
        slot_start = (rtimer_clock_t) abs_rtick;

        // give 16 more slots for possible delay of here
        asn_given += 16;
        slot_start += (rtc_timeslot_length << 4)-10;

        elap_num_slotframes = (elap_num_timeslots+16)/TSCH_DB_SLOTFRAME_SIZE_MSB(acm_db_hnd, 0);
        if (TSCH_MACConfig.bcn_ch_mode != 0xff)
        {
            rx_bcn_channel_idx = (rx_bcn_channel_idx + elap_num_slotframes/TSCH_MACConfig.beacon_period_sf)% (TSCH_MACConfig.bcn_ch_mode);
            next_rx_bcn_channel =TSCH_MACConfig.bcn_chan[rx_bcn_channel_idx];
        }
    }

    // timeslot_cb will start with increase of asn, so reduce by one over here
    asn_given -= 1;

    TSCH_DB_set_mac_asn(acm_db_hnd, asn_given);
    rtimer_set(&acm_rt, slot_start, 1, (void (*)(struct rtimer *, void *)) timeslot_cb, NULL);

    return LOWMAC_STAT_SUCCESS;
}

/**************************************************************************************************//**
 *
 * @brief       This function is to start TSCH ACM module
 *
 *
 * @param[in]
 *
 * @return     status of operation
 *
 ***************************************************************************************************/
LOWMAC_status_e TSCH_ACM_start(void) {
    if (TSCH_DB_NUM_OF_SLOTFRAMES(acm_db_hnd) == 0) {
        return LOWMAC_STAT_NO_SYNC;
    }

#if IS_ROOT
        clock_time_t clock;
        clock_time_t now_clock;
        rtimer_clock_t now_clock_fine;
        uint16_t interval;

        interval = RTIMER_SECOND / CLOCK_SECOND;
        clock = Timestamp_get32();
        now_clock = clock / interval;
        now_clock_fine = (unsigned short)(clock%interval);
        init_sync(RTIMER_NOW(), now_clock, now_clock_fine, 0);
#else
       // non pan coordinator
        uint8_t iter_given;
        uint64_t asn_given;
        uint32_t timestamp;


        iter_given = NHLDB_TSCH_NONPC_default_adventry_index();
        
        asn_given = NHLDB_TSCH_NONPC_default_adventry_asn(iter_given);
        timestamp = NHLDB_TSCH_NONPC_default_adventry_timestamp(iter_given);
        init_sync(0,timestamp,0,asn_given);
#endif

    TSCH_DB_TSCH_STARTED(acm_db_hnd) = LOWMAC_TRUE;

    return LOWMAC_STAT_SUCCESS;
}

/**************************************************************************************************//**
 *
 * @brief       This function is to stop TSCH ACM module
 *
 *
 * @param[in]
 *
 * @return     status of operation
 *
 ***************************************************************************************************/
LOWMAC_status_e TSCH_ACM_stop(void) {

    // stop timer
    TSCH_DB_TSCH_STARTED(acm_db_hnd) = LOWMAC_FALSE;
    TSCH_DB_TSCH_SYNC_INITED(acm_db_hnd) = LOWMAC_FALSE;

    return LOWMAC_STAT_SUCCESS;
}

/**************************************************************************************************//**
 *
 * @brief       This function is to setup beacon sync info when the beacon is received in
 *              channel scan
 *
 *
 * @param[in]   usm_buf -- pointer of BM buffeer
 *              mpi_recv_len  -- received MAC packet length
 *              cur_asn -- current ASN
 *
 * @return
 *
 ***************************************************************************************************/
void TSCH_ACM_set_adv_syncinfo(BM_BUF_t* usm_buf,
                               uint16_t mpi_recv_len, uint64_t cur_asn)
{

    rtimer_clock_t adv_recv_clock;
    rtimer_clock_t pkt_dur_rt;

    uint8_t iter;
    uint8_t found;
    uint8_t recv_addr_mode;
    const ulpsmacaddr_t* recv_addr;

    pkt_dur_rt = HAL_RADIO_DURATION_TICKS(mpi_recv_len) + HAL_RADIO_RX_CB_DELAY_TICKS;

    adv_recv_clock = MRXM_getPktTimestamp();

    adv_recv_clock -= pkt_dur_rt;

    /* Find a proper advertiser entry */

    recv_addr_mode = MRXM_getPktSrcAddrMode();
    recv_addr = MRXM_getPktSrcAddr();

    found = 0;
    for (iter = 0; iter < TSCH_DB_MAX_ADV_ENTRIES; iter++) {

        if (TSCH_DB_ADV_ENTRY_IN_USE(acm_db_hnd, iter)== LOWMAC_TRUE) {
            if(TSCH_DB_ADV_ENTRY_ADDR_MODE(acm_db_hnd, iter) == recv_addr_mode) {
                if(recv_addr_mode == ULPSMACADDR_MODE_LONG) {
                    found = ulpsmacaddr_long_cmp(recv_addr, TSCH_DB_ADV_ENTRY_ADDR(acm_db_hnd, iter));
                } else if(recv_addr_mode == ULPSMACADDR_MODE_SHORT) {
                    found = ulpsmacaddr_short_cmp(recv_addr, TSCH_DB_ADV_ENTRY_ADDR(acm_db_hnd, iter));
                }
            }
        }

        if(found) {
            break;
        }
    }

    if (!found) {
        for (iter = 0; iter < TSCH_DB_MAX_ADV_ENTRIES; iter++) {
            if (TSCH_DB_ADV_ENTRY_IN_USE(acm_db_hnd, iter)== LOWMAC_FALSE) {
                found =1;
                TSCH_DB_ADV_ENTRY_IN_USE(acm_db_hnd, iter) = LOWMAC_TRUE;
                break;
            }
        }
    }

    if (found) {
        TSCH_DB_ADV_ENTRY_ADDR_MODE(acm_db_hnd, iter)= recv_addr_mode;

        if(recv_addr_mode == ULPSMACADDR_MODE_SHORT) {
            ulpsmacaddr_short_copy(TSCH_DB_ADV_ENTRY_ADDR(acm_db_hnd, iter), recv_addr);
        } else if(recv_addr_mode == ULPSMACADDR_MODE_LONG) {
            ulpsmacaddr_long_copy(TSCH_DB_ADV_ENTRY_ADDR(acm_db_hnd, iter), recv_addr);
        }

        TSCH_DB_ADV_ENTRY_RECV_RTICK(acm_db_hnd, iter) = 0;
        TSCH_DB_ADV_ENTRY_RECV_CLOCK(acm_db_hnd, iter) = adv_recv_clock;
        TSCH_DB_ADV_ENTRY_RECV_CLOCK_FINE(acm_db_hnd, iter) = 0;
        TSCH_DB_adv_entry_asn_set(acm_db_hnd, iter, cur_asn);
    }
}

/*---------------------------------------------------------------------------*/
void TSCH_ACM_set_slotdelta(const rtimer_clock_t slotdelta_ticks, const uint8_t slotdelta_phase) {
    acm_slotdelta = slotdelta_ticks;
    acm_slotdelta_phase = slotdelta_phase;
}


uint16_t TSCH_ACM_timeslotLenMS(void){

    return (TSCH_DB_TIMESLOT_TS_LEN(acm_db_hnd)/1000);
}

uint64_t TSCH_ACM_get_ASN(){
    return TSCH_DB_get_mac_asn(acm_db_hnd);
}

void TSCH_rxISR(RF_EventMask rfevent, uint8_t* pRxPacket)
{
    volatile uint8_t len;
    uint8_t fcf;
    uint8_t *dataPtr;
    BM_BUF_t* pTempBuf, *pDedBuf;
    rtimer_clock_t curTime;
    signed char rssi;
    uint8_t correlation;

    if(rfevent& RF_EventMdmSoft)
    {
        eventPost(ACM_EVENT_RX_SYNC);
    }
    else if(rfevent & RF_EventRxOk)
    {
        curTime = RTIMER_NOW();

        len = *pRxPacket;

        if (len == 0)
        {
            return;
        }

        pTempBuf = BM_alloc(15);
        if (pTempBuf == NULL)
        {
            return ;
        }

#if PHY_PROP_50KBPS
        memcpy(BM_getDataPtr(pTempBuf), pRxPacket+1, len);
        memcpy(&rssi, pRxPacket+len+1, 1);
        correlation = 100;
#elif PHY_IEEE_MODE
        uint8_t footer[2];
        /* read RF FIFO data to data buf */
        len = len - 2;
        if (len <= BM_BUFFER_SIZE)
        {
           memcpy(BM_getDataPtr(pTempBuf), pRxPacket+1, len);  //JIRA 44, done
        }
        else
        {
#if DEBUG_MEM_CORR
            volatile int errloop = 0;
            while(1)
            {
               errloop++;
            }
#else
            BM_free_6tisch(pTempBuf);
            return;
#endif
        }
   
        memcpy(footer, pRxPacket+len+1, 2);
        rssi = footer[0];
        correlation = (uint8_t) (footer[1] & 0x7f);
#endif
       
        /* set rx buf length */
        BM_setDataLen(pTempBuf,len);
        dataPtr = BM_getDataPtr(pTempBuf);

        /* check the packet type */
        fcf = (uint8_t)(dataPtr[0] & 0x7);

        /* save RX parameter */
        MRXM_saveMacPHYInfo(curTime,len,rssi, correlation,fcf);

        if ((fcf ==  ULPSMACBUF_ATTR_PACKET_TYPE_DATA) ||
            (fcf == ULPSMACBUF_ATTR_PACKET_TYPE_COMMAND))
        {
            pDedBuf = usm_buf_datarecv;

            BM_reset(pDedBuf);
            BM_copyfrom(pDedBuf, dataPtr, len);
            BM_setDataLen(pDedBuf,len);
            rxRxmFlag = 1;
            eventPost(ACM_EVENT_RX_DONE_OK);
        }
        else if (fcf == ULPSMACBUF_ATTR_PACKET_TYPE_BEACON)
        {
            if((TSCH_DB_TSCH_SYNC_INITED(rxm_db_hnd) == LOWMAC_FALSE) &&
               (TSCH_DB_TSCH_STARTED(rxm_db_hnd) == LOWMAC_FALSE))
            {
                last_rx_bcn_channel =  HAL_RADIO_getChannel();
                ULPSMAC_LOWMAC.input(pTempBuf);
            }
            else
            {
                pDedBuf = usm_buf_datarecv;

                BM_reset(pDedBuf);
                BM_copyfrom(pDedBuf, dataPtr, len);
                BM_setDataLen(pDedBuf,len);

                rxRxmFlag = 1;
                eventPost(ACM_EVENT_RX_DONE_OK);
            }
        }
        else if (fcf == ULPSMACBUF_ATTR_PACKET_TYPE_ACK)
        {
            pDedBuf = usm_buf_ackrecv;

            BM_reset(pDedBuf);
            BM_copyfrom(pDedBuf, dataPtr, len);
            BM_setDataLen(pDedBuf,len);

            rxAckFlag = 1;
            eventPost(ACM_EVENT_RX_DONE_OK);
        }

        BM_free_6tisch(pTempBuf);
    }
}
