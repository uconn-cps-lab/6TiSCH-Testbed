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
#if (I3_MOTE_SPLIT_ARCH)
#include "../uip/net/uip-udp-packet.h"
#include "../uip/net/uip.h"
#include "../uip/lib/random.h"
#include "../uip/sys/process.h"
#include "../uip/uip_rpl_impl/sysbios/uip_rpl_process.h"
#include "../uip/net/uip-ds6-nbr.h"
#include "nv_params.h"
#include "hct_msp432/hct/msp432hct.h"
#include "remoteFuncCall.h"

#include "../tsch/nhl_api.h"
#include "../tsch/nhl_setup.h"
#include "../tsch/nhl_mgmt.h"
#include "../tsch/mac_reset.h"
#include "../tsch/mac_tsch_security.h"
#include "../apps/coap/config.h"

#include "Board.h"
#include "SensorBatMon.h"
#include "led.h"
#if SENSORTAG || I3_MOTE
#include "SensorI2C.h"
#include "SensorTmp007.h"
#include "SensorHDC1000.h"
#include "SensorOpt3001.h"
#include "SensorBmp280.h"
#include "SensorMpu9250.h"
#include "SensorLIS2HH12.h"
#include "SensorLIS2DW12.h"
#endif

#include "pwrest/pwrest.h"

#if (FEATURE_DTLS)
#include "../tsch/bsp_aes.h"
#include "../dtls/dtls_client.h"
#endif  //FEATURE_DTLS

#if (FEATURE_RFC_DTLS)
#define DTLS_HANDSHAKE_TIMEOUT      (2*60*CLOCK_SECOND)

dtls_context_t dtls_context_store[MAX_NUMBER_DTLS_CONTEXT];
dtls_context_t *dtls_context[MAX_NUMBER_DTLS_CONTEXT];

void coap_task_ipc_handler(int timeoutVal);

static bool RFC_dtls_app_data_from_peer_handler();
static bool RFC_dtls_encrypted_data_to_peer_handler();
static bool RFC_dtls_set_etimer_handler();
static bool RFC_dtls_etimer_stop_handler();
static bool RFC_dtls_bspAesCcmAuthEncrypt_handler();
static bool RFC_dtls_bspAesCcmAuthDecrypt_handler();
static bool RFC_dtls_clock_time_handler();
static bool RFC_dtls_reandom_rand_handler();
static bool RFC_dtls_enc_key_handler();
static bool RFC_dtls_activate_key_handler();
static bool RFC_dtls_handshake_failed_handler();

#endif //FEATURE_RFC_DTLS
#if (FEATURE_RFC_COAP)
static bool RFC_coap_set_notifier_etimer_handler();
static bool RFC_coap_query_notifier_etimer_expired_handler();
static bool RFC_coap_notifier_etimer_reset_handler();
static bool RFC_coap_query_retransmit_etimer_expired_handler();
static bool RFC_coap_set_retransmit_etimer_handler();
static bool RFC_coap_get_schedule_handler();
static bool RFC_coap_schedule_add_slot_handler();
static bool RFC_coap_schedule_remove_slot_handler();
static bool RFC_coap_webmsg_update_handler();
static bool RFC_coap_led_set_handler();
static bool RFC_coap_get_diagnosis_message_handler();
static bool RFC_coap_put_diagnosis_message_handler();
static bool RFC_coap_get_nvm_params_handler();
static bool RFC_coap_put_nvm_params_handler();
static bool RFC_uip_udp_packet_send_handler();
#endif //FEATURE_RFC_COAP

static bool RFC_pull_handler();

const struct structRfcFunctionDispatchTableEntry rfc_function_dispatch_table_x[] =
{
#if (FEATURE_RFC_DTLS)
   {RFC_FUNC_ID_DTLS_APP_DATA_FROM_PEER,              RFC_dtls_app_data_from_peer_handler},
   {RFC_FUNC_ID_DTLS_ENCRYPTED_DATA_TO_PEER,          RFC_dtls_encrypted_data_to_peer_handler},
   {RFC_FUNC_ID_DTLS_SET_RETRANSMIT_ETIMER,           RFC_dtls_set_etimer_handler},
   {RFC_FUNC_ID_DTLS_RETRANSMIT_ETIMER_STOP,          RFC_dtls_etimer_stop_handler},
   {RFC_FUNC_ID_DTLS_BSP_AES_CCM_AUTH_ENCRYPT,        RFC_dtls_bspAesCcmAuthEncrypt_handler},
   {RFC_FUNC_ID_DTLS_BSP_AES_CCM_AUTH_DECRYPT,        RFC_dtls_bspAesCcmAuthDecrypt_handler},
   {RFC_FUNC_ID_DTLS_ENC_KEY,                         RFC_dtls_enc_key_handler},
   {RFC_FUNC_ID_DTLS_ACTIVATE_KEY,                    RFC_dtls_activate_key_handler},
   {RFC_FUNC_ID_DTLS_HANDSHAKE_FAILED,                RFC_dtls_handshake_failed_handler},
#endif //FEATURE_RFC_DTLS
#if (FEATURE_RFC_COAP)
   {RFC_FUNC_ID_COAP_SET_NOTIFIER_ETIMER,             RFC_coap_set_notifier_etimer_handler},
   {RFC_FUNC_ID_COAP_QUERY_NOTIFIER_ETIMER_EXPIRED,   RFC_coap_query_notifier_etimer_expired_handler},
   {RFC_FUNC_ID_COAP_NOTIFIER_ETIMER_RESET,           RFC_coap_notifier_etimer_reset_handler},
   {RFC_FUNC_ID_COAP_QUERY_RETRANSMIT_ETIMER_EXPIRED, RFC_coap_query_retransmit_etimer_expired_handler},
   {RFC_FUNC_ID_COAP_SET_RETRANSMIT_ETIMER,           RFC_coap_set_retransmit_etimer_handler},
   {RFC_FUNC_ID_COAP_GET_SCHEDULE,                    RFC_coap_get_schedule_handler},
   {RFC_FUNC_ID_COAP_SCHEDULE_ADD_SLOT,               RFC_coap_schedule_add_slot_handler},
   {RFC_FUNC_ID_COAP_SCHEDULE_REMOVE_SLOT,            RFC_coap_schedule_remove_slot_handler},
   {RFC_FUNC_ID_COAP_WEBMSG_UPDATE,                   RFC_coap_webmsg_update_handler},
   {RFC_FUNC_ID_COAP_LED_SET,                         RFC_coap_led_set_handler},
   {RFC_FUNC_ID_COAP_GET_DIAGNOSIS_MESSAGE,           RFC_coap_get_diagnosis_message_handler},
   {RFC_FUNC_ID_COAP_PUT_DIAGNOSIS_MESSAGE,           RFC_coap_put_diagnosis_message_handler},
   {RFC_FUNC_ID_COAP_GET_NVM_PARAMS,                  RFC_coap_get_nvm_params_handler},
   {RFC_FUNC_ID_COAP_PUT_NVM_PARAMS,                  RFC_coap_put_nvm_params_handler},
   {RFC_FUNC_ID_UIP_UDP_PACKET_SEND,                  RFC_uip_udp_packet_send_handler},
#endif //FEATURE_RFC_COAP
   {RFC_FUNC_ID_DTLS_CLOCK_TIME,                      RFC_dtls_clock_time_handler},
   {RFC_FUNC_ID_DTLS_RANDOM_RAND,                     RFC_dtls_reandom_rand_handler},
   {RFC_FUNC_ID_PULL,                                 RFC_pull_handler},
};
#define DISPATCH_TABLE_SIZE   (sizeof(rfc_function_dispatch_table_x) / sizeof(rfc_function_dispatch_table_x[0]))
void RFC_set_dispatch_table()
{
   RFC_registerDispatchTable(REMOTE_MSP432, (const struct structRfcFunctionDispatchTableEntry *)rfc_function_dispatch_table_x, DISPATCH_TABLE_SIZE);
}

extern void uip_udp_packet_send_post(struct udp_simple_socket *s, void *data, int len,
                                     const uip_ipaddr_t *toaddr, uint16_t toport);


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
#define MAX_PULL_NEST_LEVEL (0)
uint8_t pullNestLevel;
static bool RFC_pull_handler()
{
   bool status = 0;
   uint8_t rspBuf[1] = {1};

   //Get the RUN message
   status = RFC_getFunctionRunCommand(REMOTE_MSP432);

   if (status)  //Handle the call
   {
#if (MAX_PULL_NEST_LEVEL > 0)
       if (pullNestLevel < MAX_PULL_NEST_LEVEL)
       {
           pullNestLevel++;
           coap_task_ipc_handler(BIOS_NO_WAIT);
           pullNestLevel--;
       }
#endif //(MAX_PULL_NEST_LEVEL > 0)
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, rspBuf, 1);
   }

   return(status);
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

static bool RFC_dtls_clock_time_handler()
{
   bool status = 0;
   clock_time_t rspBuf;

   //Get the RUN message
   status = RFC_getFunctionRunCommand(REMOTE_MSP432);

   if (status)  //Handle the call
   {
      rspBuf = clock_time();
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, (uint8_t *)&rspBuf, sizeof(rspBuf));
   }

   return(status);
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

