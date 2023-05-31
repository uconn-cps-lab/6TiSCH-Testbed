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
 *  ====================== prepare_api.c =============================================
 *  
 */

//*****************************************************************************
// Includes
//*****************************************************************************
#include "nhl_setup.h"
#include "mac_config.h"

//*****************************************************************************
// Functions
//*****************************************************************************

/***************************************************************************//**
 * @fn          NHL_setupScanArg
 *
 * @brief       NHL layer set up channel scan parameters.
 *              The updated value is in the pointer
 *
 * @param[in]
 *
 * @param[out]  scanReqArg -- pointer of NHL_MlmeScanReq_t
 *
 * @return
 *
 ******************************************************************************/
void NHL_setupScanArg(NHL_MlmeScanReq_t* scanReqArg)
{
    scanReqArg->scanType = SCAN_TYPE_PASSIVE;

    memset(scanReqArg->scanChannels,0,SCAN_CHANNELS_LEN);

    if (TSCH_MACConfig.bcn_ch_mode == 0xff)
    {
       scanReqArg->scanChannels[0]=HAL_RADIO_MIN_CHANNEL;

    }
    else if (TSCH_MACConfig.bcn_ch_mode == 3)
    {
        scanReqArg->scanChannels[0] = TSCH_MACConfig.bcn_chan[0];
        scanReqArg->scanChannels[1] = TSCH_MACConfig.bcn_chan[1];
        scanReqArg->scanChannels[2] = TSCH_MACConfig.bcn_chan[2];
    }
    else if (TSCH_MACConfig.bcn_ch_mode == 1)
    {
        scanReqArg->scanChannels[0] = TSCH_MACConfig.bcn_chan[0];
    }

    scanReqArg->scanDuration = TSCH_ACM_timeslotLenMS() * TSCH_MACConfig.slotframe_size
                *(TSCH_MACConfig.beacon_period_sf*TSCH_MACConfig.scan_interval+1)*TSCH_MACConfig.bcn_ch_mode/1000;
}

/***************************************************************************//**
 * @fn          NHL_setupStartArg
 *
 * @brief       NHL layer set up network start parameters.
 *              The updated value is in the pointer
 *
 * @param[in]
 *
 * @param[out]  scanStartArg -- pointer of NHL_MlmeStartReq_t
 *
 * @return
 *
 ******************************************************************************/
void NHL_setupStartArg(NHL_MlmeStartPAN_t* scanStartArg)
{
  //What to consider:
  // PANID
  // Beacon Channel
  // Security?
  // Others?
  return;
}
