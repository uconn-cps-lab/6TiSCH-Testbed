/*
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  ====================== coap_gw_app.c =============================================
 *  This file is coap server application file to run in gateway.
 */

/*******************************************************************************
*                                            INCLUDES
*******************************************************************************/
#include "stdio.h"
#include <string.h>
#include "config.h"

#include "coap.h"
#include "net/tcpip.h"

#include "stdbool.h"

#include "uip_rpl_process.h"

#ifdef WITH_JSON
#include "json/jsontree.h"
json_output json_output_context;
#endif

#if FEATURE_DTLS
#include "pltfrm_mbx.h"
#include "../../dtls/dtls_client.h"
#endif

#include "json/jsonparse.h"
#include "nv_params.h"
#include "device.h"
#include "coap_common_app.h"
#include "harp.h"
/* -----------------------------------------------------------------------------
 *                                         Local Variables
 * -----------------------------------------------------------------------------
 */
static char msg_ptr[150];

/* -----------------------------------------------------------------------------
 *                                         Global Variables
 * -----------------------------------------------------------------------------
 */
coap_context_t *coap_context;
uint16_t coapObserveDis = 0;

/* -----------------------------------------------------------------------------
 *                                           Defines
 * -----------------------------------------------------------------------------
 */

/*******************************************************************************
 * @fn          coap_event_handler
 *
 * @brief       CoAP event handler
 *
 * @param       *ctx - pointer to CoAP context
 *
 * @return      none
 *******************************************************************************
 */
