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
 *  ====================== nhl_mgmt.c =============================================
 *  
 */

//*****************************************************************************
// the includes
//*****************************************************************************
#include "tsch_os_fun.h"
#include "tsch_api.h"

#include "nhl_mgmt.h"
#include "nhl_mlme_cb.h"

#include "ie802154e.h"
#include "pib-802154e.h"

#include "tsch-acm.h"
#include "tsch-txm.h"
#include "nhldb-tsch-cmm.h"
#include "nhldb-tsch-pc.h"
#include "nhl_device_table.h"
#include "tsch-db.h"

#include "mac_pib.h"
#include "mac_pib_pvt.h"
#include "pnm.h"
#include <stdlib.h>

#if WITH_UIP6
#include "net/sicslowpan.h"
#include "net/uip.h"
#include "rpl/rpl.h"
#endif

#include "uip_rpl_process.h"

#include "lib/random.h"

#include "rtimer.h"

#include "nhl_api.h"
#include "nhl_setup.h"

#if IS_ROOT
#include "nhldb-tsch-sched.h"
#else
#include "nhldb-tsch-nonpc.h"

#if WITH_UIP6
#include "coap/resource.h"
#endif

#endif
#include "mac_config.h"

#if DMM_ENABLE
#include "dmm_scheduler.h"
#include "dmm_policy_blesp_wsnnode.h"
#endif //DMM_ENABLE

#if IS_ROOT
extern tschMlmeSetSlotframeReq_t pc_sfs[NHLDB_TSCH_PC_NUM_SLOTFRAMES];
#endif
extern BM_BUF_t* usm_buf_ackrecv;
extern BM_BUF_t* usm_buf_keepalive;
extern BM_BUF_t* usmBufBeaconTx;

// jiachen
#if !IS_ROOT
#include "coap/harp.h"

#ifndef HARP_MINIMAL

extern uint64_t packet_sent_asn_others;
#endif
extern uint64_t packet_sent_asn_sensors;
extern uint16_t received_asn;

extern uint64_t packet_sent_asn_ping;


extern HARP_self_t HARP_self;
extern HARP_child_t HARP_children[NODE_MAX_NUM];
#endif

extern void nhl_AssociateResponseAckIndication(uint8_t addrMode, ulpsmacaddr_t *pDstAddr, uint8_t eventID);
extern void coap_msg_post_start_scan();
void start_scan(void);
//*****************************************************************************
// defines
//*****************************************************************************
#define SCAN_SLEEP_MIN_INTERVAL (0)
#define SCAN_SLEEP_MAX_INTERVAL (SCAN_SLEEP_MIN_INTERVAL+8) 

#define DISABLE_ASSOC (0)

#define NUM_BLACK_LIST (10)

//*****************************************************************************
//globals
//*****************************************************************************


extern pnmPib_t* mtxm_pnmdb_ptr;

extern uint16_t selfShortAddr;

//*****************************************************************************
// Local variables
//*****************************************************************************
uint8_t iam_coord = NHL_TRUE;
uint16_t numScanErr=0;

nhl_recv_callback_t nhl_recv_cb = NULL;
nhl_sent_callback_t nhl_sent_cb = NULL;
#if WITH_UIP6
nhl_ctrl_callback_t nhl_ctrl_cb = NULL;
extern uint8_t joinedDODAG;
#endif

extern unsigned char node_mac[8];
extern uint8_t is_scanning;
extern uint8_t  TSCH_Started;
uint8_t has_short_addr = NHL_FALSE;
uint8_t has_bcn_tx_link = NHL_FALSE;
#if !IS_ROOT
static bool done_scan = false;
static NHL_BlackList_t blackList[NUM_BLACK_LIST];
static uint8_t blackListIndex = 0;
#endif

uint8_t beacon_timer_started = ULPSMAC_FALSE;
uint8_t next_tx_bcn_channel;
uint8_t curScanSleepInt = SCAN_SLEEP_MIN_INTERVAL;

/**************************************************************************************************//**
 *
 * @brief       This function will check if a node has finished assoication and joined a DODAG
 *
 *
 * @param[in]
 *
 *
 * @return      true , false
 *
 ***************************************************************************************************/
uint8_t TSCH_checkConnectionStatus(void)
{
    if (TSCH_checkAssociationStatus() == NHL_FALSE)
    {
        return NHL_FALSE;
    }
    else
    {
#if WITH_UIP6
    if (joinedDODAG == 0)
    {
        return NHL_FALSE;
    }
#endif
    return NHL_TRUE;
    }
}

/**************************************************************************************************//**
 *
 * @brief       This function will check if a node has been associated
 *
 *
 * @param[in]
 *
 *
 * @return      true , false
 *
 ***************************************************************************************************/
uint8_t TSCH_checkAssociationStatus(void)
{
#if (!IS_ROOT &!DISABLE_ASSOC)
    uint8_t iter;
      static uint16_t count=0;
      count++;

    if (!NHLDB_TSCH_NONPC_default_adventry())
    {
        return NHL_FALSE;
    }

    for (iter = 0; iter < ULPSMAC_MAX_NUM_ADVERTISER; iter++)
    {
        NHLDB_TSCH_NONPC_advEntry_t* adventry;

        adventry = NHLDB_TSCH_NONPC_get_adventry(iter);
        if (adventry->inUse && adventry->preferred)
        {
            if (adventry->astat != NHLDB_TSCH_NONPC_ASTAT_ACONFED)
            {
                return NHL_FALSE;
            }
        }

    }

#endif
    return NHL_TRUE;
}

/**************************************************************************************************//**
 *
 * @brief       This function will start the TSCH MAC. It will init TSCH MAC PIB, nonPC data base
 *              ACM init. and start the channel scan.
 *
 *              This function is called when
 *              1. node issue the dis-association command and receive dis-association confirm
 *              2. received the dis-assocaition indication before root node want node to leaf
 *              3. lose the beacon sync
 *
 *
 * @param[in]
 *
 *
 * @return
 *
 ***************************************************************************************************/
void TSCH_restartNode(void)
{
    TSCH_DB_handle_t* db_hnd_ptr;

    // reset PAN ID and short address
    TSCH_PIB_init();

    NHLDB_TSCH_NONPC_init();

    // init TSCH DB
    db_hnd_ptr = TSCH_DB_init();
  
    TSCH_ACM_init(db_hnd_ptr);
    
    // restart scan process

    start_scan();  //JIRA53
#if WITH_UIP6
    coap_delete_all_observers();
#endif 
    memset(&ULPSMAC_Dbg.numTxNoAckPerCh,0,sizeof(ULPSMAC_Dbg.numTxNoAckPerCh));
    memset(&ULPSMAC_Dbg.numTxTotalPerCh,0,sizeof(ULPSMAC_Dbg.numTxTotalPerCh));
    memset(&ULPSMAC_Dbg.avgRssi,0,sizeof(ULPSMAC_Dbg.avgRssi));
    memset(&ULPSMAC_Dbg.rssiCount,0,sizeof(ULPSMAC_Dbg.rssiCount));
    memset(&ULPSMAC_Dbg.avgRxRssi,0,sizeof(ULPSMAC_Dbg.avgRxRssi));             //zelin
    memset(&ULPSMAC_Dbg.rxRssiCount,0,sizeof(ULPSMAC_Dbg.rxRssiCount));         //zelin
    
    
    ULPSMAC_Dbg.numSyncAdjust = 0;
    ULPSMAC_Dbg.avgSyncAdjust = 0;
    ULPSMAC_Dbg.maxSyncAdjust = 0;

    ULPSMAC_Dbg.numTxNoAck = 0;
    ULPSMAC_Dbg.numTxTotal = 0;
    ULPSMAC_Dbg.numRxTotal = 0;
    ULPSMAC_Dbg.numTxLengthTotal = 0;
    
    // jiachen
#if !IS_ROOT
    memset(&HARP_self, 0, sizeof(HARP_self));
    memset(&HARP_children[0], 0, sizeof(HARP_child_t) * MAX_CHILDREN_NUM);
    selfShortAddr = 0;
    received_asn = 0;

    packet_sent_asn_sensors = 0;
//    packet_sent_asn_others  = 0;

    packet_sent_asn_ping = 0;
    BM_free_6tisch(usm_buf_ackrecv);
    BM_free_6tisch(usm_buf_keepalive);
    BM_free_6tisch(usmBufBeaconTx);
    usm_buf_ackrecv = BM_alloc(20);
    usm_buf_keepalive = BM_alloc(21);
    usmBufBeaconTx = BM_alloc(22);
#endif
}

/**************************************************************************************************//**
 *
 * @brief       This function will set up the beacon payload IE for root or intermediate node
 *
 *
 * @param[in]   panCoordinator : true for root node (PAN coordinator)
 *                               false for intermediate node
 * @return      status (sucess or failure)
 *
 ***************************************************************************************************/