static bool RFC_dtls_reandom_rand_handler()
{
   bool status = 0;
   uint16_t retv;

   //Get the RUN message
   status = RFC_getFunctionRunCommand(REMOTE_MSP432);

   if (status)  //Handle the call
   {
      retv = random_rand();
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, (uint8_t*)&retv, sizeof(retv));
   }

   return(status);
}

static bool RFC_dtls_enc_key_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint8_t *payloadBuf_p;
   uint16_t payloadBufSize;
   uip_ip6addr_t srcipaddr;
   uint8_t dtls_context_idx;
   uint16_t paramLength;
   uint8_t rsp[1] = {1};

   RFC_getReceiveBuffer(REMOTE_MSP432, &payloadBuf_p, &payloadBufSize);

   //Get the DTLS_dtls_context_idx as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, &dtls_context_idx, 1);
   parameterOk = status && parameterOk && (paramLength > 0) && (dtls_context_idx < MAX_NUMBER_DTLS_CONTEXT);

   if (status)
   {
      //Get the srcipaddr_p as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, srcipaddr.u8, sizeof(srcipaddr.u8));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcipaddr.u8));
   }

   if (status)
   {
      //Get the key as the 3rd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, payloadBuf_p, payloadBufSize);
      parameterOk = status && parameterOk && (paramLength == DTLS_KEY_LENGTH);
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }


   if (status && parameterOk)  //Handle the call
   {
      uip_lladdr_t *lladdr_p = uip_ds6_nbr_lladdr_from_ipaddr(&srcipaddr);
      macSecurityAddPairwiseKey(lladdr_p->addr[1] + lladdr_p->addr[0]*256, payloadBuf_p);  //BE
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, rsp, 1);
   }

   return(status && parameterOk);
}


static bool RFC_dtls_activate_key_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uip_ip6addr_t srcipaddr;
   uint8_t dtls_context_idx;
   uint16_t paramLength;
   uint8_t rsp[1] = {1};

   //Get the DTLS_dtls_context_idx as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, &dtls_context_idx, 1);
   parameterOk = status && parameterOk && (paramLength > 0) && (dtls_context_idx < MAX_NUMBER_DTLS_CONTEXT);

   if (status)
   {
      //Get the srcipaddr_p as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, srcipaddr.u8, sizeof(srcipaddr.u8));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcipaddr.u8));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }


   if (status && parameterOk)  //Handle the call
   {
      uip_lladdr_t *lladdr_p = uip_ds6_nbr_lladdr_from_ipaddr(&srcipaddr);
      macSecurityActivatePairwiseKey(lladdr_p->addr[1] + lladdr_p->addr[0]*256);  //BE
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, rsp, 1);
   }

   return(status && parameterOk);
}


static bool RFC_dtls_handshake_failed_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uip_ip6addr_t srcipaddr;
   uint8_t dtls_context_idx;
   uint16_t paramLength;
   uint8_t rsp[1] = {1};

   //Get the DTLS_dtls_context_idx as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, &dtls_context_idx, 1);
   parameterOk = status && parameterOk && (paramLength > 0) && (dtls_context_idx < MAX_NUMBER_DTLS_CONTEXT);

   if (status)
   {
      //Get the srcipaddr_p as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, srcipaddr.u8, sizeof(srcipaddr.u8));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcipaddr.u8));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }


   if (status && parameterOk)  //Handle the call
   {
      uip_lladdr_t *lladdr_p = uip_ds6_nbr_lladdr_from_ipaddr(&srcipaddr);
      macSecurityDeletePairwiseKey(lladdr_p->addr[1] + lladdr_p->addr[0]*256);  //BE
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, rsp, 1);
   }

   return(status && parameterOk);
}
#if (FEATURE_RFC_DTLS)
/***************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 * @param[Out]
 * @return
 ******************************************************************************/
static bool get_peer_callback(void *cbCtxt_p, struct structRfcMsgheader *msgHdr_p, uint8_t *resp_p)
{
   uint8_t status = 1;
   uint16_t idx = 0;
   DtlsPeer_t *peer_p = *(DtlsPeer_t **)cbCtxt_p;

   if (resp_p[idx] == 0)
   {
      status = 0;
      *(DtlsPeer_t **)cbCtxt_p = NULL;
   }

   idx++;

   if (status && peer_p != NULL)
   {
      peer_p->role = (dtls_peer_type)resp_p[idx++];
      peer_p->state = (dtls_state_t)resp_p[idx++];
   }

   return(status);
}
/***************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 * @param[Out]
 * @return
 * *******************************************/
DtlsPeer_t *dtlsGetPeer(const dtls_context_t *ctx, const session_t *session, DtlsPeer_t *peerBuf_p)
{
   uint8_t *arg[3];
   uint16_t argLen[3];
   DtlsPeer_t *peer_p = peerBuf_p;
   bool status;

   argLen[0] = 1;
   arg[0] = (uint8_t *) & (ctx->dtls_context_idx);
   argLen[1] = sizeof(session->addr.u8);
   arg[1] = (uint8_t *)session->addr.u8;
   argLen[2] = sizeof(session->port);
   arg[2] = (uint8_t*) & (session->port);

   status = RFC_call(REMOTE_MSP432, RFC_FUNC_ID_DTLS_GET_PEER_INFO, 3, argLen, arg, get_peer_callback, &peer_p);

   if (!status)
   {
      peer_p = NULL;
   }

   return(peer_p);
}
/***************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 * @param[Out]
 * @return     > 0 if successful
 ******************************************************************************/
static dtls_context_t *dtls_get_context(uint8_t cntxtIdx)
{
   if (cntxtIdx < MAX_NUMBER_DTLS_CONTEXT)
   {
      return(dtls_context[cntxtIdx]);
   }
   else
   {
      return(NULL);
   }
}
/***************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 * @param[Out]
 * @return
 ******************************************************************************/
static bool peer_uint8_return_callback(void *cbCtxt_p, struct structRfcMsgheader *msgHdr_p, uint8_t *resp_p)
{
   uint8_t status = 1;
   *(uint8_t *)cbCtxt_p = resp_p[0];
   return(status);
}
/**
 * Handles incoming data as DTLS message from given peer.
 */

int dtls_connect_to_server(dtls_context_t *ctx,
                        session_t *session)
{
   uint8_t *arg[3];
   uint16_t argLen[3];
   uint8_t status;
   uint8_t retv;
   int retVal = 0;

   argLen[0] = 1;
   arg[0] = &(ctx->dtls_context_idx);
   argLen[1] = sizeof(session->addr.u8);
   arg[1] = session->addr.u8;
   argLen[2] = sizeof(session->port);
   arg[2] = (uint8_t*) & (session->port);

   status = RFC_call(REMOTE_MSP432, RFC_FUNC_ID_DTLS_CONNECT_PEER, 3, argLen, arg, peer_uint8_return_callback, &retv);

   if (!(status && retv))
   {
      retVal = -5678;
   }

   return(retVal);
}
/**
 * Handles incoming data as DTLS message from given peer.
 */

int dtls_handle_message(dtls_context_t *ctx,
                        session_t *session,
                        uint8 *msg, int msglen)
{
   uint8_t *arg[4];
   uint16_t argLen[4];
   uint8_t status;
   uint8_t retv;
   int retVal = 0;

   argLen[0] = 1;
   arg[0] = &(ctx->dtls_context_idx);
   argLen[1] = msglen;
   arg[1] = msg;
   argLen[2] = sizeof(session->addr.u8);
   arg[2] = session->addr.u8;
   argLen[3] = sizeof(session->port);
   arg[3] = (uint8_t*) & (session->port);

   status = RFC_call(REMOTE_MSP432, RFC_FUNC_ID_DTLS_HANDLE_READ, 4, argLen, arg, peer_uint8_return_callback, &retv);

   if (!(status && retv))
   {
      retVal = -3456;
   }

   return(retVal);
}

/***************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 * @param[Out]
 * @return
 ******************************************************************************/
