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
*  ====================== uip_rpl_process.c =============================================
*  uip thread related functions implementation in Linux
*/

/*******************************************************************************
*                                            INCLUDES
*******************************************************************************/
#include <stdbool.h>

#include "proj_assert.h"

#include "pltfrm_mbx.h"
#include "plat-conf.h"

#include "uip_rpl_process.h"

#include "sys/ctimer.h"
#include "lib/random.h"
#include "rpl/rpl.h"
#include "rime/rimeaddr.h"

#include "net/sicslowpan.h"
#include "net/uip.h"
#include "net/queuebuf.h"
#include "net/nbr-table.h"
#include "net/uip-ds6-route.h"
#include "net/packetbuf.h"
#include "net/udp-simple-socket.h"

#include "net/uip-ds6.h"

#include <util.h>

#include "mac-dep.h"

#include "comm.h"
#include "nm.h"
#include "device.h"

#include "string.h"
#if FEATURE_DTLS
#include "../../../dtls/dtls_client.h"
#endif

#include "nv_params.h"

/* -----------------------------------------------------------------------------
*                                           Typedefs
* -----------------------------------------------------------------------------
*/
typedef struct __UDP_msg
{
  struct udp_simple_socket *pSock;
  void *pMsg;
  const uip_ipaddr_t *pDestAddr;
  uint32_t destPort;
  uint16_t msgSize;
} UDP_msg_t;

#define PREFIX_LEN (8)

#define HCT_TLV_TYPE_COAP_DATA_TYPE 0x21

/* -----------------------------------------------------------------------------
*                                         Global Variables
* -----------------------------------------------------------------------------
*/
const uint8_t ipv6_prefix[PREFIX_LEN] = {0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34,
                                         0xff, 0xff};

TCPIP_DBG_s TCPIP_Dbg;
LowPAN_DBG_s LOWPAN_Dbg;

pthread_t UIP_threadInfo;
pltfrm_mbx_t tcpip_mailbox;

IP_addr_init_t addr;
uip_connection_callback app_setup_conn_callback = NULL;

bool nodeMacHasChanged;
unsigned char node_mac[EUI_SIZE];

uint16_t panId;
uint16_t macRxPktLen, macTxPktLen;
uint16_t tsch_slot_frame_size = 127;
uint16_t tsch_number_shared_slots = 4;
uint16_t beacon_freq = 8;
uint8_t txMsduHandler = 0;
uint8_t joinedDODAG = 0;
uint8_t first_time_init_uip = 0;
extern uint8_t tun_ip_prefix[8];
uint8_t MAC_txBuff[PACKETBUF_SIZE];

uint16_t gateway_mac_address = 1;
/* -----------------------------------------------------------------------------
*                                         Local Variables
* -----------------------------------------------------------------------------
*/
static UDP_msg_t udpMsg;

static uint8_t
ulpsmacaddr_addr_mode(ulpsmacaddr_t *addr)
{
  uint8_t addr_mode;

  if ((addr->u8[2] == 0x00) && (addr->u8[3] == 0x00) &&
      (addr->u8[4] == 0x00) && (addr->u8[5] == 0x00) &&
      (addr->u8[6] == 0x00) && (addr->u8[7] == 0x00))
  {

    if ((addr->u8[0] == 0x00) && (addr->u8[1] == 0x00))
    {
      addr_mode = ULPSMACADDR_MODE_NULL;
    }
    else
    {
      addr_mode = ULPSMACADDR_MODE_SHORT;
    }
  }
  else
  {
    addr_mode = ULPSMACADDR_MODE_LONG;
  }

  return addr_mode;
}

/*******************************************************************************
* @fn          uip_rpl_clean
*
* @brief       Stops and cleans timers (and memory)
*
* @param       none
*
* @return      none
*******************************************************************************
*/
void uip_rpl_clean(void)
{
  tcpip_clean();

#ifndef RPL_DISABLE
  rpl_clean();
#endif
}

