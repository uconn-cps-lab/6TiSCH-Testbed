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
 *  ====================== mac-config.h =============================================
 *  
 */

#ifndef __MAC_CONFIG_H__
#define __MAC_CONFIG_H__

#include <stdint.h>

#if IS_ROOT
#ifdef ENABLE_HCT
#if DeviceFamily_CC26X2
#define BM_NUM_BUFFER		      64
#define TSCH_MAX_NUM_LINKS 	      220
#else
#define BM_NUM_BUFFER		      32
#define TSCH_MAX_NUM_LINKS 	      110
#endif //DeviceFamily_CC26X2
#else //ENABLE_HCT
#define BM_NUM_BUFFER		      10
#define TSCH_MAX_NUM_LINKS 	      40
#endif
#elif IS_INTERMEDIATE
#if DeviceFamily_CC26X2
#define BM_NUM_BUFFER                 32
#define TSCH_MAX_NUM_LINKS            40
#else  //DeviceFamily_CC26X2
#define BM_NUM_BUFFER                 15
#define TSCH_MAX_NUM_LINKS 	      25
#endif  //DeviceFamily_CC26X2
#else //LEAF
#define TSCH_MAX_NUM_LINKS 	      30
#define BM_NUM_BUFFER		      20
#endif

#define TSCH_BASIC_SCHED 0
#define TSCH_PER_SCHED 1
#define ULPSMAC_L2MH 1

#define TSCH_SKIP_TS_WO_LINK   1
#define COMPRESS_SH_LINK_IE     0

#define BC_IGNORE_RX_BEACON_MASK               (0x01)   //Beacon control: ignore RX beacon
#define BC_DISABLE_TX_BEACON_MASK              (0x02)   //Beacon control: disable the TX beacon transmission

typedef struct
{
   uint16_t slotframe_size;
   uint16_t restrict_to_node;
   uint16_t keepAlive_period;
   uint16_t restrict_to_pan;
   uint16_t assoc_req_timeout_sec;
   uint16_t active_share_slot_time;
   uint16_t panID;
   uint16_t tx_power;
   uint16_t gateway_address;
   uint8_t bcn_ch_mode;
   uint8_t bcn_chan[3];
   uint8_t fixed_channel_num;
   uint8_t beacon_period_sf;
   uint8_t sh_period_sf;
   uint8_t uplink_data_period_sf;
   uint8_t downlink_data_period_sf;
   uint8_t num_shared_slot;
   uint8_t scan_interval;
   uint8_t debug_level;
   uint8_t phy_mode;
   uint8_t beaconControl;
} TSCH_MAC_CONFIG_t;

typedef struct
{
    uint8_t   keySource[8];                       /* Key source */
    uint8_t   securityLevel;                      /* Security level */
    uint8_t   keyIdMode;                          /* Key identifier mode */
    uint8_t   keyIndex;                           /* Key index */
    uint8_t   frameCounterSuppression;            /* frame cnt suppression */
    uint8_t   frameCounterSize;                   /* frame cnt size */
} macSec_t;

extern TSCH_MAC_CONFIG_t TSCH_MACConfig;

void set_defaultTschMacConfig(void);

#endif /* __MAC_CONFIG_H__ */

