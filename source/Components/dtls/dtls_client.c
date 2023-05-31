/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */
#if FEATURE_DTLS
#ifndef LINUX_GATEWAY
#include "../uip/uip_rpl_impl/sysbios/uip_rpl_process.h"
#include "../tsch/mac_tsch_security.h"
#include "led.h"
#endif  //LINUX_GATEWAY
#include "../uip/net/uip-udp-packet.h"
#include "../uip/net/uip.h"
#include "../apps/coap/pdu.h"
#include "../apps/dtls/debug.h"
#include "../apps/dtls/tinydtls.h"
#include "../apps/dtls/dtls.h"
#include "../apps/dtls/crypto.h"
#include "dtls_client.h"
#include "nv_params.h"

#ifdef DTLS_PSK
/* The PSK information for DTLS */
/* make sure that default identity and key fit into buffer, i.e.
 * sizeof(PSK_DEFAULT_IDENTITY) - 1 <= PSK_ID_MAXLEN and
 * sizeof(PSK_DEFAULT_KEY) - 1 <= PSK_MAXLEN
*/

#define PSK_ID_MAXLEN 32
#define PSK_MAXLEN 32
#define PSK_DEFAULT_IDENTITY "Client_identity"
#define PSK_DEFAULT_KEY      "secretPSK"
#endif /* DTLS_PSK */

#ifdef DTLS_ECC
static const unsigned char ecdsa_priv_key[] = {
         0x41, 0xC1, 0xCB, 0x6B, 0x51, 0x24, 0x7A, 0x14,
         0x43, 0x21, 0x43, 0x5B, 0x7A, 0x80, 0xE7, 0x14,
         0x89, 0x6A, 0x33, 0xBB, 0xAD, 0x72, 0x94, 0xCA,
         0x40, 0x14, 0x55, 0xA1, 0x94, 0xA9, 0x49, 0xFA};

static const unsigned char ecdsa_pub_key_x[] = {
         0x36, 0xDF, 0xE2, 0xC6, 0xF9, 0xF2, 0xED, 0x29,
         0xDA, 0x0A, 0x9A, 0x8F, 0x62, 0x68, 0x4E, 0x91,
         0x63, 0x75, 0xBA, 0x10, 0x30, 0x0C, 0x28, 0xC5,
         0xE4, 0x7C, 0xFB, 0xF2, 0x5F, 0xA5, 0x8F, 0x52};

static const unsigned char ecdsa_pub_key_y[] = {
         0x71, 0xA0, 0xD4, 0xFC, 0xDE, 0x1A, 0xB8, 0x78,
         0x5A, 0x3C, 0x78, 0x69, 0x35, 0xA7, 0xCF, 0xAB,
         0xE9, 0x3F, 0x98, 0x72, 0x09, 0xDA, 0xED, 0x0B,
         0x4F, 0xAB, 0xC3, 0x6F, 0xC7, 0x72, 0xF8, 0x29};
#endif /* DTLS_ECC */

dtls_context_t the_dtls_context[MAX_NUMBER_DTLS_CONTEXT];

dtls_context_t *dtls_context[MAX_NUMBER_DTLS_CONTEXT];
/***************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 * @param[Out]
 * @return
 ******************************************************************************/
DtlsPeer_t *dtlsGetPeer(const dtls_context_t *ctx, const session_t *session, DtlsPeer_t *peerBuf_p)
{
   return(dtls_get_peer(ctx, session));
}
/*****************************************************************************
 *  FUNCTION:
 *
 *  DESCRIPTION:
 *
 *  PARAMETERS:
 *
 *  RETURNS:
 *
 *****************************************************************************/
static uint16_t chksum(uint16_t sum, uint8_t *data, uint16_t len)
{
   uint16_t t;
   int i;

   len -= 1;

   for (i = 0; i < len; i += 2)     /* At least two more bytes */
   {
      t = (((uint16_t)data[i]) << 8) + data[i + 1];
      sum += t;

      if(sum < t)
      {
         sum++;      /* carry */
      }
   }

   if(i == len)
   {
      t = ((uint16_t)data[i]) << 8;
      sum += t;

      if(sum < t)
      {
         sum++;      /* carry */
      }
   }

   /* Return sum in host byte order. */
   return sum;
}
/*****************************************************************************
 *  FUNCTION:  udp_chksum
 *
 *  DESCRIPTION: calculate the checksum of an IP-UDP packet.
 *
 *  PARAMETERS:
 *  buf: points to the IP packet containing a UDP packet.
 *
 *  RETURNS: If the UDP checksum
 *  in the packet contains a valid UDP checksum, the return value is 0xffff.  If the
 *  UDP checksum bytes are 0 in the packet, the return value contains the valid 16-bit UDP
 *  checksum with the byte order set to the network order.
 *
 *****************************************************************************/