/*******************************************************************************
* @fn          ip_address_init
*
* @brief       This function is called by MAC layer to init IP address
*
* @param       ctrl_type - short or long MAC address
* @param       ctrl_val - MAC address
*
* @return      none
*******************************************************************************
*/
void ip_address_init(uint16_t ctrl_type, void *ctrl_val)
{

  uint16_t short_addr;

  if (ctrl_type != NHL_CTRL_TYPE_SHORTADDR)
  {
    return;
  }

  if (first_time_init_uip == 1)
  {
    // This is not first time. Need to free UIP objetcs
    uip_rpl_clean();
  }
  //Invert the MAC address
  short_addr = UIP_HTONS(*((uint16_t *)ctrl_val));

  //Now we can initialize the uIP stack
  uip_rpl_init((uip_lladdr_t *)&short_addr);
  if (app_setup_conn_callback)
  {
    app_setup_conn_callback();
  }

  if (first_time_init_uip == 0)
  {
    first_time_init_uip = 1;
  }
}

/*******************************************************************************
* @fn          UIP_setMacPib
*
* @brief       MAC Pib Set Function 
*
* @param       attrId - attribute ID
* @param       attrValLen - attribute value length
* @param       attrVal_p - pointer to attribute
*
* @return      none
*******************************************************************************
*/
void UIP_setMacPib(uint32_t attrId, uint16_t attrValLen, uint8_t *attrVal_p)
{
  switch (attrId)
  {
  case MAC_ATTR_ID_ROOT_SCHEDULE:
    if (rootTschScheduleBuffer_p != NULL)
    {
      rootTschScheduleLen = (rootTschScheduleBufferSize < attrValLen) ? rootTschScheduleBufferSize : attrValLen;
      memcpy(rootTschScheduleBuffer_p, attrVal_p, rootTschScheduleLen);
    }
    break;
  case MAC_ATTR_ID_PAN_ID:
    if (attrValLen == sizeof(panId))
    {
      memcpy(&panId, attrVal_p, attrValLen);
      nvParams.panid = panId;
    }
    break;
  case MAC_ATTR_ID_EUI:
    if (attrValLen == sizeof(node_mac[0]))
    {
      memcpy(&node_mac[0], attrVal_p, attrValLen);
      nodeMacHasChanged = true;
    }
    break;
  case MAC_ATTR_ID_BCN_CHAN:
    if (attrValLen == sizeof(nvParams.bcn_chan_0))
    {
      memcpy(&(nvParams.bcn_chan_0), attrVal_p, attrValLen);
    }
    break;
  case MAC_ATTR_ID_MAC_PSK_07:
    memcpy(nvParams.mac_psk, attrVal_p, 8);
    break;
  case MAC_ATTR_ID_MAC_PSK_8F:
    memcpy(nvParams.mac_psk + 8, attrVal_p, 8);
    break;
  case MAC_ATTR_ID_ASSOC_REQ_TIMEOUT_SEC:
    if (attrValLen == sizeof(nvParams.assoc_req_timeout_sec))
    {
      memcpy(&(nvParams.assoc_req_timeout_sec), attrVal_p, attrValLen);
    }
    break;
  case MAC_ATTR_ID_SLOTFRAME_SIZE:
    if (attrValLen == sizeof(nvParams.slotframe_size))
    {
      memcpy(&(nvParams.slotframe_size), attrVal_p, attrValLen);
    }
    break;
  case MAC_ATTR_ID_KEEPALIVE_PERIOD:
    if (attrValLen == sizeof(nvParams.keepAlive_period))
    {
      memcpy(&(nvParams.keepAlive_period), attrVal_p, attrValLen);
    }
    break;
  case MAC_ATTR_ID_COAP_RESOURCE_CHECK_TIME:
    if (attrValLen == sizeof(nvParams.coap_resource_check_time))
    {
      memcpy(&(nvParams.coap_resource_check_time), attrVal_p, attrValLen);
      coapConfig.coap_resource_check_time = nvParams.coap_resource_check_time;
    }
    break;
  case MAC_ATTR_ID_COAP_DEFAULT_PORT:
    if (attrValLen == sizeof(nvParams.coap_port))
    {
      memcpy(&(nvParams.coap_port), attrVal_p, attrValLen);
      coapConfig.coap_port = nvParams.coap_port;
    }
    break;
  case MAC_ATTR_ID_COAP_DEFAULT_DTLS_PORT:
    if (attrValLen == sizeof(nvParams.coap_dtls_port))
    {
      memcpy(&(nvParams.coap_dtls_port), attrVal_p, attrValLen);
      coapConfig.coap_dtls_port = nvParams.coap_dtls_port;
    }
    break;
  case MAC_ATTR_ID_TX_POWER:
    if (attrValLen == sizeof(nvParams.tx_power))
    {
      memcpy(&(nvParams.tx_power), attrVal_p, attrValLen);
    }
    break;
  case MAC_ATTR_ID_BCN_CH_MODE:
    if (attrValLen == sizeof(nvParams.bcn_ch_mode))
    {
      memcpy(&(nvParams.bcn_ch_mode), attrVal_p, attrValLen);
    }
    break;
  case MAC_ATTR_ID_SCAN_INTERVAL:
    if (attrValLen == sizeof(nvParams.scan_interval))
    {
      memcpy(&(nvParams.scan_interval), attrVal_p, attrValLen);
    }
    break;
  case MAC_ATTR_ID_NUM_SHARED_SLOT:
    if (attrValLen == sizeof(nvParams.num_shared_slot))
    {
      memcpy(&(nvParams.num_shared_slot), attrVal_p, attrValLen);
    }
    break;
  case MAC_ATTR_ID_FIXED_CHANNEL_NUM:
    if (attrValLen == sizeof(nvParams.fixed_channel_num))
    {
      memcpy(&(nvParams.fixed_channel_num), attrVal_p, attrValLen);
    }
    break;
  case MAC_ATTR_ID_RPL_DIO_DOUBLINGS:
    if (attrValLen == sizeof(nvParams.rpl_dio_doublings))
    {
      memcpy(&(nvParams.rpl_dio_doublings), attrVal_p, attrValLen);
      rplConfig.rpl_dio_doublings = nvParams.rpl_dio_doublings;
    }
    break;
  case MAC_ATTR_ID_COAP_OBS_MAX_NON:
    if (attrValLen == sizeof(nvParams.coap_obs_max_non))
    {
      memcpy(&(nvParams.coap_obs_max_non), attrVal_p, attrValLen);
      coapConfig.coap_obs_max_non = nvParams.coap_obs_max_non;
    }
    break;
  case MAC_ATTR_ID_COAP_DEFAULT_RESPONSE_TIMEOUT:
    if (attrValLen == sizeof(nvParams.coap_default_response_timeout))
    {
      memcpy(&(nvParams.coap_default_response_timeout), attrVal_p, attrValLen);
      coapConfig.coap_default_response_timeout = nvParams.coap_default_response_timeout;
    }
    break;
  case MAC_ATTR_ID_DEBUG_LEVEL:
    if (attrValLen == sizeof(nvParams.debug_level))
    {
      memcpy(&(nvParams.debug_level), attrVal_p, attrValLen);
    }
    break;
  case MAC_ATTR_ID_PHY_MODE:
    if (attrValLen == sizeof(nvParams.phy_mode))
    {
      memcpy(&(nvParams.phy_mode), attrVal_p, attrValLen);
    }
    break;
  case MAC_ATTR_ID_FIXED_PARENT:
    if (attrValLen == sizeof(nvParams.fixed_parent))
    {
      memcpy(&(nvParams.fixed_parent), attrVal_p, attrValLen);
    }
    break;
  default:
    break;
  }
}