uint16_t TSCH_setBeacon(bool panCoordinator)
{
    BM_BUF_t *pBuf;
    uint8_t parsed_len,*pData;
    uint16_t status;
    tschMlmeStartReq_t startReqArg;

    status = STAT802154E_ERR;
    pBuf = BM_alloc(18);

    if (pBuf == NULL)
    {
        return status;
    }
    // BZ14397
    memset(&startReqArg, 0, sizeof(tschMlmeStartReq_t));

    pData = BM_getBufPtr(pBuf);
#if IS_ROOT
    parsed_len = NHLDB_TSCH_PC_create_advie(pData, BM_BUFFER_SIZE);
#else
    parsed_len = NHLDB_TSCH_NONPC_create_advie(pData, BM_BUFFER_SIZE);
#endif
    if (parsed_len == 0)
    {
        BM_free_6tisch(pBuf);
        return status;
    }

    // update the length
    BM_setDataLen(pBuf,parsed_len);

    startReqArg.pIE_List = pBuf;

    /* 4. Start for setting up the short address and panid of mine */
    startReqArg.panCoordinator = panCoordinator;
    startReqArg.panId = TSCH_MACConfig.panID;

    status = TSCH_MlmeStartReq(&startReqArg);

    BM_free_6tisch(pBuf);

    return status;
}

/**************************************************************************************************//**
 *
 * @brief       This function handle beacon request message in NHL task context.
 *              It allocates the BM buffer and copy the beacon payload context to BM buffer.
 *              This buffer will assign to pIElist in tschMlmeBeaconReq_t.
 *
 *              When the beacon transmission (broadcast) is complete, this pIEList buffer
 *              will be freed.
 *
 *
 * @param[in]
 *
 * @return
 *
 ***************************************************************************************************/
void beacon_req_proc(void)
{
    tschMlmeBeaconReq_t beaconReqArg;
    BM_BUF_t* usm_buf;
    static uint8_t tx_bcn_channel_idx=2;

    usm_buf = usmBufBeaconTx;
    if(BM_getDataLen(usm_buf)!=0){
        //overgen
        ULPSMAC_Dbg.numBeaconOverGen++;
        return;
    }

    if (!usm_buf)
    {
        return;
    }

    memset(&beaconReqArg, 0, sizeof(tschMlmeBeaconReq_t));

    beaconReqArg.beaconType = BEACON_TYPE_ENHANCED;
    beaconReqArg.dstAddrMode = ULPSMACADDR_MODE_SHORT;
    memcpy(beaconReqArg.dstAddr.u8,ulpsmacaddr_broadcast.u8,ULPSMACADDR_LONG_SIZE);

    beaconReqArg.bsnSuppression = 0;
    
    // copy the beacon payload IE to pIEList
    BM_copyfrom(usm_buf,mtxm_pnmdb_ptr->ebPayloadIeList, mtxm_pnmdb_ptr->ebPayloadIeListLen);

    beaconReqArg.pIE_List = usm_buf;

    if (TSCH_MACConfig.bcn_ch_mode== 1)
    {
        next_tx_bcn_channel = TSCH_MACConfig.bcn_chan[0];
    }
    else if (TSCH_MACConfig.bcn_ch_mode == 3)
    {
        tx_bcn_channel_idx = (tx_bcn_channel_idx+1)%3;
        next_tx_bcn_channel =  TSCH_MACConfig.bcn_chan[tx_bcn_channel_idx];
    }

     TSCH_MlmeBeaconReq(&beaconReqArg);
}


/**************************************************************************************************//**
 *
 * @brief       This function handles disassociation request message in NHL task context. *
 *
 *
 * @param[in]   addrMode -- address mode of device is disassociating
 *              pDst     -- pointer of device address
 *
 * @return
 *
 ***************************************************************************************************/
void disassoc_req_proc(uint8_t addrMode,ulpsmacaddr_t* pDst)
{
    tschMlmeDisassociateReq_t disassocReqArg;
    uint16_t panID;

    // BZ14397
    memset(&disassocReqArg, 0, sizeof(tschMlmeDisassociateReq_t));

    // get PAN ID from PIB
    TSCH_MlmeGetReq(TSCH_MAC_PIB_ID_macPANId, &panID);
    disassocReqArg.devicePanId = panID;

    disassocReqArg.deviceAddrMode = addrMode;
    if (addrMode == ULPSMACADDR_MODE_LONG)
    {
        ulpsmacaddr_long_copy(&disassocReqArg.deviceAddress, pDst);
    }
    else
    {
        ulpsmacaddr_short_copy(&disassocReqArg.deviceAddress,pDst);
    }
#if IS_ROOT
    // root node want device to leave
    disassocReqArg.disassociateReason = DISASSOC_REASON_BY_COORD;
#else
    // device wants to leave the PAN
    disassocReqArg.disassociateReason = DISASSOC_REASON_BY_DEVICE;
#endif


    TSCH_MlmeDisassociateReq(&disassocReqArg);
}


/**************************************************************************************************//**
 *
 * @brief       This function handles disassociation confirm message in NHL task context. *
 *
 *
 * @param[in]   arg --  void pointer which can be casted to NHLDB_TSCH_NONPC_advEntry_t
 *
 * @return
 *
 ***************************************************************************************************/
void disassoc_conf_proc(void* arg)
{
}


/**************************************************************************************************//**
 *
 * @brief       This function handles association request message in NHL task context. *
 *
 *
 * @param[in]   arg --  void pointer which can be casted to tschMlmeAssociateIndication_t
 *              buf --  buffer to payload IE
 *              buflen -- IE length
 *
 * @return
 *
 ***************************************************************************************************/
void assoc_indc_proc(void* arg,uint8_t* buf, uint8_t buflen)
{
    tschMlmeAssociateIndication_t* assoc_indc_arg;
    ulpsmacaddr_t* deviceLongAddr;

    assoc_indc_arg = (tschMlmeAssociateIndication_t*) arg;
    deviceLongAddr = (ulpsmacaddr_t *) &assoc_indc_arg->deviceAddress;

#if IS_ROOT
    uint16_t deviceShortAddr;
    uint8_t shrtAddrReqed;

    shrtAddrReqed = assoc_indc_arg->capabilityInformation
        & ASSOC_CAPINFO_ASSIGN_ADDR_MASK;

    if (shrtAddrReqed == ASSOC_CAPINFO_ASSIGN_ADDR_TRUE)
    {
        deviceShortAddr = NHL_deviceTableNewShortAddr(deviceLongAddr);
        if (deviceShortAddr > ulpsmacaddr_null_short)
        {
          uint16_t linkHandle;
          uint8_t slotframeHandle;
          uint16_t delStatus=LOWMAC_STAT_SUCCESS;
          while (delStatus == LOWMAC_STAT_SUCCESS)
          {
            delStatus  = TSCH_TXM_delete_dedicated_link(deviceShortAddr,&slotframeHandle,&linkHandle);
          }
          
          NHLDB_TSCH_PC_request_assocresp_ie(ulpsmacaddr_short_node,
                                             deviceShortAddr, 
                                             ulpsmacaddr_short_node);
        }
    }
#ifdef FEATURE_MAC_SECURITY
    macSecurityDeviceReset(deviceShortAddr);  //reset the device in the MAC security table if already there
#endif

#else /*IS_ROOT*/
#if ULPSMAC_L2MH
    BM_BUF_t* usm_buf = BM_alloc(24);

    if (!usm_buf)
    {
        return;
    }
    
    /* Intermediate node forwards the received association indication to root */
    if( ulpsmacaddr_short_node != ulpsmacaddr_null_short)
    {
        tschTiAssociateReqL2mh_t assoc_req_l2mh;
        NHLDB_TSCH_NONPC_advEntry_t* def_adventry;

        if(!NHLDB_TSCH_NONPC_find_l2mhent(deviceLongAddr, ulpsmacaddr_short_node))
        {
            NHLDB_TSCH_NONPC_add_l2mhent(deviceLongAddr, ulpsmacaddr_short_node,
                                         ULPSMACADDR_MODE_LONG, deviceLongAddr);
        }

        def_adventry = NHLDB_TSCH_NONPC_default_adventry();
        assoc_req_l2mh.coordPanId = def_adventry->panid;
        assoc_req_l2mh.coordAddress = def_adventry->addr;
        assoc_req_l2mh.coordAddrMode = def_adventry->addrmode;
        assoc_req_l2mh.capabilityInformation = assoc_indc_arg->capabilityInformation;
        ulpsmacaddr_long_copy(&assoc_req_l2mh.deviceExtAddr, deviceLongAddr);
        assoc_req_l2mh.requestorShrtAddr = ulpsmacaddr_short_node;

        /*buf[0]=MAC_COMMAND_TYPE, buf[1]=Capability Information*/
        if (buflen >2)
        {
            BM_copyfrom(usm_buf, buf, buflen);
            assoc_req_l2mh.fco.iesIncluded = 1;
        }

        TSCH_TiAssociateReqL2mh(&assoc_req_l2mh, usm_buf);
    }
    else
    {
        BM_free_6tisch(usm_buf);
    }
#endif /* ULPSMAC_L2MH */
#endif /*IS_ROOT*/
}

/**************************************************************************************************//**
 *
 * @brief       This function sends association response after it gets the 
 *              associate response IE from scheduler.
 *
 * @param[in]   buf --  pointer to associate response IE
 *              buflen -- IE length
 *
 * @return
 *
 ***************************************************************************************************/
