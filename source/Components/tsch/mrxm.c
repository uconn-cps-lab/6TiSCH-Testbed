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
 *  ====================== mrxm.c =============================================
 *  
 */
#include "sys/clock.h"

#include "mac_config.h"
#include "ulpsmacbuf.h"
#include "mac_pib.h"

#include "framer-802154e.h"
#include "mrxm.h"
#ifdef FEATURE_MAC_SECURITY
//#include "mac_tsch_security_pib.h"
#endif
#include "tsch-acm.h"

#if !IS_ROOT
#include "nhldb-tsch-nonpc.h"
#include "nhl_device_table.h"
#endif

macRxPacketParam_t macRxPktParam;

/*---------------------------------------------------------------------------*/
static pnmPib_t* mrxm_pnmdb_ptr;

extern void TSCH_TXM_freeDevPkts(uint8_t addrMode, ulpsmacaddr_t *pDstAddr);

/**************************************************************************************************//**
 *
 * @brief       This function initializes the mrxm_pnmdb_ptr handler. It will point to PNM DB
 *
 * @param[in]
 *
 * @return
 *
 ***************************************************************************************************/

void MRXM_init(pnmPib_t* pnm_db_ptr)
{
  mrxm_pnmdb_ptr = pnm_db_ptr;
  mrxm_pnmdb_ptr = mrxm_pnmdb_ptr;  //Get rid of the compiler warning
}


/**************************************************************************************************//**
 *
 * @brief       This function processs the input data packet.It prepares the data indication
 *              callback parameters and invoke the callback function.
 *              Current implementation does not support IE in data packet, so the IEList pointer is
 *              set to NULL.
 *
 * @param[in]   usm_buf -- the BM buffer pointer to input data packet
 *
 * @return
 *
 ***************************************************************************************************/
static void input_data_packet(BM_BUF_t* usm_buf)
{
    tschMcpsDataInd_t dataind;
    tschDataInd_t     *pmac;
    tschFrmCtrlOpt_t  *pfco;
    ulpsmacaddr_t *srcAddr, *dstAddr;

    pmac = &(dataind.mac);
    pfco = &(pmac->fco);

    pfco->iesIncluded = MRXM_getPktIEFlag();
    /// no IE list
    pmac->pIEList = NULL;

    pfco->seqNumSuppressed = MRXM_getPktSeqSuppression();
    if (pfco->seqNumSuppressed == 0 )
    {
        pmac->dsn = MRXM_getPktSeqNo();
    }

    pmac->srcAddrMode = MRXM_getPktSrcAddrMode();
    srcAddr = MRXM_getPktSrcAddr();

    // source address
    if (pmac->srcAddrMode == MAC_ADDR_MODE_EXT)
    {
        ulpsmacaddr_long_copy(&pmac->srcAddr, srcAddr);
    }
    else if (pmac->srcAddrMode == MAC_ADDR_MODE_SHORT)
    {
        ulpsmacaddr_short_copy(&pmac->srcAddr, srcAddr);
    }
    else
    {
        pmac->srcAddrMode = 0;
    }

    pmac->dstAddrMode = MRXM_getPktDstAddrMode();
    dstAddr = MRXM_getPktDstAddr();
    // destination address
    if (pmac->dstAddrMode == MAC_ADDR_MODE_EXT)
    {
        ulpsmacaddr_long_copy(&pmac->dstAddr, dstAddr);
    }
    else if (pmac->dstAddrMode == MAC_ADDR_MODE_SHORT)
    {
        ulpsmacaddr_short_copy(&pmac->dstAddr, dstAddr);
    }
    else
    {
        pmac->dstAddrMode = 0;
    }

    // MAC security infomation
    // assume there is no security
    memset(&(dataind.sec), 0x00,sizeof(dataind.sec));

    // RSSI and LQI
    pmac->rssi = macRxPktParam.rssi;
    pmac->mpduLinkQuality = macRxPktParam.mpduLinkQuality;

    /* convert clock_time_t to rtimer_clock_t */
    pmac->timestamp = macRxPktParam.timestamp * (RTIMER_SECOND / CLOCK_SECOND ) -
                      HAL_RADIO_DURATION_TICKS(macRxPktParam.pkt_len);

    if (McpsCb)
    {
        dataind.hdr.event = MAC_MCPS_DATA_IND;
        usm_buf->recv_asn = (uint16_t)TSCH_ACM_get_ASN(); // jiachen
        McpsCb((tschCbackEvent_t *)(&dataind), usm_buf);
    }

}