void dtls_handle_retransmit(dtls_context_t *ctx)
{
   uint8_t *arg[1];
   uint16_t argLen[1];
   uint8_t retv;

   argLen[0] = 1;
   arg[0] = &(ctx->dtls_context_idx);

   RFC_call(REMOTE_MSP432, RFC_FUNC_ID_DTLS_HANDLE_RETRANSMISSION, 1, argLen, arg, peer_uint8_return_callback, &retv);
}

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
      ctx->udp_socket = coap_dtls_udp_socket_p;
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
   if (dtlsCntxtIdx == 0)
   {
      clock_time_t now;
      clock_init();
      now = clock_time();
      random_init(now);
   }

   if (dtlsCntxtIdx < MAX_NUMBER_DTLS_CONTEXT)
   {
      uint8_t *arg[1];
      uint16_t argLen[1];
      uint8_t retv;

      dtls_context[dtlsCntxtIdx] = &(dtls_context_store[dtlsCntxtIdx]);
      memset(dtls_context[dtlsCntxtIdx], 0, sizeof(*(dtls_context[dtlsCntxtIdx])));
      dtls_context[dtlsCntxtIdx]->dtls_process_obj_p = proc_obj_p;
      dtls_context[dtlsCntxtIdx]->dtls_context_idx = dtlsCntxtIdx;
      dtls_context[dtlsCntxtIdx]->udp_socket = coap_dtls_udp_socket_p;
      dtls_context[dtlsCntxtIdx]->appDataHandler = appDataHandler;
      dtls_context[dtlsCntxtIdx]->appData_p = appData_p;

      argLen[0] = 1;
      arg[0] = &dtlsCntxtIdx;
      RFC_call(REMOTE_MSP432, RFC_FUNC_ID_DTLS_INIT, 1, argLen, arg, peer_uint8_return_callback, &retv);
   }
}
/***************************************************************************
 * @fn          dtls_write
 *
 * @brief      Encode and send the app data
 * @param[in]
 * @param[Out]
 ******************************************************************************/
int dtls_write(struct dtls_context_t *ctx,
               session_t *dst, uint8 *buf, size_t len)
{
   uint8_t *arg[4];
   uint16_t argLen[4];
   uint8_t status;
   uint8_t retv;
   int retVal = 0;

   argLen[0] = 1;
   arg[0] = &(ctx->dtls_context_idx);
   argLen[1] = len;
   arg[1] = buf;
   argLen[2] = sizeof(dst->addr.u8);
   arg[2] = dst->addr.u8;
   argLen[3] = sizeof(dst->port);
   arg[3] = (uint8_t*) & (dst->port);

   status = RFC_call(REMOTE_MSP432, RFC_FUNC_ID_DTLS_WRITE, 4, argLen, arg, peer_uint8_return_callback, &retv);

   if (!(status && retv))
   {
      retVal = -3456;
   }

   return(retVal);
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
   if (appRxLenFifo[appRxFifo_ReadIdx] > 0)
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
          uint8_t *uip_rx_appdata = (uint8_t*) & (appRxFifo[appRxFifo_ReadIdx].c[UIP_IPUDPH_LEN + UIP_LLH_LEN]);
          uint16_t uip_rx_app_datalen = appRxLenFifo[appRxFifo_ReadIdx];
          session_t session;
          uint8_t *payloadBuf_p;
          uint16_t payloadBufSize;

          RFC_getTransmitBuffer(REMOTE_MSP432, &payloadBuf_p, &payloadBufSize);

          if (uip_rx_app_datalen <= payloadBufSize )
          {
              memcpy(payloadBuf_p, uip_rx_appdata, uip_rx_app_datalen);
              uip_rx_appdata = payloadBuf_p;

              memset(&session, 0, sizeof(session));
              uip_ipaddr_copy(&session.addr, &APP_UIP_IP_BUF->srcipaddr);
              session.port = UIP_NTOHS(APP_UIP_UDP_BUF->srcport);
              session.size = sizeof(session.addr) + sizeof(session.port);

              appRxLenFifo[appRxFifo_ReadIdx] = 0;
              //checkStack();
              dtls_handle_message(ctx, &session, uip_rx_appdata, uip_rx_app_datalen);
          }
          else
          {
              appRxLenFifo[appRxFifo_ReadIdx] = 0;
          }
       }
   }
}

/*******************************************************************************
* @fn
*
* @brief  Handles packet received from the DTLS peer
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

static bool RFC_dtls_app_data_from_peer_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   int16_t retv = -1;
   uint16_t uip_datalen;
   uip_ip6addr_t srcipaddr;
   uint16_t srcport;
   uint8_t dtls_context_idx;
   uint16_t paramLength;
   uint8_t *payloadBuf_p;
   uint16_t payloadBufSize;

   RFC_getReceiveBuffer(REMOTE_MSP432, &payloadBuf_p, &payloadBufSize);

   //Get the DTLS_dtls_context_idx as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, &dtls_context_idx, 1);
   parameterOk = status && parameterOk && (paramLength > 0) && (dtls_context_idx < MAX_NUMBER_DTLS_CONTEXT);

   if (status)
   {
      //Get the uip_trx_data as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &uip_datalen, payloadBuf_p, payloadBufSize);
      parameterOk = status && parameterOk && (uip_datalen > 0);
   }

   if (status)
   {
      //Get the srcipaddr_p as the 3rd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, srcipaddr.u8, sizeof(srcipaddr.u8));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcipaddr.u8));
   }

   if (status)
   {
      //Get the srcport as the 4th function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t *)&srcport, sizeof(srcport));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcport));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }

   if (status && parameterOk)  //Handle the call
   {
      dtls_context_t *ctx_p = dtls_get_context(dtls_context_idx);

      if (ctx_p->appDataHandler != NULL && appRxLenFifo[appRxFifo_ReadIdx] == 0)
      {
         appRxLenFifo[appRxFifo_ReadIdx] = uip_datalen;
         memcpy(appRxFifo[appRxFifo_ReadIdx].c + UIP_IPUDPH_LEN + UIP_LLH_LEN, payloadBuf_p,
                uip_datalen);
         ctx_p->appDataHandler(ctx_p->appData_p);
      }

      retv = 1;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, (uint8_t*)&retv, sizeof(retv));
   }

   return(status && parameterOk);
}

/*******************************************************************************
* @fn
*
* @brief  Handles packet received from the DTLS peer
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

static bool RFC_dtls_encrypted_data_to_peer_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   int16_t retv = -1;
   uint16_t uip_datalen;
   uip_ip6addr_t srcipaddr;
   uint16_t srcport;
   uint8_t dtls_context_idx;
   uint16_t paramLength;
   uint8_t *payloadBuf_p;
   uint16_t payloadBufSize;

   RFC_getReceiveBuffer(REMOTE_MSP432, &payloadBuf_p, &payloadBufSize);

   //Get the DTLS_dtls_context_idx as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, &dtls_context_idx, 1);
   parameterOk = status && parameterOk && (paramLength > 0) && (dtls_context_idx < MAX_NUMBER_DTLS_CONTEXT);

   if (status)
   {
      //Get the uip_trx_data as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &uip_datalen, payloadBuf_p, payloadBufSize);
      parameterOk = status && parameterOk && (uip_datalen > 0);
   }

   if (status)
   {
      //Get the srcipaddr_p as the 3rd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, srcipaddr.u8, sizeof(srcipaddr.u8));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcipaddr.u8));
   }

   if (status)
   {
      //Get the srcport as the 4th function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t *)&srcport, sizeof(srcport));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcport));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }

   if (status && parameterOk)  //Handle the call
   {
      dtls_context_t *ctx_p = dtls_get_context(dtls_context_idx);
      uip_udp_packet_send_post((struct udp_simple_socket*)ctx_p->udp_socket, payloadBuf_p, uip_datalen, &srcipaddr, srcport);
      retv = RFC_FUNC_RETV_ENCRYPTED_DATA_TO_PEER_OK;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, (uint8_t*)&retv, sizeof(retv));
   }

   return(status && parameterOk);
}

/*******************************************************************************
* @fn
*
* @brief  Handles packet received from the DTLS peer
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

static bool RFC_dtls_set_etimer_handler()
{
   bool status = 0;
   uint16_t paramLength;
   bool parameterOk = 1;
   int16_t retv = -1;
   clock_time_t interval;
   uint8_t dtls_context_idx;

   //Get the DTLS_dtls_context_idx as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, &dtls_context_idx, 1);
   parameterOk = status && parameterOk && (paramLength > 0) && (dtls_context_idx < MAX_NUMBER_DTLS_CONTEXT);

   if (status)
   {
      //Get the timer interval as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&interval, sizeof(interval));
      parameterOk = status && parameterOk && (paramLength == sizeof(interval));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }

   if (status && parameterOk)  //Handle the call
   {
      dtls_context_t *ctx_p = dtls_get_context(dtls_context_idx);
      etimer_set(&ctx_p->retransmit_timer, ctx_p->dtls_process_obj_p, interval);
      retv = 1;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, (uint8_t*)&retv, sizeof(retv));
   }

   return(status && parameterOk);
}

/*******************************************************************************
* @fn
*
* @brief  Handles packet received from the DTLS peer
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

static bool RFC_dtls_etimer_stop_handler()
{
   bool status = 0;
   int16_t retv = -1;
   bool parameterOk = 1;
   uint8_t dtls_context_idx;
   uint16_t paramLength;

   //Get the DTLS_dtls_context_idx as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, &dtls_context_idx, 1);
   parameterOk = status && parameterOk && (paramLength > 0) && (dtls_context_idx < MAX_NUMBER_DTLS_CONTEXT);

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }

   if (status && parameterOk)  //Handle the call
   {
      dtls_context_t *ctx_p = dtls_get_context(dtls_context_idx);
      etimer_stop(&ctx_p->retransmit_timer);
      retv = 1;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, (uint8_t*)&retv, sizeof(retv));
   }

   return(status && parameterOk);
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
static bool RFC_dtls_bspAesCcmAuthEncDec_handler(uint8_t isEnc)
{
   bool status = 0;
   bool parameterOk = 1;
   uint16_t paramLength;
   char key[16];
   uint32_t keylocation;
   uint32_t authlen;
   char nonce[DTLS_CCM_BLOCKSIZE];
   char *plaintext;
   uint32_t plaintextlen;
   char header[13];
   uint32_t fieldlen;
   char tag[8];
   uint8_t *payloadBuf_p;
   uint16_t payloadBufSize;

   RFC_getReceiveBuffer(REMOTE_MSP432, &payloadBuf_p, &payloadBufSize);
   payloadBuf_p[0] = 0;
   plaintext = (char *)(payloadBuf_p + 1);

   //Get the key as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)key, sizeof(key));
   parameterOk = status && parameterOk && (paramLength == sizeof(key));

   if (status)
   {
      //Get the keylocation as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&keylocation, sizeof(keylocation));
      parameterOk = status && parameterOk && (paramLength == sizeof(keylocation));
   }

   if (status)
   {
      //Get the authlen as the 3rd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&authlen, sizeof(authlen));
      parameterOk = status && parameterOk && (paramLength == sizeof(authlen));
   }

   if (status)
   {
      //Get the nonce as the 4th function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)nonce, sizeof(nonce));
      parameterOk = status && parameterOk && (paramLength == sizeof(nonce));
   }

   if (status)
   {
      //Get the plaintext as the 5th function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)plaintext, payloadBufSize - 9);
      plaintextlen = paramLength;
      parameterOk = status && parameterOk && (paramLength > 0);
   }

   if (status)
   {
      //Get the header as the 6th function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)header, sizeof(header));
      parameterOk = status && parameterOk && (paramLength == sizeof(header));
   }

   if (status)
   {
      //Get the fieldlen as the 7th function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&fieldlen, sizeof(fieldlen));
      parameterOk = status && parameterOk && (paramLength == sizeof(fieldlen));
   }

   if (status)
   {
      //Get the tag as the 8th function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)tag, sizeof(tag));
      parameterOk = status && parameterOk && (paramLength == sizeof(tag));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }

   paramLength = 0;

   if (status && parameterOk)  //Handle the call
   {
      if (isEnc)
      {
         uint16_t i;

         payloadBuf_p[0] = bspAesCcmAuthEncrypt(key, CRYPTOCC26XX_KEY_ANY, authlen, nonce, plaintext, plaintextlen, header, sizeof(header), fieldlen, tag);

         /* Append MIC to end of message */
         for (i = 0; i < 8; i++)
         {
            plaintext[plaintextlen + i] = tag[i];
         }

         paramLength = plaintextlen + 9;
      }
      else
      {
         //decrypt
         payloadBuf_p[0] = bspAesCcmDecryptAuth(key, CRYPTOCC26XX_KEY_ANY, authlen, nonce, plaintext, plaintextlen, header, sizeof(header), fieldlen, tag);
         paramLength = plaintextlen - 7;
      }
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, payloadBuf_p, paramLength);
   }

   return(status && parameterOk);
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