void assoc_resp_proc(uint8_t* buf, uint8_t buflen)
{
#if IS_ROOT
    ie_tsch_slotframelink_t new_slotframelink_ie;
    ie_link_info_field_t* new_link;
    ie_tsch_link_period_t linkPeriod;
    uint8_t sf_iter;
    uint8_t link_iter;
    uint8_t slotframelink_ie_length = 0;
    
    uint8_t assoc_status;
    uint16_t suggestion;
    uint8_t num_tot_links;
    uint16_t childOriginalShortAddr;
    uint16_t childShortAddr;
    uint16_t parentShortAddr;
    uint16_t nextHopShortAddr;
    uint8_t* ptemp = buf;
    /* Association response buffer*/
    BM_BUF_t *pBuf = BM_alloc(1);
    
    if (pBuf == NULL)
        return;
    
    memcpy(&assoc_status,ptemp,1);
    ptemp+=1;
    memcpy(&suggestion,ptemp,2);
    ptemp+=2;
    memcpy(&num_tot_links,ptemp,1);
    ptemp+=1;
    memcpy(&parentShortAddr,ptemp,2);
    ptemp+=2;
    memcpy(&childOriginalShortAddr,ptemp,2);
    ptemp+=2;
    memcpy(&childShortAddr,ptemp,2);
    ptemp+=2;
    memcpy(&nextHopShortAddr,ptemp,2);
    ptemp+=2;
    
    NHL_deviceTableUpdateShortAddr(childOriginalShortAddr, childShortAddr);

    new_slotframelink_ie.numSlotframes = NHLDB_TSCH_PC_NUM_SLOTFRAMES;
    slotframelink_ie_length += IE_SLOTFRAME_LINK_INFO_LEN;
    
    for(sf_iter=0; sf_iter<new_slotframelink_ie.numSlotframes; sf_iter++)
    {
        ie_slotframe_info_field_t* new_slotframe = &new_slotframelink_ie.slotframe_info_list[sf_iter];
     
        new_slotframe->macSlotframeHandle = pc_sfs[sf_iter].slotframeHandle;
        new_slotframe->macSlotframeSize = pc_sfs[sf_iter].size;
        new_slotframe->numLinks = num_tot_links;
        new_link = new_slotframe[sf_iter].link_info_list;
        slotframelink_ie_length += (IE_SLOTFRAME_INFO_LEN 
                                    + new_slotframe->numLinks * IE_LINK_INFO_LEN);
        
        for(link_iter=0;link_iter<num_tot_links;++link_iter)
        {
            memcpy(&new_link[link_iter].macTimeslot,ptemp,2);
            ptemp+=2;
            memcpy(&new_link[link_iter].macChannelOffset,ptemp,2);
            ptemp+=2;
            memcpy(&linkPeriod.period[link_iter],ptemp,1);
            ptemp+=1;
            memcpy(&linkPeriod.periodOffset[link_iter],ptemp,1);
            ptemp+=1;
            memcpy(&new_link[link_iter].macLinkOption,ptemp,1);
            ptemp+=1;
        }
        
        if(parentShortAddr == ulpsmacaddr_short_node)
        {
            //Direct join, add cooresponding links myself.
            for(link_iter=0;link_iter<num_tot_links;++link_iter)
            {
                uint8_t status;
                tschMlmeSetLinkReq_t link_arg;
                
                // Skip beacon link setup
                if (new_link[link_iter].macLinkOption == 
                    (MAC_LINK_OPTION_TX | MAC_LINK_OPTION_TK))
                    continue;
                
                link_arg.slotframeHandle = new_slotframe[sf_iter].macSlotframeHandle;
                link_arg.timeslot = new_link[link_iter].macTimeslot;
                link_arg.channelOffset = new_link[link_iter].macChannelOffset;
                link_arg.period = linkPeriod.period[link_iter];
                link_arg.periodOffset =  linkPeriod.periodOffset[link_iter];
                
                if(new_link[link_iter].macLinkOption & MAC_LINK_OPTION_TX)
                {
                    link_arg.linkOptions = MAC_LINK_OPTION_RX;
                } else if(new_link[link_iter].macLinkOption & MAC_LINK_OPTION_RX)
                {   // for TX link, we need to add WAIT_CONFIRM option
                    // only when WAIT-CONFIRM option is cleared, we can use this TX link
                    link_arg.linkOptions = MAC_LINK_OPTION_TX | MAC_LINK_OPTION_WAIT_CONFIRM ;
                }
                link_arg.linkType = MAC_LINK_TYPE_NORMAL;
                link_arg.nodeAddr = childShortAddr;
                
                status = NHLDB_TSCH_assign_link_handle(sf_iter, &link_arg);
                
                if(status == ASSIGN_NEW_LINK)
                {
                    link_arg.operation = SET_LINK_ADD;
                    TSCH_MlmeSetLinkReq(&link_arg);
                }
                else if (status == ASSIGN_LINK_EXIST)
                {
                    link_arg.operation = SET_LINK_MODIFY;
                    TSCH_MlmeSetLinkReq(&link_arg);
                }
                else
                {
                    assoc_status =  ASSOC_STATUS_PAN_AT_CAPA;
                    break;
                }
            }
        }
    }
    
    /* Prepare the association response IE*/
#if TSCH_PER_SCHED
    {
        ie802154e_pie_t new_ie;
        uint8_t* cur_buf_ptr;
        uint8_t  cur_remain_len;
        uint8_t  added_len;
        
        uint8_t dataLen;
        uint8_t  total_len;
        uint8_t link_period_ie_length;
        
        total_len = 0;
        link_period_ie_length = num_tot_links*2;
        
	if((assoc_status == ASSOC_STATUS_SUCCESS) &&  (num_tot_links != 0))
        {
            dataLen = BM_getDataLen(pBuf);
            cur_buf_ptr = BM_getDataPtr(pBuf)+ dataLen;
            
            // 1. Reserve for 1st IE
            cur_buf_ptr += (MIN_PAYLOAD_IE_LEN+MIN_PAYLOAD_IE_LEN+link_period_ie_length);
            cur_remain_len = ULPSMACBUF_DATA_SIZE - dataLen -
                (MIN_PAYLOAD_IE_LEN+MIN_PAYLOAD_IE_LEN+link_period_ie_length);
            total_len += (MIN_PAYLOAD_IE_LEN+link_period_ie_length);
            
            // 2nd IE: Slotframe and link IE
            new_ie.ie_type = IE_TYPE_PAYLOAD;
            new_ie.ie_group_id = IE_GROUP_ID_MLME;
            new_ie.ie_content_length = 0;
            new_ie.ie_sub_type = IE_SUBTYPE_SHORT;
            new_ie.ie_elem_id = IE_ID_TSCH_SLOTFRAMELINK;
            new_ie.ie_sub_content_length = slotframelink_ie_length;
            new_ie.ie_content_ptr = (uint8_t*) &new_slotframelink_ie;
            new_ie.outer_descriptor = 0;
            
            added_len = ie802154e_add_payloadie(&new_ie, cur_buf_ptr, cur_remain_len);
            total_len += added_len;
            
            //1st IE: Link period IE
            new_ie.ie_type = IE_TYPE_PAYLOAD;
            new_ie.ie_group_id = IE_GROUP_ID_MLME;
            new_ie.ie_content_length = total_len;
            new_ie.ie_sub_type = IE_SUBTYPE_SHORT;
            new_ie.ie_elem_id = IE_ID_TSCH_LINK_PERIOD;
            new_ie.ie_sub_content_length = link_period_ie_length;
            new_ie.ie_content_ptr = (uint8_t*) &linkPeriod;
            new_ie.outer_descriptor = 1;
            
            added_len = ie802154e_add_payloadie(&new_ie, BM_getDataPtr(pBuf)+ dataLen, ULPSMACBUF_DATA_SIZE - dataLen);
            BM_setDataLen(pBuf, dataLen + total_len+MIN_PAYLOAD_IE_LEN);
    	}
        else if(assoc_status == ASSOC_STATUS_PAN_ALT_PARENT)
        {
            ie802154e_pie_t new_ie;
            
            new_ie.ie_type = IE_TYPE_PAYLOAD;
            new_ie.ie_group_id = IE_GROUP_ID_MLME;
            new_ie.ie_content_length = MIN_PAYLOAD_IE_LEN+sizeof(suggestion);
            new_ie.ie_sub_type = IE_SUBTYPE_SHORT;
            new_ie.ie_elem_id = IE_ID_TSCH_SUGGEST_PARENT;
            new_ie.ie_sub_content_length = sizeof(suggestion);
            new_ie.ie_content_ptr = (uint8_t*) &suggestion;
            new_ie.outer_descriptor = 1;
            
            dataLen = BM_getDataLen(pBuf);
            cur_buf_ptr = BM_getDataPtr(pBuf)+ dataLen;
            cur_remain_len = ULPSMACBUF_DATA_SIZE - dataLen;
            added_len = ie802154e_add_payloadie(&new_ie, cur_buf_ptr, cur_remain_len);
            BM_setDataLen(pBuf, dataLen + added_len);
	}
        
        dataLen = BM_getDataLen(pBuf);
        cur_buf_ptr = BM_getDataPtr(pBuf)+ dataLen;
        cur_remain_len = ULPSMACBUF_DATA_SIZE - dataLen;
        // add the list terminate IE to the buffer 
        added_len = ie802154e_add_payloadie_term(cur_buf_ptr, cur_remain_len);
        BM_setDataLen(pBuf, dataLen + added_len);
    }
#else
   {
        ie802154e_pie_t new_ie;
        uint8_t* cur_buf_ptr;
        uint8_t  cur_remain_len;
        uint8_t  added_len;

        // Create a slotframe and link IE
        new_ie.ie_type = IE_TYPE_PAYLOAD;
        new_ie.ie_group_id = IE_GROUP_ID_MLME;
        new_ie.ie_content_length = MIN_PAYLOAD_IE_LEN+slotframelink_ie_length;
        new_ie.ie_sub_type = IE_SUBTYPE_SHORT;
        new_ie.ie_elem_id = IE_ID_TSCH_SLOTFRAMELINK;
        new_ie.ie_sub_content_length = slotframelink_ie_length;
        new_ie.ie_content_ptr = (uint8_t*) &new_slotframelink_ie;
        new_ie.outer_descriptor = 1;

        {
            uint8_t dataLen;
            
            // add the new slotframelinkinfo IE to the buffer 
            dataLen = BM_getDataLen(pBuf);
            cur_buf_ptr = BM_getDataPtr(pBuf)+ dataLen;
            cur_remain_len = ULPSMACBUF_DATA_SIZE - dataLen;

            added_len = ie802154e_add_payloadie(&new_ie, cur_buf_ptr, cur_remain_len);
            BM_setDataLen(pBuf, dataLen + added_len);

             dataLen = BM_getDataLen(pBuf);
            cur_buf_ptr = BM_getDataPtr(pBuf)+ dataLen;
            cur_remain_len = ULPSMACBUF_DATA_SIZE - dataLen;
            // add the list terminate IE to the buffer 
            added_len = ie802154e_add_payloadie_term(cur_buf_ptr, cur_remain_len);
            BM_setDataLen(pBuf, dataLen + added_len);
        }
    }
#endif
    
    if(parentShortAddr == ulpsmacaddr_short_node)
    {
       ulpsmacaddr_t childLongAddr;
       tschMlmeAssociateResp_t assoc_response;
       
       childLongAddr = NHL_deviceTableGetLongAddr(childShortAddr);
       memset(&assoc_response, 0, sizeof(tschMlmeAssociateResp_t));
       
       assoc_response.status = assoc_status;
       ulpsmacaddr_long_copy(&assoc_response.destExtAddr, &childLongAddr);
       assoc_response.destShortAddr = childShortAddr;
       assoc_response.pIE_List = pBuf;
       assoc_response.fco.iesIncluded = 1;
       TSCH_MlmeAssociateResp(&assoc_response);
    }
    else
    {
      ulpsmacaddr_t childLongAddr;
      tschTiAssociateRespL2mh_t assoc_response;
      
      childLongAddr = NHL_deviceTableGetLongAddr(childShortAddr);
      memset(&assoc_response, 0, sizeof(tschTiAssociateRespL2mh_t));
      
      assoc_response.status = assoc_status;
      ulpsmacaddr_long_copy(&assoc_response.deviceExtAddr, &childLongAddr);
      assoc_response.deviceShortAddr = childShortAddr;
      assoc_response.requestorShrtAddr = parentShortAddr;
      assoc_response.dstAddrMode = ULPSMACADDR_MODE_SHORT;
      ulpsmacaddr_from_short(&assoc_response.dstAddr, nextHopShortAddr);
      assoc_response.fco.iesIncluded = 1;
      TSCH_TiAssociateRespL2mh(&assoc_response,pBuf);
      
      
    }
#endif
}


