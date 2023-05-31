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
 *  ====================== framer-802154e.c =============================================
 *  
 */
#include <string.h>

#include "ulpsmacaddr.h"

#include "framer-802154e.h"

#include "ie802154e.h"
#include "mrxm.h"

#ifdef FEATURE_MAC_SECURITY
#include <ti/drivers/Power.h>
#include "bsp_aes.h"
#include "mac_tsch_security.h"
#include "nhl_device_table.h"
#include "mac_pib.h"
#include "nhl_api.h"

unsigned char *pKeyTx = NULL;
unsigned char *pKeyRx = NULL;
#endif


uint8_t framer_802154e_parseHdr(BM_BUF_t* prx_buf, frame802154e_t *frame_p)
{
   uint16_t len;

   len = BM_getDataLen(prx_buf);

   len = frame802154e_parse_header(BM_getDataPtr(prx_buf), len, frame_p);

   // save the parsed MAC header info
   MRXM_saveMacHeaderInfo(frame_p);

   return len;
}

uint8_t framer_802154e_parsePayload(BM_BUF_t* prx_buf, uint16_t len, frame802154e_t *frame_p)
{
#ifdef FEATURE_MAC_SECURITY
   uint8_t authlen;
   unsigned char *pAData, *pCData;
   uint8_t aDataLen, cDataLen;
   uint8_t open_payload_len;
   ulpsmacaddr_t extAddr;
   uint16_t srcAddr = 0;
   uint8_t securityLevel;
   struct structDeviceDescriptor *devDesc_p;
   uint8_t nonce[13];

   securityLevel = frame_p->aux_hdr.security_control.security_level;

   if(frame_p->fcf.security_enabled && securityLevel)
   {
      if(MRXM_getPktSrcAddrMode() == MAC_ADDR_MODE_SHORT)
      {
         srcAddr = (frame_p->src_addr[0]) | (frame_p->src_addr[1] << 8);
         extAddr = NHL_deviceTableGetLongAddr(srcAddr);
      }
      else
      {
         ulpsmacaddr_long_copy(&extAddr, MRXM_getPktSrcAddr());
      }

      if(ulpsmacaddr_long_cmp(&extAddr, &ulpsmacaddr_null))
      {
         return 0;
      }

      if((macIncomingFrameSecurity(srcAddr, frame_p, &pKeyRx, &devDesc_p)) != MAC_SUCCESS)
      {
         return 0;
      }

      authlen = securityLevel & 3;
      authlen = (authlen * 4 + ((authlen + 1) >> 2) * 4);

      switch(frame_p->fcf.frame_type)
      {
         case MAC_FRAME_TYPE_COMMAND:
            open_payload_len = 1;
            break;

         default:
            open_payload_len = 0;
            break;
      }

      /* Initialize secured payload pointer and length */
      pAData = BM_getDataPtr(prx_buf);
      aDataLen = len + frame_p->payloadie_len + open_payload_len;
      pCData = pAData + aDataLen;
      cDataLen = BM_getDataLen(prx_buf) + BM_getDataOffset(prx_buf) - aDataLen;

      if(aDataLen + cDataLen + 2 > 127)
      {
         return 0;
      }

      memcpy(nonce, extAddr.u8, 8);
      {
    	  uint32_t frameCounter = frame_p->aux_hdr.frame_counter;
    	  int i;
    	   for (i = 11; i >= 8; i--)
    	   {
    	      nonce[i] = frameCounter & 0xFF;
    	      frameCounter >>= 8;
    	   }
      }

      if (frame_p->aux_hdr.security_control.frame_counter_size == MAC_FRAME_CNT_5_BYTES)
      {
    	  nonce[12] = frame_p->aux_hdr.frame_counter_ext;
      }
      else
      {
    	  nonce[12] = securityLevel;
      }

      if ((macCcmStarInverseTransform( pKeyRx, authlen, nonce, pCData, cDataLen, pAData, aDataLen )) != MAC_SUCCESS)
      {
         return 0;
      }

      if (frame_p->aux_hdr.frame_counter < 0xFFFFFFFF)
      {
         devDesc_p->FrameCounter = frame_p->aux_hdr.frame_counter + 1;
      }
      else if (frame_p->aux_hdr.security_control.frame_counter_size == MAC_FRAME_CNT_5_BYTES && frame_p->aux_hdr.frame_counter_ext < 0xFF)
      {
         devDesc_p->FrameCounter = 0;
         devDesc_p->FrameCounterExt = frame_p->aux_hdr.frame_counter_ext + 1;
      }

      BM_setDataLen(prx_buf, BM_getDataLen(prx_buf) - authlen);
   }

#endif

   // adjust the data offset to remove the MAC header
   if (BM_incDataOffset(prx_buf, len) != BM_SUCCESS)
     return 0;
   
   return len;
}