static bool RFC_dtls_bspAesCcmAuthEncrypt_handler()
{
   bool status = RFC_dtls_bspAesCcmAuthEncDec_handler(1);
   return(status);
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

static bool RFC_dtls_bspAesCcmAuthDecrypt_handler()
{
   bool status = RFC_dtls_bspAesCcmAuthEncDec_handler(0);
   return(status);
}


/***************************************************************************//**
 * @fn          macSecurityKeyExchangeWithParent
 *
 * @brief      check to see if key exchange with the parent should be started
 *
 * @param[in]  None
 *
 * @param[Out] N/A
 * @return     1, if needs to start, 0 otherwise
 ******************************************************************************/
uint8_t macSecurityKeyExchangeWithParent()
{
   uint8_t retVal = 0;
   clock_time_t now = clock_time();

   if ((macSecurityPib.macDeviceTable[0].shortAddress != 0) &&
           ((macSecurityPib.macDeviceTable[0].keyUsage == 0) ||
           (((macSecurityPib.macDeviceTable[0].keyUsage & MAC_KEY_USAGE_TX) == 0) &&
           (clock_delta(now, macSecurityPib.macDeviceTable[0].handshakeStartTime) > DTLS_HANDSHAKE_TIMEOUT)))
      )
   {
      if (macSecurityPib.macDeviceTable[0].connectAttemptCount < MAC_SEC_MAX_DTLS_CONNECT_ATTEMPT)
      {
         retVal = 1;
      }
   }

   return(retVal);
}

/***************************************************************************//**
 * @fn          macSecurityStartKeyExchangeWithParent
 *
 * @brief      start the key exchange with the parent
 *
 * @param[in]  *destipaddr_p, IPv6 adress of the peer
 *
 * @param[Out] N/A
 * @return
 ******************************************************************************/
void macSecurityStartKeyExchangeWithParent(uip_ipaddr_t *destipaddr_p)
{
#if (FEATURE_RFC_DTLS)
    clock_time_t now = clock_time();
    //JIRA39 fengmo:  Send a message to the CoAP task to start the DTLS handshake with the neighbor to
    //generate the pairwise MAC security key here:
    extern void coap_connect_dtls_mac_sec(uip_ipaddr_t *destipaddr_p);
    coap_connect_dtls_mac_sec(destipaddr_p);
    macSecurityPib.macDeviceTable[0].keyUsage =  MAC_KEY_USAGE_NEGOTIATING;
    macSecurityPib.macDeviceTable[0].handshakeStartTime = now;
    macSecurityPib.macDeviceTable[0].connectAttemptCount++;
#endif //  FEATURE_RFC_COAP
}

/***************************************************************************//**
 * @fn          macSecurityDeletePairwiseKey
 *
 * @brief      delete the pairwise key with a peer
 * @param[in]  *destipaddr_p, IPv6 adress of the peer
 *
 * @param[Out] N/A
 * @return
 ******************************************************************************/
void macSecurityDeletePairwiseKey(uint16_t shortAddr)
{
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

/***************************************************************************//**
 * @fn          macSecurityAddPairwiseKey
 *
 * @brief     add the pairwise key with a peer
 * @param[in]  *destipaddr_p: IPv6 adress of the peer, key the pairwise key
 *
 * @param[Out] N/A
 * @return
 ******************************************************************************/
void macSecurityAddPairwiseKey(uint16_t shortAddr, uint8_t *key)
{
   uint16_t i;

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

/***************************************************************************//**
 * @fn          macSecurityActivatePairwiseKey
 *
 * @brief     Activate the pairwise key with a peer for TX (for RX it is always activated)
 * @param[in]  *destipaddr_p: IPv6 adress of the peer
 *
 * @param[Out] N/A
 * @return
 ******************************************************************************/
void macSecurityActivatePairwiseKey(uint16_t shortAddr)
{
   uint16_t i;

   for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
   {
      if (macSecurityPib.macDeviceTable[i].shortAddress == shortAddr)
      {
         macSecurityPib.macDeviceTable[i].keyUsage |= MAC_KEY_USAGE_TX;
         break;
      }
   }
}
#endif //FEATURE_RFC_DTLS

#if (FEATURE_RFC_COAP)
extern void TSCH_scan_timer_handler();
extern void TSCH_start_scan_handler();

/** The CoAP stack's global state is stored in a coap_context_t object */
typedef struct coap_context_t
{
   struct udp_simple_socket udp_socket;
#if FEATURE_RFC_DTLS
   struct udp_simple_socket udp_dtls_socket;
   struct udp_simple_socket udp_mac_sec_socket;
#endif

   struct etimer retransmit_timer; /**< fires when the next packet must be sent */
   struct etimer notify_timer;     /**< used to check resources periodically */
} coap_context_t;

coap_context_t the_coap_context;
uint8_t globalRestart = 0;
uint8_t nvm_commit_pending = 0;
coap_context_t *coap_context;
uint16_t lostCoapPacketCount = 0;
uint32_t coap_ObservePeriod = 10000;
uint16_t led_status;
static uint8_t msg_ptr[150];

/*******************************************************************************
* @fn
*
* @brief       Post the DTLS connect to the COAP mail box
*
* @param
*
* @return      none
********************************************************************************
*/
void coap_connect_dtls_mac_sec(uip_ipaddr_t *destipaddr_p)
{
   TCPIP_cmdmsg_t  msg;
   msg.event_type = COAP_DTLS_MAC_SEC_CONNECT;
   msg.data = destipaddr_p;
   Mailbox_post(coap_mailbox, &msg, BIOS_NO_WAIT);
}

/*******************************************************************************
* @fn          coap_dtls_timer_handler
*
* @brief       Post the DTLS timer expiration event to the COAP mail box
*
* @param       event:  Should have a value of EVENT_TIMER
*              data: Should point to the timer data structure
*
* @return      none
********************************************************************************
*/
void coap_dtls_timer_handler(process_event_t event, process_data_t data)
{
   TCPIP_cmdmsg_t  msg;

   if (event == EVENT_TIMER)
   {
      msg.event_type = COAP_DTLS_TIMER;
      Mailbox_post(coap_mailbox, &msg, BIOS_NO_WAIT);
   }
}

/*******************************************************************************
* @fn          coap_mac_sec_timer_handler
*
* @brief       Post the MAC sec timer expiration event to the COAP mail box
*
* @param       event:  Should have a value of EVENT_TIMER
*              data: Should point to the timer data structure
*
* @return      none
********************************************************************************
*/
void coap_mac_sec_timer_handler(process_event_t event, process_data_t data)
{
   TCPIP_cmdmsg_t  msg;

   if (event == EVENT_TIMER)
   {
      msg.event_type = COAP_MAC_SEC_TIMER;
      Mailbox_post(coap_mailbox, &msg, BIOS_NO_WAIT);
   }
}

process_obj_t coap_dtls_timer_handler_obj = {.process_post = coap_dtls_timer_handler};
process_obj_t coap_mac_sec_timer_handler_obj = {.process_post = coap_mac_sec_timer_handler};
/*******************************************************************************
* @fn          coap_timer_handler
*
* @brief       Post the COAP timer expiration event to the COAP mail box
*
* @param       event:  Should have a value of EVENT_TIMER
*              data: Should point to the timer data structure
*
* @return      none
********************************************************************************
*/
void coap_timer_handler(process_event_t event, process_data_t data)
{
   TCPIP_cmdmsg_t  msg;
   msg.event_type = COAP_TIMER;

   Mailbox_post(coap_mailbox, &msg, BIOS_NO_WAIT);
}

process_obj_t coap_timer_handler_obj = {.process_post = coap_timer_handler};
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
static bool RFC_coap_set_notifier_etimer_handler()
{
   bool status = 0;
   uint16_t paramLength;
   bool parameterOk = 1;
   int16_t retv = -1;
   clock_time_t interval;


   //Get the timer interval as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&interval, sizeof(interval));
   parameterOk = status && parameterOk && (paramLength == sizeof(interval));

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }

   if (status && parameterOk)  //Handle the call
   {
      extern coap_context_t the_coap_context;
      extern process_obj_t coap_timer_handler_obj;
      etimer_set(&the_coap_context.notify_timer, &coap_timer_handler_obj, interval);
      retv = 1;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, (uint8_t*)&retv, sizeof(retv));
   }

   return(status && parameterOk);
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
static bool RFC_coap_query_notifier_etimer_expired_handler()
{
   bool status = 0;
   int16_t retv = -1;

   //Get the RUN message
   status = RFC_getFunctionRunCommand(REMOTE_MSP432);

   if (status)  //Handle the call
   {
      retv = etimer_expired(&the_coap_context.notify_timer);
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, (uint8_t*)&retv, sizeof(retv));
   }

   return(status);
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
static bool RFC_coap_notifier_etimer_reset_handler()
{
   bool status = 0;
   int16_t retv = -1;

   //Get the RUN message
   status = RFC_getFunctionRunCommand(REMOTE_MSP432);

   if (status)  //Handle the call
   {
      etimer_reset(&the_coap_context.notify_timer);
      retv = 0;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, (uint8_t*)&retv, sizeof(retv));
   }

   return(status);
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
static bool RFC_coap_query_retransmit_etimer_expired_handler()
{
   bool status = 0;
   int16_t retv = -1;

   //Get the RUN message
   status = RFC_getFunctionRunCommand(REMOTE_MSP432);

   if (status)  //Handle the call
   {
      retv = etimer_expired(&the_coap_context.retransmit_timer);
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, (uint8_t*)&retv, sizeof(retv));
   }

   return(status);
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
static bool RFC_coap_set_retransmit_etimer_handler()
{
   bool status = 0;
   uint16_t paramLength;
   bool parameterOk = 1;
   int16_t retv = -1;
   clock_time_t interval;


   //Get the timer interval as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&interval, sizeof(interval));
   parameterOk = status && parameterOk && (paramLength == sizeof(interval));

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }

   if (status && parameterOk)  //Handle the call
   {
      extern coap_context_t the_coap_context;
      extern process_obj_t coap_timer_handler_obj;
      etimer_set(&the_coap_context.retransmit_timer, &coap_timer_handler_obj, interval);
      retv = 1;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, (uint8_t*)&retv, sizeof(retv));
   }

   return(status && parameterOk);
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
static bool RFC_coap_get_schedule_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint16_t paramLength;
   uint16_t startLinkIdx;
   uint8_t *payloadBuf_p;
   uint16_t payloadBufSize;

   //Get the startLinkIdx as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&startLinkIdx, sizeof(startLinkIdx));
   parameterOk = status && parameterOk && (paramLength == sizeof(startLinkIdx));

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }

   RFC_getTransmitBuffer(REMOTE_MSP432, &payloadBuf_p, &payloadBufSize);
   payloadBuf_p[0] = 1;
   paramLength = 0;

   if (status && parameterOk)  //Handle the call
   {
      paramLength = TSCH_DB_getSchedule(startLinkIdx, payloadBuf_p + 1, 69);
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, payloadBuf_p, paramLength + 1);
   }

   return(status && parameterOk);
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
static bool RFC_coap_schedule_add_slot_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint16_t paramLength;
   uint16_t slotOffset;
   uint16_t channelOffset;
   uint8_t linkOptions;
   uint16_t nodeAddr;
   uint8_t rsp[1] = {1};

   //Get the slotOffset as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&slotOffset, sizeof(slotOffset));
   parameterOk = status && parameterOk && (paramLength == sizeof(slotOffset));

   if (status)
   {
      //Get the channelOffset as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&channelOffset, sizeof(channelOffset));
      parameterOk = status && parameterOk && (paramLength == sizeof(channelOffset));
   }

   if (status)
   {
      //Get the linkOptions as the 3rd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&linkOptions, sizeof(linkOptions));
      parameterOk = status && parameterOk && (paramLength == sizeof(linkOptions));
   }

   if (status)
   {
      //Get the nodeAddr as the 4th function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&nodeAddr, sizeof(nodeAddr));
      parameterOk = status && parameterOk && (paramLength == sizeof(nodeAddr));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }


   if (status && parameterOk)  //Handle the call
   {
      rsp[0] = NHL_scheduleAddSlot(slotOffset, channelOffset, linkOptions, nodeAddr);
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, rsp, 1);
   }

   return(status && parameterOk);
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
static bool RFC_coap_schedule_remove_slot_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint16_t paramLength;
   uint16_t slotOffset;
   uint16_t nodeAddr;
   uint8_t rsp[1] = {1};

   //Get the slotOffset as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&slotOffset, sizeof(slotOffset));
   parameterOk = status && parameterOk && (paramLength == sizeof(slotOffset));

   if (status)
   {
      //Get the nodeAddr as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&nodeAddr, sizeof(nodeAddr));
      parameterOk = status && parameterOk && (paramLength == sizeof(nodeAddr));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }


   if (status && parameterOk)  //Handle the call
   {
      rsp[0] = NHL_scheduleRemoveSlot(slotOffset, nodeAddr);
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, rsp, 1);
   }

   return(status && parameterOk);
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
static bool RFC_coap_webmsg_update_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint16_t paramLength;
   uint8_t seq;
   uint16_t datalen = 1;

   //Get the seq as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, &seq, sizeof(seq));
   parameterOk = status && parameterOk && (paramLength == sizeof(seq));

   if (status)
   {
      //Get the led_status as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t *)&led_status, sizeof(led_status));
      parameterOk = status && parameterOk && (paramLength == sizeof(led_status));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }

   msg_ptr[0] = 0;

   if (status && parameterOk)  //Handle the call
   {
      unsigned char size = 0;

      msg_ptr[0] = 1;
      webmsg_update((unsigned char *)(msg_ptr + 1), &size, seq);
      datalen = size + 1;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, msg_ptr, datalen);
   }

   return(status && parameterOk);
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
static bool RFC_coap_led_set_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint16_t paramLength;
   uint32_t pinId;
   uint8_t value;
   uint8_t rsp[1] = {1};

   //Get the pinId as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&pinId, sizeof(pinId));
   parameterOk = status && parameterOk && (paramLength == sizeof(pinId));

   if (status)
   {
      //Get the value as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&value, sizeof(value));
      parameterOk = status && parameterOk && (paramLength == sizeof(value));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }


   if (status && parameterOk)  //Handle the call
   {
      LED_set(pinId, value);
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, rsp, 1);
   }

   return(status && parameterOk);
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
uint8_t nwPerfTpe = 0;
static bool RFC_coap_get_diagnosis_message_handler()
{
   bool status = 0;
   uint16_t paramLength;
   bool parameterOk = 1;
   uint16_t coapObserveDis;
   uint16_t msgSize = 1;
   uint8_t *payloadBuf_p;
   uint16_t payloadBufSize;

   //Get the coapObserveDis as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&coapObserveDis, sizeof(coapObserveDis));
   parameterOk = status && parameterOk && (paramLength == sizeof(coapObserveDis));

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }

   RFC_getTransmitBuffer(REMOTE_MSP432, &payloadBuf_p, &payloadBufSize);
   payloadBuf_p[0] = 0;

   if (status && parameterOk)  //Handle the call
   {
      uint8_t dummy = 255;

      payloadBuf_p[0] = 1;
      memcpy(payloadBuf_p + msgSize, &nwPerfTpe, 1);
      msgSize += 1;

      if (nwPerfTpe == 0)
      {
         uint8_t i;
         int8_t rssi;
         uint8_t channel;
         uint8_t count = 0;

         for(i = 0; i < 64; i++)
         {
            if (ULPSMAC_Dbg.numTxTotalPerCh[i] > 0)
            {
               count++;
            }
         }

         /* Due to message size limitation, do not send channel information if over
         the 128 MAC frame length*/
         if (count <= 14)
         {
            for(i = 0; i < 64; i++)
            {
               rssi = (int8_t)ULPSMAC_Dbg.avgRssi[i];

               if (ULPSMAC_Dbg.numTxTotalPerCh[i] > 0)
               {
                  channel = i;
                  memcpy(payloadBuf_p + msgSize, &channel, 1);
                  msgSize += 1;
                  memcpy(payloadBuf_p + msgSize, &rssi, 1);
                  msgSize += 1;
                  memcpy(payloadBuf_p + msgSize, &ULPSMAC_Dbg.numTxNoAckPerCh[i], 1);
                  msgSize += 1;
                  memcpy(payloadBuf_p + msgSize, &ULPSMAC_Dbg.numTxTotalPerCh[i], 1);
                  msgSize += 1;
               }
            }
         }

         memcpy(payloadBuf_p + msgSize, &dummy, 1);
         msgSize += 1;

         memcpy(payloadBuf_p + msgSize, &ULPSMAC_Dbg.numTxFail, 4);
         msgSize += 4;
         memcpy(payloadBuf_p + msgSize, &ULPSMAC_Dbg.numTxNoAck, 4);
         msgSize += 4;
         memcpy(payloadBuf_p + msgSize, &ULPSMAC_Dbg.numTxTotal, 4);
         msgSize += 4;
      }
      else
      {
         /* Topology Information*/
         memcpy(payloadBuf_p + msgSize, &ULPSMAC_Dbg.parent, 2);
         msgSize += 2;
         memcpy(payloadBuf_p + msgSize, &ULPSMAC_Dbg.numParentChange, 2);
         msgSize += 2;
         memcpy(payloadBuf_p + msgSize, &ULPSMAC_Dbg.numSyncLost, 2);
         msgSize += 2;
         memcpy(payloadBuf_p + msgSize, &ULPSMAC_Dbg.avgSyncAdjust, 2);
         msgSize += 2;
         memcpy(payloadBuf_p + msgSize, &ULPSMAC_Dbg.maxSyncAdjust, 2);
         msgSize += 2;

         /*Internal Packet Drop Counting*/
         memcpy(payloadBuf_p + msgSize, &ULPSMAC_Dbg.numOutOfBuffer, 2);
         msgSize += 2;
         memcpy(payloadBuf_p + msgSize, &uipRxPacketLossCount, 2);
         msgSize += 2;
         memcpy(payloadBuf_p + msgSize, &lowpanTxPacketLossCount, 2);
         msgSize += 2;
         memcpy(payloadBuf_p + msgSize, &lowpanRxPacketLossCount, 2);
         msgSize += 2;
         memcpy(payloadBuf_p + msgSize, &lostCoapPacketCount, 2);
         msgSize += 2;
         memcpy(payloadBuf_p + msgSize, &coapObserveDis, 2);
         msgSize += 2;
      }
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, payloadBuf_p, msgSize);
   }

   return(status && parameterOk);
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
static bool RFC_coap_put_diagnosis_message_handler()
{
   bool status = 0;
   uint16_t paramLength;
   bool parameterOk = 1;
   uint8_t *payloadBuf_p;
   uint16_t payloadBufSize;
   uint8_t rsp[1];

   RFC_getReceiveBuffer(REMOTE_MSP432, &payloadBuf_p, &payloadBufSize);

   //Get the data as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, payloadBuf_p, payloadBufSize);
   parameterOk = status && parameterOk && (paramLength > 0);

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }


   rsp[0] = 0;

   if (status && parameterOk)  //Handle the call
   {
      rsp[0] = 1;
      payloadBuf_p[paramLength] = '\0';
      uint16_t parent = atoi((const char *)payloadBuf_p);
      TSCH_MACConfig.restrict_to_node = parent;
      TSCH_prepareDisConnect();
      TSCH_restartNode();
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, rsp, 1);
   }

   return(status && parameterOk);
}