/**************************************************************************************************//**
 *
 * @brief       This function handles disassociation request message in NHL task context. *
 *
 *
 * @param[in]   arg --  void pointer which can be casted to tschMlmeAssociateIndication_t
 *              buf --  pointer of buffer whihc contians the disassociation message
 *              buflen -- number of bytes in buffer
 *
 * @return
 *
 ***************************************************************************************************/
void disassoc_indc_proc(void* arg, uint8_t* buf, uint8_t buflen)
{
#if IS_ROOT
    /* Parse disassociation notification */
    tschMlmeDisassociateIndication_t* disassoc_indc_arg;
    ulpsmacaddr_t* deviceLongAddr;

    disassoc_indc_arg = (tschMlmeDisassociateIndication_t*) arg;

    deviceLongAddr = &disassoc_indc_arg->deviceAddress;

    // remove this device from device table
   NHL_deviceTableDelAddr(deviceLongAddr);


#else /*IS_ROOT*/
    /* Intermediate node forwards disassociation notification to root node */

    if (!iam_coord)
    {   // for leaf node

        return;
    }
#if ULPSMAC_L2MH
    BM_BUF_t * usm_buf;
    tschMlmeDisassociateIndication_t* disassoc_indc_arg;
    const ulpsmacaddr_t* deviceLongAddr;


    // parse notification command for creation of l2mh notification to pan coord
    disassoc_indc_arg = (tschMlmeDisassociateIndication_t*) arg;

    deviceLongAddr = (const ulpsmacaddr_t *)&disassoc_indc_arg->deviceAddress;

    usm_buf = BM_alloc(25);

    if(!usm_buf)
    {
        return;
    }

    // forward only when this coordinator has been associated
    if( ulpsmacaddr_short_node == ulpsmacaddr_null_short)
    {
        BM_free_6tisch( usm_buf);
        return;
    }

    if(!NHLDB_TSCH_NONPC_find_l2mhent(deviceLongAddr, ulpsmacaddr_short_node))
    {
        NHLDB_TSCH_NONPC_add_l2mhent(deviceLongAddr, ulpsmacaddr_short_node,
                                     ULPSMACADDR_MODE_LONG, deviceLongAddr);
    }

    {
        tschTiDisassociateReqL2mh_t disassoc_req_l2mh;
        NHLDB_TSCH_NONPC_advEntry_t* def_adventry;

        def_adventry = NHLDB_TSCH_NONPC_default_adventry();

        disassoc_req_l2mh.coordPanId = def_adventry->panid;
        disassoc_req_l2mh.coordAddress = def_adventry->addr;
        disassoc_req_l2mh.coordAddrMode = def_adventry->addrmode;
        disassoc_req_l2mh.reason = disassoc_indc_arg->disassociateReason;
        ulpsmacaddr_long_copy(&disassoc_req_l2mh.deviceExtAddr, deviceLongAddr);
        disassoc_req_l2mh.requestorShrtAddr = ulpsmacaddr_short_node;

        // copy IE
        disassoc_req_l2mh.fco.iesIncluded = 1;

        BM_copyfrom(usm_buf, buf, buflen);

        TSCH_TiDisassociateReqL2mh(&disassoc_req_l2mh, usm_buf);
    }
#endif /* ULPSMAC_L2MH */
#endif /* IS_ROOT*/
}

#if ULPSMAC_L2MH

/**************************************************************************************************//**
 *
 * @brief       This function will hanlde the Level 2 Multi Hop association indication message
 *
 *
 *
 * @param[in]   arg -- void pointer (can be cast to tschTiAssociateIndicationL2mh_t)
 *              buf -- pointer of data buffer
 *              buflen -- number of bytes in the buffer
 *
 *
 * @return
 *
 ***************************************************************************************************/
void assoc_indc_l2mh_proc(void* arg, uint8_t* buf, uint8_t buflen)
{
    tschTiAssociateIndicationL2mh_t* assoc_indc_arg;
    ulpsmacaddr_t* deviceLongAddr;
    uint16_t requestorShrtAddr;

    ulpsmacaddr_t* srcAddr;
  
    assoc_indc_arg = (tschTiAssociateIndicationL2mh_t*) arg;
    deviceLongAddr = (ulpsmacaddr_t *)&assoc_indc_arg->deviceAddress;
    requestorShrtAddr = assoc_indc_arg->requestorShrtAddr;
    
    srcAddr = &assoc_indc_arg->srcAddr;

#if IS_ROOT
    uint16_t deviceShortAddr;
    uint8_t shrtAddrReqed;   
 
    shrtAddrReqed = assoc_indc_arg->capabilityInformation
      & ASSOC_CAPINFO_ASSIGN_ADDR_MASK;

    if(shrtAddrReqed == ASSOC_CAPINFO_ASSIGN_ADDR_TRUE)
    {
       deviceShortAddr = NHL_deviceTableNewShortAddr(deviceLongAddr);
       if (deviceShortAddr > ulpsmacaddr_null_short)
       {
         NHLDB_TSCH_PC_request_assocresp_ie(requestorShrtAddr, 
                                            deviceShortAddr,
                                            ulpsmacaddr_to_short(srcAddr));
       }
    }
#else /*IS_ROOT*/
    uint8_t srcAddrMode;
    BM_BUF_t* usm_buf = BM_alloc(26);

    if(!usm_buf)
    {
        return;
    }
    
    srcAddrMode = assoc_indc_arg->srcAddrMode;
    
    // forward it to PAN coord
    if( ulpsmacaddr_short_node != ulpsmacaddr_null_short)
    {
        tschTiAssociateReqL2mh_t assoc_req_l2mh;
        NHLDB_TSCH_NONPC_advEntry_t* def_adventry;

        if(!NHLDB_TSCH_NONPC_find_l2mhent(deviceLongAddr, requestorShrtAddr))
        {
            NHLDB_TSCH_NONPC_add_l2mhent(deviceLongAddr, requestorShrtAddr,
                                         srcAddrMode, srcAddr);
        }

        def_adventry = NHLDB_TSCH_NONPC_default_adventry();
        assoc_req_l2mh.coordPanId = def_adventry->panid;
        assoc_req_l2mh.coordAddress = def_adventry->addr;
        assoc_req_l2mh.coordAddrMode = def_adventry->addrmode;

        assoc_req_l2mh.capabilityInformation = assoc_indc_arg->capabilityInformation;
        ulpsmacaddr_long_copy(&assoc_req_l2mh.deviceExtAddr, deviceLongAddr);
        assoc_req_l2mh.requestorShrtAddr = requestorShrtAddr;

        // copy IE
        if (buflen >2)
        {
            uint8_t *ptr;
            uint16_t elemID;
            ptr = buf+MIN_PAYLOAD_IE_LEN;

            elemID = ((*ptr>>7)&0x1);
            ptr++;
            elemID +=((*ptr)&0x7F);

            if (elemID == IE_ID_TSCH_PERIODIC_SCHED)
            {
             ptr = ptr+3;
             *ptr=(*ptr+1);
            }
            BM_copyfrom(usm_buf, buf, buflen);
            assoc_req_l2mh.fco.iesIncluded = 1;
        }

        TSCH_TiAssociateReqL2mh(&assoc_req_l2mh, usm_buf);
    } else
    {
        BM_free_6tisch(usm_buf);
    }
#endif /*IS_ROOT*/
}