uint16_t udp_chksum(uint8_t *buf)
{
   struct uip_ip_hdr *uip_hdr_p = (struct uip_ip_hdr *)(buf + UIP_LLH_LEN);
   uint16_t upper_layer_len = ((uint16_t)(uip_hdr_p->len0) << 8) + uip_hdr_p->len1;
   uint16_t sum = upper_layer_len + UIP_PROTO_UDP;

   /* Sum IP source and destination addresses. */
   sum = chksum(sum, (uint8_t *)&uip_hdr_p->srcipaddr, 2 * sizeof(uip_ipaddr_t));

   /* Sum UDP header and data. */
   sum = chksum(sum, &buf[UIP_IPH_LEN + UIP_LLH_LEN], upper_layer_len);
   sum = ~((sum == 0) ? 0xffff : sum);

   return (sum == 0) ? 0xffff : UIP_HTONS(sum);
}

/***************************************************************************
 * @fn          read_from_peer
 *
 * @brief      Handles the decrypted packet from the peer.
 * @param[in]
 * @param[Out]
 * @return     > 0 if successful
 ******************************************************************************/
static int read_from_peer(struct dtls_context_t *ctx,
                          session_t *session, uint8 *data, size_t len)
{
   uint16_t i;

   for (i = 0; i < len; i++)
   {
	   appRxFifo[appRxFifo_ReadIdx].c[UIP_IPUDPH_LEN + UIP_LLH_LEN + i] = data[i];
   }
   appRxLenFifo[appRxFifo_ReadIdx] = len;

   if (ctx->appDataHandler != NULL)
   {
      ctx->appDataHandler(ctx->appData_p);
   }

   return 0;
}
extern void uip_udp_packet_send_post(struct udp_simple_socket *s, void *data, int len,
                              const uip_ipaddr_t *toaddr, uint16_t toport);

/***************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 * @param[Out]
 * @return
 ******************************************************************************/
void dtls_retransmit_etimer_set(struct dtls_context_t *ctx, clock_time_t interval)
{
   etimer_set(&ctx->retransmit_timer, ctx->dtls_process_obj_p, interval);
}

/***************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 * @param[Out]
 * @return
 ******************************************************************************/
void dtls_retransmit_etimer_stop(struct dtls_context_t *ctx)
{
   etimer_stop(&ctx->retransmit_timer);
}
#ifndef LINUX_GATEWAY
/***************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 * @param[Out]
 * @return
 ******************************************************************************/
void dtls_send_key(dtls_context_t *ctx, dtls_peer_t *peer)
{
   uint16_t i;
   uip_lladdr_t *lladdr_p = uip_ds6_nbr_lladdr_from_ipaddr(&(peer->session.addr));
   uint16_t shortAddr = lladdr_p->addr[1] + lladdr_p->addr[0]*256;
   uint8_t *key;

   if (peer->role == DTLS_CLIENT)
   {
      key = dtls_kb_remote_write_key(dtls_security_params(peer), peer->role); //If we are the client
   }
   else
   {
      key = dtls_kb_local_write_key(dtls_security_params(peer), peer->role);  //If we are the server
   }

   for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
   {
      if (macSecurityPib.macDeviceTable[i].shortAddress == shortAddr)
      {
         macSecurityPib.macDeviceTable[i].keyUsage |= MAC_KEY_USAGE_RX;
         memcpy(macSecurityPib.macDeviceTable[i].pairwiseKey, key, sizeof(macSecurityPib.macDeviceTable[i].pairwiseKey));
         macSecurityPib.macDeviceTable[i].connectAttemptCount = 0;
         break;
      }
   }
}