/**************************************************************************************************//**
 *
 * @brief       This function processs the input beacon packet. It prepares the beacon inidction
 *              callback parameters and invoke the callback function.
 *              Current implementation does not support IE in data packet, so the IEList pointer is
 *              set to NULL.
 *
 * @param[in]   usm_buf -- the BM buffer pointer to input beacon data packet
 *
 *
 * @return
 *
 ***************************************************************************************************/
static void input_beacon_packet(BM_BUF_t* usm_buf)
{
  tschMlmeBeaconNotifyIndication_t bcn_indc;
  
  bcn_indc.fco.iesIncluded = MRXM_getPktIEFlag();
  bcn_indc.fco.seqNumSuppressed = MRXM_getPktSeqSuppression();
  if (bcn_indc.fco.seqNumSuppressed == 0)
  {
    bcn_indc.bsn = MRXM_getPktSeqNo();
  }
  
  bcn_indc.panDescriptor.coordPanId = MRXM_getPktSrcPanID();
  bcn_indc.panDescriptor.coordAddrMode = MRXM_getPktSrcAddrMode();
  
  ulpsmacaddr_copy(bcn_indc.panDescriptor.coordAddrMode, &bcn_indc.panDescriptor.coordAddr, MRXM_getPktSrcAddr());
  
#ifdef FEATURE_MAC_SECURITY
  //TO DO : store short address of panCoordinator and panID into device table
  if(bcn_indc.panDescriptor.coordAddrMode == ULPSMACADDR_MODE_SHORT)
    MAC_MlmeSetSecurityReq(MAC_PAN_COORD_SHORT_ADDRESS, (uint8_t*)&bcn_indc.panDescriptor.coordAddr);
  else if(bcn_indc.panDescriptor.coordAddrMode == ULPSMACADDR_MODE_LONG)
    MAC_MlmeSetSecurityReq(MAC_PAN_COORD_EXTENDED_ADDRESS, (uint8_t*)&bcn_indc.panDescriptor.coordAddr);
  else
    ; //nothing to do
#endif
  
  bcn_indc.panDescriptor.rssi = macRxPktParam.rssi;
  bcn_indc.panDescriptor.linkQuality = macRxPktParam.mpduLinkQuality;
  bcn_indc.panDescriptor.timestamp = macRxPktParam.timestamp -
    HAL_RADIO_DURATION_TICKS(macRxPktParam.pkt_len);
  
  if (MlmeCb)
  {
    bcn_indc.hdr.event = MAC_MLME_BEACON_NOTIFY_IND;
    MlmeCb((tschCbackEvent_t *)(&bcn_indc), usm_buf);
  }
}

/**************************************************************************************************//**
 *
 * @brief       This function processs the input packet frame control information. It is shared
 *              by beacon, MAC data and command packet. It will check IE inclusion.
 *
 * @param[in]   usm_buf         -- the BM buffer pointer to input MAC packet
 *              payloadie_len   -- the length of payload IE
 *              pfco            -- pointer to Command Frame Control option
 *
 * @return
 *
 ***************************************************************************************************/
void MRXM_procCmdPktFCO(BM_BUF_t*usm_buf,const uint8_t payloadie_len,tschCmdFrmCtrlOpt_t *pfco)
{
    // set up the fco option
    if (payloadie_len)
    {
        pfco->iesIncluded = 1;
    }
    else
    {
        pfco->iesIncluded = 0;
    }

    // set up the buffer length only include IE

    BM_setDataLen(usm_buf,payloadie_len);
}

/**************************************************************************************************//**
 *
 * @brief       This function processs the input MAC command frames. control information. Based on
 *              command type, it will invoke the corresponding callback functions.
 *
 * @param[in]   usm_buf -- the BM buffer pointer to input data packet
 *
 * @return
 *
 ***************************************************************************************************/
