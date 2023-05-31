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
 *  ====================== nhl_device_table.h =============================================
 *  
 */

#ifndef __NHL_DEVICE_TABLE__H__
#define __NHL_DEVICE_TABLE__H__
/*******************************************************************************
 * INCLUDES
 */
#include "ulpsmacaddr.h"
#include "nhldb-tsch-sched.h"

#if IS_ROOT
#if DeviceFamily_CC26X2
#define NODE_MAX_NUM              200
#else
#define NODE_MAX_NUM              50
#endif
#elif IS_INTERMEDIATE
#define NODE_MAX_NUM              5 // jiachen: 1 parent + 4 children
#else
#define NODE_MAX_NUM              1
#endif

#define MAX_NUM_LINKS_IN_ASSOC    5

/******************************************************************************
* TYPEDEFS
*/
typedef struct __NHL_DeviceTable
{
  ulpsmacaddr_t    extAddr;
  uint16_t    shortAddr;
  uint16_t    lastMacSeqNumber;
  
#if IS_ROOT

#else
  uint32_t    rxTotal;
#endif
  
#if IS_ROOT
  uint16_t timeslot[MAX_NUM_LINKS_IN_ASSOC];
  uint8_t channelOffset[MAX_NUM_LINKS_IN_ASSOC];
  uint8_t periodOffset[MAX_NUM_LINKS_IN_ASSOC];
#endif
} NHL_DeviceTable_t;

/*******************************************************************************
 * APIs
 */
extern void NHL_deviceTableInit(void);
extern uint16_t NHL_deviceTableUpdateShortAddr(uint16_t oldShortAddr, uint16_t newShortAddr);
extern uint16_t NHL_deviceTableGetShortAddr(const ulpsmacaddr_t *longAddr);
extern ulpsmacaddr_t NHL_deviceTableGetLongAddr(uint16_t shortAddr);
extern uint16_t NHL_getTableIndex(uint16_t shortAddr);
extern void NHL_markDepartedTableEntry(uint16_t shortAddr);
extern void NHL_compactDeviceTable();


extern uint16_t NHL_deviceTableNewShortAddr(const ulpsmacaddr_t *pLongAddr);
extern uint16_t NHL_deviceTableDelAddr(const ulpsmacaddr_t *long_addr);
extern uint16_t NHL_deviceTableGetTimeSlot(uint16_t tableIndex, uint8_t linkIter);
extern uint8_t NHL_deviceTableGetChannelOffset(uint16_t tableIndex, uint8_t linkIter);
extern uint8_t NHL_deviceTableGetPeriodOffset(uint16_t tableIndex, uint8_t linkIter);
extern void NHL_deviceTableSetTimeSlot(uint16_t tableIndex, uint8_t linkIter, uint16_t timeslot);
extern void NHL_deviceTableSetChannelOffset(uint16_t tableIndex, uint8_t linkIter, uint8_t channelOffset);
extern void NHL_deviceTableSetPeriodOffset(uint16_t tableIndex, uint8_t linkIter, uint8_t periodOffset);
extern uint16_t NHL_deviceTableGetMacSeqNumber(uint16_t tableIndex);
extern void NHL_deviceTableSetMacSeqNumber(uint16_t tableIndex, uint16_t macSeqNumber);
#if IS_ROOT
extern uint32_t NHL_deviceTableGetRxTotalNumber(uint8_t tableIndex);
extern void NHL_deviceTableSetRxTotalNumber(uint8_t tableIndex, uint32_t macRxTotal);
#else
extern uint32_t NHL_deviceTableGetRxTotalNumber(uint16_t tableIndex);
extern void NHL_deviceTableSetRxTotalNumber(uint16_t tableIndex, uint32_t macRxTotal);
#endif
extern uint8_t NHL_newTableEntry(uint16_t shortAddr,const ulpsmacaddr_t *pLongAddr);
extern void NHL_deleteTableEntry(uint16_t shortAddr,const ulpsmacaddr_t *pLongAddr);


#endif