/**************************************************************************************************//**
 *
 * @brief       This function will hanlde the Level 2 Multi Hop disassociation indication message
 *
 *
 *
 * @param[in]   arg -- void pointer (can be cast to tschTiDisassociateIndicationL2mh_t)
 *              buf -- pointer of data buffer
 *              buflen -- number of bytes in the buffer
 *
 *
 * @return
 *
 ***************************************************************************************************/
void disassoc_indc_l2mh_proc(void* arg, uint8_t* buf, uint8_t buflen)
{
    BM_BUF_t* usm_buf;

    tschTiDisassociateIndicationL2mh_t* disassoc_indc_arg;
    ulpsmacaddr_t* deviceLongAddr;
    uint16_t requestorShrtAddr;

    uint8_t srcAddrMode;
    const ulpsmacaddr_t* srcAddr;

    usm_buf = BM_alloc(2);

    if(!usm_buf)
    {
        return;
    }


#if IS_ROOT
    tschTiDisassociateAckL2mh_t disassoc_ack_arg;

    // parse req command
    disassoc_indc_arg = (tschTiDisassociateIndicationL2mh_t*) arg;

    deviceLongAddr = &disassoc_indc_arg->deviceExtAddress;
    requestorShrtAddr = disassoc_indc_arg->requestorShrtAddr;

    srcAddrMode = disassoc_indc_arg->srcAddrMode;
    srcAddr = &disassoc_indc_arg->srcAddr;

    disassoc_ack_arg.status = DISASSOC_STATUS_ACK;

    ulpsmacaddr_long_copy(&disassoc_ack_arg.deviceExtAddr, deviceLongAddr);
    disassoc_ack_arg.requestorShrtAddr = requestorShrtAddr;

    disassoc_ack_arg.dstAddrMode = srcAddrMode;
    if(srcAddrMode == ULPSMACADDR_MODE_LONG)
    {
        ulpsmacaddr_long_copy(&disassoc_ack_arg.dstAddr, srcAddr);
    } else
    {
        ulpsmacaddr_short_copy(&disassoc_ack_arg.dstAddr, srcAddr);
    }

    // copy IE
    disassoc_ack_arg.fco.iesIncluded = 1;

    BM_copyfrom(usm_buf, buf, buflen);
    TSCH_TiDisassociateAckL2mh(&disassoc_ack_arg, usm_buf);
#else /*IS_ROOT*/

    if( ulpsmacaddr_short_node == ulpsmacaddr_null_short)
    {
        BM_free_6tisch( usm_buf);
        return;
    }

    // parse req command
    disassoc_indc_arg = (tschTiDisassociateIndicationL2mh_t*) arg;

    deviceLongAddr = &disassoc_indc_arg->deviceExtAddress;
    requestorShrtAddr = disassoc_indc_arg->requestorShrtAddr;

    srcAddrMode = disassoc_indc_arg->srcAddrMode;
    srcAddr = &disassoc_indc_arg->srcAddr;

    if(!NHLDB_TSCH_NONPC_find_l2mhent(deviceLongAddr, requestorShrtAddr))
    {
        NHLDB_TSCH_NONPC_add_l2mhent(deviceLongAddr, requestorShrtAddr,
                                     srcAddrMode, srcAddr);
    }

    {
        NHLDB_TSCH_NONPC_advEntry_t* def_adventry;
        tschTiDisassociateReqL2mh_t disassoc_req_l2mh;

        def_adventry = NHLDB_TSCH_NONPC_default_adventry();
        disassoc_req_l2mh.coordPanId = def_adventry->panid;
        disassoc_req_l2mh.coordAddress = def_adventry->addr;
        disassoc_req_l2mh.coordAddrMode = def_adventry->addrmode;

        disassoc_req_l2mh.reason = disassoc_indc_arg->reason;
        ulpsmacaddr_long_copy(&disassoc_req_l2mh.deviceExtAddr, deviceLongAddr);
        disassoc_req_l2mh.requestorShrtAddr = requestorShrtAddr;

        // copy IE
        disassoc_req_l2mh.fco.iesIncluded = 1;

        BM_copyfrom(usm_buf, buf, buflen);

        TSCH_TiDisassociateReqL2mh(&disassoc_req_l2mh, usm_buf);
    }
#endif
}
#endif /* ULPSMAC_L2MH*/


/**************************************************************************************************//**
 *
 * @brief       This function will handle COMM Status Indication
 *
 *
 * @param[in]   arg -- void pointer (can be cast to tschMlmeCommStatusIndication_t)
 *
 *
 * @return
 *
 ***************************************************************************************************/
void comm_status_indc_proc(void* arg)
{
}

void join_indication_proc(void* arg)
{
    tschMlmeCommStatusIndication_t *pindc = (tschMlmeCommStatusIndication_t *) arg;
    nhl_AssociateResponseAckIndication(pindc->dstAddrMode, &(pindc->dstAddr), pindc->reason);
}
/**************************************************************************************************//**
 *
 * @brief       This function will handle beacon notification message. It is used in channel scan.
 *
 *
 * @param[in]   arg -- void pointer (can be cast to tschMlmeBeaconNotifyIndication_t)
 *              buf -- pointer of data buffer
 *              buflen -- number of bytes in the buffer
 *
 * @return
 *
 ***************************************************************************************************/
void beacon_notify_indc_proc(void* arg, uint8_t* buf, uint8_t buflen)
{
#if !IS_ROOT
    tschMlmeBeaconNotifyIndication_t* bcn_iarg;
    NHLDB_TSCH_NONPC_advEntry_t* adv_entry;
    uint8_t i;
    uint16_t parentID;
    int8_t rssi;
    
    bcn_iarg = (tschMlmeBeaconNotifyIndication_t*) arg;
    parentID = bcn_iarg->panDescriptor.coordAddr.u8[0] 
        + (bcn_iarg->panDescriptor.coordAddr.u8[1]<<8);
    
    
    if (TSCH_MACConfig.restrict_to_node 
        && (parentID != TSCH_MACConfig.restrict_to_node))
    {
        return;
    }

    rssi = (int8_t)bcn_iarg->panDescriptor.rssi;
    if (rssi < BEACON_RX_RSSI_THRESHOLD)
    {
        return;
    }
    
    if (TSCH_MACConfig.restrict_to_pan 
       && (bcn_iarg->panDescriptor.coordPanId != TSCH_MACConfig.restrict_to_pan))
    {
        return;
    }
    
    adv_entry = NULL;
    if (is_scanning == NHL_TRUE)
    {                  
        /* find if we already have the same advertiser entry */
        adv_entry = NHLDB_TSCH_NONPC_find_adventry(bcn_iarg->panDescriptor.coordPanId,
                                                   bcn_iarg->panDescriptor.coordAddrMode,
                                                   &bcn_iarg->panDescriptor.coordAddr);

        /* no the same advertiser entry, create one */
        if (!adv_entry)
        {
             uint8_t isBlackListed = NHL_FALSE;
             for(i = 0; i < NUM_BLACK_LIST; i++)
             {
                 if ((blackList[i].isUsed == true) && 
                     (blackList[i].parentShortAddr == parentID))
                 {
                     isBlackListed = NHL_TRUE;
                     break;
                 }
             }
             
             adv_entry = NHLDB_TSCH_NONPC_new_adventry(&(bcn_iarg->panDescriptor),
                                                       buf, buflen,isBlackListed);
        }
    } else
    {
        /* find if we already have the same advertiser entry */
        adv_entry = NHLDB_TSCH_NONPC_find_adventry( bcn_iarg->panDescriptor.coordPanId,
                                                   bcn_iarg->panDescriptor.coordAddrMode,
                                                   &bcn_iarg->panDescriptor.coordAddr);
    } // else if after synch

    if (adv_entry)
    {
        if (is_scanning) {
            NHLDB_TSCH_NONPC_setup_adventry(adv_entry, &(bcn_iarg->panDescriptor),
                                            buf, buflen);
        } else {
            NHLDB_TSCH_NONPC_update_adventry(adv_entry,
                                             &(bcn_iarg->panDescriptor), buf, buflen);
        }
    }
#endif
}

