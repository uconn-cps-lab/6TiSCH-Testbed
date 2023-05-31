/*
* Copyright (c) 2016 Texas Instruments Incorporated
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
 *  ====================== nhl_api.c =============================================
 *  
 */

//*****************************************************************************
// Includes
//*****************************************************************************
#include "ulpsmacaddr.h"
#include "mac_pib.h"
#include "nhldb-tsch-nonpc.h"
#include "nhl_api.h"
#include "nhl_device_table.h"
#if DMM_ENABLE
#include "dmm_scheduler.h"
#include "dmm_policy_blesp_wsnnode.h"
#endif //DMM_ENABLE

uint8_t TSCH_Started = NHL_FALSE;
uint8_t is_scanning = NHL_FALSE;

void (*NHL_dataConfCbLocal)(NHL_McpsDataConf_t *pDataConf);
void (*NHL_scanConfCbLocal)(NHL_MlmeScanConf_t *pScanConf);
void (*NHL_startPANConfCbLocal)(NHL_MlmeStartPANConf_t *pStartPANConf);

//*****************************************************************************
// Global Function
//*****************************************************************************
//! Setting Beacon
extern uint16_t TSCH_setBeacon(bool panCoordinator);
#if WITH_UIP6
extern nhl_ctrl_callback_t nhl_ctrl_cb ;
#endif



/***************************************************************************//**
 * @fn          NHL_dataReq
 *
 * @brief       This function handle data request message from application.
 *
 *              It prepares the parameter for TSCH_McpsDataReq parameter and
 *              invoke the common MAC API TSCH_McpsDataReq.
 *
 *
 * @param[in]   nhlMcpsDataReq  -- The pointer to  NHL_McpsDataReq_t structure
 *              pDataConfCall   -- The callback for data confirmation
 *
 *
 * @return      int    -- There is immediate status return if there is an issue
 *
 ******************************************************************************/
NHL_status_e NHL_dataReq(NHL_McpsDataReq_t* nhlMcpsDataReq,
                         NHL_cbFuncDataConf_p pDataConfCall)
{
    NHL_dataConfCbLocal = pDataConfCall;
    if (!nhlMcpsDataReq)
    {
      return  NHL_STAT_ERR;
    }
    TSCH_McpsDataReq(nhlMcpsDataReq);
    return NHL_STAT_SUCCESS;
}

/***************************************************************************//**
 * @fn          NHL_setPIB
 *
 * @brief       This direct execute function sets a value in the TSCH MAC PIB.
 *              The caller should reserve the enough space to hold the
 *              returned value. otherwise it will corrupt memory.
 *
 * @param[in]   pibAttribute - The attribute identifier.
 *
 * @param[out]  pValue - pointer to the passed attribute value.
 *
 * @return      Status
 *
 ******************************************************************************/
 uint16_t NHL_setPIB(uint16_t pibAttribute, void *pValue)
 {

    //! Setting the PIB values should not be allowed during the runtime for
    //! some particular PIBs. E.g., timeslot template, PANID, etc. Also,
    //! extended MAC address is READ-ONLY attribute. Leaf nodes should not
    //! attempt to set PIBs. It is difficult for now to exclude these ones
#if IS_ROOT 
    if ((!TSCH_Started) || (TSCH_Started && pibAttribute == TSCH_PIB_LINK))
    {
      return TSCH_MlmeSetReq(pibAttribute,pValue);
    }
    else
    {
      return TSCH_MAC_PIB_STATUS_CHANGE_NOT_ALLOWED;
    }
#else
    if (!TSCH_Started)
    {
      return TSCH_MlmeSetReq(pibAttribute,pValue);
    }
    else
    {
      return TSCH_MAC_PIB_STATUS_CHANGE_NOT_ALLOWED;
    }
#endif
 }

/***************************************************************************//**
 * @fn          NHL_getPIB
 *
 * @brief       This direct execute function retrieves an attribute value
 *              from the TSCH MAC PIB. The caller should reserve the enough
 *              space to hold the returned value. otherwise it will
 *              corrupt memory.
 *
 * @param[in]   pibAttribute - The attribute identifier.
 *
 * @param[out]  pValue - pointer to the returned attribute value.
 *
 * @return      Status
 *
 ******************************************************************************/
