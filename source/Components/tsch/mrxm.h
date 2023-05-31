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
 *  ====================== mrxm.h =============================================
 *  
 */

#ifndef __MRXM_H__
#define __MRXM_H__

#include "prmtv802154e.h"
#include "pnm-db.h"

// save MAC RX packet parameters
typedef struct
{
    uint8_t     mpduLinkQuality;
    uint8_t     dsn;
    uint32_t    timestamp;
    uint8_t     pkt_len;

    uint8_t     rssi;       /* additional information for link quality*/
    uint8_t     pktType;    /* RX MAC frame type */

    // framer
    frame802154e_t  rxFramer;
    ulpsmacaddr_t   dstAddr;
    ulpsmacaddr_t   srcAddr;

} macRxPacketParam_t;

extern void MRXM_init(pnmPib_t* pnm_db_ptr);
extern void MRXM_input(BM_BUF_t* usm_buf);

void MRXM_saveMacHeaderInfo(frame802154e_t *rxFrame);
void MRXM_saveMacPHYInfo(uint32_t timestamp,uint8_t len,uint8_t rssi,uint8_t lqi,uint8_t frameType);
ulpsmacaddr_t *  MRXM_getPktDstAddr(void);
ulpsmacaddr_t *  MRXM_getPktSrcAddr(void);
uint32_t  MRXM_getPktTimestamp(void);
uint8_t  MRXM_getPktType(void);
uint16_t  MRXM_getPktSrcPanID(void);
uint16_t  MRXM_getPktDstPanID(void);
uint8_t  MRXM_getPktDstAddrMode(void);
uint8_t  MRXM_getPktSrcAddrMode(void);
uint8_t  MRXM_getPktAck(void);
uint8_t  MRXM_getPktIEFlag(void);
uint8_t  MRXM_getPktSeqSuppression(void);
uint8_t  MRXM_getPktSeqNo(void);
uint8_t *MRXM_getHdrIePtr(void);
uint8_t MRXM_getHdrIeLen(void);
uint8_t MRXM_getPktRssi(void);
#endif /* __MRXM_H__ */