/**************************************************************************************************//**
 *
 * @brief       This function start the scan process.
 *
 * @param[in]  
 *
 *
 * @return      
 *
 ***************************************************************************************************/
void start_scan(void)  //JIRA53
{
#if !IS_ROOT
    coap_msg_post_start_scan();
#endif
}

/**************************************************************************************************//**
 *
 * @brief       This function start the scan process.
 *
 * @param[in]
 *
 *
 * @return
 *
 ***************************************************************************************************/
void TSCH_start_scan_handler()  //JIRA53
{
    NHL_MlmeScanReq_t nhlMlmeScanReq;
#if DMM_ENABLE
    DMM_BleOperations((CONCURRENT_STACKS ? BLE_OPS_BEACONING : 0) | (BLE_LOCALIZATION_BEFORE_TISCH ? BLE_OPS_SCANNING : 0));
#endif //DMM_ENABLE
    NHL_setupScanArg(&nhlMlmeScanReq);
    NHL_startScan(&nhlMlmeScanReq,NULL);
}

/**************************************************************************************************//**
 *
 * @brief       This function will handle scan confirm message
 *
 * @param[in]   arg -- void pointer (can be cast to tschMlmeScanConfirm_t)
 *
 *
 * @return      
 *
 ***************************************************************************************************/
void scan_conf_proc(void* arg)
{
#if !IS_ROOT
    tschMlmeScanConfirm_t *scanArg;
    scanArg = (tschMlmeScanConfirm_t *) arg;

    if (scanArg->hdr.status == UPMAC_STAT_SUCCESS)
    {
        
        done_scan = false;
        if (NHLDB_TSCH_NONPC_num_adventry() > 0)
        {
          done_scan = true;
        } 

        if (done_scan)
        {
            NHLDB_TSCH_NONPC_advEntry_t* def_adv_entry;
            uint16_t res;

            is_scanning = NHL_FALSE;
#if DMM_ENABLE && CONCURRENT_STACKS
            restartBleBeaconReq = 1;
            DMMPolicy_updateStackState(DMMPolicy_StackType_WsnNode, DMMPOLICY_STACKSTATE_WSNNODE_SLEEPING);
#endif //DMM_ENABLE

            def_adv_entry = NHLDB_TSCH_NONPC_mark_prefer_adventry();
            res = (def_adv_entry==NULL)?STAT802154E_ERR:STAT802154E_SUCCESS;
           
            if ( res == STAT802154E_SUCCESS)
            {
              res = NHLDB_TSCH_NONPC_setup_prefer_adventry();
            }
            
            if ( res == STAT802154E_SUCCESS)
            {
                res = NHL_tschModeOn();
            }

            if (res == STAT802154E_SUCCESS)
            {
                NHLDB_TSCH_NONPC_astat_change(def_adv_entry,NHLDB_TSCH_NONPC_ASTAT_NEED_AREQ);
                
#if DISABLE_ASSOC
                ulpsmacaddr_short_node = (uint16_t)(node_mac[6]<<8)+(uint16_t)node_mac[7];
                TSCH_MlmeSetReq(TSCH_MAC_PIB_ID_macShortAddress,&ulpsmacaddr_short_node);
#if WITH_UIP6
                if (nhl_ctrl_cb)
                {
                  nhl_ctrl_cb(NHL_CTRL_TYPE_SHORTADDR, &ulpsmacaddr_short_node);
                }
#endif
#else
                invoke_astat_req();
#endif
            }
            else
            {
                done_scan = false;
            }
        }
        
        if (done_scan == false)
        {
//          uint16_t scanSleepTime;        
//          
//          if (curScanSleepInt  < SCAN_SLEEP_MAX_INTERVAL)
//            curScanSleepInt++;
//          
//          scanSleepTime = (1<<curScanSleepInt);
//          SN_clockSet(scan_sleep_ctimer, 0, scanSleepTime*CLOCK_SECOND);

          NHLDB_TSCH_NONPC_init();
          TSCH_MACConfig.restrict_to_node = 0;
          start_scan();
        }
//        else
//        {
//          curScanSleepInt = SCAN_SLEEP_MIN_INTERVAL;
//        }
    }
    else
    {
#if !DTLS_LEDS_ON
      HAL_LED_set(2,1);
#endif
      numScanErr++;
    }
#endif
}

#if !IS_ROOT

/**************************************************************************************************//**
 *
 * @brief       This function will start the assocaition porcess. It will use the selected
 *              beacon and sent the association message to PC (defined in beacon)
 *
 *
 * @param[in]   adv_entry -- pointer of NHLDB_TSCH_NONPC_advEntry_t
 *
 *
 * @return
 *
 ***************************************************************************************************/
void assoc_req_proc(NHLDB_TSCH_NONPC_advEntry_t* adv_entry)
{
    tschMlmeAssociateReq_t assocReqArg;
    BM_BUF_t* usm_buf;

    usm_buf = BM_alloc(27);

    if (!usm_buf)
    {
        return;
    }
    // BZ14397
    memset(&assocReqArg, 0, sizeof(tschMlmeAssociateReq_t));

    assocReqArg.coordAddrMode = adv_entry->addrmode;
    if (adv_entry->addrmode == ULPSMACADDR_MODE_LONG) {
        ulpsmacaddr_long_copy(&assocReqArg.coordAddress, &adv_entry->addr);
    } else {
        ulpsmacaddr_short_copy(&assocReqArg.coordAddress, &adv_entry->addr);
    }

    assocReqArg.coordPanId = adv_entry->panid;

    if (has_short_addr == NHL_FALSE) {
        // the first request
        assocReqArg.capabilityInformation = ASSOC_CAPINFO_ASSIGN_ADDR_TRUE;
    }

    if ( (iam_coord) && (has_bcn_tx_link == NHL_FALSE) )
    {
        // request bcn tx link in addition to uplink and downlink
        assocReqArg.capabilityInformation |= ASSOC_CAPINFO_DEV_TYPE_FFD;
    }
    else
    {
        // request only uplink and downlink
        assocReqArg.capabilityInformation |= ASSOC_CAPINFO_DEV_TYPE_RFD;
    }

#if !TSCH_BASIC_SCHED
    assocReqArg.capabilityInformation |= ASSOC_CAPINFO_ASSIGN_DED_LINK_TRUE;
#endif

#if 1
    NHLDB_TSCH_NONPC_create_assocreq_ie(2,2,usm_buf);
    assocReqArg.fco.iesIncluded = 1;
#endif
    assocReqArg.pIE_List = usm_buf;

    TSCH_MlmeAssociateReq(&assocReqArg);
}

static float assoc_random_backoff(float min_slotframe){
    float t=((float)rand())/RAND_MAX*(min_slotframe)+min_slotframe;
    if(t>64)t=64;
    Task_sleep((uint32_t)(CLOCK_SECOND/1000uL * 
        TSCH_ACM_timeslotLenMS() * 
        TSCH_MACConfig.slotframe_size * 
        t)
    );
    return t;
}
/**************************************************************************************************//**
 *
 * @brief       This function will handle the association confirm message. after received this message
 *              if the status is good, it will mark the association confirm and process the assigned
 *              links.
 *              if the status is not success, it will continue the association process.
 *
 *
 * @param[in]   arg -- void pointer (can be cast to tschMlmeAssociateConfirm_t)
 *              buf -- pointer of data buffer
 *              buflen -- number of bytes in the buffer
 *
 * @return
 *
 ***************************************************************************************************/
