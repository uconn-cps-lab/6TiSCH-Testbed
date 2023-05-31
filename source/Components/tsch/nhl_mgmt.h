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
 *  ====================== nhl_mgmt.h =============================================
 *  
 */


#ifndef NHL_MGMT_H
#define NHL_MGMT_H

//*****************************************************************************
// includes
//*****************************************************************************
#include "ulpsmacaddr.h"
#include "ulpsmacbuf.h"
#include "bm_api.h"
#include "nhldb-tsch-nonpc.h"
#include "mac_config.h"
#include "nhl_api.h"

/*********************************************************************
 * TYPEDEFS
 */
typedef struct __NHL_BlackList
{
  bool        isUsed;
  uint16_t    parentShortAddr;
} NHL_BlackList_t;

#if CC26XX_DEVICES
//IEEE 250 kbps Mode 
#define BEACON_RX_RSSI_THRESHOLD (-80)
#else
// Prop 50 kbps mode
#define BEACON_RX_RSSI_THRESHOLD (-92)
#endif

//*****************************************************************************
//globals
//*****************************************************************************
extern uint8_t iam_coord;

extern uint8_t beacon_timer_started;
extern uint8_t next_tx_bcn_channel;


//*****************************************************************************
// function prototypes
//*****************************************************************************
uint8_t TSCH_checkAssociationStatus(void);
uint8_t TSCH_checkConnectionStatus(void);
void beacon_req_proc(void);
void assoc_indc_proc(void* arg,uint8_t* buf, uint8_t buflen);
void disassoc_indc_proc(void* arg, uint8_t* buf, uint8_t buflen);

#if ULPSMAC_L2MH
void assoc_indc_l2mh_proc(void* arg, uint8_t* buf, uint8_t buflen);
void disassoc_indc_l2mh_proc(void* arg, uint8_t* buf, uint8_t buflen);
#endif

void comm_status_indc_proc(void* arg);
void join_indication_proc(void* arg);
void send_packet(uint8_t dest_addr_mode, ulpsmacaddr_t* dest_addr);

void set_sf_conf_proc(void * arg);
void set_link_conf_proc(void * arg);

void disassoc_req_proc(uint8_t addrMode,ulpsmacaddr_t* pDst);
void disassoc_conf_proc(void* arg);
void beacon_notify_indc_proc(void* arg, uint8_t* buf, uint8_t buflen);
void scan_conf_proc(void* arg);
void assoc_req_proc(NHLDB_TSCH_NONPC_advEntry_t* adv_entry);
void assoc_conf_proc(void* arg, uint8_t* buf, uint8_t buflen);
void assoc_resp_proc(uint8_t* buf, uint8_t buflen);

uint8_t invoke_astat_req(void);
#if ULPSMAC_L2MH
void assoc_conf_l2mh_proc(void* arg, uint8_t* buf, uint8_t buflen);
void disassoc_conf_l2mh_proc(void* arg, uint8_t* buf, uint8_t buflen);
#endif

void TSCH_restartNode(void);
void TSCH_transmit_packet_handler(uint8_t *arg);

uint16_t NHL_tschModeOn(void);
void NHL_startBeaconTimer(void);
void NHL_stopBeaconTimer(void);
void NHL_modify_share_link(void);
void NHL_addBlackListParent(uint16_t parent);
void NHL_resetBlackListParentTable(void);
#endif /* NHL_MGMT_H */