uint16_t NHL_getPIB(uint16_t pibAttribute, void *pValue)
 {
    return TSCH_MlmeGetReq(pibAttribute,pValue);
 }

/***************************************************************************//**
 * @fn          NHL_startScan
 *
 * @brief       NHL layer starts the channel scan process
 *
 * @param[in]   nhlMlmeScanReq - pointer to the scan req data
 *
 * @param[in]   pScanConfCb - scan confirmation callback
 *
 *
 * @param[out]
 *
 * @return
 *
 ******************************************************************************/
void NHL_startScan(NHL_MlmeScanReq_t* nhlMlmeScanReq,
                   NHL_cbFuncScanConf_p pScanConfCall)
{
#if DISABLE_TISCH_WHEN_BLE_CONNECTED && DMM_ENABLE && CONCURRENT_STACKS
   if (!bleConnected)
#endif //DMM_ENABLE && CONCURRENT_STACKS
   {
    TSCH_MACConfig.beaconControl &= ~(BC_IGNORE_RX_BEACON_MASK | BC_DISABLE_TX_BEACON_MASK);
    NHL_scanConfCbLocal = pScanConfCall;
    is_scanning = NHL_TRUE;
#if DMM_ENABLE && CONCURRENT_STACKS
    DMMPolicy_updateStackState(DMMPolicy_StackType_WsnNode, DMMPOLICY_STACKSTATE_WSNNODE_RX);
#endif //DMM_ENABLE
    TSCH_MlmeScanReq(nhlMlmeScanReq);
    NHL_deviceTableInit();
#ifdef FEATURE_MAC_SECURITY
    macSecurityDeviceReset(0);  //reset all devices in the MAC security table
#endif
   }
}

/***************************************************************************//**
 * @fn          NHL_startPAN
 *
 * @brief       NHL layer starts the Pand Coordinator
 *
 * @param[in]   nhlMlmeStartPANReq - pointer to the start req data
 *
 * @param[in]   pStartPANConfCb - start confirmation callback
 *
 * @param[out]
 *
 * @return
 *
 ******************************************************************************/
void NHL_startPAN(NHL_MlmeStartPAN_t* nhlMlmeStartReq,
                  NHL_cbFuncStartPANConf_p pStartPANConfCall)
{
    uint16_t status;

    NHL_startPANConfCbLocal = pStartPANConfCall;

    /* 1. Set node addresses */
    ulpsmacaddr_short_node = TSCH_MACConfig.gateway_address;

    //NHL_setPIB
    NHL_setPIB(TSCH_MAC_PIB_ID_macShortAddress, &ulpsmacaddr_short_node);
    NHL_setPIB(TSCH_MAC_PIB_ID_macPANCoordinatorShortAddress,
               &ulpsmacaddr_short_node);

#if WITH_UIP6
    nhl_ctrl_cb(NHL_CTRL_TYPE_SHORTADDR, &ulpsmacaddr_short_node);
#endif

    /* 2. Set Join Priority to zero, since I am PAN coordinator */
    {
        uint8_t join_prior = 0;
        uint16_t size = 1;

        PNM_set_db(PIB_802154E_JOIN_PRIO, &join_prior, &size);
    }


    status = NHLDB_TSCH_PC_set_def_sflink();

    if (status == STAT802154E_SUCCESS)
    {
        /* 3. create a beacon payload for Start.req*/
        status = TSCH_setBeacon(NHL_TRUE);
    }

    if (status == STAT802154E_SUCCESS)
    {
        status = NHL_tschModeOn();
    }

    if (status == STAT802154E_SUCCESS)
    {
        NHL_startBeaconTimer();
    }
}
#ifdef FEATURE_MAC_SECURITY
/***************************************************************************//**
 * @fn          NHL_setMacSecurityParameters
 *
 * @brief       Set the MAC security parameters for all the packets to be sent with encryption/authentication
 *
 * @param[in]
 *
 * @param[out]  secReq - pointer to the security parameter data structure to be filled
 *
 * @return
 *
 ******************************************************************************/