/*******************************************************************************
* @fn
*
* @brief
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/
void dtls_activate_key(dtls_context_t *ctx, dtls_peer_t *peer)
{
   uip_lladdr_t *lladdr_p = uip_ds6_nbr_lladdr_from_ipaddr(&(peer->session.addr));
   if (lladdr_p != NULL)
   {
   uint16_t shortAddr = lladdr_p->addr[1] + lladdr_p->addr[0]*256;
      uint16_t i;

   for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
   {
      if (macSecurityPib.macDeviceTable[i].shortAddress == shortAddr)
      {
         macSecurityPib.macDeviceTable[i].keyUsage |= MAC_KEY_USAGE_TX;
#if DTLS_LEDS_ON
         LED_set(1,1);
         LED_set(2,0);
#endif  //DTLS_LEDS_ON
         break;
         }
      }
   }
}

/*******************************************************************************
* @fn
*
* @brief
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/
void dtls_deactivate_key(uip_ipaddr_t *ipaddr)
{
   uip_lladdr_t *lladdr_p = uip_ds6_nbr_lladdr_from_ipaddr(ipaddr);

   if (lladdr_p != NULL)
   {
      uint16_t shortAddr = lladdr_p->addr[1] + lladdr_p->addr[0]*256;
      uint16_t i;

      for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
      {
         if (macSecurityPib.macDeviceTable[i].shortAddress == shortAddr)
         {
            macSecurityPib.macDeviceTable[i].keyUsage &= ~(MAC_KEY_USAGE_RX | MAC_KEY_USAGE_TX);
            break;
         }
      }
   }
}

/*******************************************************************************
* @fn
*
* @brief
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/
void dtls_handshake_failed(dtls_context_t *ctx, dtls_peer_t *peer)
{
   uint16_t i;
   uip_lladdr_t *lladdr_p = uip_ds6_nbr_lladdr_from_ipaddr(&(peer->session.addr));
   uint16_t shortAddr = lladdr_p->addr[1] + lladdr_p->addr[0]*256;

   for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
   {
      if (macSecurityPib.macDeviceTable[i].shortAddress == shortAddr &&  macSecurityPib.macDeviceTable[i].keyUsage != 0)
      {
         macSecurityPib.macDeviceTable[i].keyUsage &= ~(MAC_KEY_USAGE_RX | MAC_KEY_USAGE_TX);
         break;
      }
   }
}
#endif // LINUX_GATEWAY

static uint16_t lockCount;
/***************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 * @param[Out]
 * @return
 ******************************************************************************/
uint32_t dtls_context_lock()
{
   uint32_t key = 0;
#ifndef LINUX_GATEWAY
   key = Task_disable();
#endif //LINUX_GATEWAY
   lockCount++;
   return(key);
}
/***************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 * @param[Out]
 * @return
 ******************************************************************************/
void dtls_context_unlock(uint32_t key)
{
    lockCount--;
#ifndef LINUX_GATEWAY
    Task_restore(key);
#endif //LINUX_GATEWAY
}

/***************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 * @param[Out]
 * @return
 ******************************************************************************/
void dtls_task_sleep(uint32_t millisecs)
{
#ifdef LINUX_GATEWAY
   //usleep(millisecs*1000);
#else //
   //Task_sleep(millisecs*CLOCK_SECOND/1000);
#endif  //LINUX_GATEWAY
}

/***************************************************************************
 * @fn          send_to_peer
 *
 * @brief      Sends the encrypted packet to the peer.
 * @param[in]
 * @param[Out]
 * @return     > 0 if successful
 ******************************************************************************/
