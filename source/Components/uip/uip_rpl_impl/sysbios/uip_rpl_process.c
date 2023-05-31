/*
* Copyright (c) 2014 Texas Instruments Incorporated
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
* include this software, other than combinations with devices manufactured by or for TI (�TI Devices�). 
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
* THIS SOFTWARE IS PROVIDED BY TI AND TI�S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TI�S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *  ====================== uip_rpl_process.c =============================================
 *  uip process thread related functions implementation in SYSBIOS
 */

/*******************************************************************************
*                                            INCLUDES
*******************************************************************************/
#include "string.h"
#include "uip_rpl_process.h"
#include "net/sicslowpan.h"

#include "sys/ctimer.h"
#include "lib/random.h"
#include "net/queuebuf.h"
#include "net/nbr-table.h"
#include "net/uip-ds6.h"
#include "net/uip-ds6-route.h"
#include "net/uip-icmp6.h"

#include "hct_msp432/hct/hct.h" //Jiachen

#ifndef RPL_DISABLE
#include "rpl/rpl.h"
#include "rpl/rpl-private.h"
#endif

#include "rime/rimeaddr.h"

#include "mac_pib.h"
#include "ulpsmac.h"
#include "ulpsmacaddr.h"
#include "nhl_mgmt.h"
#include "nhl_mlme_cb.h"
#include "nhldb-tsch-cmm.h"
#include "nhl_api.h"
#include "nhldb-tsch-nonpc.h"
#include "nhl_mgmt.h"

#include <stdio.h>
#include "net/uip.h"
#include "net/udp-simple-socket.h"

#include "pwrest/pwrest.h"
#include "net/packetbuf.h"   
#if DMM_ENABLE
#include "dmm_scheduler.h"
#include "dmm_policy_blesp_wsnnode.h"
#endif //DMM_ENABLE
/* Max 20% MAC PER is allowed */
#define MAC_PER_DISCON_THRESH (20)

#include "coap/harp.h"
extern HARP_self_t HARP_self;

/* -----------------------------------------------------------------------------
*                                           Typedefs
* -----------------------------------------------------------------------------
*/
typedef struct __IP_addr_init
{
    uint16_t addrType;
    void* addr;
}IP_addr_init_t;

typedef struct __UDP_msg
{
    struct udp_simple_socket * pSock;
    void *pMsg;
    const uip_ipaddr_t *pDestAddr;
    uint32_t destPort;
    uint16_t msgSize;
} UDP_msg_t;

/* -----------------------------------------------------------------------------
*                                         Global Variables
* -----------------------------------------------------------------------------
*/
TCPIP_DBG_s TCPIP_Dbg;
LowPAN_DBG_s LOWPAN_Dbg;
uint8_t joinedDODAG=0;

// Jiachen
uint16_t received_asn;
extern uint8_t srcipaddr_short;
uint16_t selfShortAddr;
/* -----------------------------------------------------------------------------
*                                         Local Variables
* -----------------------------------------------------------------------------
*/
static int first_time_init_uip=0;
static IP_addr_init_t addr;
static UDP_msg_t udpMsg;
static uip_connection_callback app_setup_conn_callback= NULL;

unsigned char node_mac[8];
uint8_t macPerDisconnThresh = MAC_PER_DISCON_THRESH;

/*******************************************************************************
* @fn          uip_setup_appConnCallback
*
* @brief       This function registers the application connection callback
*              function.
*
* @param       callBack - callback function
*
* @return      none
*******************************************************************************
*/
void uip_setup_appConnCallback(uip_connection_callback callBack)
{
    app_setup_conn_callback = callBack;
}