uint8_t framer_802154e_createHdr(BM_BUF_t* ptx_buf, frame802154e_t* frame)
{

  uint8_t len;
  uint8_t payload_len;
  uint8_t *pdata;
  uint8_t *pTemp;
  BM_BUF_t *pBuf;

#ifdef FEATURE_MAC_SECURITY
   uint8_t securityEnabled = 1;
  uint8_t securityLevel;

  TSCH_MlmeGetReq(MAC_SECURITY_ENABLED, &securityEnabled);

  securityLevel = frame->aux_hdr.security_control.security_level;

  if(frame->fcf.security_enabled)
  {
    if(securityLevel == MAC_SEC_LEVEL_NONE)
    {
       frame->fcf.security_enabled = 0;
       return MAC_UNSUPPORTED_SECURITY;
    }

    if((!securityEnabled) && (securityLevel > MAC_SEC_LEVEL_NONE))
    {
       frame->fcf.security_enabled = 0;
       return MAC_UNSUPPORTED_SECURITY;
    }

    frame->aux_hdr.frame_counter = 0xBEBEBEBE;
    frame->aux_hdr.frame_counter_ext = 0xBE;
  }

#endif

  pBuf = BM_alloc(3);
  if (pBuf == NULL)
  { // working buffer is not available
    return 0;
  }
  pTemp = BM_getBufPtr(pBuf);

  // copy pay to temp buffer
  // at this moment, data offset =0,
  payload_len = BM_getDataLen(ptx_buf);
  BM_copyto(ptx_buf, pTemp);

  len = frame802154e_hdrlen(frame);
  pdata = BM_getDataPtr(ptx_buf);

  // generate the MAC header (to overrid the original payload)
  frame802154e_create(frame, pdata,len);

  // append original payload back to buffer
  pdata += len;

  if (payload_len + len <= BM_BUFFER_SIZE)
  {
     memcpy(pdata,pTemp,payload_len);  //Jira 44, done
  }
  else
  {
#if DEBUG_MEM_CORR
     volatile int errloop = 0;
     while(1)
     {
        errloop++;
     }
#else
     BM_free_6tisch(pBuf);
     return 0;
#endif
  }

  // set the data offset and length
  BM_setDataOffset(ptx_buf,0);
  BM_setDataLen(ptx_buf, len+ payload_len);

  BM_free_6tisch(pBuf);

  return len;

}

