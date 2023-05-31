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
 *  ====================== nhldb-tsch-nonpc.h =============================================
 *  
 */
#ifndef __NHLDB_TSCH_NONPC_H__
#define __NHLDB_TSCH_NONPC_H__

#include "mac_config.h"
#include "ulpsmacaddr.h"
#include "prmtv802154e.h"

/*---------------------------------------------------------------------------*/
#define NHLDB_TSCH_NONPC_MAX_NUM_SLOTFRAMES_IN_ADV    1
#define NHLDB_TSCH_NONPC_MAX_NUM_LINKS_IN_ADV         IE_SLOTFRAMELINK_MAX_LINK


#define NHLDB_TSCH_NONPC_MAX_NUM_SLOTFRAMES_IN_ASSOC  2
#define NHLDB_TSCH_NONPC_MAX_NUM_LINKS_IN_ASSOC       2

#define NHLDB_TSCH_NONPC_PANID_ANY          0xffff

#define ASSIGN_LINK_EXIST 0
#define ASSIGN_NEW_LINK 1
#define ASSIGN_NO_LINK 2

enum {
  NHLDB_TSCH_NONPC_ASTAT_NONE = 0,
  NHLDB_TSCH_NONPC_ASTAT_NEED_AREQ,        // Association needed
  NHLDB_TSCH_NONPC_ASTAT_AREQING,          // Association is requesting
  NHLDB_TSCH_NONPC_ASTAT_ACONFED,          // Association confirmed
};

/*********************************************************************
 * TYPEDEFS
 */

typedef struct __advertiserEntry {
  uint8_t        isBlackListed;
  uint8_t        inUse;
  uint8_t        preferred;
  uint8_t        join_prio;
  uint8_t        addrmode;
  uint8_t        astat;
  uint8_t        astat_revoke;
  uint8_t        is_retry;
  uint8_t        rx_channel;
  struct __advertiserEntry* astat_next;
  uint8_t        rssi;
  uint8_t        lqi;
  uint8_t       rcvdBeacons;
  uint32_t      timestamp;
  uint16_t       panid;
  uint64_t       asn;
  ulpsmacaddr_t  addr;
  uint8_t        ts_template_len;
  uint8_t        hop_seq_id;
  uint8_t        num_sfs;
  uint8_t        num_links_in_adv[NHLDB_TSCH_NONPC_MAX_NUM_SLOTFRAMES_IN_ADV];
  ie_timeslot_template_t ts_template;
  tschMlmeSetSlotframeReq_t  sf[NHLDB_TSCH_NONPC_MAX_NUM_SLOTFRAMES_IN_ADV];
  tschMlmeSetLinkReq_t       sf_link_in_adv[NHLDB_TSCH_NONPC_MAX_NUM_SLOTFRAMES_IN_ADV][NHLDB_TSCH_NONPC_MAX_NUM_LINKS_IN_ADV];
} NHLDB_TSCH_NONPC_advEntry_t;

typedef struct __prevPerferredParent{
  uint8_t        addrMode;
  ulpsmacaddr_t  addr;
}NHLDB_TSCH_prevPreferredParent_t;

#if ULPSMAC_L2MH
/*---------------------------------------------------------------------------*/
typedef struct __NHLDB_TSCH_CMM_l2mhent {
  uint8_t       inUse;

  uint8_t       prevNodeAddrMode;
  ulpsmacaddr_t prevNodeAddr;

  ulpsmacaddr_t deviceExtAddr;
  uint16_t requestorShortAddr;

} NHLDB_TSCH_NONPC_l2mhent_t;
#endif /*ULPSMAC_L2MH */

/*---------------------------------------------------------------------------*/
extern void
NHLDB_TSCH_NONPC_init(void);
/*---------------------------------------------------------------------------*/
extern inline uint8_t
NHLDB_TSCH_NONPC_num_adventry(void);
/*---------------------------------------------------------------------------*/
extern NHLDB_TSCH_NONPC_advEntry_t*
NHLDB_TSCH_NONPC_get_adventry(uint8_t idx);
/*---------------------------------------------------------------------------*/
extern NHLDB_TSCH_NONPC_advEntry_t*
NHLDB_TSCH_NONPC_find_adventry(uint16_t panid, uint8_t addrmode, ulpsmacaddr_t* addr);
/*---------------------------------------------------------------------------*/
extern NHLDB_TSCH_NONPC_advEntry_t*
NHLDB_TSCH_NONPC_new_adventry(tschPanDesc_t* panDescriptor, uint8_t* ieBuf,
                              uint8_t ieBufLen, uint8_t isBlackListed);
/*---------------------------------------------------------------------------*/
extern void
NHLDB_TSCH_NONPC_setup_adventry(NHLDB_TSCH_NONPC_advEntry_t* adv_entry,
                 tschPanDesc_t* pan_desc, uint8_t* ie_buf, uint8_t ie_buflen);