static int send_to_peer(struct dtls_context_t *ctx,
                        session_t *session, uint8 *data, size_t len)
{
#if FEATURE_DTLS_TEST
   extern uint8_t testDtlsUdpIpHeader[UIP_IPH_LEN + UIP_UDPH_LEN];

   if (uip_input_len != 0)
   {
      //FIFO full
      uipTxPacketLossCount++;
      len = 0;
   }
   else
   {
   uipBuffer_LockWriteBuffer();
   memcpy(&uip_input_buf[UIP_LLH_LEN], testDtlsUdpIpHeader, UIP_IPH_LEN + UIP_UDPH_LEN);
   memcpy(&uip_input_buf[UIP_LLH_LEN + UIP_IPH_LEN + UIP_UDPH_LEN], data, len);
   uip_input_len = UIP_LLIPH_LEN + UIP_UDPH_LEN + len;

   UIP_IP_INPUT_BUF->len0 = ((len + UIP_UDPH_LEN) >> 8);
   UIP_IP_INPUT_BUF->len1 = ((len + UIP_UDPH_LEN) & 0xff);
   UIP_UDP_INPUT_BUF->udplen = UIP_HTONS(len + UIP_UDPH_LEN);
   UIP_UDP_INPUT_BUF->udpchksum = 0;
   UIP_UDP_INPUT_BUF->destport = UIP_HTONS(coapConfig.coap_dtls_port);

   /* Calculate UDP checksum. */
   UIP_UDP_INPUT_BUF->udpchksum = udp_chksum(uip_input_buf);
   uipBufferIndex_inc(&uipBuffer_WriteIdx);
   uipBuffer_UnlockWriteBuffer();

   tcpip_input(NULL);
   }
#else  //FEATURE_DTLS_TEST
   uip_udp_packet_send_post((struct udp_simple_socket*)ctx->udp_socket, data, len, &session->addr, session->port);
#endif  //FEATURE_DTLS_TEST
   return len;
}

#ifdef DTLS_PSK
struct keymap_t psk[] =
{
   {
      (unsigned char *)"Client_identity", 15,
      (unsigned char *)"secretPSK", 9
   },
};
#define NUM_OF_PSK (sizeof(psk) / sizeof(sizeof(psk[0])))

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__((unused))
#else  //__GNUC__
#define UNUSED_PARAM
#endif /* __GNUC__ */

/***************************************************************************
 * @fn          get_psk_info
 *
 * @brief      This function is the "key store" for tinyDTLS. It is called to
 * retrieve a key for the given identity within this particular
 * session.
 * @param[in]
 * @param[Out]
 * @return     > 0 if successful
 ******************************************************************************/
static int get_psk_info(struct dtls_context_t *ctx, const session_t *session,
                        dtls_credentials_type_t type,
                        const unsigned char *id, size_t id_len,
                        unsigned char *result, size_t result_length)
{
   switch(type)
   {
      case DTLS_PSK_KEY:
         if (id != NULL)
         {
            int i;

            for (i = 0; i < NUM_OF_PSK; i++)
            {
               if (id_len == psk[i].id_length && memcmp(id, psk[i].id, id_len) == 0)
               {
                  if (result_length < psk[i].key_length)
                  {
                     dtls_warn("buffer too small for PSK");
                     return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
                  }

                  memcpy(result, psk[i].key, psk[i].key_length);
                  return psk[i].key_length;
               }
            }
         }

         break;

      case DTLS_PSK_IDENTITY:
         if (result_length < psk[0].id_length)
         {
            dtls_warn("cannot set psk_identity -- buffer too small\n");
            return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
         }

         memcpy(result, psk[0].id, psk[0].id_length);
         return psk[0].id_length;

      default:
         return 0;
   }

   return dtls_alert_fatal_create(DTLS_ALERT_DECRYPT_ERROR);
}
#endif /* // DTLS_PSK */

#ifdef DTLS_ECC
static int
get_ecdsa_key(struct dtls_context_t *ctx,
         const session_t *session,
         const dtls_ecdsa_key_t **result) {
  static const dtls_ecdsa_key_t ecdsa_key = {
    .curve = DTLS_ECDH_CURVE_SECP256R1,
    .priv_key = ecdsa_priv_key,
    .pub_key_x = ecdsa_pub_key_x,
    .pub_key_y = ecdsa_pub_key_y
  };

  *result = &ecdsa_key;
  return 0;
}

static int
verify_ecdsa_key(struct dtls_context_t *ctx,
       const session_t *session,
       const unsigned char *other_pub_x,
       const unsigned char *other_pub_y,
       size_t key_size) {
  return 0;
}
#endif /* DTLS_ECC */

/***************************************************************************
 * @fn          dtls_set_udp_socket
 *
 * @brief   Set the UDP socket for the DTLS
 * @param[in]  ctx - DTLS global variables
 * @param[in]  coap_dtls_udp_socket_p - The DLTLS UDP socket
 * @param[Out]
 ******************************************************************************/