/*******************************************************************************
* @fn          UIP_address_init
*
* @brief       UIP address init
*
* @param       none
*
* @return      True or False
*******************************************************************************
*/
int UIP_address_init(void)
{

  COMM_setSysCfg_s sysConfig;

  panId = 0xFFFF;
  memset(&node_mac, 0, EUI_SIZE);
  nodeMacHasChanged = false;

  COMM_getMacPib_s req;

  COMM_waitCommThreadReady();
  // reset the hardware
  DEV_resetSys();

  // load system config
  // parameters can be from user input
  sysConfig.slot_frame_size = tsch_slot_frame_size;
  sysConfig.number_shared_slot = tsch_number_shared_slots;
  sysConfig.beacon_freq = beacon_freq;
  gateway_mac_address = sysConfig.gateway_address = NM_requestGatewayAddress();
  NM_set_minimal_config(&sysConfig);

  DEV_setSysConfig(&sysConfig);

  // use DEV API to get mac pib
  printf("Wait for root node to start network\n");
  do
  {
    DEV_getMACPib(MAC_ATTR_ID_PAN_ID, 0);
    sleep(5);
  } while (panId == 0xFFFF);

  printf("root node starts with PAN-ID (0x%04x)\n", panId);

  // try to get EUI
  DEV_getMACPib(MAC_ATTR_ID_EUI, 0);

  printf("LMAC EUI: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
         node_mac[7], node_mac[6], node_mac[5], node_mac[4],
         node_mac[3], node_mac[2], node_mac[1], node_mac[0]);

  return 1;
}

