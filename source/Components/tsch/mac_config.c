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
*  ====================== mac-config.c =============================================
*  
*/

#include "mac_config.h"
#include <string.h>

TSCH_MAC_CONFIG_t TSCH_MACConfig;

/***************************************************************************//**
* @brief      This function sets the default TSCH MAC configuration
*
* @param[in]   none
*
* @return      none
******************************************************************************/
void set_defaultTschMacConfig(void)
{
  memset(&TSCH_MACConfig, 0, sizeof(TSCH_MACConfig)); 
    
  /*For now only single beacon channel mode is supported*/
  TSCH_MACConfig.bcn_ch_mode = 1;
  
  /* beacon channel*/
  TSCH_MACConfig.bcn_chan[0] = 11;
  
  /* Scan interval given as number of beacon periods */
  TSCH_MACConfig.scan_interval = 2;
  
  /* Fixe data and command packets to specific channel for testing. 0 means
  run default channel hopping.*/
  TSCH_MACConfig.fixed_channel_num = 0;
  
  /* beacon tranmission period. unit: slotframe*/ 
  TSCH_MACConfig.beacon_period_sf   = 1;
  /* share slot wake up period. unit: slotframe */
  TSCH_MACConfig.sh_period_sf       = 1;
  /* dedicated tx slot wake up period. unit: slotframe */
  TSCH_MACConfig.uplink_data_period_sf = 1;
  /* dedicated rx slot wake up period. unit: slotframe */
  TSCH_MACConfig.downlink_data_period_sf = 1;
  
  /* Active share slot time couting from start of network time. unit: slotframe */
  TSCH_MACConfig.active_share_slot_time = 65535;  //10000 slotframes ~3.3hour
  
  /* During active share slot time,  assign this number of share slot for fast network connection. 
  After active share slot, the number of share slot will be reduced to 1 by default.*/
  TSCH_MACConfig.num_shared_slot = 4;
  
#if PHY_IEEE_MODE
  TSCH_MACConfig.slotframe_size = 251;
#elif PHY_PROP_50KBPS
  TSCH_MACConfig.slotframe_size = 59; 
#endif  
  
  /* Keep alive period.  unit: ms */
  TSCH_MACConfig.keepAlive_period = 5000;
  
  /* timeout for reception of association response.  unit: s */
  TSCH_MACConfig.assoc_req_timeout_sec = 120;
    
  /* Restrict the network connection to a specific node with the specific node 
  short address for network topology testing */
  TSCH_MACConfig.restrict_to_node = 0;           
  
  TSCH_MACConfig.panID = 0x2023;
  
  /* Restrict the network connection to a specific PAN */
  TSCH_MACConfig.restrict_to_pan = TSCH_MACConfig.panID;

  TSCH_MACConfig.tx_power = 0xFFFF;
  TSCH_MACConfig.debug_level = 0xFF;
  TSCH_MACConfig.phy_mode = 0xFF;
  TSCH_MACConfig.gateway_address = 1;
}