static void input_command_packet(BM_BUF_t* usm_buf)
{
  uint8_t* buf_ptr;
  uint8_t  payload_ie_len;
  ulpsmacaddr_t *rxSrcAddr;

  // Jump to CFI
  payload_ie_len = 0;

  if (MRXM_getPktIEFlag())
  {
    payload_ie_len = ie802154e_payloadielist_len(BM_getDataPtr(usm_buf),BM_getDataLen(usm_buf));
  }
  //datalen = ulpsmacbuf_datalen(usm_buf);
  rxSrcAddr = MRXM_getPktSrcAddr();

  buf_ptr = BM_getDataPtr(usm_buf)+ payload_ie_len;

  switch(buf_ptr[0])
  {
    case MAC_COMMAND_ASSOC_REQ:
    {
      tschMlmeAssociateIndication_t assocind;

      // TODO: Should deal with multihop case
      //if (!mrxm_pnmdb_ptr->is_coord) {}

      // buf_ptr[0] is CFI
      assocind.capabilityInformation = buf_ptr[1];

      /* assoc request always uses long address for src address */
      ulpsmacaddr_long_copy(&assocind.deviceAddress, MRXM_getPktSrcAddr());

      if (payload_ie_len)
      {
         BM_setDataLen(usm_buf,payload_ie_len);
      }

      if (MlmeCb)
      {
        assocind.hdr.event =  MAC_MLME_ASSOCIATE_IND;
        MlmeCb((tschCbackEvent_t *)(&assocind),usm_buf);
      }
      break;
    }
    case MAC_COMMAND_ASSOC_RESP:
    {
#if !IS_ROOT
      tschMlmeAssociateConfirm_t assocconf;

      if( (mrxm_pnmdb_ptr->doingAssocReq == 1) &&
          (Clock_isActive(assoc_req_ctimer))) {
        // only the case where assoc req inited and timer started
        // we can confirm that we are waiting for assoc response
    	Clock_stop(assoc_req_ctimer);
        mrxm_pnmdb_ptr->doingAssocReq = 0;
        //printf("mrxm:stop assoc_ct\n");

        // buf_ptr[0] is CFI
        // BZ14395
        assocconf.assocShortAddress = buf_ptr[1] + (buf_ptr[2]<<8);

        assocconf.hdr.status = buf_ptr[3];
        // save the PAN coordinator extended address for MAC secuirty API
        if (assocconf.hdr.status == 0)
        {   // association is success
            if ( MRXM_getPktSrcAddrMode() == MAC_ADDR_MODE_EXT)
            {   // save the extended address to PIB
                uint16_t shortAddr;
                NHLDB_TSCH_NONPC_advEntry_t *defaultAdv;

                defaultAdv = NHLDB_TSCH_NONPC_default_adventry();
                shortAddr = ulpsmacaddr_to_short(&(defaultAdv->addr));

                NHL_newTableEntry(shortAddr,rxSrcAddr);

                TSCH_MlmeSetReq(TSCH_MAC_PIB_ID_PAN_COORDINATOR_EUI,rxSrcAddr->u8);

#ifdef FEATURE_MAC_SECURITY
                //Device Table Update for Security
                {
                  uint16_t panCoordShortAddress;
                  MAC_MlmeSetSecurityReq(MAC_PAN_COORD_EXTENDED_ADDRESS, (uint8_t *)rxSrcAddr);
                  MAC_MlmeGetSecurityReq(MAC_PAN_COORD_SHORT_ADDRESS, (uint8_t*)&panCoordShortAddress);
                  MAC_SecurityDeviceTableUpdate(MRXM_getPktSrcPanID(), &panCoordShortAddress, rxSrcAddr);
                  MAC_SecurityKeyIdUpate(MRXM_getPktSrcPanID(), (uint8_t *)&panCoordShortAddress, rxSrcAddr);
                }
#endif
            }
        }
        // update the FCO option
        MRXM_procCmdPktFCO(usm_buf,payload_ie_len,&assocconf.fco);

        if (MlmeCb)
        {
            assocconf.hdr.event = MAC_MLME_ASSOCIATE_CNF;
            MlmeCb((tschCbackEvent_t *)(&assocconf), usm_buf);
        }
      }

      // Corner cases are:
      // - assoc request delivered, but no ack
      // - very late assoc request
      // Just drop
#endif
      break;
    }
    case MAC_COMMAND_DISASSOC_NOT:
    {
      tschMlmeDisassociateIndication_t disassoc_indc;

      // buf_ptr[0] is CFI
      disassoc_indc.disassociateReason = buf_ptr[1];

      /* disassoc request always uses long address for src address */
      ulpsmacaddr_long_copy(&disassoc_indc.deviceAddress,  MRXM_getPktSrcAddr());

      // there is no IE in this message

#if IS_ROOT
     // from the source address
     TSCH_TXM_freeDevPkts(ULPSMACADDR_MODE_LONG, &disassoc_indc.deviceAddress);
#else
     TSCH_prepareDisConnect();
#endif

#if !IS_ROOT
      if (MlmeCb)
      {
          disassoc_indc.hdr.event = MAC_MLME_DISASSOCIATE_IND;
          MlmeCb((tschCbackEvent_t *)(&disassoc_indc),usm_buf);
      }
#endif
      break;
    }
#if ULPSMAC_L2MH
    case MAC_COMMAND_ASSOC_REQ_TI_L2MH:
    {
      tschTiAssociateIndicationL2mh_t assocind;

      // buf_ptr[0] is CFI
      assocind.capabilityInformation = buf_ptr[1];
      ulpsmacaddr_long_copy(&assocind.deviceAddress, (ulpsmacaddr_t *) &buf_ptr[2]);
      // BZ14508
      //assocind.requestorShrtAddr = buf_ptr[10]+(buf_ptr[11]>>8);
      assocind.requestorShrtAddr = buf_ptr[10]+(buf_ptr[11]<<8);
      assocind.srcAddrMode = MRXM_getPktSrcAddrMode();

      if(assocind.srcAddrMode == MAC_ADDR_MODE_EXT) {
        ulpsmacaddr_long_copy(&assocind.srcAddr, rxSrcAddr);
      } else { //if(assocind.srcAddrMode == MAC_ADDR_MODE_SHORT) {
        ulpsmacaddr_short_copy(&assocind.srcAddr,rxSrcAddr);
      }

      // update the FCO option
      MRXM_procCmdPktFCO(usm_buf,payload_ie_len,&assocind.fco);

      if (MlmeCb)
      {
          assocind.hdr.event =  MAC_MLME_ASSOCIATE_IND_L2MH;
          MlmeCb((tschCbackEvent_t *)(&assocind),usm_buf);
      }

      break;
    }
    case MAC_COMMAND_ASSOC_RESP_TI_L2MH:
    {
      tschTiAssociateConfirmL2mh_t assocconf;

      // buf_ptr[0] is CFI
      assocconf.assocShortAddress = buf_ptr[1] + (buf_ptr[2]<<8);
      assocconf.hdr.status = buf_ptr[3];
      ulpsmacaddr_long_copy(&assocconf.deviceExtAddr, (ulpsmacaddr_t *)&buf_ptr[4]);
      assocconf.requestorShrtAddr = buf_ptr[12] + (buf_ptr[13]<<8);

      // update the FCO option
      MRXM_procCmdPktFCO(usm_buf,payload_ie_len,&assocconf.fco);

      if (MlmeCb)
      {
          assocconf.hdr.event =  MAC_MLME_ASSOCIATE_CNF_L2MH;
          MlmeCb((tschCbackEvent_t *)(&assocconf),usm_buf);
      }

      break;
    }
    case MAC_COMMAND_DISASSOC_NOT_TI_L2MH:
    {
      tschTiDisassociateIndicationL2mh_t disassocind_l2mh;

      // buf_ptr[0] is CFI
      disassocind_l2mh.reason = buf_ptr[1];
      ulpsmacaddr_long_copy(&disassocind_l2mh.deviceExtAddress, (ulpsmacaddr_t *) &buf_ptr[2]);
      // BZ14508
      disassocind_l2mh.requestorShrtAddr = buf_ptr[10]+ (buf_ptr[11]<<8);

      disassocind_l2mh.srcAddrMode = MRXM_getPktSrcAddrMode();

      if(disassocind_l2mh.srcAddrMode == MAC_ADDR_MODE_EXT) {
        ulpsmacaddr_long_copy(&disassocind_l2mh.srcAddr, rxSrcAddr);
      } else { //if(disassocind_l2mh.srcAddrMode == MAC_ADDR_MODE_SHORT) {
        ulpsmacaddr_short_copy(&disassocind_l2mh.srcAddr, rxSrcAddr);
      }

      // update the FCO option
      MRXM_procCmdPktFCO(usm_buf,payload_ie_len,&disassocind_l2mh.fco);

      if (MlmeCb)
      {
          disassocind_l2mh.hdr.event =  MAC_MLME_DISASSOCIATE_IND_L2MH;
          MlmeCb((tschCbackEvent_t *)(&disassocind_l2mh),usm_buf);
      }

      break;
    }
    case MAC_COMMAND_DISASSOC_ACK_TI_L2MH:
    {
      tschTiDisassociateConfirmL2mh_t disassocconf_l2mh;

      // buf_ptr[0] is CFI
      disassocconf_l2mh.hdr.status = buf_ptr[1];
      ulpsmacaddr_long_copy(&disassocconf_l2mh.deviceExtAddr, (ulpsmacaddr_t *)&buf_ptr[2]);
      //BZ14508
      disassocconf_l2mh.requestorShrtAddr = buf_ptr[10]+ (buf_ptr[11]<<8);

      // update the FCO option
      MRXM_procCmdPktFCO(usm_buf,payload_ie_len,&disassocconf_l2mh.fco);

      if (MlmeCb)
      {
          disassocconf_l2mh.hdr.event =  MAC_MLME_DISASSOCIATE_CNF_L2MH;
          MlmeCb((tschCbackEvent_t *)(&disassocconf_l2mh),usm_buf);
      }
      break;
    }
#endif
    case MAC_COMMAND_DATA_REQ:
    case MAC_COMMAND_PANID_CONFLICT:
    case MAC_COMMAND_ORPHAN_NOT:
    case MAC_COMMAND_COORD_REALIGNMENT:
      break;
    default:
      break;
  }

}