uint8_t framer_802154e_createAckPkt(BM_BUF_t * pBufAck,uint8_t ie_inc)
{
  frame802154e_t frame;
  uint8_t len;
  
  memset(&frame,0,sizeof(frame));

  frame.fcf.frame_type = MAC_FRAME_TYPE_ACK;
  frame.fcf.security_enabled = 0;  // Not yet supported
  frame.fcf.frame_pending = 0;
  frame.fcf.ack_required = 0;
  frame.fcf.panid_compression = 1;

  frame.fcf.seq_num_suppress  = MRXM_getPktSeqSuppression();
  if (frame.fcf.seq_num_suppress == 0 )
  {
    frame.seq = MRXM_getPktSeqNo();
  }
  frame.fcf.ie_list_present = ie_inc;
  frame.fcf.dest_addr_mode = MRXM_getPktSrcAddrMode();
  frame.fcf.src_addr_mode = MRXM_getPktDstAddrMode();
  frame.dest_pid  = MRXM_getPktSrcPanID();
  frame.src_pid   = MRXM_getPktDstPanID();
  
  if ((frame.fcf.dest_addr_mode == ULPSMACADDR_MODE_LONG) &&
    (frame.fcf.src_addr_mode == ULPSMACADDR_MODE_LONG))
    frame.fcf.panid_compression = 0;

  if (frame.fcf.dest_addr_mode == MAC_ADDR_MODE_EXT){
    ulpsmacaddr_long_copy((ulpsmacaddr_t *)&frame.dest_addr, MRXM_getPktSrcAddr());
  } else if(frame.fcf.dest_addr_mode == MAC_ADDR_MODE_SHORT) {
    ulpsmacaddr_short_copy((ulpsmacaddr_t *)&frame.dest_addr, MRXM_getPktSrcAddr());
  }

  if (frame.fcf.src_addr_mode == MAC_ADDR_MODE_EXT){
    ulpsmacaddr_long_copy((ulpsmacaddr_t *)&frame.src_addr, MRXM_getPktDstAddr());
  } else if (frame.fcf.src_addr_mode == MAC_ADDR_MODE_SHORT) {
    ulpsmacaddr_short_copy((ulpsmacaddr_t *)&frame.src_addr, MRXM_getPktDstAddr());
  }


  len = framer_802154e_createHdr(pBufAck, &frame);

  return len;
}

uint8_t framer_802154e_createKaPkt(BM_BUF_t * pBufKa, ulpsmacaddr_t* dst_ulpsmacaddr)
{
    frame802154e_t frame;
    uint8_t len;
    
    memset(&frame,0,sizeof(frame));

    frame.fcf.frame_type = MAC_FRAME_TYPE_DATA;
    framer_genFCF(&frame,0,1,ulpsmacaddr_panid_node,
                    MAC_ADDR_MODE_SHORT,dst_ulpsmacaddr,MAC_ADDR_MODE_SHORT,1);

  len = framer_802154e_createHdr(pBufKa, &frame);

  return len;
}

/**************************************************************************************************
 * @fn          framer_genFCF
 *
 * @brief       This function will generate the MAC FCF. Caller must set up the frame type
 *
 * input parameters
 *
 * @param       pframe      - the pointer to FCF data structure
 *              ieInclude   - IEinclude flag
 *              sn_suppreesion - sequence number suppression
 *              destPanID   - destination PAN ID
 *              dstAddrMode - destination address mode
 *              pDstAddr    - pointer to destination address
 *              srcAddrMode - source address mode
 *              panidComp  - pan ID compression
 *
 * output parameters
 *              pframe      - update the FCF data structure
 * None.
 *
 * @return       0 success
 *               other failure
 **************************************************************************************************
 */
