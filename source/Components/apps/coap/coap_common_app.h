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
 *  ====================== coap_common_app.h =============================================
 */

#ifndef COAP_COMMON_APP_H
#define COAP_COMMON_APP_H

int coap_client_send_request(unsigned int method, uint8_t dst_short,
                             unsigned char *uri_path, uint8_t payload_len, char *payload);
void *send_init();
void *send_subpartition();
void *send_interface();
uint8_t send_schedule(uint16_t ts, uint16_t ch, uint8_t option, uint16_t child_short);
void *distributed_scheduling();

void hnd_post_harp_init(coap_context_t *ctx, struct coap_resource_t *resource,
                        coap_address_t *peer, coap_pdu_t *request, str *token,
                        coap_pdu_t *response);

void hnd_post_harp_iface(coap_context_t *ctx, struct coap_resource_t *resource,
                         coap_address_t *peer, coap_pdu_t *request, str *token,
                         coap_pdu_t *response);
void hnd_get_harp_iface(coap_context_t *ctx, struct coap_resource_t *resource,
                        coap_address_t *peer, coap_pdu_t *request, str *token,
                        coap_pdu_t *response);
void hnd_put_harp_iface(coap_context_t *ctx, struct coap_resource_t *resource,
                        coap_address_t *peer, coap_pdu_t *request, str *token,
                        coap_pdu_t *response);

void hnd_post_harp_sp(coap_context_t *ctx, struct coap_resource_t *resource,
                      coap_address_t *peer, coap_pdu_t *request, str *token,
                      coap_pdu_t *response);
void hnd_get_harp_sp(coap_context_t *ctx, struct coap_resource_t *resource,
                     coap_address_t *peer, coap_pdu_t *request, str *token,
                     coap_pdu_t *response);
void hnd_put_harp_sp(coap_context_t *ctx, struct coap_resource_t *resource,
                        coap_address_t *peer, coap_pdu_t *request, str *token,
                        coap_pdu_t *response);

void hnd_get_schedule(coap_context_t *ctx, struct coap_resource_t *resource,
                      coap_address_t *peer, coap_pdu_t *request, str *token,
                      coap_pdu_t *response);
void hnd_put_schedule(coap_context_t *ctx, struct coap_resource_t *resource,
                      coap_address_t *peer, coap_pdu_t *request, str *token,
                      coap_pdu_t *response);
void hnd_delete_schedule(coap_context_t *ctx, struct coap_resource_t *resource,
                         coap_address_t *peer, coap_pdu_t *request, str *token,
                         coap_pdu_t *response);
void COAP_fatalErrorHandler();
#endif //COAP_COMMON_APP_H