/**************************************************************************************************//**
 *
 * @brief       This is entry function of MAC input for beacon, data and MAC commands.
 *
 * @param[in]   usm_buf -- the BM buffer pointer to input data packet

 * @return
 *
 ***************************************************************************************************/
void MRXM_input(BM_BUF_t* usm_buf)
{
  switch(MRXM_getPktType())
  {
    case ULPSMACBUF_ATTR_PACKET_TYPE_DATA:
      input_data_packet(usm_buf);
      break;
    case ULPSMACBUF_ATTR_PACKET_TYPE_BEACON:
      input_beacon_packet(usm_buf);
      break;
    case ULPSMACBUF_ATTR_PACKET_TYPE_COMMAND:
      input_command_packet(usm_buf);
      break;
    default:
      break;
  }
}

/**************************************************************************************************//**
 *
 * @brief       This function will save the MAC RX frame information by later usage. This function is
 *              called after MAC RX parser
 *
 * @param       rxFrame - the pointer to parsed MAC RX framer
 *
 * @return
 *
 *
 ***************************************************************************************************/
void MRXM_saveMacHeaderInfo(frame802154e_t *rxFrame)
{

  macRxPktParam.rxFramer = *rxFrame;

  // save the src and dst in address format
  ulpsmacaddr_long_copy(&(macRxPktParam.dstAddr), (ulpsmacaddr_t *)(&(rxFrame->dest_addr)));
  ulpsmacaddr_long_copy(&(macRxPktParam.srcAddr), (ulpsmacaddr_t *)(&(rxFrame->src_addr)));

}