extern uint16_t panId;

#define NVM_TLV_TYPE_MPSK           0x01
#define NVM_TLV_TYPE_PNID           0x02
#define NVM_TLV_TYPE_BCH0           0x03
#define NVM_TLV_TYPE_BCHMO          0x04
#define NVM_TLV_TYPE_ASRQTO         0x05
#define NVM_TLV_TYPE_SLFS           0x06
#define NVM_TLV_TYPE_KALP           0x07
#define NVM_TLV_TYPE_SCANI          0x08
#define NVM_TLV_TYPE_NSSL           0x09
#define NVM_TLV_TYPE_FXCN           0x0A
#define NVM_TLV_TYPE_TXPW           0x0B
#define NVM_TLV_TYPE_FXPA           0x0C
#define NVM_TLV_TYPE_RPLDD          0x0D
#define NVM_TLV_TYPE_COARCT         0x0E
#define NVM_TLV_TYPE_COAPPT         0x0F
#define NVM_TLV_TYPE_CODTPT         0x10
#define NVM_TLV_TYPE_COAOMX         0x11
#define NVM_TLV_TYPE_CODFTO         0x12
#define NVM_TLV_TYPE_PHYM           0x13
#define NVM_TLV_TYPE_DBGL           0x14
#define NVM_TLV_TYPE_COMMIT         0x15
#define NVM_TLV_TYPE_EOL            0x16
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
static int add_uchar_tlv(char *msg, int idx, uint8_t tlv_type, uint8_t tlv_val)
{
   msg[idx++] = tlv_type;
   msg[idx++] = 1;
   msg[idx++] = tlv_val;
   return(idx);
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
static int add_ushort_tlv(char *msg, int idx, uint8_t tlv_type, uint16_t tlv_val)
{
   msg[idx++] = tlv_type;
   msg[idx++] = 2;
   msg[idx++] = (tlv_val >> 8) & 0xFF;
   msg[idx++] = tlv_val & 0xFF;
   return(idx);
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
static void get_uchar_tlv(uint8_t *tlvdata, uint16_t idx, uint8_t lenact, int *error_p, uint8_t *outVal_p)
{
   if (lenact == 1)
   {
      *outVal_p = tlvdata[idx];
   }
   else
   {
      *error_p = 1;
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
static void get_ushort_tlv(uint8_t *tlvdata, uint16_t idx, uint8_t lenact, int *error_p, uint16_t *outVal_p)
{
   if (lenact == 2)
   {
      *outVal_p = (tlvdata[idx] * 256) + tlvdata[idx + 1];
   }
   else
   {
      *error_p = 1;
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
static bool RFC_coap_get_nvm_params_handler()
{
   bool status = 0;
   uint16_t paramLength;
   bool parameterOk = 1;
   uint16_t chunkIdx;
   uint16_t msgSize;
   char *nvmParamsMsg;
   uint16_t payloadBufSize;

   //Get the chunkIdx as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t*)&chunkIdx, sizeof(chunkIdx));
   parameterOk = status && parameterOk && (paramLength == sizeof(chunkIdx));

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }

   RFC_getTransmitBuffer(REMOTE_MSP432, (uint8_t**)&nvmParamsMsg, &payloadBufSize);
   nvmParamsMsg[0] = 0;
   msgSize = 1;

   if (status && parameterOk)  //Handle the call
   {
      nvmParamsMsg[0] = 1;

      switch (chunkIdx)
      {
         case 0:  //43 bytes
            nvmParamsMsg[msgSize++] = NVM_TLV_TYPE_MPSK;
            nvmParamsMsg[msgSize++] = 16;
            memcpy(nvmParamsMsg + msgSize, nvParams.mac_psk, 16);
            msgSize += 16;

            msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_PNID, nvParams.panid);
            msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_BCH0, nvParams.bcn_chan_0);
            msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_BCHMO, nvParams.bcn_ch_mode);
            msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_ASRQTO, nvParams.assoc_req_timeout_sec);
            msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_SLFS, nvParams.slotframe_size);
            msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_KALP, nvParams.keepAlive_period);
            msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_SCANI, nvParams.scan_interval);
            break;

         case 1:  //40 bytes
            msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_NSSL, nvParams.num_shared_slot);
            msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_FXCN, nvParams.fixed_channel_num);
            msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_TXPW, nvParams.tx_power);
            msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_FXPA, nvParams.fixed_parent);
            msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_RPLDD, nvParams.rpl_dio_doublings);
            msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_COARCT, nvParams.coap_resource_check_time);
            msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_COAPPT, nvParams.coap_port);
            msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_CODTPT, nvParams.coap_dtls_port);
            msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_COAOMX, nvParams.coap_obs_max_non);
            msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_CODFTO, nvParams.coap_default_response_timeout);
            msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_PHYM, nvParams.phy_mode);
            msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_DBGL, nvParams.debug_level);
            break;

         default:  //3 bytes
            msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_EOL, 1);
            break;
      }
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, (uint8_t *)nvmParamsMsg, msgSize);
   }

   return(status && parameterOk);
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
static bool RFC_coap_put_nvm_params_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint16_t size;
   uint8_t *tlvdata;
   uint16_t payloadBufSize;
   uint8_t rsp[1];

   RFC_getReceiveBuffer(REMOTE_MSP432, &tlvdata, &payloadBufSize);

   //Get the NVM params msg as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &size, tlvdata, payloadBufSize);
   parameterOk = status && parameterOk && (size > 0);

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }

   rsp[0] = 0;

   if (status && parameterOk)  //Handle the call
   {
      int idx = 0;
      uint8_t type;
      uint8_t len;
      uint8_t lenrem;
      uint8_t lenact;
      int error = 0;

      rsp[0] = 1;

      while (idx < size)
      {
         type = tlvdata[idx++];
         len = tlvdata[idx++];
         lenrem = size - idx;
         lenact = (len <= lenrem) ? len : lenrem;

         switch (type)
         {
            case NVM_TLV_TYPE_MPSK:
               if (lenact == 16)
               {
                  memcpy(nvParams.mac_psk, tlvdata + idx, lenact);
               }
               else
               {
                  error = 1;
               }

               break;

            case NVM_TLV_TYPE_PNID:
               get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.panid);
               break;

            case NVM_TLV_TYPE_BCH0:
               get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.bcn_chan_0);
               break;

            case NVM_TLV_TYPE_BCHMO:
               get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.bcn_ch_mode);
               break;

            case NVM_TLV_TYPE_ASRQTO:
               get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.assoc_req_timeout_sec);
               break;

            case NVM_TLV_TYPE_SLFS:
               get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.slotframe_size);
               break;

            case NVM_TLV_TYPE_KALP:
               get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.keepAlive_period);
               break;

            case NVM_TLV_TYPE_SCANI:
               get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.scan_interval);
               break;

            case NVM_TLV_TYPE_NSSL:
               get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.num_shared_slot);
               break;

            case NVM_TLV_TYPE_FXCN:
               get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.fixed_channel_num);
               break;

            case NVM_TLV_TYPE_TXPW:
               get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.tx_power);
               break;

            case NVM_TLV_TYPE_FXPA:
               get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.fixed_parent);
               break;

            case NVM_TLV_TYPE_RPLDD:
               get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.rpl_dio_doublings);
               break;

            case NVM_TLV_TYPE_COARCT:
               get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.coap_resource_check_time);
               break;

            case NVM_TLV_TYPE_COAPPT:
               get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.coap_port);
               break;

            case NVM_TLV_TYPE_CODTPT:
               get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.coap_dtls_port);
               break;

            case NVM_TLV_TYPE_COAOMX:
               get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.coap_obs_max_non);
               break;

            case NVM_TLV_TYPE_CODFTO:
               get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.coap_default_response_timeout);
               break;

            case NVM_TLV_TYPE_PHYM:
               get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.phy_mode);
               break;

            case NVM_TLV_TYPE_DBGL:
               get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.debug_level);
               break;

            case NVM_TLV_TYPE_COMMIT:
               rsp[0] = 2;
               nvm_commit_pending = 1;
               break;

            default:
               break;
         }

         idx = idx + len;
      }
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, rsp, 1);
   }

   return(status && parameterOk);
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
static bool RFC_uip_udp_packet_send_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint16_t uip_datalen;
   uip_ip6addr_t destipaddr;
   uint16_t destport;
   uint16_t paramLength;
   uint8_t *payloadBuf_p;
   uint16_t payloadBufSize;
   uint8_t rsp[1] = {1};

   RFC_getReceiveBuffer(REMOTE_MSP432, &payloadBuf_p, &payloadBufSize);

   //Get the uip_trx_data as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_MSP432, &uip_datalen, payloadBuf_p, payloadBufSize);
   parameterOk = status && parameterOk && (uip_datalen > 0);

   if (status)
   {
      //Get the destipaddr_p as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, destipaddr.u8, sizeof(destipaddr.u8));
      parameterOk = status && parameterOk && (paramLength == sizeof(destipaddr.u8));
   }

   if (status)
   {
      //Get the destport as the 3rd function parameter
      status = RFC_getFunctionParameter(REMOTE_MSP432, &paramLength, (uint8_t *)&destport, sizeof(destport));
      parameterOk = status && parameterOk && (paramLength == sizeof(destport));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_MSP432);
   }

   if (status && parameterOk)  //Handle the call
   {
      extern coap_context_t the_coap_context;
      uip_udp_packet_send_post(&the_coap_context.udp_socket, payloadBuf_p, uip_datalen, &destipaddr, UIP_NTOHS(destport));//port comes in Network order, change to Host order
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_MSP432, 0, rsp, 1);
   }

   return(status && parameterOk);
}