/*******************************************************************************
* @fn          prefix_setup
*
* @brief       This function sets up the IP prefix
*
* @param       pAddr - IP prefix
*
* @return      none
*******************************************************************************
*/
void prefix_setup(uint8_t *pAddr)
{
  uint8_t i;

  for (i = 0; i < UIP_RPL_ATTR_PREFIX_LEN; i++)
  {
    pAddr[i] = uip_rpl_attributes.prefix[i];
  }
}

/*******************************************************************************
* @fn          rpl_event_notification
*
* @brief       rpl event notification
*
* @param       event - event type
* @param       instance_id - RPL instance ID
*
* @return      none
*******************************************************************************
*/
void rpl_event_notification(int event, uint16_t instance_id)
{
  rpl_instance_t *instance = NULL;
  if (event == RPL_EVENT_INSTANCE_JOINED)
  {
    //We need to make sure that the RPL Instance current dag prefix is the one we are using as Context 0 for 6LoWPAN HC
    instance = rpl_get_instance(instance_id);
    if (instance != NULL)
    {
      if (instance->current_dag != NULL)
      {
        if (instance->current_dag->prefix_info.flags & UIP_ND6_RA_FLAG_AUTONOMOUS)
        {
          //Set the attribute
          uip_rpl_attr_set_ip_global_prefix(&instance->current_dag->prefix_info.prefix);
          //Set the 6LoWPAN context 0 prefix
          sicslowpan_setContext0((uint8_t *)&instance->current_dag->prefix_info.prefix);
        }
      }
    }
  }
}

/*******************************************************************************
* @fn          sicslowpan_tx_request
*
* @brief       IP layer data request function to MAC layer
*
* @param       dest - destination addr
*
* @return      none
*******************************************************************************
*/
void sicslowpan_tx_request(rimeaddr_t *dest)
{
  COMM_dataReq_s *pdataReq;
  COMM_TX_msg_s txMsg;
  BM_BUF_t *dataBuf;
  ulpsmacaddr_t ulpsmac_addr_dest;
  uint8_t ulpsmac_addrmode;

  // copy data from packetbuf to local buf
  macTxPktLen = packetbuf_copyto(MAC_txBuff);

  txMsg.id = MAC_DATA_REQ;
  pdataReq = &txMsg.info.macDataReq;
  memset(pdataReq, 0x0, sizeof(COMM_dataReq_s));

  dataBuf = BM_alloc();
  BM_copyfrom(dataBuf, MAC_txBuff, macTxPktLen);
  BM_setDataLen(dataBuf, macTxPktLen);
  pdataReq->pMsdu = dataBuf;

  pltfrm_byte_memcpy_swapped(ulpsmac_addr_dest.u8, sizeof(ulpsmacaddr_t),
                             dest, UIP_LLADDR_LEN);
  ulpsmac_addrmode = ulpsmacaddr_addr_mode(&ulpsmac_addr_dest);

  pdataReq->dataReq.dstAddrMode = ulpsmac_addrmode;
  pdataReq->dataReq.srcAddrMode = ulpsmac_addrmode;
  memcpy(&(pdataReq->dataReq.dstAddr), &ulpsmac_addr_dest, sizeof(ulpsmac_addr_dest));
  pdataReq->dataReq.dstPanId = panId;
  pdataReq->dataReq.mdsuHandle = txMsduHandler++;
  pdataReq->dataReq.txOptions = 1;
  pdataReq->dataReq.power = 0;
  
  if (MAC_txBuff[macTxPktLen - 3] == HCT_TLV_TYPE_COAP_DATA_TYPE &&
      MAC_txBuff[macTxPktLen - 2] == 1)
  {
    pdataReq->dataReq.txOptions = 2; // special coap
  }

  if (COMM_sendMsg(&txMsg) <= 0)
  {
    printf("%s: Failed to send packet to MAC layer\n", __FUNCTION__);
    sicslowpan_sent_cb(MAC_TX_ERR, 0);
  }
}