void coap_event_handler(coap_context_t *ctx)
{
  // Read from CoAP ctx - receive queue and parse data into new PDU
  coap_read(ctx);

  // Read newly parsed PDU and handle request/response - includes sending message
  coap_dispatch(ctx);
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
void coap_mac_sec_event_handler(coap_context_t *ctx)
{
  //This function should never be called.  Something is wrong!
  //For MAC sec, DTLS performs handshake only.   There should be no DTLS payload data for MAC sec

  COAP_fatalErrorHandler();
}
/*******************************************************************************
* @fn          json_putchar
*
* @brief
*
* @param
*
* @return      none
********************************************************************************
*/
#ifdef WITH_JSON
static int json_putchar(int c)
{
  if (json_output_context.outbuf_pos < 128)
  {
    json_output_context.outbuf[json_output_context.outbuf_pos++] = c;
    return c;
  }
  return 0;
}
#endif

/*******************************************************************************
* @fn          hnd_get_sensor
*
* @brief       Sensor get handler function
*
* @param
*
* @return      none
********************************************************************************
*/
void hnd_get_sensors(coap_context_t *ctx, struct coap_resource_t *resource,
                     coap_address_t *peer, coap_pdu_t *request, str *token,
                     coap_pdu_t *response)
{
  coap_opt_iterator_t opt_iter;
  coap_subscription_t *subscription;

  if (request != NULL && coap_check_option(request, COAP_OPTION_OBSERVE, &opt_iter))
  {
    subscription = coap_add_observer(resource, peer, token);
    if (subscription)
    {
      subscription->non = request->hdr->type == COAP_MESSAGE_NON;
      coap_add_option(response, COAP_OPTION_OBSERVE, 0, NULL);
    }
  }
  else if (request != NULL)
  {
    coap_delete_observer(resource, peer, token);
  }

  if (resource->dirty == 1)
    coap_add_option(response, COAP_OPTION_OBSERVE,
                    coap_encode_var_bytes(msg_ptr, ctx->observe), msg_ptr);

  sprintf(msg_ptr, "BIUBIUBIU GATEWAY!\n");

  response->hdr->code = coap_add_data(response, strlen(msg_ptr), (unsigned char *)msg_ptr) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
}

/*******************************************************************************
* @fn          hnd_put_led
*
* @brief       This function turn LED on or off based on put message
*
* @param
*
* @return      none
********************************************************************************
*/
uint8_t cnt = 0;
void hnd_put_led(coap_context_t *ctx, struct coap_resource_t *resource,
                 coap_address_t *peer, coap_pdu_t *request, str *token,
                 coap_pdu_t *response)
{
  size_t size = 0;
  unsigned char *data;
  char msg[64] = "";

  // coap_get_data(request, &size, &data);
  // char resp[64] = "";
  // memcpy(resp, data, size);
  // sprintf(msg, "size: %d, %s\n", size, resp);

  response->hdr->code = coap_add_data(response, size, (unsigned char *)msg) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
  /*
  if (size == 0)
  {
    response->hdr->code = COAP_RESPONSE_CODE(400);
    return;
  }
  else
  {
    if (atoi(data) == 1)
    {
#define Board_LED2 2
#define LED_set(x, y) printf("LED_set(%d,%d)\n", x, y)
      if (led_status)
      {
        response->hdr->code = COAP_RESPONSE_CODE(203);
        LED_set(Board_LED2, 1);
      }
      //  OFF
      else
      {
        //  Changed - OFF->ON 
        led_status = 1;
        LED_set(Board_LED2, 1);
        response->hdr->code = COAP_RESPONSE_CODE(204);
      }
    }
    else
    {
      //  ON
      if (led_status)
      {
      //  Changed - ON->OFF 
        response->hdr->code = COAP_RESPONSE_CODE(204);
        LED_set(Board_LED2, 0);
        led_status = 0;
      }
      // OFF
      else
      {
        response->hdr->code = COAP_RESPONSE_CODE(203);
      }
    }
  }
    */
}

extern void hnd_get_nvm_params(coap_context_t *ctx, struct coap_resource_t *resource,
                               coap_address_t *peer, coap_pdu_t *request, str *token,
                               coap_pdu_t *response); //fengmo NVM
extern void hnd_put_nvm_params(coap_context_t *ctx, struct coap_resource_t *resource,
                               coap_address_t *peer, coap_pdu_t *request, str *token,
                               coap_pdu_t *response);

/*******************************************************************************
* @fn          init_resources
*
* @brief       This function initializes CoAP resources
*
* @param       ctx - CoAP context
*
* @return      none
********************************************************************************
*/
void init_resources(coap_context_t *ctx)
{
  coap_resource_t *r;

  /* This is just one single resource to encapsulate all sensor readings */
  r = coap_resource_init("sensors", 7, 0);
  coap_register_handler(r, COAP_REQUEST_GET, hnd_get_sensors);

  /* Add attributes as needed - ct=50 for JSON, ct=0 for plain text */
#ifdef WITH_JSON
  coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"50", 2, 0);
#else
  coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
#endif /* WITH_JSON */
  coap_add_resource(ctx, r);

  /* Resource for HARP */
  coap_resource_t *harp_init_resource;
  harp_init_resource = coap_resource_init("harp_init", 9, 0);
  coap_register_handler(harp_init_resource, COAP_REQUEST_POST, hnd_post_harp_init);
  coap_add_attr(harp_init_resource, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_resource(ctx, harp_init_resource);

  coap_resource_t *harp_iface_resource;
  harp_iface_resource = coap_resource_init("harp_iface", 10, 0);
  coap_register_handler(harp_iface_resource, COAP_REQUEST_POST, hnd_post_harp_iface);
  coap_register_handler(harp_iface_resource, COAP_REQUEST_GET, hnd_get_harp_iface);
  coap_register_handler(harp_iface_resource, COAP_REQUEST_PUT, hnd_put_harp_iface);
  coap_add_attr(harp_iface_resource, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_resource(ctx, harp_iface_resource);

  coap_resource_t *harp_sp_resource;
  harp_sp_resource = coap_resource_init("harp_sp", 7, 0);
  coap_register_handler(harp_sp_resource, COAP_REQUEST_POST, hnd_post_harp_sp);
  coap_register_handler(harp_sp_resource, COAP_REQUEST_GET, hnd_get_harp_sp);
  coap_register_handler(harp_iface_resource, COAP_REQUEST_PUT, hnd_put_harp_sp);
  coap_add_attr(harp_sp_resource, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_resource(ctx, harp_sp_resource);

  /* Resource to turn on/off all LEDS */
  r = coap_resource_init("led", 3, 0);
  coap_register_handler(r, COAP_REQUEST_PUT, hnd_put_led);
  coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_resource(ctx, r);

  /* Resource for scheduler interface */
  coap_resource_t *schedule_resource;
  schedule_resource = coap_resource_init("schedule", 8, 0);
  coap_register_handler(schedule_resource, COAP_REQUEST_DELETE, hnd_delete_schedule);
  coap_register_handler(schedule_resource, COAP_REQUEST_PUT, hnd_put_schedule);
  coap_register_handler(schedule_resource, COAP_REQUEST_GET, hnd_get_schedule);
  coap_add_resource(ctx, schedule_resource);

  /* Resource for NVM parameters*/ //fengmo NVM
  //   r = coap_resource_init("nvmparams", 9, 0);
  //   coap_register_handler(r, COAP_REQUEST_GET, hnd_get_nvm_params);
  //   coap_register_handler(r, COAP_REQUEST_PUT, hnd_put_nvm_params);
  //   coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  //   coap_add_resource(ctx, r);
}

/*******************************************************************************
* @fn          get_context
*
* @brief       This function gets context based IP address and UDP port
*
* @param       ipaddr - IP address
*              port - udp port
*
* @return      none
********************************************************************************
*/
coap_context_t *get_context(uip_ipaddr_t *ipaddr_p, unsigned short port)
{
  coap_context_t *ctx = NULL;

  coap_address_t addr;
  addr.addr = *ipaddr_p;
  addr.port = port;

  ctx = coap_new_context(&addr);

  return ctx;
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
  //fengmo debug node packet loss begin
  if (datalen <= UIP_BUFSIZE - UIP_IPUDPH_LEN)
  {
    memcpy(appRxFifo[appRxFifo_WriteIdx].c, uip_buf, datalen + UIP_IPUDPH_LEN); //fengmo debug node packet loss Jira 44 done
  }
  else
  {
    // printf("<%d>: Buffer overflow: %d > %d\n", datalen, UIP_BUFSIZE - UIP_IPUDPH_LEN);
    exit(0);
  }
  appRxLenFifo[appRxFifo_WriteIdx] = uip_app_datalen; //fengmo debug node packet loss
  appRxFifoIndex_inc(&appRxFifo_WriteIdx);
  //fengmo debug node packet loss end
  appRxFifoIndex_inc(&appRxFifo_ReadIdx);
#if FEATURE_DTLS && !FEATURE_DTLS_TEST
  dtls_handle_read(dtls_context[0]);
#else
  coap_event_handler(coap_context);
#endif
  appRxLenFifo[appRxFifo_ReadIdx] = 0;
}
#if FEATURE_DTLS && !FEATURE_DTLS_TEST
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
  TCPIP_cmdmsg_t msg;
  extern pltfrm_mbx_t tcpip_mailbox;

  if (event == EVENT_TIMER)
  {
    msg.event_type = COAP_DTLS_TIMER;
    msg.data = (void *)data;
    pltfrm_mbx_send(&tcpip_mailbox, (unsigned char *)&msg, PLTFRM_MBX_OPN_NO_WAIT);
  }
}

void coap_mac_sec_timer_handler(process_event_t event, process_data_t data)
{
  TCPIP_cmdmsg_t msg;
  extern pltfrm_mbx_t tcpip_mailbox;

  if (event == EVENT_TIMER)
  {
    msg.event_type = COAP_MAC_SEC_TIMER;
    msg.data = (void *)data;
    pltfrm_mbx_send(&tcpip_mailbox, (unsigned char *)&msg, PLTFRM_MBX_OPN_NO_WAIT);
  }
}

process_obj_t coap_dtls_timer_handler_obj = {.process_post = coap_dtls_timer_handler};
process_obj_t coap_mac_sec_timer_handler_obj = {.process_post = coap_mac_sec_timer_handler};
#endif

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
void coap_task()
{
  int ret;
  uip_ipaddr_t server_ipaddr;
  coap_context = get_context(&server_ipaddr, UDP_CLIENT_PORT);
  init_resources(coap_context);

#if FEATURE_DTLS && !FEATURE_DTLS_TEST
  coapConfig.coap_mac_sec_port = COAP_MAC_SEC_UDP_PORT;
  ret = udp_simple_socket_open(&coap_context->udp_mac_sec_socket, NULL, 0, coapConfig.coap_mac_sec_port, NULL, udp_receive_callback);
  if (ret == -1)
  {
    printf("COAP MAC SEC Socket ERROR: %d, %d!\n", coapConfig.coap_dtls_port, coapConfig.coap_mac_sec_port);
  }
  else
  {
    printf("COAP MACSEC Socket Open!\n");
  }

  ret = udp_simple_socket_open(&coap_context->udp_dtls_socket, NULL, 0, coapConfig.coap_dtls_port, NULL, udp_receive_callback);
  if (ret == -1)
  {
    printf("COAP DTLS Socket ERROR!\n");
  }
  else
  {
    printf("COAP DTLS Socket Open!\n");
  }

  init_dtls(0, &coap_context->udp_dtls_socket, &coap_dtls_timer_handler_obj, (appDataHandler_t)coap_event_handler, coap_context);
  init_dtls(1, &coap_context->udp_mac_sec_socket, &coap_mac_sec_timer_handler_obj, (appDataHandler_t)coap_mac_sec_event_handler, coap_context);
#endif //FEATURE_DTLS && !FEATURE_DTLS_TEST

  ret = udp_simple_socket_open(&coap_context->udp_socket, NULL, 0, coapConfig.coap_port, NULL, udp_receive_callback);
  if (ret == -1)
  {
    printf("COAP ERROR!\n");
  }
  else
  {
    printf("COAP Socket Open!\n");
  }
}