/*******************************************************************************
* @fn          udp_receive_callback
*
* @brief       UDP Connection Callback Function
*
* @param       *c - udp connection socket pointer
*              *source_addr - source IP address
*              source_port - UDP source port
*              *dest_addr - destination IP address
*              dest_port - UDP desintation port
*              *data - pointer to data message
*              dataLen - data message length
*
* @return      none
********************************************************************************
*/
void udp_receive_callback(struct udp_simple_socket *c, void *ptr,
                          const uip_ipaddr_t *source_addr,
                          uint16_t source_port,
                          const uip_ipaddr_t *dest_addr,
                          uint16_t dest_port, const char *data,
                          uint16_t datalen)
{
   TCPIP_cmdmsg_t  msg;

   if (appRxLenFifo[appRxFifo_WriteIdx] != 0)
   {
      lostCoapPacketCount++;
   }
   else
   {
      //fengmo debug node packet loss begin
      if (datalen  <= UIP_BUFSIZE - UIP_IPUDPH_LEN && datalen > 0)
      {
         memcpy(appRxFifo[appRxFifo_WriteIdx].c, uip_buf, datalen + UIP_IPUDPH_LEN); //fengmo debug node packet loss, Jira 44, done
      }
      else
      {
         extern void RFC_fatalErrorHandler();
         RFC_fatalErrorHandler();
      }

      appRxLenFifo[appRxFifo_WriteIdx] = datalen;
      appRxFifoIndex_inc(&appRxFifo_WriteIdx);

      msg.event_type = COAP_DATA;

      if (Mailbox_post(coap_mailbox, &msg, BIOS_NO_WAIT) != TRUE)
      {
         TCPIP_Dbg.tcpip_post_app_err++;
      }
   }
}