/*******************************************************************************
* @fn          sent_cb
*
* @brief       MAC TX confirmation callback function to IP layer
*
* @param       status - TX status
* @param       status - Number of TX
*
* @return      none
*******************************************************************************
*/
void sent_cb(uint8_t status, uint8_t num_tx)
{
  if (num_tx)
  {
    switch (status)
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
    //Now we can call the sicslowpan packet sent function
    sicslowpan_sent_cb(status, num_tx);
  }
}

/*******************************************************************************
* @fn          recv_cb
*
* @brief       MAC packet receive callback function to notify IP layer
*
* @param       none
*
* @return      none
*******************************************************************************
*/
void recv_cb()
{
  if (tcpip_isInitialized())
  {
    uipPakcets[uipPackets_WriteIdx].buf_len = buflen;
    uipPacketIndex_inc(&uipPackets_WriteIdx);
    tcpip_process_post_func(SICSLOWPAN_PACKET_INPUT, NULL);
  }
}

/*******************************************************************************
* @fn          tcpip_process_post_func
*
* @brief       This function post events to UIP thread
*
* @param       event - Event type
* @param       data - Event data
*
* @return      none
*******************************************************************************
*/
void tcpip_process_post_func(process_event_t event, process_data_t data)
{
  TCPIP_cmdmsg_t msg;
  msg.event_type = event;
  msg.data = (void *)data;

  if (pltfrm_mbx_send(&tcpip_mailbox, (unsigned char *)&msg, PLTFRM_MBX_OPN_NO_WAIT) == 0)
  {
    TCPIP_Dbg.tcpip_post_err++;
    printf("%s TCP IP POST ERROR %d\n", __FUNCTION__, TCPIP_Dbg.tcpip_post_err);
  }
}

/*******************************************************************************
* @fn          rpl_process_post_func
*
* @brief       This function post RPL event to UIP thread for processing
*
* @param       event - RPL event type
* @param       data - Event data
*
* @return      none
*******************************************************************************
*/
void rpl_process_post_func(process_event_t event, process_data_t data)
{
  TCPIP_cmdmsg_t msg;
  msg.event_type = event;
  msg.data = data;

  if (pltfrm_mbx_send(&tcpip_mailbox, (unsigned char *)&msg, PLTFRM_MBX_OPN_NO_WAIT) == 0)
  {
    TCPIP_Dbg.tcpip_post_err++;
    printf("%s TCP IP POST ERROR\n", __FUNCTION__);
  }
}