void assoc_conf_proc(void* arg, uint8_t* buf, uint8_t buflen)
{
    tschMlmeAssociateConfirm_t* confirm_arg;
    NHLDB_TSCH_NONPC_advEntry_t* adv_entry;

    uint8_t got_adv_tx_link;
    uint16_t status;

    confirm_arg = (tschMlmeAssociateConfirm_t*) arg;
    static float min_backoff = 2;

    if(confirm_arg->hdr.status != STAT802154E_SUCCESS)
    {
	if (confirm_arg->hdr.status == ASSOC_STATUS_PAN_ALT_PARENT)
        {
	    uint16_t suggestion = NHLDB_TSCH_NONPC_parse_assocrespsuggestion(buf,buflen);
            TSCH_MACConfig.restrict_to_node = suggestion;
	    if( suggestion ==0 ){
	       min_backoff += assoc_random_backoff(min_backoff);
	    }
        }
        
        TSCH_prepareDisConnect();
        TSCH_restartNode();
        return; 
    }
    min_backoff = 2;
    
    /* 0. Set node addresses, if not set yet */
    TSCH_MlmeSetReq(TSCH_MAC_PIB_ID_macShortAddress,&confirm_arg->assocShortAddress);
    ulpsmacaddr_short_node = confirm_arg->assocShortAddress;

#if WITH_UIP6
        if (nhl_ctrl_cb)
        {
            nhl_ctrl_cb(NHL_CTRL_TYPE_SHORTADDR, &ulpsmacaddr_short_node);
        }
#endif

    adv_entry = NHLDB_TSCH_NONPC_astat_req_adventry();
    NHLDB_TSCH_NONPC_astat_change(adv_entry, NHLDB_TSCH_NONPC_ASTAT_ACONFED);
        
    if ((ULPSMAC_Dbg.parent!=0) && 
        (ULPSMAC_Dbg.parent!=ulpsmacaddr_to_short(&(adv_entry->addr))))
    {
      ULPSMAC_Dbg.numParentChange++;
    }
    ULPSMAC_Dbg.parent  = ulpsmacaddr_to_short(&(adv_entry->addr));

    got_adv_tx_link = NHLDB_TSCH_NONPC_parse_assocrespie(adv_entry, buf,buflen);
    if (iam_coord && got_adv_tx_link)
    {
        status = TSCH_setBeacon(NHL_FALSE);

        if (status ==STAT802154E_SUCCESS)
        {
            NHL_startBeaconTimer();
        }

        has_bcn_tx_link = NHL_TRUE;
    }
}

#if ULPSMAC_L2MH
/**************************************************************************************************//**
 *
 * @brief       This function will handle Level 2 Multi Hop association confirm message
 *
 *
 * @param[in]   arg -- void pointer (can be cast to tschTiAssociateConfirmL2mh_t)
 *              buf -- pointer of data buffer
 *              buflen -- number of bytes in the buffer
 *
 * @return
 *
 ***************************************************************************************************/
void assoc_conf_l2mh_proc(void* arg, uint8_t* buf, uint8_t buflen)
{
    BM_BUF_t* usm_buf;

    tschTiAssociateConfirmL2mh_t* confirm_arg;
    ulpsmacaddr_t* deviceExtAddr;
    uint16_t requestorShrtAddr;
    uint16_t assocShortAddr;
 
    NHLDB_TSCH_NONPC_l2mhent_t* l2mhent;

    // parse confirm l2mh
    confirm_arg = (tschTiAssociateConfirmL2mh_t*) arg;

    deviceExtAddr = &confirm_arg->deviceExtAddr;
    requestorShrtAddr = confirm_arg->requestorShrtAddr;
    assocShortAddr = confirm_arg->assocShortAddress;

    l2mhent = NHLDB_TSCH_NONPC_find_l2mhent(deviceExtAddr, requestorShrtAddr);
    if(!l2mhent) {
        return;
    }
    // allocate the buffer for association response IE
    usm_buf = BM_alloc(28);

    if(!usm_buf)
    {
        return;
    }
    
    if(requestorShrtAddr == ulpsmacaddr_short_node)
    {
        tschMlmeAssociateResp_t assoc_response;

        memset(&assoc_response, 0, sizeof(tschMlmeAssociateResp_t));
        
        //Tao: before install new links, clean all previous links
        uint16_t linkHandle;
        uint8_t slotframeHandle;
        uint16_t delStatus=LOWMAC_STAT_SUCCESS;
        while (delStatus == LOWMAC_STAT_SUCCESS)
        {
            delStatus  = TSCH_TXM_delete_dedicated_link(assocShortAddr,&slotframeHandle,&linkHandle);
        }
        
        /* 1. remove temporary table */
        NHLDB_TSCH_NONPC_rem_l2mhent(l2mhent);
         
        /* 2. Parse and copy IE */
        if(confirm_arg->hdr.status == ASSOC_STATUS_SUCCESS)
        {          
          NHLDB_TSCH_NONPC_parse_assocrespl2mh_ie(assocShortAddr, buf, buflen);
        }
        
        BM_copyfrom(usm_buf, buf, buflen);
        ulpsmacaddr_long_copy(&assoc_response.destExtAddr, deviceExtAddr);
        assoc_response.destShortAddr = confirm_arg->assocShortAddress;
        assoc_response.status = confirm_arg->hdr.status;
        assoc_response.pIE_List = usm_buf;
        assoc_response.fco.iesIncluded = 1;
        TSCH_MlmeAssociateResp(&assoc_response);

        NHL_newTableEntry(assocShortAddr,deviceExtAddr);

#if WITH_UIP6
    rpl_instance_t* default_instance = rpl_get_default_instance();
    if (default_instance != NULL && confirm_arg->hdr.status == ASSOC_STATUS_SUCCESS)
    {    
      //only reset if success
      rpl_reset_dio_timer(default_instance);
    }
#endif
    } 
    else 
    {
        /* just forwar to the previous child node */
        tschTiAssociateRespL2mh_t assoc_response;
        
        memset(&assoc_response, 0, sizeof(tschTiAssociateRespL2mh_t));
        
        /* 1. remove temporary table */
        NHLDB_TSCH_NONPC_rem_l2mhent(l2mhent);
     
        /* 2. copy IE */
        BM_copyfrom(usm_buf, buf, buflen);
        
        ulpsmacaddr_long_copy(&assoc_response.deviceExtAddr, deviceExtAddr);
        assoc_response.deviceShortAddr = assocShortAddr;
        assoc_response.requestorShrtAddr = requestorShrtAddr;
        assoc_response.status = confirm_arg->hdr.status;

        assoc_response.dstAddrMode = l2mhent->prevNodeAddrMode;
        if(l2mhent->prevNodeAddrMode == ULPSMACADDR_MODE_LONG)
        {
            ulpsmacaddr_long_copy(&assoc_response.dstAddr, &l2mhent->prevNodeAddr);
        } else
        {
            ulpsmacaddr_short_copy(&assoc_response.dstAddr, &l2mhent->prevNodeAddr);
        }
        assoc_response.fco.iesIncluded = 1;

        TSCH_TiAssociateRespL2mh(&assoc_response, usm_buf);

    }
}

/**************************************************************************************************//**
 *
 * @brief       This function will handle Level 2 Multi Hop disassociation confirm message
 *
 *
 * @param[in]   arg -- void pointer (can be cast to tschTiDisassociateConfirmL2mh_t)
 *              buf -- pointer of data buffer
 *              buflen -- number of bytes in the buffer
 *
 * @return
 *
 ***************************************************************************************************/
void disassoc_conf_l2mh_proc(void* arg, uint8_t* buf, uint8_t buflen)
{

    tschTiDisassociateConfirmL2mh_t* confirm_arg;
    ulpsmacaddr_t* deviceExtAddr;
    uint16_t requestorShrtAddr;

    NHLDB_TSCH_NONPC_l2mhent_t* l2mhent;

    // parse confirm l2mh
    confirm_arg = (tschTiDisassociateConfirmL2mh_t*) arg;

    deviceExtAddr = &confirm_arg->deviceExtAddr;
    requestorShrtAddr = confirm_arg->requestorShrtAddr;

    l2mhent = NHLDB_TSCH_NONPC_find_l2mhent(deviceExtAddr, requestorShrtAddr);
    if(!l2mhent) {
        return;
    }

    if( requestorShrtAddr ==ulpsmacaddr_short_node) {

        if(confirm_arg->hdr.status == DISASSOC_STATUS_ACK)
        {

        } // status success

    } else {
        //just forwarding to the previous (child) node

        BM_BUF_t* usm_buf;
        tschTiDisassociateAckL2mh_t disassoc_ack_arg;

        usm_buf = BM_alloc(29);

        if(!usm_buf) {
            return;
        }

        ulpsmacaddr_long_copy(&disassoc_ack_arg.deviceExtAddr, deviceExtAddr);
        disassoc_ack_arg.requestorShrtAddr = requestorShrtAddr;
        disassoc_ack_arg.status = confirm_arg->hdr.status;

        disassoc_ack_arg.dstAddrMode = l2mhent->prevNodeAddrMode;
        if(l2mhent->prevNodeAddrMode == ULPSMACADDR_MODE_LONG) {
            ulpsmacaddr_long_copy(&disassoc_ack_arg.dstAddr, &l2mhent->prevNodeAddr);
        } else {
            ulpsmacaddr_short_copy(&disassoc_ack_arg.dstAddr, &l2mhent->prevNodeAddr);
        }

        disassoc_ack_arg.fco.iesIncluded = 1;

        BM_copyfrom(usm_buf, buf, buflen);

        TSCH_TiDisassociateAckL2mh(&disassoc_ack_arg, usm_buf);
    }

    /* remove temporary table */
    NHLDB_TSCH_NONPC_rem_l2mhent(l2mhent);

}
#endif // ULPSMAC_L2MH

//! \brief     Invoke astat request
uint8_t invoke_astat_req(void) {
    clock_time_t delay;
    NHLDB_TSCH_NONPC_advEntry_t* adv_entry;

    /*astat = advertiser stat*/
    if (Clock_isActive(astat_req_ctimer)) {
        return NHL_FALSE;
    }

    adv_entry = NHLDB_TSCH_NONPC_astat_req_adventry();

    if (adv_entry)
    {
        delay = (CLOCK_SECOND * (random_rand() % 5)) + 1;
        SN_clockSet(astat_req_ctimer, 0, delay);
    }

    return NHL_TRUE;
}