void dtls_set_udp_socket(dtls_context_t *ctx, struct udp_simple_socket *coap_dtls_udp_socket_p)
{
   if (ctx)
   {
      dtls_peer_t *p;

      for (p = list_head(ctx->peers); p; p = list_item_next(p))
      {
         p->state = DTLS_STATE_CLOSING;
         dtls_destroy_peer(ctx, p, 1);
      }

      ctx->udp_socket = coap_dtls_udp_socket_p;
      ctx->app = coap_dtls_udp_socket_p->udp_conn;
   }
}

/***************************************************************************
 * @fn          init_dtls
 *
 * @brief
 * @param[in]  appDataHandler - function to handle the data after the decryption is done.
 * @param[in]  appData_p - argument to appDataHandler
 * @param[Out]
 ******************************************************************************/
void init_dtls(uint8_t dtlsCntxtIdx, struct udp_simple_socket *coap_dtls_udp_socket_p,
      process_obj_t *proc_obj_p, appDataHandler_t appDataHandler, void *appData_p)
{
   static dtls_handler_t cb;

   if (cb.write == NULL)
   {
      cb.write = send_to_peer;
      cb.read  = read_from_peer;
      cb.event = NULL;
#ifdef DTLS_PSK
      cb.get_psk_info = get_psk_info;
#endif /* DTLS_PSK */
#ifdef DTLS_ECC
      cb.get_ecdsa_key = get_ecdsa_key;
      cb.verify_ecdsa_key = verify_ecdsa_key;
#endif /* DTLS_ECC */

      dtls_init();
      dtls_set_log_level(DTLS_LOG_INFO);
   }

   if (dtlsCntxtIdx < MAX_NUMBER_DTLS_CONTEXT)
   {
      dtls_context[dtlsCntxtIdx] = dtls_new_context(&the_dtls_context[dtlsCntxtIdx], coap_dtls_udp_socket_p->udp_conn);
      if (dtls_context[dtlsCntxtIdx])
      {
#ifdef DTLS_WITH_TIRTOS
         dtls_context[dtlsCntxtIdx]->dtls_process_obj_p = proc_obj_p;
#endif //DTLS_WITH_TIRTOS
         dtls_context[dtlsCntxtIdx]->udp_socket = coap_dtls_udp_socket_p;
         dtls_context[dtlsCntxtIdx]->appDataHandler = appDataHandler;
         dtls_context[dtlsCntxtIdx]->appData_p = appData_p;
         dtls_context[dtlsCntxtIdx]->dtls_context_idx = dtlsCntxtIdx;
         dtls_set_handler(dtls_context[dtlsCntxtIdx], &cb);
      }
   }
}

/***************************************************************************
 * @fn          dtls_handle_read
 *
 * @brief      Handles the received DTLS packets
 * @param[in]  ctx - DTLS global variables data structure
 * @param[Out]
 ******************************************************************************/
void dtls_handle_read(dtls_context_t *ctx)
{
   uint16_t destPort = UIP_NTOHS(APP_UIP_UDP_BUF->destport);

   if (destPort != coapConfig.coap_dtls_port  && destPort != coapConfig.coap_mac_sec_port)
   {
      if (ctx->appDataHandler != NULL)
      {
         ctx->appDataHandler(ctx->appData_p);
      }
   }
   else
   {
      if (destPort == coapConfig.coap_mac_sec_port)
      {
         ctx = dtls_context[1];
      }

      uint8_t *uip_rx_appdata = (uint8_t*)&(appRxFifo[appRxFifo_ReadIdx].c[UIP_IPUDPH_LEN + UIP_LLH_LEN]);
      uint16_t uip_rx_app_datalen = appRxLenFifo[appRxFifo_ReadIdx];
      session_t session;
      memset(&session, 0, sizeof(session));
      uip_ipaddr_copy(&session.addr, &APP_UIP_IP_BUF->srcipaddr);
      session.port = UIP_NTOHS(APP_UIP_UDP_BUF->srcport);
      session.size = sizeof(session.addr) + sizeof(session.port);

      dtls_handle_message(ctx, &session, uip_rx_appdata, uip_rx_app_datalen);
   }
}

#endif  //FEATURE_DTLS
