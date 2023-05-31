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
 *  ====================== ulpsmac.h =============================================
 *  
 */

#ifndef __ULPSMAC_H__
#define __ULPSMAC_H__

#include "rtimer.h"

#include "nhl.h"
#include "upmac.h"
#include "lowmac.h"

#define ULPSMAC_TRUE   1
#define ULPSMAC_FALSE  0

#define ULPSMAC_ON     1
#define ULPSMAC_OFF    0

#define ULPSMAC_MAX_NUM_ADVERTISER         4

#define MIN_ETX_COUNT   (20)

#define ULPSMAC_NHL nhl_tsch_driver
#define ULPSMAC_UPMAC upmac_802154e_driver
#define ULPSMAC_LOWMAC lowmac_tsch_driver

extern const struct nhl_driver ULPSMAC_NHL;
extern const struct upmac_driver ULPSMAC_UPMAC;
extern const struct lowmac_driver ULPSMAC_LOWMAC;

void ulpsmac_init(void);

#define ULPSMAC_US_TO_TICKS(us)      ( (((uint32_t) (us)) * RTIMER_SECOND) / 1000000UL)
#define ULPSMAC_TICKS_TO_US(ticks)   ( (((uint32_t) (ticks)) * 1000000UL) / RTIMER_SECOND)
#define ULPSMAC_US_TO_CTICKS(us)     ( (((uint32_t) (us)) * CLOCK_SECOND) / 1000000UL)
#define ULPSMAC_CTICKS_TO_US(ticks)  ( (((uint32_t) (ticks)) * 1000000UL) / CLOCK_SECOND)
#define ULPSMAC_MS_TO_CTICKS(ms)     ( (((uint32_t) (ms)) * CLOCK_SECOND) / 1000UL)
#define ULPSMAC_CTICKS_TO_MS(ticks)  ( (((uint32_t) (ticks)) * 1000UL) / CLOCK_SECOND)
#define ULPSMAC_S_TO_CTICKS(s)      (((uint32_t) (s)) * CLOCK_SECOND)
#define ULPSMAC_CTICKS_TO_S(ticks)   (((uint32_t) (ticks)) / CLOCK_SECOND)

typedef struct __beacon_info
{
  uint8_t joinPriority;
  uint8_t rssi;
  uint8_t lqi;
  uint16_t shortAddress;
} BEACON_INFO_s;

typedef struct __ulpsmac_debug
{
  uint16_t numEncrypError;
  uint16_t numAcmEventPostError;
  uint16_t numAcmEventPendError; 
  uint16_t numTimerMiss;  
  uint16_t numCmdMsgAllocError;
  uint16_t numNhlEventPostError;
  uint16_t numSyncAdjust;
  uint16_t avgSyncAdjust;
  uint16_t maxSyncAdjust;
  uint16_t numParentChange;
  uint16_t numSyncLost;
  uint16_t numOutOfBuffer;
  uint16_t numTxQueueFull;
  uint16_t numCandParent;
  uint16_t scanInProg;
  uint16_t scanInvalidPar;
  uint16_t numSetChannelErr;
  uint16_t parent;
  uint32_t numTxFail;
  uint8_t numBlackListedParent;
  uint8_t avgRssi[64];
  uint8_t rssiCount[64];
  uint8_t avgRxRssi[64];        //zelin
  uint8_t rxRssiCount[64];      //zelin
  uint8_t numTxNoAckPerCh[64];
  uint8_t numTxTotalPerCh[64];
  uint32_t numTxNoAck;
  uint32_t numTxTotal;
  uint32_t numRxTotal;          //zelin
  uint32_t numTxLengthTotal;    //zelin
  uint32_t curTimeSlotEndTime;
  uint32_t nextTimeSlotStartTime;
  uint32_t numAesCcmError;
  uint32_t numBeaconOverGen;
#if !IS_ROOT
  BEACON_INFO_s beaonInfo[ULPSMAC_MAX_NUM_ADVERTISER];
#endif
} ULPSMAC_DBG_s;

extern ULPSMAC_DBG_s ULPSMAC_Dbg;


#endif /* __ULPSMAC_H__ */