/*******************************************************************************
* @fn          coap_socket_open
*
* @brief       Can be triggered when node desync and resync
*
* @param
*
* @return      none
********************************************************************************
*/
void coap_socket_open(void)
{
   udp_simple_socket_open(&coap_context->udp_socket, NULL, 0, coapConfig.coap_port, NULL, udp_receive_callback);
#if FEATURE_RFC_DTLS
   udp_simple_socket_open(&coap_context->udp_dtls_socket, NULL, 0, coapConfig.coap_dtls_port, NULL, udp_receive_callback);
   dtls_set_udp_socket(dtls_context[0], &coap_context->udp_dtls_socket);

   udp_simple_socket_open(&coap_context->udp_mac_sec_socket, NULL, 0, coapConfig.coap_mac_sec_port, NULL, udp_receive_callback);
   dtls_set_udp_socket(dtls_context[1], &coap_context->udp_mac_sec_socket);
#endif
}

/*******************************************************************************
* @fn
*
* @brief
*
* @param
*
* @return      none
********************************************************************************/
void coap_msg_post_scan_timer()  //JIRA50
{
   TCPIP_cmdmsg_t  msg;
   msg.event_type = COAP_RESTART_SCAN;

   Mailbox_post(coap_mailbox, &msg, BIOS_NO_WAIT);
}

/*******************************************************************************
* @fn
*
* @brief
*
* @param
*
* @return      none
********************************************************************************/
void coap_msg_post_start_scan()  //JIRA53
{
   TCPIP_cmdmsg_t  msg;
   msg.event_type = COAP_START_SCAN;

   Mailbox_post(coap_mailbox, &msg, BIOS_NO_WAIT);
}

/*******************************************************************************
* @fn
*
* @brief
*
* @param
*
* @return      none
********************************************************************************/
void coap_msg_post_delete_all_observers()  //JIRA39
{
   TCPIP_cmdmsg_t  msg;
   msg.event_type = COAP_DELETE_ALL_OBSERVERS;

   Mailbox_post(coap_mailbox, &msg, BIOS_NO_WAIT);
}

