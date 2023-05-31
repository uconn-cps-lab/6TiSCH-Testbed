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
 *  ====================== mtxm.h =============================================
 *  
 */
#ifndef __MTXM_H__
#define __MTXM_H__

#include "mac_config.h"
#include "prmtv802154e.h"
#include "pnm-db.h"

// save MAC TX packet parameters
typedef struct
{
  frame802154e_t  txFramer;
  ulpsmacaddr_t   dstAddr;
  ulpsmacaddr_t   srcAddr;
  uint8_t         macHdrLen;
} macTxPacketParam_t;

extern void MTXM_init(pnmPib_t* pnm_db_ptr);

#if ULPSMAC_L2MH
extern void MTXM_send_assoc_req_l2mh(tschTiAssociateReqL2mh_t* arg, BM_BUF_t* usm_buf);
extern void MTXM_send_assoc_resp_l2mh(tschTiAssociateRespL2mh_t* arg, BM_BUF_t* usm_buf);
extern void MTXM_send_disassoc_req_l2mh(tschTiDisassociateReqL2mh_t* arg, BM_BUF_t* usm_buf);
extern void MTXM_send_disassoc_ack_l2mh(tschTiDisassociateAckL2mh_t* arg, BM_BUF_t* usm_buf);
#endif

void MTXM_peekMacHeader(uint8_t *pMacBuf,uint16_t *pkt_type,uint16_t *dst_addr_mode,ulpsmacaddr_t *pDstAddr);
void MTXM_saveMacHeaderInfo(uint8_t *pMacBuf);
ulpsmacaddr_t *  MTXM_getPktDstAddr(void);
ulpsmacaddr_t *  MTXM_getPktSrcAddr(void);
uint8_t MTXM_getPktType(void);
uint16_t MTXM_getPktDstPanID(void);
uint8_t MTXM_getPktDstAddrMode(void);
uint8_t MTXM_getPktSrcAddrMode(void);
uint8_t MTXM_getPktAck(void);
uint8_t MTXM_getPktSeqSuppression(void);
uint8_t MTXM_getPktSeqNo(void);
uint8_t * MTXM_getBeaconASNPtr(uint8_t *pMacPayload);
uint8_t MTXM_getPktSecurityEnable(void);
uint8_t MTXM_getPktSecurityLevel(void);
frame802154e_t *MTXM_getFrameInfo(void);
uint8_t MTXM_getPktFrmCntSuppression(void);
uint8_t MTXM_getPktFrmCntSize(void);
uint8_t MTXM_getAuxHdrOffset(void);
uint8_t MTXM_getPayloadIeLen(void);
uint8_t* MTXM_getPayloadPtr(void);
uint8_t MTXM_getPayloadLen(void);
uint8_t MTXM_getPktHdrLen(void);

#endif /* __MTXM_H__ */
