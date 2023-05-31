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
 *  ====================== nhl_api.h =============================================
 *  
 */
#ifndef __NHL_API_H__
#define __NHL_API_H__

/***********************************************************//**
* INCLUDES
**************************************************************/

#include "ulpsmac.h"
#include "stat802154e.h"
#include "ulpsmacaddr.h"
#include "tsch_api.h"
#include "nhl.h"
#include "mac_pib.h"
#include "tsch-acm.h"
#include "pnm.h"
#include "nhldb-tsch-pc.h"
#include "nhl_mgmt.h"

/***********************************************************//**
* MACROS AND DEFINES
**************************************************************/

#ifdef FEATURE_MAC_SECURITY
#define TSCH_SEC_LEVEL                 MAC_SEC_LEVEL_ENC_MIC_32   //We only support NO MIC or MIC32 in TX.  To support MIC64 and MIC128, the MAC_MAX_PAYLOAD in sicslowpan.c must be adjusted.
//Tested: 	MAC_SEC_LEVEL_NONE,
//			MAC_SEC_LEVEL_MIC_32,
//			MAC_SEC_LEVEL_MIC_64,    //to test MIC64, the constant MAX_MAC_MIC_LEN must be redefined to 8.
//			MAC_SEC_LEVEL_MIC_128,   //to test MIC128, the constant MAX_MAC_MIC_LEN must be redefined to 16.
//			MAC_SEC_LEVEL_ENC,
//			MAC_SEC_LEVEL_ENC_MIC_32,
//			MAC_SEC_LEVEL_ENC_MIC_64,   //to test MIC64, the constant MAX_MAC_MIC_LEN must be redefined to 8.
//			MAC_SEC_LEVEL_ENC_MIC_128.  //to test MIC128, the constant MAX_MAC_MIC_LEN must be redefined to 16
#define TSCH_KEY_ID_MODE               MAC_KEY_ID_MODE_1    //We support key ID mode 1 only in TX.  To support the key ID mode 2 and 3, MAX_KEY_ID_SIZE must be changed to 5 and 9 respectively.
															//Tested: MAC_KEY_ID_MODE_3, MAC_KEY_ID_MODE_2, MAC_KEY_ID_MODE_1
															//Not tested: MAC_KEY_ID_MODE_0, KM0 requires PanID which is dynamic.  For KM0 to work
															// a method for the dynamic key distribution from the pan coordinator to the nodes is necessary
#define TSCH_KEY_INDEX                 0
#define TSCH_FRAMECNT_SUPPRESS         MAC_FRAME_CNT_NOSUPPRESS
#define TSCH_FRAMECNT_SIZE             MAC_FRAME_CNT_5_BYTES
#endif

#define NHL_McpsDataReq_t  tschMcpsDataReq_t //! Re-defining from TSCH layer
#define NHL_DataReq_t      tschDataReq_t     //! Re-defining from TSCH layer
#define NHL_McpsDataConf_t tschMcpsDataCnf_t //! Re-defining from TSCH layer
#define NHL_Sec_t          tschSec_t         //! Re-defining from TSCH layer
#define NHL_McpsDataInd_t  tschMcpsDataInd_t //! Re-defining from TSCH layer
#define NHL_DataInd_t      tschDataInd_t     //! Re-defining from TSCH layer

//Scan and Start
#define NHL_MlmeScanReq_t  tschMlmeScanReq_t
#define NHL_MlmeStartPAN_t tschMlmeStartReq_t
#define NHL_MlmeScanConf_t tschMlmeScanConfirm_t


/***********************************************************//**
* Typedefines
**************************************************************/
//! Typedef for Start Confirmation
typedef struct
{
  uint8_t status; //status of confirmation for start PAN
} NHL_MlmeStartPANConf_t;


//! Pointer function for callbacks - data confirm callback
typedef void (*NHL_cbFuncDataConf_p) (NHL_McpsDataConf_t *pDataConf);
//! Pointer function for callbacks - data indication callback
typedef void (*NHL_cbFuncDataInd_p) (NHL_McpsDataInd_t *pDataInd);

//! Pointer function for callbacks - scan confirmation
typedef void (*NHL_cbFuncScanConf_p) (NHL_MlmeScanConf_t *pScanConf);

//! Pointer function for callbacks - start confirmation
typedef void (*NHL_cbFuncStartPANConf_p)(NHL_MlmeStartPANConf_t *pStartPANConf);

/***********************************************************//**
* Global Variables
**************************************************************/
extern void (*NHL_dataConfCbLocal)(NHL_McpsDataConf_t *pDataConf);
extern void (*NHL_scanConfCbLocal)(NHL_MlmeScanConf_t *pScanConf);

/***********************************************************//**
* FUNCTIONS
**************************************************************/
//! function/API for data request
NHL_status_e NHL_dataReq(NHL_McpsDataReq_t *params,
                          NHL_cbFuncDataConf_p pDataConfCall);
//! Defining callback function for data indication
void NHL_dataIndCb(NHL_McpsDataInd_t *pDataInd);

uint16_t NHL_setPIB(uint16_t pibAttribute, void *pValue);//! Set PIB API
uint16_t NHL_getPIB(uint16_t pibAttribute, void *pValue);//! Get PIB API

void NHL_startScan(NHL_MlmeScanReq_t* nhlMlmeScanReq,
                   NHL_cbFuncScanConf_p pScanConfCall);

void NHL_startPAN(NHL_MlmeStartPAN_t* nhlMlmeStartReq,
                  NHL_cbFuncStartPANConf_p pStartPANConfCall);


//----Scheduling Interface
uint16_t NHL_scheduleAddSlot(uint16_t slotOffset, uint16_t channelOffset, uint8_t linkOptions, uint16_t nodeAddr);
uint16_t NHL_scheduleRemoveSlot(uint16_t slotOffset, uint16_t nodeAddr);

#ifdef FEATURE_MAC_SECURITY
void NHL_setMacSecurityParameters(NHL_Sec_t *secReq);
#endif
#endif /*__NHL_API_H__*/