/***************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 * @param[Out]
 * @return
 ******************************************************************************/
static bool coap_trivial_callback(void *cbCtxt_p, struct structRfcMsgheader *msgHdr_p, uint8_t *resp_p)
{
   uint8_t status = 1;
   return(status);
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
void RFC_coap_timer_event()
{
   RFC_call(REMOTE_MSP432, RFC_FUNC_ID_COAP_TIMER_EVENT, 0, NULL, NULL, coap_trivial_callback, NULL);
}
/*******************************************************************************
 * @fn          coap_event_handler
 *
 * @brief       CoAP event handler
 *
 * @param
 *
 * @return      none
 *******************************************************************************
 */
void coap_event_handler(void *ctx)
{
   uint8_t *arg[5];
   uint16_t argLen[5];
   uint8_t *payloadBuf_p;
   uint16_t payloadBufSize;
   uint16_t dataLen = appRxLenFifo[appRxFifo_ReadIdx];
   uip_ip6addr_t srcipaddr, destipaddr;
   uint16_t srcport, destport;

   RFC_getTransmitBuffer(REMOTE_MSP432, &payloadBuf_p, &payloadBufSize);

   if (dataLen <= payloadBufSize && dataLen > 0)
   {
       memcpy(payloadBuf_p, &(appRxFifo[appRxFifo_ReadIdx].c[UIP_IPUDPH_LEN + UIP_LLH_LEN]), dataLen);
       srcipaddr = APP_UIP_IP_BUF->srcipaddr;
       srcport = APP_UIP_UDP_BUF->srcport;
       destipaddr = APP_UIP_IP_BUF->destipaddr;
       destport = APP_UIP_UDP_BUF->destport;

       argLen[0] = dataLen;
       arg[0] = payloadBuf_p;
       argLen[1] = sizeof(srcipaddr);
       arg[1] = (uint8_t *) & (srcipaddr);
       argLen[2] = sizeof(srcport);
       arg[2] = (uint8_t*) & (srcport);
       argLen[3] = sizeof(destipaddr);
       arg[3] = (uint8_t *) & (destipaddr);
       argLen[4] = sizeof(destport);
       arg[4] = (uint8_t*) & (destport);
       appRxLenFifo[appRxFifo_ReadIdx] = 0;
       RFC_call(REMOTE_MSP432, RFC_FUNC_ID_COAP_COAP_EVENT, 5, argLen, arg, coap_trivial_callback, NULL);
   }
   else
   {
       appRxLenFifo[appRxFifo_ReadIdx] = 0;
   }
}
/*******************************************************************************
 * @fn          coap_mac_sec_event_handler
 *
 * @brief       CoAP coap_mac_sec_event_handler handler
 *
 * @param
 *
 * @return      none
 *******************************************************************************
 */
void coap_mac_sec_event_handler(void *ctx)
{
//This function should never be called.  Something is wrong!
//For MAC sec, DTLS performs handshake only.   There should be no DTLS payload data for MAC sec
   extern void RFC_fatalErrorHandler();
   RFC_fatalErrorHandler();
}
/*******************************************************************************
 * @fn          coap_init
 *
 * @brief
 *
 * @param
 *
 * @return      none
 *******************************************************************************
 */
static void coap_init()
{
   uint8_t *arg[3];
   uint16_t argLen[3];

   argLen[0] = sizeof(coapConfig.coap_resource_check_time);
   arg[0] = (uint8_t *) & (coapConfig.coap_resource_check_time);
   argLen[1] = sizeof(coapConfig.coap_obs_max_non);
   arg[1] = (uint8_t *) & (coapConfig.coap_obs_max_non);
   argLen[2] = sizeof(coapConfig.coap_default_response_timeout);
   arg[2] = (uint8_t *) & (coapConfig.coap_default_response_timeout);

   RFC_call(REMOTE_MSP432, RFC_FUNC_ID_COAP_INIT, 3, argLen, arg, coap_trivial_callback, NULL);
}

void coap_delete_all_observers(void)
{
   coap_msg_post_delete_all_observers();
}

void coap_delete_all_observers_handler(void)
{
   RFC_call(REMOTE_MSP432, RFC_FUNC_ID_COAP_DELETE_ALL_OBSERVERS, 0, NULL, NULL, coap_trivial_callback, NULL);
}

void coap_reboot(void)
{
   RFC_call(REMOTE_MSP432, RFC_FUNC_ID_REBOOT, 0, NULL, NULL, coap_trivial_callback, NULL);
}

void coap_task_ipc_handler(int timeoutVal)


{
   TCPIP_cmdmsg_t coap_msg;

   if (Mailbox_pend(coap_mailbox, &coap_msg, timeoutVal))
   {
      PWREST_on(PWREST_TYPE_CPU);

      if(coap_msg.event_type == COAP_START_SCAN)  //JIRA53
      {
         TSCH_start_scan_handler();
         RFC_call(REMOTE_MSP432, RFC_FUNC_ID_NEW_SCAN, 0, NULL, NULL, NULL, NULL);
      }
      else if(coap_msg.event_type == COAP_RESTART_SCAN)  //JIRA50
      {
         TSCH_scan_timer_handler();
      }
      else if(coap_msg.event_type == COAP_TIMER)
      {
         RFC_coap_timer_event();
      }
      else
#if FEATURE_RFC_DTLS
         if(coap_msg.event_type == COAP_DTLS_TIMER)
         {
            dtls_handle_retransmit(dtls_context[0]);
         }
         else if(coap_msg.event_type == COAP_MAC_SEC_TIMER)
         {
            dtls_handle_retransmit(dtls_context[1]);
         }
         else if(coap_msg.event_type == COAP_DTLS_MAC_SEC_CONNECT)
         {
            session_t session;
            memcpy(session.addr.u8, coap_msg.data, sizeof(session.addr.u8));
            session.port = coapConfig.coap_mac_sec_port;
            dtls_connect_to_server(dtls_context[1], &session);
         }
         else if(coap_msg.event_type == COAP_DELETE_ALL_OBSERVERS)
         {
            coap_delete_all_observers_handler();
         }
         else
         {
            appRxFifoIndex_inc(&appRxFifo_ReadIdx);   //fengmo debug node packet loss
            dtls_handle_read(dtls_context[0]);
         }
#else
      {
         appRxFifoIndex_inc(&appRxFifo_ReadIdx);   //fengmo debug node packet loss
         coap_event_handler(NULL);
      }
#endif

      if (nvm_commit_pending)
      {
         coap_reboot();
         NVM_update();
         nvm_commit_pending = 0;
         globalRestart = 1;
      }

      if (globalRestart)  //fengmo NVM
      {
         Task_sleep(5 * CLOCK_SECOND);
         Board_reset();
      }

      PWREST_off(PWREST_TYPE_CPU);
   }
}
/*******************************************************************************
* @fn          coap_task
*
* @brief       CoAP task
*
* @param       arg0 - meaningless
*              arg1 - meaningless
*
* @return      none
********************************************************************************
*/
void coap_task(UArg a0, UArg a1)
{
   coap_context = &the_coap_context;
   memset(coap_context, 0, sizeof(coap_context_t));

   SensorBatmon_init();

#if SENSORTAG
   status = SensorI2C_open();
   status = SensorTmp007_init();
   status = SensorOpt3001_init();
   status = SensorHdc1000_init();
   status = SensorBmp280_init();
   status = SensorMpu9250_init();
#elif I3_MOTE
   status = SensorI2C_open();
   status = SensorOpt3001_init();
   status = SensorBmp280_init();

#if CC13XX_DEVICES
   status = SensorLIS2DW12_init();
#elif CC26XX_DEVICES
   status = SensorTmp007_init();
   status = SensorHdc1000_init();
#endif
#endif

   uip_setup_appConnCallback(coap_socket_open);

   Task_sleep(CLOCK_SECOND / 4);

   {
       NHL_MlmeScanReq_t nhlMlmeScanReq;
       NHL_setupScanArg(&nhlMlmeScanReq);
       NHL_startScan(&nhlMlmeScanReq, NULL);
   }

   if (!(RFC_open(REMOTE_MSP432) && MSP432HCT_getConfig(&coap_ObservePeriod)))
   {
      HAL_LED_set(2, 1);
      {
         extern void RFC_fatalErrorHandler();
         RFC_fatalErrorHandler();
      }
   }

   coap_init();
#if FEATURE_RFC_DTLS
   init_dtls(0, &coap_context->udp_dtls_socket, &coap_dtls_timer_handler_obj, (appDataHandler_t)coap_event_handler, NULL);
   init_dtls(1, &coap_context->udp_mac_sec_socket, &coap_mac_sec_timer_handler_obj, (appDataHandler_t)coap_mac_sec_event_handler, NULL);
#endif

   while (1)
   {
      coap_task_ipc_handler(BIOS_WAIT_FOREVER);
   }
}
#endif //FEATURE_RFC_COAP
#endif  //I3_MOTE_SPLIT_ARCH