/**************************************************************************************************//**
 *
 * @brief       This function will save the MAC RX PHY information by later usage. This function is
 *              called in RX ISR
 *
 *
 * @param[in]   timestamp -- timestamp of packet received in ISR
 *              len -- number of bytes in the RX packet
 *              rssi -- RSSI of received packet
 *              LQI  -- LQI of received packet
 *              frameType -- frame type of received packet
 *
 * @return
 *
 *
 ***************************************************************************************************/
void MRXM_saveMacPHYInfo(uint32_t timestamp,uint8_t len,uint8_t rssi,uint8_t lqi,uint8_t frameType)
{
  macRxPktParam.timestamp = timestamp;
  macRxPktParam.rssi = rssi;
  macRxPktParam.mpduLinkQuality = lqi;
  macRxPktParam.pkt_len = len;
  macRxPktParam.pktType = frameType;
}


/**************************************************************************************************//**
 *
 * @brief       This function will return pointer to 802.15.4 MAC packet destination addr for
 *              RX packet only
 *
 *
 * @param[in]   pMac - the pointer to the start of MAC header
 *
 * @return      pointer to 802.15.4e MAC destination address
 *
 ***************************************************************************************************/
ulpsmacaddr_t *  MRXM_getPktDstAddr(void)
{

  return &(macRxPktParam.dstAddr);

}

/**************************************************************************************************//**
 *
 * @brief       This function will return pointer to 802.15.4 MAC packet source addr for
 *              RX packet only
 *
 * @param[in]
 *
 *
 * @return      pointer to 802.15.4e MAC source address
 *
 ***************************************************************************************************/
ulpsmacaddr_t *  MRXM_getPktSrcAddr(void)
{

  return &(macRxPktParam.srcAddr);

}