void NHL_setMacSecurityParameters(NHL_Sec_t *secReq)
{
	uint8_t frameCounterMode;
	uint8_t keySource[8] = {2, 2, 2, 2, 2, 2, 2, 2};

	secReq->securityLevel = TSCH_SEC_LEVEL;
	secReq->keyIdMode = TSCH_KEY_ID_MODE;
	secReq->keyIndex = TSCH_KEY_INDEX;
	secReq->frameCounterSuppression = TSCH_FRAMECNT_SUPPRESS;
	secReq->frameCounterSize = TSCH_FRAMECNT_SIZE;
	memcpy(secReq->keySource, keySource, 8);
	frameCounterMode = (secReq->frameCounterSize == MAC_FRAME_CNT_5_BYTES) ? 5 : 4;
	MAC_MlmeSetSecurityReq(MAC_FRAME_COUNTER_MODE, &frameCounterMode);
}
#endif

uint16_t NHL_scheduleAddSlot(uint16_t slotOffset, uint16_t channelOffset, uint8_t linkOptions, uint16_t nodeAddr)
{
    uint16_t res;
    tschMlmeSetLinkReq_t setLinkArg;

    if (slotOffset == 0xFFFF && channelOffset == 0xFFFF && nodeAddr == 0xFFFF)
    {
       //pseudo link assignment to restart the beacon transmission
       TSCH_MACConfig.beaconControl &= ~BC_DISABLE_TX_BEACON_MASK;
       return(LOWMAC_STAT_SUCCESS);
    }

    setLinkArg.operation = SET_LINK_ADD;
    setLinkArg.slotframeHandle = 0;
    setLinkArg.period = 1;
    setLinkArg.periodOffset = 0;
    setLinkArg.timeslot = slotOffset;
    setLinkArg.channelOffset = channelOffset;
    setLinkArg.nodeAddr = nodeAddr;
    setLinkArg.linkOptions = linkOptions;
    setLinkArg.linkType = MAC_LINK_TYPE_NORMAL;

    uint8_t assign_link_res = NHLDB_TSCH_assign_link_handle(0, &setLinkArg);
    
    if(assign_link_res == ASSIGN_NEW_LINK)
    {
        res = TSCH_MlmeSetLinkReq(&setLinkArg);
    }
    else if(assign_link_res == ASSIGN_LINK_EXIST)
    {
        setLinkArg.operation = SET_LINK_MODIFY;
        res = TSCH_MlmeSetLinkReq(&setLinkArg);
    }
    else
    {
        res = LOWMAC_STAT_ERR;
    }
    
    return res;
}
    
uint16_t NHL_scheduleRemoveSlot(uint16_t slotOffset, uint16_t nodeAddr)
{
    tschMlmeSetLinkReq_t setLinkArg;
    setLinkArg.operation = SET_LINK_DELETE;
    setLinkArg.slotframeHandle = 0;
    setLinkArg.period = 1;
    setLinkArg.periodOffset = 0;
    setLinkArg.linkType = MAC_LINK_TYPE_NORMAL;
    setLinkArg.timeslot = slotOffset;
    setLinkArg.nodeAddr = nodeAddr;
    
    if (slotOffset == 0xFFFF)
    {
       //pseudo slot remove to disable the TX beacon transmission
       TSCH_MACConfig.beaconControl |= BC_DISABLE_TX_BEACON_MASK;
       return(LOWMAC_STAT_SUCCESS);
    }
    else if (slotOffset == 0xFFFE) //0XFFFE means delete the node/peer
    {
       TSCH_MlmeSetLinkReq(&setLinkArg);
       return(LOWMAC_STAT_SUCCESS);
    }

    uint16_t res;
    if(NHL_TRUE == NHLDB_TSCH_find_link_handle(setLinkArg.slotframeHandle, &setLinkArg))
    {
        res = TSCH_MlmeSetLinkReq(&setLinkArg);
    }
    else
    {
        res = LOWMAC_STAT_ERR;
    }
    return res;
}