/*******************************************************************************
* @fn          ip_address_init
*
* @brief       This functions initializes IP address based on MAC address.
*
* @param       ctrl_type - MAC address mode
*              ctrl_val - MAC address
*
* @return      none
*******************************************************************************
*/
void ip_address_init(uint16_t ctrl_type, void* ctrl_val)
{
    uint16_t        short_addr;

    if (ctrl_type != NHL_CTRL_TYPE_SHORTADDR)
    {
        return;
    }

    if (first_time_init_uip==1)
    {
        /*
        *  This is not first time ip address initialization Need to free UIP
        *  objetcs first
        */
        uip_rpl_clean();
    }

    short_addr = UIP_HTONS(*((uint16_t*)ctrl_val));

    /* Initialize the uIP stack */
    uip_rpl_init((uip_lladdr_t*)&short_addr);

    if (app_setup_conn_callback)
    {
        app_setup_conn_callback();
    }

    if (first_time_init_uip==0)
    {
        first_time_init_uip = 1;
    }
}

/*******************************************************************************
* @fn          ctrl_proc_cb
*
* @brief       This function is called from MAC layer to initialize IP address.
*
* @param       ctrl_type - MAC address mode
*              ctrl_val - MAC address
*
* @return      none
*******************************************************************************
*/
void ctrl_proc_cb(uint16_t ctrl_type, void* ctrl_val)
{

    if (first_time_init_uip==1)
    {
        TCPIP_cmdmsg_t cmdMsg;

        addr.addrType = ctrl_type;
        addr.addr = ctrl_val;
        cmdMsg.data = &addr;
        cmdMsg.event_type = IP_ADDR_INIT;

        Mailbox_post(tcpip_mailbox, &cmdMsg, BIOS_NO_WAIT);
    }
    else
    {
        ip_address_init(ctrl_type,ctrl_val);
    }
}

void prefix_setup(uint8_t *pAddr)
{
    uint8_t i;

    for(i=0;i<UIP_RPL_ATTR_PREFIX_LEN;i++)
    {
	pAddr[i] = uip_rpl_attributes.prefix[i];
    }
}
#ifndef RPL_DISABLE
void rpl_event_notification(int event, uint16_t instance_id)
{
    rpl_instance_t *instance = NULL;
    if (event == RPL_EVENT_INSTANCE_JOINED)
    {
        instance = rpl_get_instance(instance_id);
        if (instance != NULL)
        {
            if (instance->current_dag != NULL)
            {
                if(instance->current_dag->prefix_info.flags &
                   UIP_ND6_RA_FLAG_AUTONOMOUS)
                {
                    uip_rpl_attr_set_ip_global_prefix
                      (&instance->current_dag->prefix_info.prefix);
                    sicslowpan_setContext0
                      ((uint8_t*)&instance->current_dag->prefix_info.prefix);
                }
            }
        }
    }
}
#endif

/*******************************************************************************
* @fn         NHL_dataConfCb
*
* @brief      Data sent confirmation function to provide to NHL layer
*
* @param       pDataConf - pointer to confirmation data structure
*
* @return      none
********************************************************************************
*/
void NHL_dataConfCb(NHL_McpsDataConf_t *pDataConf)
{
    uint8_t status;

    if (ULPSMAC_Dbg.numTxTotal > MIN_ETX_COUNT*5)
    {  
       uint8_t macPer;
       macPer = (uint8_t)(ULPSMAC_Dbg.numTxNoAck*100/ULPSMAC_Dbg.numTxTotal);
       
       if (macPer > macPerDisconnThresh)
       {
           ULPSMAC_Dbg.numBlackListedParent++;
           if (ULPSMAC_Dbg.numBlackListedParent > 10)
           {
              macPerDisconnThresh = macPerDisconnThresh >= 20 ? 40 : macPerDisconnThresh*2;
              ULPSMAC_Dbg.numBlackListedParent = 0;
              NHL_resetBlackListParentTable();
           }
           
           NHL_addBlackListParent(ULPSMAC_Dbg.parent);
           return;
       }
    }

    if (pDataConf->numBackoffs)
    {
        switch (pDataConf->hdr.status)
        {
            case (MAC_SUCCESS):
            status = MAC_TX_OK;
            break;
            case (MAC_NO_ACK):
            status = MAC_TX_NOACK;
            break;
            default:
            status = MAC_TX_ERR;
            break;
        }
        sicslowpan_sent_cb(status, pDataConf->numBackoffs); 
    }
}