/*******************************************************************************
* @fn          uip_udp_packet_send_post
*
* @brief       This function sends the UDP send request from application thread
*              to UIP thread
*
* @param       s - UDP socket 
* @param       data - application data
* @param       len - application length
* @param       toaddr - dest addr
* @param       toport - dest port
*
* @return      none
*******************************************************************************
*/
void uip_udp_packet_send_post(struct udp_simple_socket *s, void *data, int len,
                              const uip_ipaddr_t *toaddr, uint16_t toport)
{
  TCPIP_cmdmsg_t cmdMsg;
  static UDP_msg_t udpMsg[8];
  static uint8_t dataBuffers[8][256];
  static uip_ipaddr_t toaddrs[8];
  static int idx = 0;

  memcpy(dataBuffers[idx], data, (len < 256) ? len : 256);
  toaddrs[idx] = *toaddr;

  udpMsg[idx].pSock = s;
  udpMsg[idx].pMsg = dataBuffers[idx];
  udpMsg[idx].pDestAddr = &(toaddrs[idx]);
  udpMsg[idx].destPort = toport;
  udpMsg[idx].msgSize = len;
  cmdMsg.event_type = UIP_UDP_SEND;
  cmdMsg.data = &(udpMsg[idx]);
  idx = (idx + 1) % 8;

  if (pltfrm_mbx_send(&tcpip_mailbox, (unsigned char *)&cmdMsg, PLTFRM_MBX_OPN_NO_WAIT) == 0)
  {
    TCPIP_Dbg.tcpip_post_err++;
    printf("%s TCP IP POST ERROR\n", __FUNCTION__);
  }
}

/*******************************************************************************
* @fn          uip_ds6_tsch_notification
*
* @brief       This function is used by MAC to check if IP layer has joined a 
*              DODAG or not
*
* @param       event - DS6 event type
* @param       data - DS6 event data
*
* @return      none
*******************************************************************************
*/
void uip_ds6_tsch_notification(int event, void *data)
{
  uip_ds6_nbr_t *nbr_ptr;
  switch (event)
  {
  case UIP_DS6_ROUTE_DEFRT_ADD:
    joinedDODAG = 1;
    break;
  case UIP_DS6_ROUTE_DEFRT_RM:
    joinedDODAG = 0;
    break;
  }
  //Now call the rpl_route_noticifcation as we are using RPL
  rpl_ipv6_ds6_notification(event, data);
}

/*******************************************************************************
* @fn          uip_rpl_init
*
* @brief       uIP and RPL init function
*
* @param       mac_address - mac layer address of the node
*
* @return      none
*******************************************************************************
*/
void uip_rpl_init(uip_lladdr_t *mac_address)
{

#if WITH_SICSLOWPAN
  sicslowpan_init();
  queuebuf_init();
#endif

  tcpip_init(mac_address, tcpip_process_post_func);
  rpl_init(rpl_process_post_func);
}

/*******************************************************************************
* @fn          tsch_has_ded_link
*
* @brief       This function create if MAC layer has link to a neighbor
*
* @param       target_lladdr - target node link layer address
*
* @return      True or False
*******************************************************************************
*/
int tsch_has_ded_link(rimeaddr_t *target_lladdr)
{
  return 1;
}

/*******************************************************************************
* @fn          uip_start_rpl_dag
*
* @brief       This function create a RPL DAG for root node
*
* @param       prefix - IP address prefix
*
* @return      none
*******************************************************************************
*/
void uip_start_rpl_dag(uip_ipaddr_t *prefix)
{
  uip_ipaddr_t ipaddr;
  rpl_dag_t *dag;

  //Set manually the global address only for the ROOT
  memcpy(&ipaddr, &prefix->u8, sizeof(ipaddr));
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);

  //Now start the instance and dag
  uip_ds6_set_addr_iid((uip_ipaddr_t *)&prefix->u8, &uip_lladdr);
  dag = rpl_set_root(RPL_DEFAULT_INSTANCE, (uip_ip6addr_t *)&prefix->u8);
  rpl_set_prefix(dag, (uip_ipaddr_t *)&prefix->u8, 64);
}

/*******************************************************************************
* @fn          UIP_resetDIOTime
*
* @brief       This function resets the DIO timer.
*
* @param       none
*
* @return      none
*******************************************************************************
*/
void UIP_resetDIOTime(void)
{
  rpl_instance_t *default_instance = rpl_get_default_instance();
  if (default_instance != NULL)
  {
    rpl_reset_dio_timer(default_instance);
  }
}

