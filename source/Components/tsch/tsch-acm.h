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
 *  ====================== tsch-acm.h =============================================
 *  
 */

#ifndef __TSCH_ACM_H__
#define __TSCH_ACM_H__

#include "hal_api.h"

#include "lowmac.h"
#include "tsch-db.h"
#include "bm_api.h"
#include "nhl_mgmt.h"
#include "nhldb-tsch-cmm.h"
#include "nhl_mlme_cb.h"
/*---------------------------------------------------------------------------*/
#define TSCH_ACM_SLOTDELTA_INPHASE     0
#define TSCH_ACM_SLOTDELTA_IAM_EARLY   1
#define TSCH_ACM_SLOTDELTA_IAM_LATE    2

extern rtimer_clock_t rtc_timeslot_length;
#define TSCH_ACM_RTC_TIMESLOT_LENGTH   (rtc_timeslot_length)
extern rtimer_clock_t rtc_tx_offset;
#define TSCH_ACM_RTC_TX_OFFSET         (rtc_tx_offset)
extern rtimer_clock_t rtc_rx_offset;
#define TSCH_ACM_RTC_RX_OFFSET         (rtc_rx_offset)
extern rtimer_clock_t rtc_rx_wait;
#define TSCH_ACM_RTC_RX_WAIT           (rtc_rx_wait)
extern rtimer_clock_t rtc_rx_ack_delay;
#define TSCH_ACM_RTC_RX_ACK_DELAY      (rtc_rx_ack_delay)
extern rtimer_clock_t rtc_tx_ack_delay;
#define TSCH_ACM_RTC_TX_ACK_DELAY      (rtc_tx_ack_delay)
extern rtimer_clock_t rtc_ack_wait;
#define TSCH_ACM_RTC_ACK_WAIT          (rtc_ack_wait)
extern rtimer_clock_t rtc_max_tx;
#define TSCH_ACM_RTC_MAX_TX            (rtc_max_tx)
extern rtimer_clock_t rtc_max_ack;
#define TSCH_ACM_RTC_MAX_ACK           (rtc_max_ack)

/*---------------------------------------------------------------------------*/
LOWMAC_status_e TSCH_ACM_init(TSCH_DB_handle_t* db_hnd);
/*---------------------------------------------------------------------------*/
LOWMAC_status_e TSCH_ACM_start(void);
/*---------------------------------------------------------------------------*/
LOWMAC_status_e TSCH_ACM_stop(void);
/*---------------------------------------------------------------------------*/
void TSCH_ACM_set_adv_syncinfo(BM_BUF_t* usm_buf, uint16_t mpi_recv_len, uint64_t cur_asn);
/*---------------------------------------------------------------------------*/
void TSCH_ACM_set_slotdelta(const rtimer_clock_t slotdelta_ticks, const uint8_t slotdelta_phase);
/*---------------------------------------------------------------------------*/
uint16_t TSCH_ACM_timeslotLenMS(void);
/*---------------------------------------------------------------------------*/
uint64_t TSCH_ACM_get_ASN(void);
/*---------------------------------------------------------------------------*/
void TSCH_rxISR(RF_EventMask e, uint8_t* pPacket);

typedef struct TSCH_hopping_info{
    uint64_t asn;
    uint8_t timeslot;
    uint8_t channel;
} TSCH_hopping_info_t;

// for APP API
extern TSCH_hopping_info_t link_hopping_info[4];

//Robert
#define NUM_SLOT_DBG    32
typedef struct __time_slot_debug
{
    uint8_t slot_num;
    uint64_t cur_asn;
    rtimer_clock_t  frame_slot_start;
    rtimer_clock_t  ack_start;
    rtimer_clock_t  ack_stop;
    rtimer_clock_t  next_frame_slot_start;
} SLOT_DBG_s;

// define the RF related events
#define ACM_EVENT_TX_DATA_DONE              (0x00)
#define ACM_EVENT_TX_ACK_DONE               (0x01)
#define ACM_EVENT_RX_DONE_OK                (0x02)
#define ACM_EVENT_RX_DONE_NOK               (0x03)
#define ACM_EVENT_DUMMY_EVENT               (0x04)
#define ACM_EVENT_RX_SYNC                   (0x05)


typedef struct __eventMsg
{
    //!!!!The size of this data structure (1 byte right now)
    // must agree with the mailbox definition in the .cfg file.  If this data structure is modified,
    // make sure the .cfg file is modified as well.
  uint8_t   eventID;
} EVT_MSG_s;

#define ACM_WAIT_EVT_SUCCESS                (0)
#define ACM_WAIT_EVT_TIMEOUT                (1)
#define ACM_WAIT_EVT_NOT_EXPECTED           (2)


void eventPost(uint8_t eventID);
void eventFree(void);
uint8_t eventWait(uint8_t eventID, uint32_t timeoutValue, uint32_t reftime);
void TSCH_prepareDisConnect(void);

#endif /* __TSCH_ACM_H__ */