/************************************************************************//***
* @fn          NHL_dataIndCb
*
* @brief       Application data receive callback function.
*
* @param       pDataInd -- Pointer to NHL_McpsDataInd_t structure to be
*                           confirmed
*
* @return      none
******************************************************************************/
void NHL_dataIndCb(NHL_McpsDataInd_t *pDataInd)
{
    NHL_DataInd_t *pMac;
    rimeaddr_t tmpRimeAddr;
    uint8_t destIsBcast,addrLen;
    
    if (uipPakcets[uipPackets_WriteIdx].buf_len != 0)
    {
    	//Packet FIFO overflow
    	lowpanRxPacketLossCount++;
    }
    else
    {
    uipPakcets_lock(); //fengmo debug GW packet loss
    packetbuf_copyfrom(pDataInd->pMsdu->buf_aligned, pDataInd->pMsdu->datalen);
    pMac = &(pDataInd->mac);
    
    // downlink from parent
    if(pDataInd->mac.srcAddr.u8[0]==HARP_self.parent)
      received_asn = pDataInd->recv_asn; // jiachen
    
    if (pMac->srcAddrMode == ULPSMACADDR_MODE_LONG)
    {
      addrLen = 8;
    }
    else if (pMac->dstAddrMode == ULPSMACADDR_MODE_SHORT)
    {
      addrLen = 2;
    }
    else
    {
      addrLen = 0;
    }
    
    memset(tmpRimeAddr.u8,0x00,sizeof(rimeaddr_t));
    ulpsmacaddr_copy_swapped((uint8_t *)&tmpRimeAddr, RIMEADDR_SIZE,
                             pMac->srcAddr.u8, addrLen);
    packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &tmpRimeAddr);
    
    destIsBcast = 0;
    if ((pMac->dstAddrMode == ULPSMACADDR_MODE_LONG)
        && ulpsmacaddr_long_cmp(&pMac->dstAddr,&ulpsmacaddr_broadcast))
    {
      destIsBcast = 1;
    }
    else if ((pMac->dstAddrMode == ULPSMACADDR_MODE_SHORT)
             && ulpsmacaddr_short_cmp(&pMac->dstAddr, &ulpsmacaddr_broadcast))
    {
      destIsBcast = 1;
    }

    memset(tmpRimeAddr.u8,0x00,sizeof(rimeaddr_t));
    if (!destIsBcast)
    {
      if (pMac->dstAddrMode == ULPSMACADDR_MODE_LONG)
      {
        addrLen = 8;
      }
      else if (pMac->dstAddrMode == ULPSMACADDR_MODE_SHORT)
      {
        addrLen = 2;
      }
      else
      {
        addrLen = 0;
      }
      ulpsmacaddr_copy_swapped((uint8_t *)&tmpRimeAddr, RIMEADDR_SIZE,
                               pMac->dstAddr.u8, addrLen);
    }

    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &tmpRimeAddr);
    
    if (tcpip_isInitialized())
    {
      uipPakcets[uipPackets_WriteIdx].buf_len = buflen;    //fengmo debug GW packet loss
      uipPacketIndex_inc(&uipPackets_WriteIdx); //fengmo debug GW packet loss
      tcpip_process_post_func(SICSLOWPAN_PACKET_INPUT, NULL);
    }
    uipPakcets_unlock(); //fengmo debug GW packet loss
    }
    BM_free_6tisch(pDataInd->pMsdu);
}