extern uint8_t
NHLDB_TSCH_NONPC_create_associe_l2mh(NHLDB_TSCH_NONPC_advEntry_t* adv_entry,
                                     uint8_t* ie_buf, uint8_t ie_buflen);
/*---------------------------------------------------------------------------*/
extern void
NHLDB_TSCH_NONPC_update_adventry(NHLDB_TSCH_NONPC_advEntry_t* adv_entry,
                 tschPanDesc_t* pan_desc, uint8_t* ie_buf, uint8_t ie_buflen);
/*---------------------------------------------------------------------------*/
extern uint16_t NHLDB_TSCH_mofify_adv(void);
/*---------------------------------------------------------------------------*/
extern uint8_t NHLDB_TSCH_find_link_handle(uint8_t slotframeHandle, tschMlmeSetLinkReq_t* setLinkArg);
extern void NHLDB_TSCH_delete_link_handle(uint8_t slotframeHandle, uint16_t linkHandle);
extern uint8_t NHLDB_TSCH_assign_link_handle(uint8_t slotframeHandle, tschMlmeSetLinkReq_t* setLinkArg);
/*---------------------------------------------------------------------------*/
//extern void
//NHLDB_TSCH_NONPC_rmove_adventry(NHLDB_TSCH_NONPC_advEntry_t* adv_entry);
/*---------------------------------------------------------------------------*/
extern NHLDB_TSCH_NONPC_advEntry_t*
NHLDB_TSCH_NONPC_mark_prefer_adventry(void);
/*---------------------------------------------------------------------------*/
extern uint16_t
NHLDB_TSCH_NONPC_setup_prefer_adventry(void);
/*---------------------------------------------------------------------------*/
extern void
NHLDB_TSCH_NONPC_update_prefer_adventry(void);
/*---------------------------------------------------------------------------*/
extern uint16_t
NHLDB_TSCH_NONPC_remove_prefer_adventry(void);
/*---------------------------------------------------------------------------*/
NHLDB_TSCH_NONPC_advEntry_t*
NHLDB_TSCH_NONPC_default_adventry(void);
uint8_t
NHLDB_TSCH_NONPC_default_adventry_index(void);
/*---------------------------------------------------------------------------*/
NHLDB_TSCH_NONPC_advEntry_t*
NHLDB_TSCH_NONPC_astat_req_adventry(void);
/*---------------------------------------------------------------------------*/
void
NHLDB_TSCH_NONPC_astat_change(NHLDB_TSCH_NONPC_advEntry_t* adv_entry, uint8_t astat);
/*---------------------------------------------------------------------------*/
extern uint8_t
NHLDB_TSCH_NONPC_create_advie(uint8_t* ie_buf, uint8_t ie_buflen);
/*---------------------------------------------------------------------------*/
extern uint8_t
NHLDB_TSCH_NONPC_create_disassocie(NHLDB_TSCH_NONPC_advEntry_t* adv_entry,
                                   uint8_t* ie_buf, uint8_t ie_buflen);
/*---------------------------------------------------------------------------*/
extern uint8_t
NHLDB_TSCH_NONPC_create_birie(uint8_t* ie_buf, uint8_t ie_buflen);
/*---------------------------------------------------------------------------*/
extern uint8_t
NHLDB_TSCH_NONPC_parse_assocrespie(NHLDB_TSCH_NONPC_advEntry_t* adv_entry,
                                    uint8_t* ie_buf, uint8_t ie_buflen);

extern uint8_t
NHLDB_TSCH_NONPC_create_assocreq_ie(uint8_t period, uint8_t numFragPackets,
                                    BM_BUF_t * req_usm_buf);

uint16_t
NHLDB_TSCH_NONPC_parse_assocrespsuggestion(uint8_t* ie_buf, uint8_t ie_buflen);

/*---------------------------------------------------------------------------*/
#if ULPSMAC_L2MH
extern void
NHLDB_TSCH_NONPC_parse_assocrespl2mh_ie(uint16_t assocShortAddr, 
                                   uint8_t* ie_buf, uint8_t ie_buflen);
extern uint8_t
NHLDB_TSCH_NONPC_add_l2mhent(const ulpsmacaddr_t* deviceExtAddr,
                  const uint16_t requestorShrtAddr,
                  const uint8_t prevAddrMode, const ulpsmacaddr_t* prevAddr);
/*---------------------------------------------------------------------------*/
extern uint8_t
NHLDB_TSCH_NONPC_rem_l2mhent(NHLDB_TSCH_NONPC_l2mhent_t* cur_table);
/*---------------------------------------------------------------------------*/
extern NHLDB_TSCH_NONPC_l2mhent_t*
NHLDB_TSCH_NONPC_find_l2mhent(const ulpsmacaddr_t* deviceExtAddr,
                       const uint16_t requestorShrtAddr);
/*---------------------------------------------------------------------------*/
#endif

#endif /* __NHLDB_TSCH_NONPC_H__ */

