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
 *  ====================== tsch-txm.h =============================================
 *  
 */


#ifndef __TSCH_TXM_H__
#define __TSCH_TXM_H__

#include "rtimer.h"

#include "ulpsmacbuf.h"
#include "bm_api.h"
#include "lowmac.h"
#include "tsch-db.h"


/*---------------------------------------------------------------------------*/
typedef struct __TSCH_TXM_queued_packet {

  struct __TSCH_TXM_queued_packet *next;

  struct ulpsmacbuf             *buf;
  LOWMAC_txcb_t                 sent;
  void                          *cptr;
  //uint8_t                       ack_req;
  uint8_t                       num_tx;
  uint8_t                       reserve;
  //rimeaddr_t                    rxaddr;

} TSCH_TXM_queued_packet_t;
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
LOWMAC_status_e TSCH_TXM_init(TSCH_DB_handle_t* db_hnd);
/*---------------------------------------------------------------------------*/
LOWMAC_status_e TSCH_TXM_que_pkt(BM_BUF_t* usm_buf, LOWMAC_txcb_t sent_callback, void* ptr);
/*---------------------------------------------------------------------------*/
void TSCH_TXM_que_pkt_thread_safe(BM_BUF_t* usm_buf, LOWMAC_txcb_t sent_callback, void* ptr);
/*---------------------------------------------------------------------------*/
// TODO: return of the function should be slot_delta
LOWMAC_status_e TSCH_TXM_timeslot(TSCH_DB_link_t* cur_link, rtimer_clock_t this_slot_start, uint64_t cur_asn);
/*---------------------------------------------------------------------------*/
uint8_t TSCH_TXM_can_send(TSCH_DB_link_t* link, uint8_t link_type);
/*---------------------------------------------------------------------------*/
void TSCH_TXM_send_keepalive_pkt(void* ptr);
/*---------------------------------------------------------------------------*/
LOWMAC_status_e TSCH_TXM_delete_share_link(uint8_t* slotframeHandle, uint16_t * linkHandle);
/*---------------------------------------------------------------------------*/
void TXM_freeQueuedTxPackets(TSCH_DB_link_t* db_link);
/*---------------------------------------------------------------------------*/
void TSCH_TXM_freePkts(void);
/*---------------------------------------------------------------------------*/
LOWMAC_status_e TSCH_TXM_delete_dedicated_link(uint16_t linkPeerAddr,
                                               uint8_t* slotframeHandle, 
                                               uint16_t * linkHandle);
#endif /* __TSCH_TXM_H__ */