/*******************************************************************************
* @fn         sicslowpan_tx_request
*
* @brief      This message sends data request from 6lowpan to MAC layer
*
* @param       none
*
* @return      none
********************************************************************************
*/
void sicslowpan_tx_request(rimeaddr_t *dest)
{
  NHL_McpsDataReq_t nhlMcpsDataReq;
  NHL_DataReq_t*     nhlDataReq;
  NHL_Sec_t*   secReq;
  ulpsmacaddr_t macDestAddr;
  
  nhlMcpsDataReq.pMsdu=NULL;
  
  if(!TSCH_checkAssociationStatus())
  {
    sicslowpan_sent_cb(MAC_TX_ERR,0);
    return;
  }  
  
  nhlMcpsDataReq.pMsdu = (BM_BUF_t*)BM_new_from_packetbuf(30);
  
  if (nhlMcpsDataReq.pMsdu == NULL)
  {
    sicslowpan_sent_cb(MAC_TX_ERR,0);
    return;
  }
  
  pltfrm_byte_memcpy_swapped(macDestAddr.u8, sizeof(ulpsmacaddr_t),
                             dest, UIP_LLADDR_LEN);
  
  nhlDataReq = &(nhlMcpsDataReq.dataReq);
  nhlDataReq->dstAddrMode = ulpsmacaddr_addr_mode(&macDestAddr);

  if (nhlDataReq->dstAddrMode == ULPSMACADDR_MODE_NULL)
  {
    /* broadcast*/
    nhlDataReq->txOptions = 0;
    nhlDataReq->dstAddrMode = ULPSMACADDR_MODE_SHORT;
    ulpsmacaddr_copy(nhlDataReq->dstAddrMode,&nhlDataReq->dstAddr,
                     &ulpsmacaddr_broadcast);
  }
  else 
  {
    /* unicast*/
    // jiachen, forwarding packet, owner 31
    if(srcipaddr_short!=selfShortAddr) nhlMcpsDataReq.pMsdu->owner = 31;
    
    nhlDataReq->txOptions = 1;
    nhlDataReq->dstAddr = macDestAddr;
    // Jiachen: judge the coap packet type by the TAG (in the end) and record the data len.
    if((packetbuf[PACKETBUF_HDR_SIZE+buflen-3]==HCT_TLV_TYPE_COAP_DATA_TYPE) && // type
      (packetbuf[PACKETBUF_HDR_SIZE+buflen-2]==1)) // length
    {
      nhlMcpsDataReq.pMsdu->coap_type = packetbuf[PACKETBUF_HDR_SIZE+buflen-1];
    }
  }
  
  if (ulpsmacaddr_short_node == ulpsmacaddr_null_short)
  {
    nhlDataReq->srcAddrMode = ULPSMACADDR_MODE_LONG;
  }
  else
  {
    nhlDataReq->srcAddrMode = ULPSMACADDR_MODE_SHORT;
  }
  
  NHL_getPIB(TSCH_MAC_PIB_ID_macPANId, &nhlDataReq->dstPanId);
  nhlDataReq->fco.iesIncluded = 0;
  nhlDataReq->pIEList = NULL;
  nhlDataReq->fco.seqNumSuppressed = 0;
  
  secReq = &(nhlMcpsDataReq.sec);
#ifdef FEATURE_MAC_SECURITY
  if (((nhlDataReq->dstAddrMode == ULPSMACADDR_MODE_LONG)
       && !ulpsmacaddr_long_cmp(&nhlDataReq->dstAddr,&ulpsmacaddr_broadcast)
         && !ulpsmacaddr_long_cmp(&nhlDataReq->dstAddr,&ulpsmacaddr_null)) ||
      ((nhlDataReq->dstAddrMode == ULPSMACADDR_MODE_SHORT)
       && !ulpsmacaddr_short_cmp(&nhlDataReq->dstAddr,&ulpsmacaddr_broadcast)
         && !ulpsmacaddr_short_cmp(&nhlDataReq->dstAddr,&ulpsmacaddr_null)))
  {
	  NHL_setMacSecurityParameters(secReq);
  }
  else
  {
    secReq->securityLevel = MAC_SEC_LEVEL_NONE;
  }
#else
  secReq->securityLevel = MAC_SEC_LEVEL_NONE;
#endif
  
  NHL_dataReq(&nhlMcpsDataReq,NHL_dataConfCb);
}

/*******************************************************************************
* @fn          tcpip_process_post_func
*
* @brief       This function posts IP messages to IP task
*              context.
*
* @param       event - event type
*              data - data message
*
* @return      none
********************************************************************************
*/
void tcpip_process_post_func(process_event_t event, process_data_t data)
{
    TCPIP_cmdmsg_t  msg;

    msg.event_type = event;
    msg.data = (void*) data;

    if (Mailbox_post(tcpip_mailbox, &msg, BIOS_NO_WAIT) != TRUE)
    {
        TCPIP_Dbg.tcpip_post_err++;
    }
}

