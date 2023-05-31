/* dtls -- a very basic DTLS implementation
 *
 * Copyright (C) 2011--2013 Olaf Bergmann <bergmann@tzi.org>
 * Copyright (C) 2013 Hauke Mehrtens <hauke@hauke-m.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file dtls_client.h
 * @brief High level DTLS API and visible structures.
 */

#ifndef _DTLS_DTLS_CLIENT_H_
#define _DTLS_DTLS_CLIENT_H_

#include "../apps/dtls/dtls.h"

#ifndef UIP_IP_INPUT_BUF
#define UIP_IP_INPUT_BUF   ((struct uip_ip_hdr *)&uip_input_buf[UIP_LLH_LEN])
#define UIP_UDP_INPUT_BUF  ((struct uip_udp_hdr *)&uip_input_buf[UIP_LLIPH_LEN])
#endif

#ifndef APP_UIP_IP_BUF
#define APP_UIP_IP_BUF   ((struct uip_ip_hdr *)&(appRxFifo[appRxFifo_ReadIdx].c[UIP_LLH_LEN]))
#define APP_UIP_UDP_BUF  ((struct uip_udp_hdr *)&(appRxFifo[appRxFifo_ReadIdx].c[UIP_LLIPH_LEN]))
#endif

#define MAX_NUMBER_DTLS_CONTEXT (2)   //Two: one for the CoAP UDP and one for the MAC security peer-to-peer

struct keymap_t
{
   unsigned char *id;
   size_t id_length;
   unsigned char *key;
   size_t key_length;
};

#if I3_MOTE_SPLIT_ARCH && FEATURE_RFC_DTLS
typedef struct structDtlsPeer_t {
  dtls_peer_type role;       /**< denotes if this host is DTLS_CLIENT or DTLS_SERVER */
  dtls_state_t state;        /**< DTLS engine state */
  uint8_t kb[MAX_KEYBLOCK_LENGTH];
} DtlsPeer_t;
#else
#define  DtlsPeer_t dtls_peer_t
#endif

extern dtls_context_t *dtls_context[MAX_NUMBER_DTLS_CONTEXT];

DtlsPeer_t *dtlsGetPeer(const dtls_context_t *ctx, const session_t *session, DtlsPeer_t *peerBuf_p);

void dtls_destroy_peer(dtls_context_t *ctx, dtls_peer_t *peer, int unlink);

void init_dtls(uint8_t dtlsCntxtIdx, struct udp_simple_socket *coap_dtls_udp_socket_p, process_obj_t *proc_obj_p,
      appDataHandler_t appDataHandler, void *appData_p);
void dtls_handle_read(dtls_context_t *ctx);
void dtls_set_udp_socket(dtls_context_t *ctx, struct udp_simple_socket *coap_dtls_udp_socket_p);
uint16_t udp_chksum(uint8_t *buf);

void dtls_handle_retransmit(dtls_context_t *ctx);
#endif /* _DTLS_DTLS_CLIENT_H_ */