uint16_t framer_genFCF(frame802154e_t *pframe,uint8_t ieInclude,uint8_t sn_suppression,uint16_t dstPanID,
                                     uint8_t dstAddrMode,const ulpsmacaddr_t *pDstAddr, uint8_t srdAddrMode, uint8_t panidComp)
{
    frame802154e_fcf_t *pFCF;

    pFCF = &(pframe->fcf);

    // set up command field for all MAC command
    pFCF->security_enabled = 0;
    pFCF->frame_pending = 0;
    pFCF->ack_required = 1;
    
    pFCF->panid_compression = panidComp;
    pFCF->seq_num_suppress = sn_suppression;
    pFCF->ie_list_present = ieInclude;

    // set up destination address
    pFCF->dest_addr_mode = dstAddrMode;
    if (dstAddrMode == MAC_ADDR_MODE_EXT)
    {
        ulpsmacaddr_long_copy((ulpsmacaddr_t *)&(pframe->dest_addr), pDstAddr);
    }
    else if (dstAddrMode == MAC_ADDR_MODE_SHORT)
    {
        ulpsmacaddr_short_copy((ulpsmacaddr_t *)&(pframe->dest_addr),pDstAddr);
    }
    else
    {
        return UPMAC_STAT_ERR;
    }

    // set up the source address
    pFCF->src_addr_mode = srdAddrMode;

    if (srdAddrMode == MAC_ADDR_MODE_EXT)
    {
        ulpsmacaddr_long_copy((ulpsmacaddr_t *)&(pframe->src_addr), &ulpsmacaddr_long_node);
    }
    else if (srdAddrMode == MAC_ADDR_MODE_SHORT)
    {
        //ulpsmacaddr_short_copy((ulpsmacaddr_t *)&(pframe->src_addr), &ulpsmacaddr_short_node);
        ulpsmacaddr_from_short((ulpsmacaddr_t *)&(pframe->src_addr), TSCH_MAC_Pib.shortAddress);
    }
    else
    {
        return UPMAC_STAT_ERR;
    }

    // destination PAN ID
    pframe->dest_pid = dstPanID;


    // source PAN ID
    pframe->src_pid = TSCH_MAC_Pib.panId;

    if (pFCF->seq_num_suppress == 0)
    {
        if (pFCF->frame_type == MAC_FRAME_TYPE_BEACON)
        {
            TSCH_MAC_Pib.bsn++;
            pframe->seq = TSCH_MAC_Pib.bsn;

        }
        else
        {   // data or command frame
            TSCH_MAC_Pib.dsn++;
            pframe->seq = TSCH_MAC_Pib.dsn;
        }
    }

    return UPMAC_STAT_SUCCESS;

}

#ifdef FEATURE_MAC_SECURITY
/**************************************************************************************************
 * @fn          framer_genSecHdr
 *
 * @brief       This function will generate the MAC Security Header.
 *
 * input parameters
 *
 * @param   pframe      - the pointer to FCF data structure
            secHdr      - the pointer to security header structure

 *
 * output parameters
 *              status      - status of processing
 * None.
 *
 * @return       0 success
 *               other failure
 **************************************************************************************************
 */
uint16_t framer_genSecHdr(frame802154e_t *pframe, macSec_t *secHdr)
{
  if(!secHdr->securityLevel)
   {
      pframe->fcf.security_enabled = 0;
   }
  else
  {
    pframe->fcf.security_enabled = 1;
    pframe->aux_hdr.security_control.security_level = secHdr->securityLevel;
    pframe->aux_hdr.security_control.key_id_mode = secHdr->keyIdMode;
    pframe->aux_hdr.security_control.frame_counter_suppression = secHdr->frameCounterSuppression;
    pframe->aux_hdr.security_control.frame_counter_size = secHdr->frameCounterSize;
    pframe->aux_hdr.security_control.reserved = 0;
    pframe->aux_hdr.frame_counter_ext = 0;
    pframe->aux_hdr.frame_counter = 0;
    if(secHdr->keyIdMode != MAC_KEY_ID_MODE_NONE)
    {
         int size = (secHdr->keyIdMode == MAC_KEY_ID_MODE_2) ? MAC_KEY_ID_4_LEN - 1 : MAC_KEY_ID_8_LEN - 1;

      if(secHdr->keyIdMode != MAC_KEY_ID_MODE_1)
         {
            memcpy(pframe->aux_hdr.key, secHdr->keySource, size);
         }

      pframe->aux_hdr.key[(secHdr->keyIdMode-1)<<2] = secHdr->keyIndex; //key index
    }
  }
  return UPMAC_STAT_SUCCESS;
}
#endif