/*******************************************************************************
* @fn          rpl_process_post_func
*
* @brief       This function posts RPL messages to IP task context.
*
* @param       event - event type
*              data - data message
*
* @return      none
********************************************************************************
*/
void rpl_process_post_func(process_event_t event, process_data_t data)
{
    TCPIP_cmdmsg_t  msg;
    msg.event_type = event;
    msg.data = data;

    if (Mailbox_post(tcpip_mailbox, &msg, BIOS_NO_WAIT) != TRUE)
    {
        TCPIP_Dbg.tcpip_post_err++;
    }
}

/*******************************************************************************
* @fn          uip_udp_packet_send_post
*
* @brief       This function posts udp send message to IP task context.
*
* @param       *s - udp socket
*              *data - pointer to data message
*              len - data message length
*              toaddr - destination IP address
*              toport - destination UDP port
*
* @return      none
********************************************************************************
*/
void uip_udp_packet_send_post(struct udp_simple_socket *s, void *data, int len,
                              const uip_ipaddr_t *toaddr, uint16_t toport)
{

    TCPIP_cmdmsg_t cmdMsg;

    udpMsg.pSock = s;
    udpMsg.pMsg = data;
    udpMsg.pDestAddr = toaddr;
    udpMsg.destPort = toport;
    udpMsg.msgSize = len;
    cmdMsg.event_type = UIP_UDP_SEND;
    cmdMsg.data = &udpMsg;

    Mailbox_post(tcpip_mailbox, &cmdMsg, BIOS_NO_WAIT);
}

/*******************************************************************************
* @fn          uip_ds6_tsch_notification
*
* @brief       Notification function when a default route is added or removed.
*
* @param       event
*              data
*
* @return      none
********************************************************************************
*/
void uip_ds6_tsch_notification(int event, void* data)
{
    switch(event)
    {
        case UIP_DS6_ROUTE_DEFRT_ADD:
        joinedDODAG=1;
        break;
        case UIP_DS6_ROUTE_DEFRT_RM:
        joinedDODAG=0;
        break;
        default:
        break;
    }
#ifndef RPL_DISABLE
    rpl_ipv6_ds6_notification(event, data);
#endif
}

/*******************************************************************************
* @fn          uip_rpl_init
*
* @brief       Initialize uIP and RPL
*
* @param       mac_address - MAC layer address
*
* @return      none
********************************************************************************
*/
void uip_rpl_init(uip_lladdr_t* mac_address)
{
#if WITH_SICSLOWPAN
    sicslowpan_init(); //This one sets the tcpip TX function
    queuebuf_init();
#endif

    tcpip_init(mac_address, tcpip_process_post_func);

#ifndef RPL_DISABLE
    rpl_init(rpl_process_post_func);
#endif

}
/*******************************************************************************
* @fn          uip_rpl_clean
*
* @brief       Stops and cleans timers (and memory)
*
* @param       none
*
* @return      none
********************************************************************************
*/
void uip_rpl_clean(void)
{
    tcpip_clean();

#ifndef RPL_DISABLE
    rpl_clean();
#endif
}

/*******************************************************************************
* @fn          tsch_has_ded_link
*
* @brief       Check if TSCH MAC layer has dedicated link to a specific link layer address
*
* @param       target_lladdr  - target link layer address
*
* @return      none
********************************************************************************
*/
int tsch_has_ded_link(rimeaddr_t* target_lladdr)
{
    ulpsmacaddr_t DIO_addr_sender;

    pltfrm_byte_memcpy_swapped(&DIO_addr_sender, sizeof(ulpsmacaddr_t),
                               target_lladdr, UIP_LLADDR_LEN);

    return ULPSMAC_NHL.has_ded_link(&DIO_addr_sender);
}