/*******************************************************************************
* @fn          UIP_threadEntryFn
*
* @brief       UIP thread function
*
* @param       none
*
* @return      none
*******************************************************************************
*/
void *UIP_threadEntryFn(void *param_p)
{
  uint32_t st;
  TCPIP_cmdmsg_t task_msg;
  UDP_msg_t *udpMsg;
  IP_addr_init_t *ipAddr;
  uip_lladdr_t addr;

  //init tcpip mailbox
  if (pltfrm_mbx_init(&tcpip_mailbox, sizeof(TCPIP_cmdmsg_t), UIP_MBX_CNT) < 0)
    pltfrm_debug("tcpip_mailbox init failed");

  //Need to initialize the attributes before the Stack
  uip_rpl_attr_init();

  // set up the prefix
  uip_rpl_attr_set_ip_global_prefix(tun_ip_prefix);

  // printf the IP prefix
  printf("IPv6 prefix: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
         uip_rpl_attributes.prefix[0], uip_rpl_attributes.prefix[1],
         uip_rpl_attributes.prefix[2], uip_rpl_attributes.prefix[3],
         uip_rpl_attributes.prefix[4], uip_rpl_attributes.prefix[5],
         uip_rpl_attributes.prefix[6], uip_rpl_attributes.prefix[7]);

  addr.addr[0] = (gateway_mac_address >> 8) & 0xFF;
  addr.addr[1] = (gateway_mac_address)&0xFF;

  uip_rpl_init((uip_lladdr_t *)&addr);

  //Dag creation
#if IS_ROOT
  uip_ipaddr_t prefix;

  prefix_setup(prefix.u8);
  uip_start_rpl_dag(&prefix);
#endif

  printf("%s start main loop\n", __FUNCTION__);
  while (1)
  {
    st = pltfrm_mbx_pend(&tcpip_mailbox, (unsigned char *)&task_msg, PLTFRM_MBX_OPN_WAIT_FOREVER);

    if (st)
    {
      switch (task_msg.event_type)
      {
      case CALLBACK_TIMER:
        //Callback timers (ctimer) are the ones used by RPL
        rpl_process_handler(task_msg.event_type, task_msg.data);
        break;
      case UIP_UDP_SEND:
        //Request to send a UDP packet
        udpMsg = (UDP_msg_t *)task_msg.data;
        udp_simple_socket_sendto(udpMsg->pSock, udpMsg->pMsg, udpMsg->msgSize, udpMsg->pDestAddr, udpMsg->destPort);
        break;
      case PACKET_INPUT:
        tcpip_process_handler(task_msg.event_type, task_msg.data);
        break;
      case SICSLOWPAN_PACKET_INPUT:
        //Request to process an IPv6 packet coming from lower layer. First copy the packet to the uip_buf, then copy the length to uip_len, then process the event!
        sicslowpan_input();
        uipPakcets[uipPackets_ReadIdx].buf_len = 0;
        break;
      case IP_ADDR_INIT:
        ipAddr = (IP_addr_init_t *)task_msg.data;
        ip_address_init(ipAddr->addrType, ipAddr->addr);
        break;
#if FEATURE_DTLS
      case COAP_DTLS_TIMER:
        dtls_handle_retransmit(dtls_context[0]);
        break;

      case COAP_MAC_SEC_TIMER:
        dtls_handle_retransmit(dtls_context[1]);
        break;
#endif
      default:
        //Any other events are to tcpip to handle (tcp timer, ds6 timer)
        tcpip_process_handler(task_msg.event_type, task_msg.data);
        break;
      }
    }
    else
    {
      TCPIP_Dbg.tcpip_pend_err++;
    }

    uip_len = 0;
  }

  return NULL;
}

/*******************************************************************************
* @fn          UIP_init
*
* @brief       This function creates the UIP thread.
*
* @param       none
*
* @return      init status
*******************************************************************************
*/
int UIP_init(void)
{

  printf("Getting address from root node...\n");

  if (UIP_address_init() < 1)
    return -1;

  int rc = 0;
  rc = pthread_create(&UIP_threadInfo, NULL, UIP_threadEntryFn,
                      NULL);
  if (rc != 0)
  {
    printf("Failed to create UIP thread\n");
    proj_assert(0);
  }
  return 1;
}