/**************************************************************************************************//**
 *
 * @brief       This function resets the blacklist parent table
 *
 * @param[in]   none
 *
 * @return      none
 *
 ***************************************************************************************************/
void NHL_resetBlackListParentTable(void)
{
    uint8_t i;
    
    for(i = 0; i < NUM_BLACK_LIST; i++)
    {
        blackList[i].isUsed = false;
        blackList[i].parentShortAddr = 0;
    }
    
    blackListIndex = 0;
}

/**************************************************************************************************//**
 *
 * @brief       This function adds the current parent to blacklist and restart
 *
 * @param[in]   parent -- parent short address
 *
 * @return      none
 *
 ***************************************************************************************************/
void NHL_addBlackListParent(uint16_t parent)
{
    /* Add to black list table*/
    blackList[blackListIndex].isUsed = true;
    blackList[blackListIndex].parentShortAddr = parent;
    blackListIndex = (blackListIndex + 1)% NUM_BLACK_LIST;
    
    /* Disconnect to rejoin network*/
    TSCH_prepareDisConnect();
    TSCH_restartNode();
}
#endif //!IS_ROOT

//! \brief    Astat request process
void astat_req_proc()  //JIRA52
{
#if !IS_ROOT
   nhl_ipc_msg_post(COMMAND_ASTAT_REQ_PROC, 0, 0, 0);
#endif
}
void NHL_astat_req_handler()  //JIRA52
{
#if !IS_ROOT

    NHLDB_TSCH_NONPC_advEntry_t* adv_entry;

    adv_entry = NHLDB_TSCH_NONPC_astat_req_adventry();

    if (!adv_entry)
    {
        return;
    }

    if (adv_entry->astat == NHLDB_TSCH_NONPC_ASTAT_NEED_AREQ)
    {
        NHLDB_TSCH_NONPC_astat_change(adv_entry,NHLDB_TSCH_NONPC_ASTAT_AREQING);
        assoc_req_proc(adv_entry);
    }
#endif
}

/**************************************************************************************************//**
 *
 * @brief       NHL layer initialization. It will init
 *              1. 6LowPan_init (UIP6 mode)
 *              2. TSCHDB command  message queue
 *              3. PC init or (non PC init)
 *              4. Device Table
 *
 * @param[in]
 *
 * @return
 *
 ***************************************************************************************************/
static void init(void)
{

#if WITH_UIP6
#ifdef LPAL_ADAPT
    lpal_initAdapt();
#endif
#endif

    NHLDB_TSCH_CMM_init();
#if IS_ROOT
    NHLDB_TSCH_PC_init();
#else
    iam_coord = (IS_INTERMEDIATE & 0x1) ? NHL_TRUE : NHL_FALSE;
    NHLDB_TSCH_NONPC_init();
#endif
    NHL_deviceTableInit();
#ifdef FEATURE_MAC_SECURITY
    macSecurityDeviceReset(0);  //reset all devices in the MAC security table
#endif
}

/**************************************************************************************************//**
 *
 * @brief       NHL layer turns the TSCH mode
 *
 * @param[in]
 *
 * @return
 *
 ***************************************************************************************************/
uint16_t NHL_tschModeOn(void)
{
    tschMlmeTschModeReq_t tschModeReq;
    tschModeReq.mode = TSCH_MODE_ON;
    // TSCH Started
    TSCH_Started = NHL_TRUE;
    return TSCH_MlmeTschModeReq(&tschModeReq);
}

/**************************************************************************************************//**
 *
 * @brief       The function starts the beacon timer.
 *
 * @param[in]
 *
 * @return
 *
 ***************************************************************************************************/
void NHL_startBeaconTimer(void)
{
     beacon_timer_started = ULPSMAC_TRUE;
}

/**************************************************************************************************//**
 *
 * @brief       This function stops the beacon timer.
 *
 * @param[in]
 *
 * @return
 *
 ***************************************************************************************************/
void NHL_stopBeaconTimer(void)
{
     beacon_timer_started = ULPSMAC_FALSE;
}

/**************************************************************************************************//**
 *
 * @brief       NHL layer sends the data packet. The data packet is in packetbuf. We need to copy to
 *              TSCH BM buffer.
 *
 *
 * @param[in]   dest_addr_mode  -- address mode of destination
 *              dest_addr -- pointer of destinationa ddress
 *
 * @param[out]
 *
 * @return
 *
 ***************************************************************************************************/
void send_packet(uint8_t dest_addr_mode, ulpsmacaddr_t* dest_addr)
{
        return;
}

#if WITH_UIP6

/**************************************************************************************************//**
 *
 * @brief       NHL layer setup the callback functions
 *              1. 6LowPan input when received data packet
 *              2. 6LowPan output (TX done)
 *              3. control callback when node joins the network
 *
 * @param[in]   ctrl_cb  -- pointer of callback function
 *
 * @param[out]
 *
 * @return
 *
 ***************************************************************************************************/
static void setup_node(nhl_ctrl_callback_t ctrl_cb)
{
    nhl_ctrl_cb = ctrl_cb;
}
#else

/**************************************************************************************************//**
 *
 * @brief       NHL layer setup the callback functions
 *              1. 6LowPan input when received data packet
 *              2. 6LowPan output (TX done)
 *              3. control callback when node joins the network
 *
 * @param[in]   recv_cb  -- pointer of  data indication
 *              sent_cb --  pointer of  data confirmation
 *
 * @param[out]
 *
 * @return
 *
 ***************************************************************************************************/
static void setup_node(nhl_recv_callback_t recv_cb, nhl_sent_callback_t sent_cb)
{
    nhl_recv_cb = recv_cb;
    nhl_sent_cb = sent_cb;
}
#endif

/**************************************************************************************************//**
 *
 * @brief       NHL layer Check if the neighbor node has dedicated links with current node
 *
 *
 * @param[in]   addr -- pointer of  MAC address of neighbor node
 *
 *
 * @param[out]
 *
 * @return      true /false
 *
 ***************************************************************************************************/
static uint8_t in_preferred_advetnry(ulpsmacaddr_t *addr) {
#if !IS_ROOT
    NHLDB_TSCH_NONPC_advEntry_t* adv_entry;

    adv_entry = NHLDB_TSCH_NONPC_find_adventry(NHLDB_TSCH_NONPC_PANID_ANY,
                                               ULPSMACADDR_MODE_SHORT, addr);
    if (!adv_entry) {
        adv_entry = NHLDB_TSCH_NONPC_find_adventry(NHLDB_TSCH_NONPC_PANID_ANY,
                                                   ULPSMACADDR_MODE_LONG, addr);
        if (!adv_entry) {
            return NHL_FALSE;
        }
    }

#if !DISABLE_ASSOC
    if (adv_entry->astat != NHLDB_TSCH_NONPC_ASTAT_ACONFED) {
        return NHL_FALSE;
    }
#endif
#endif
    return NHL_TRUE;
}

/**************************************************************************************************//**
 *
 * @brief       Adaptive share link managment. This function reduces number of share link
 *              and link frequency.
 *
 *
 * @param[in]
 *
 *
 * @param[out]
 *
 * @return
 *
 ***************************************************************************************************/
void NHL_modify_share_link(void)
{
    uint16_t linkHandle;
    uint8_t slotframeHandle;
    uint16_t status;

    NHL_stopBeaconTimer();

#if IS_ROOT
    NHLDB_TSCH_SCHED_del_sh_link();
    NHLDB_TSCH_SCHED_update_periodOffset(NHLDB_TSCH_SCHED_SH_LINK,TSCH_MACConfig.sh_period_sf);
#endif

    do
    {
       status = TSCH_TXM_delete_share_link(&slotframeHandle,&linkHandle);
    }while (status == UPMAC_STAT_SUCCESS);


   NHLDB_TSCH_mofify_adv();
   NHL_startBeaconTimer();
}

/**************************************************************************************************//**
 *
 * @brief       TSCH initialization
 *
 *
 * @param[in]
 *
 *
 * @param[out]
 *
 * @return
 *
 ***************************************************************************************************/
void TSCH_Init(void)
{
#if WITH_UIP6
    /*! Since tcpip task has lower priority, a delay is required for context
    *  switching so that uip_init() will be  called before setup_node()
    */
    Task_sleep(CLOCK_SECOND / 4);
#endif

    ulpsmac_init();
    
#if WITH_UIP6
    setup_node(ctrl_proc_cb);
#endif
}

//! \brief     NHL Driver
const struct nhl_driver nhl_tsch_driver = {
    "nhl-tsch",
    init,
    setup_node,
    send_packet,
    in_preferred_advetnry
};

void TSCH_transmit_packet_handler(uint8_t *arg)
{
   BM_BUF_t *usm_buf;
   LOWMAC_txcb_t sent_callback;
   uint8_t *ptr;

   memcpy((uint32_t *)(&usm_buf), arg, sizeof(uint32_t));
   arg += sizeof(uint32_t);
   memcpy((uint32_t *)(&sent_callback), arg, sizeof(uint32_t));
   arg += sizeof(uint32_t);
   memcpy((uint32_t *)(&ptr), arg, sizeof(uint32_t));

   TSCH_TXM_que_pkt(usm_buf, sent_callback, (void *)ptr);
}