/**************************************************************************************************//**
 *
 * @brief       This function will return the timestamp of RX packet in ISR
 *              Note: this is help function. make it is inline to save code space
 * @param[in]
 *
 *
 * @return      timestamp of RX packet in ISR
 *
 ***************************************************************************************************/
uint32_t  MRXM_getPktTimestamp(void)
{
  return (macRxPktParam.timestamp);

}

/**************************************************************************************************//**
 *
 * @brief       This function will return the frame type  of RX packet in ISR
 *
 *
 * @param[in]
 *
 *
 * @return      frame type of RX packet
 *
 ***************************************************************************************************/
uint8_t  MRXM_getPktType(void)
{
  return (macRxPktParam.pktType);

}


/**************************************************************************************************//**
 *
 * @brief       This function will return the source PAN ID from RX packet
 *
 * @param[in]
 *
 * @return      source PAN ID
 *
 ***************************************************************************************************/
uint16_t  MRXM_getPktSrcPanID(void)
{
  return macRxPktParam.rxFramer.src_pid;

}

/**************************************************************************************************//**
 *
 * @brief       This function will return the destination PAN ID from RX packet
 *
 * @param[in]
 *
 *
 * @return      source PAN ID
 *
 ***************************************************************************************************/
uint16_t  MRXM_getPktDstPanID(void)
{
  return macRxPktParam.rxFramer.dest_pid;

}

/**************************************************************************************************//**
 *
 * @brief       This function will return the destination Address mode from RX packet
 *
 * @param[in]
 *
 * @return      destination address mode
 *
 ***************************************************************************************************/
uint8_t  MRXM_getPktDstAddrMode(void)
{
  return macRxPktParam.rxFramer.fcf.dest_addr_mode;

}

/**************************************************************************************************//**
 *
 * @brief       This function will return the source Address mode from RX packet
 *
 * @param[in]
 *
 * @return      destination address mode
 *
 ***************************************************************************************************/
uint8_t  MRXM_getPktSrcAddrMode(void)
{
  return macRxPktParam.rxFramer.fcf.src_addr_mode;

}

/**************************************************************************************************//**
 *
 * @brief       This function will return the ack flag from RX packet
 *
 * @param[in]
 *
 * @return      ack flag of RX packet
 *
 ***************************************************************************************************/
uint8_t  MRXM_getPktAck(void)
{
  return macRxPktParam.rxFramer.fcf.ack_required;

}

/**************************************************************************************************//**
 *
 * @brief       This function will return the IE flag from RX packet
 *
 * @param[in]
 *
 * @return      IE flag of RX packet
 *
 ***************************************************************************************************/
uint8_t  MRXM_getPktIEFlag(void)
{
  return macRxPktParam.rxFramer.fcf.ie_list_present;

}


/**************************************************************************************************//**
 *
 * @brief       This function will return the seq suppression flag from RX packet
 *
 * @param
 *
 * @return      seq suppression flag of RX packet
 *
 ***************************************************************************************************/
uint8_t  MRXM_getPktSeqSuppression(void)
{
  return macRxPktParam.rxFramer.fcf.seq_num_suppress;

}

/**************************************************************************************************//**
 *
 * @brief       This function will return the seq number from RX packet
 *
 * @param
 *
 * @return      seq number of RX packet
 *
 ***************************************************************************************************/
uint8_t  MRXM_getPktSeqNo(void)
{
  return macRxPktParam.rxFramer.seq;

}

/**************************************************************************************************//**
 *
 * @brief       This function will return Header IE pointer of TX 802.15.4e MAC packet.
 *
 * @param
 *
 * @return      header ie pointer of 802.15.4e MAC packet
 *
 ***************************************************************************************************/
uint8_t *MRXM_getHdrIePtr(void)
{

  return (macRxPktParam.rxFramer.hdrie);

}


/**************************************************************************************************//**
 *
 * @brief       This function will return Header IE length of TX 802.15.4e MAC packet.
 *
 * @param
 *
 * @return      header ie length of 802.15.4e MAC packet
 *
 ***************************************************************************************************/
uint8_t MRXM_getHdrIeLen(void)
{
  return (macRxPktParam.rxFramer.hdrie_len);
}

/**************************************************************************************************//**
 *
 * @brief       This function will return rssi of the RX packet.
 *
 * @param
 *
 * @return      rssi of the received packet
 *
 ***************************************************************************************************/
uint8_t MRXM_getPktRssi(void)
{
  return (macRxPktParam.rssi);
}