/*******************************************************************************
* @fn          uip_rpl_get_rank
*
* @brief       Gets the rank from default RPL instance
*
* @param       none
*
* @return      rank
********************************************************************************
*/
uint8_t uip_rpl_get_rank(void)
{
   uint8_t jp;
   
   if (ULPSMAC_Dbg.numTxTotal > MIN_ETX_COUNT)
   {
    rpl_instance_t* instance = rpl_get_default_instance();
    rpl_dag_t *dag = instance->current_dag;
    jp = dag->rank/(RPL_DAG_MC_ETX_DIVISOR/2);
   }
   else
   {
    jp = 0xff;
   }
   
   return jp;
}

/*******************************************************************************
* @fn
*
* @brief
*
* @param
*
* @return
********************************************************************************
*/
void tcpip_task_prolog()
{
#if DMM_ENABLE
    DMM_waitBleOpDone();
#endif //DMM_ENABLE

#ifndef RPL_DISABLE
    /* Need to initialize the attributes before the Stack */
    uip_rpl_attr_init();
#endif

    pltfrm_byte_memcpy_swapped(node_mac, sizeof(node_mac),
                               ulpsmacaddr_long_node.u8, 8);

    while(!tcpip_isInitialized())
    {
        Task_sleep(1*CLOCK_SECOND);
    }

    /*Dag creation */
#if UIP_RPL_ROOT
#ifndef RPL_DISABLE
    uip_ipaddr_t prefix;
    prefix_setup(prefix.u8);
    rpl_start_instance(&prefix);
#endif
#endif
}

/*******************************************************************************
* @fn          tcpip_task
*
* @brief       IP task that handles all the uIP, RPL functions.
*
* @param       arg0  - meaningless
* @param       arg1  - meaningless
*
* @return      none
********************************************************************************
*/
void tcpip_task(UArg arg0, UArg arg1)
{
    tcpip_task_prolog();
//    

    while (1)
    {
        if(selfShortAddr==0 || selfShortAddr==65535)
            NHL_getPIB(TSCH_MAC_PIB_ID_macShortAddress, &selfShortAddr);
        
        TCPIP_cmdmsg_t task_msg;
        Bool st;
        st = Mailbox_pend(tcpip_mailbox, &task_msg, BIOS_WAIT_FOREVER);
#if DISABLE_TISCH_WHEN_BLE_CONNECTED && DMM_ENABLE && CONCURRENT_STACKS
        if (bleConnected)
        {
            continue;
        }
#endif //DMM_ENABLE && !CONCURRENT_STACKS
        PWREST_on(PWREST_TYPE_CPU);

        if (st == TRUE)
        {
            TCPIP_cmdmsg_t *task_msg_p = &task_msg;
            UDP_msg_t* udpMsg;
            IP_addr_init_t *ipAddr;

            switch (task_msg_p->event_type)
            {
#ifndef RPL_DISABLE
            case CALLBACK_TIMER:
                /* Callback timers (ctimer) are the ones used by RPL */
                rpl_process_handler(task_msg_p->event_type, task_msg_p->data);
                break;
#endif
            case UIP_UDP_SEND:
                udpMsg = (UDP_msg_t  *)task_msg_p->data;
                udp_simple_socket_sendto(udpMsg->pSock, udpMsg->pMsg,
                                         udpMsg->msgSize, udpMsg->pDestAddr,
                                         udpMsg->destPort);
                break;
            case PACKET_INPUT:
                tcpip_process_handler(task_msg_p->event_type, task_msg_p->data);
                break;
            case SICSLOWPAN_PACKET_INPUT:
                sicslowpan_input();
                uipPakcets[uipPackets_ReadIdx].buf_len = 0;
                break;
            case IP_ADDR_INIT:
                ipAddr = (IP_addr_init_t *)task_msg_p->data;
                ip_address_init(ipAddr->addrType,ipAddr->addr);
                break;
            default:
                /* Any other events are to tcpip (tcp timer, ds6 timer)*/
                tcpip_process_handler(task_msg_p->event_type, task_msg_p->data);
                break;
            }
        }
        else
        {
            TCPIP_Dbg.tcpip_pend_err++;
        }

        uip_len = 0;  //fengmo debug GW packet loss

        PWREST_off(PWREST_TYPE_CPU);
    }
}
