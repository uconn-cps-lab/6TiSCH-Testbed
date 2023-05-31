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
 *  ====================== coap_host_app.c =============================================
 *  This file is coap server application file to run in the sensor device node.
 */

/*******************************************************************************
*                                            INCLUDES
*******************************************************************************/
#include "stdio.h"
#include <string.h>
#include "config.h"
#include "coap.h"
#include "coap_common_app.h"
#include "harp.h"
#include "net/tcpip.h"
#include "stdbool.h"
#include "nv_params.h"
#include "uip_rpl_process.h"

#include <time.h>
#include <stdlib.h>

#ifdef LINUX_GATEWAY
#include "device.h"
#include "time.h"
#include "pthread.h"
#else
#include "led.h"
#include "nhl_api.h"
#include "nhl_device_table.h"
#endif

#define TSCH_PIB_LINK_OPTIONS_TX 0x01
#define TSCH_PIB_LINK_OPTIONS_TX_COAP 0x21
#define TSCH_PIB_LINK_OPTIONS_RX 0x02

extern HARP_child_t HARP_children[MAX_CHILDREN_NUM];
extern HARP_self_t HARP_self;
extern coap_context_t *coap_context;

int coap_client_send_request(unsigned int method, uint8_t dst_short,
                             unsigned char *uri_path, uint8_t payload_len, char *payload)
{
  coap_address_t dst;
  coap_address_init(&dst);

  uint8_t addr_u8[16] = {0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, dst_short};
  uint16_t addr_u16[8] = {0x0120, 0xb80d, 0x3412, 0xffff, 0x0000, 0xff00, 0x00fe, (uint16_t)dst_short << 8};

  memcpy(dst.addr.u8, addr_u8, sizeof(addr_u8));
  memcpy(dst.addr.u16, addr_u16, sizeof(addr_u16));
  dst.port = UIP_HTONS(COAP_DEFAULT_PORT);

  coap_pdu_t *req;
  req = coap_new_pdu();
  req->hdr->type = COAP_MESSAGE_NON;
  req->hdr->id = coap_new_message_id(coap_context);
  req->hdr->code = method;

  coap_add_option(req, COAP_OPTION_URI_PATH, strlen((const char *)uri_path), uri_path);
  coap_add_data(req, payload_len, (const unsigned char *)payload);
  int res = coap_send(coap_context, &dst, req);
  coap_delete_pdu(req);
  return res;
}

// send init msg to all children
void *send_init()
{
  char init_msg[3];
  uint8_t l = HARP_self.layer + 1;
  memcpy(init_msg + 0, &l, 1);
  memcpy(init_msg + 1, &HARP_self.partition_center, 2);
  for (uint8_t i = 0; i < HARP_self.children_cnt; i++)
  {
    coap_client_send_request(COAP_REQUEST_POST, HARP_children[i].id, "harp_init", 3, init_msg);
#ifdef LINUX_GATEWAY
    printf("send init to #%d\n", HARP_children[i].id);
    usleep(HARP_SEND_DELAY * 10 * 1000);
#else
    Task_sleep(HARP_SEND_DELAY * CLOCK_SECOND / 100);
#endif
  }
}

// send self interface at all layers to parent
void *send_interface()
{
  char msg[24];
  uint8_t msg_size = 0;
  // for (uint8_t layer = HARP_self.layer; layer <= MAX_HOP; layer++) // layer cnt
  for (uint8_t layer = 0; layer <= MAX_HOP; layer++) // layer cnt
  {
    memcpy(msg + msg_size, &layer, 1);
    msg_size++;
    memcpy(msg + msg_size, &HARP_self.iface[layer].ts, 2);
    msg_size += 2;
    memcpy(msg + msg_size, &HARP_self.iface[layer].ch, 1);
    msg_size++;
  }
  coap_client_send_request(COAP_REQUEST_POST, HARP_self.parent, "harp_iface", msg_size, msg);
}

// send self interface update at one layer to parent, aka SP_ADJ_REQ
void *send_interface_update()
{
  char msg[4];
  uint8_t msg_size = 0;

  uint8_t layer = 1;
  
  memcpy(msg + msg_size, &layer, 1);
  msg_size++;
  memcpy(msg + msg_size, &HARP_self.iface[layer].ts, 2);
  msg_size += 2;
  memcpy(msg + msg_size, &HARP_self.iface[layer].ch, 1);
  msg_size++;
  
  coap_client_send_request(COAP_REQUEST_PUT, HARP_self.parent, "harp_iface", msg_size, msg);
}

// send sub-partitions to all children
void *send_subpartition()
{
  for (uint8_t i = 0; i < HARP_self.children_cnt; i++) // children cnt
  {
    // no need to send packet
    if (HARP_children[i].iface[HARP_self.layer + 1].ts == 0)
      continue;

    char msg[42];
    uint8_t msg_size = 0;
    // for (uint8_t layer = HARP_self.layer + 1; layer <= MAX_HOP; layer++) // layer cnt
    for (uint8_t layer = 0; layer <= MAX_HOP; layer++)
    {
      // no need to loop rest layers
      //      if (HARP_children[i].iface[layer].ts == 0)
      //        break;
      memcpy(msg + msg_size, &layer, 1);
      msg_size++;
      memcpy(msg + msg_size, &HARP_children[i].sp_abs[layer].ts_start, 2);
      msg_size += 2;
      memcpy(msg + msg_size, &HARP_children[i].sp_abs[layer].ts_end, 2);
      msg_size += 2;
      memcpy(msg + msg_size, &HARP_children[i].sp_abs[layer].ch_start, 1);
      msg_size++;
      memcpy(msg + msg_size, &HARP_children[i].sp_abs[layer].ch_end, 1);
      msg_size++;
    }
    coap_client_send_request(COAP_REQUEST_POST, HARP_children[i].id, "harp_sp", msg_size, msg);
#ifdef LINUX_GATEWAY
    printf("send sp to #%d\n", HARP_children[i].id);
    usleep(HARP_SEND_DELAY * 10 * 1000);
#else
    Task_sleep(HARP_SEND_DELAY * CLOCK_SECOND / 100);
#endif
  }
#ifdef LINUX_GATEWAY
  pthread_t t2;
  printf("sent all subpartition, do distributed scheduling\n");
  pthread_create(&t2, NULL, distributed_scheduling, NULL);
#endif
}

// send a schedule update to a child node (single cell)
uint8_t send_schedule(uint16_t ts, uint16_t ch, uint8_t option, uint16_t child_short)
{
  char msg[7];
  uint8_t msg_size = 0;
  uint16_t self_short = 1;
  ts = htons(ts);
  ch = htons(ch);
  self_short = htons(self_short);

  memcpy(msg + msg_size, &ts, 2);
  msg_size += 2;
  memcpy(msg + msg_size, &ch, 2);
  msg_size += 2;
  memcpy(msg + msg_size, &self_short, 2);
  msg_size += 2;
  memcpy(msg + msg_size, &option, 1);
  msg_size += 1;

  return coap_client_send_request(COAP_REQUEST_PUT, child_short, "schedule", msg_size, msg);
}

typedef struct
{
  uint16_t ts;
  uint16_t ch;
  uint8_t opt;
  uint16_t peer;
} cell_t;

// schedule cells for children in the allocated sub-partition

void *distributed_scheduling()
{
#ifdef LINUX_GATEWAY
  printf("schedule cells for children\n");
#endif

  uint16_t slot_idx = HARP_self.sp_abs[HARP_self.layer].ts_start;
  uint16_t ch_idx = (uint16_t)HARP_self.sp_abs[HARP_self.layer].ch_start;
  uint16_t self_addr = (uint16_t)HARP_self.id;
  uint8_t opt;
  uint16_t ts, ch;
  self_addr = htons(self_addr);

  for (uint8_t i = 0; i < HARP_self.children_cnt; i++)
  {
    cell_t cells[8]; // max number, maybe coap constraint
    uint8_t cell_idx = 0;
    char msg[56]; // 7 bytes each cell
    uint8_t msg_size = 0;
    //    for (uint8_t j = 0; j < HARP_children[i].traffic / TRAFFIC_UNIT; j++)
    for (uint8_t j = 0; j < 1; j++)
    {
      if (msg_size == 56) // send rest cells later
        break;
      // uplink
      ts = htons(slot_idx);
      ch = htons(ch_idx);
      opt = TSCH_PIB_LINK_OPTIONS_TX_COAP;

      memcpy(msg + msg_size, &ts, 2);
      msg_size += 2;
      memcpy(msg + msg_size, &ch, 2);
      msg_size += 2;
      memcpy(msg + msg_size, &self_addr, 2);
      msg_size += 2;
      memcpy(msg + msg_size, &opt, 1);
      msg_size += 1;
      // add local cells
      cells[cell_idx].ts = slot_idx;
      cells[cell_idx].ch = ch_idx;
      cells[cell_idx].opt = TSCH_PIB_LINK_OPTIONS_RX;
      cells[cell_idx].peer = HARP_children[i].id;
      cell_idx++;

      // downlink
      ts = htons(HARP_self.partition_center * 2 - slot_idx);
      opt = TSCH_PIB_LINK_OPTIONS_RX;
      memcpy(msg + msg_size, &ts, 2);
      msg_size += 2;
      memcpy(msg + msg_size, &ch, 2);
      msg_size += 2;
      memcpy(msg + msg_size, &self_addr, 2);
      msg_size += 2;
      memcpy(msg + msg_size, &opt, 1);
      msg_size += 1;

      // add local cells
      cells[cell_idx].ts = HARP_self.partition_center * 2 - slot_idx;
      cells[cell_idx].ch = ch_idx;
      cells[cell_idx].opt = TSCH_PIB_LINK_OPTIONS_TX_COAP;
      cells[cell_idx].peer = HARP_children[i].id;
      cell_idx++;

      slot_idx++;
    }
    coap_client_send_request(COAP_REQUEST_PUT, HARP_children[i].id, "schedule", msg_size, msg);

    for (uint8_t x = 0; x < cell_idx; x++)
    {
#ifdef LINUX_GATEWAY
      DEV_NHLScheduleAddSlot(cells[x].ts, cells[x].ch, cells[x].opt, cells[x].peer);
#else
      NHL_scheduleAddSlot(cells[x].ts, cells[x].ch, cells[x].opt, cells[x].peer);
#endif
    }

#ifdef LINUX_GATEWAY
    printf("sent schedule to #%d\n", HARP_children[i].id);
    usleep(HARP_SEND_DELAY * 10 * 1000);
#else
    Task_sleep(HARP_SEND_DELAY * CLOCK_SECOND / 100);
#endif
  }
  return 0;
}

/*******************************************************************************
* @fn          hnd_post_harp_init
*
* @brief       start the HARP process of node
               1. init HARP_self and _children variable
               2. abstract its own interface
               3. forward init msg to children, and waiting for their interfaces
               4. composite children's interfaces and upload the interface to parent
               5. waiting for sub-partition from parent
               6. allocate sub-partition for children

               7. schedule cells within the sub-partition
*
* @param
*
* @return      none
********************************************************************************
*/
void hnd_post_harp_init(coap_context_t *ctx, struct coap_resource_t *resource,
                        coap_address_t *peer, coap_pdu_t *request, str *token,
                        coap_pdu_t *response)
{
  size_t size;
  unsigned char *data;
  char msg[32];
  // 1 byte: layer, 2 bytes: partition_center, 1 byte: children num; rest each byte is a children id
  coap_get_data(request, &size, &data);

  if (size > 0)
  {
    harp_init(data);
#ifndef LINUX_GATEWAY
    LED_set(2, 1);
#endif
  }
  else
  {
    sprintf(msg, "no children info");
#ifdef LINUX_GATEWAY
    printf("no children info in the payload\n");
#endif
    response->hdr->code = coap_add_data(response, strlen(msg),
                                        (const unsigned char *)msg)
                              ? COAP_RESPONSE_CODE(205)
                              : COAP_RESPONSE_CODE(413);
    return;
  }

  if (HARP_self.children_cnt == 0)
  {
#ifndef LINUX_GATEWAY
    // is leaf
    abstractInterface();
    HARP_mailbox_msg_t h_msg;
    h_msg.type = HARP_MAILBOX_SEND_IFACE;
    Mailbox_post(harp_mailbox, &h_msg, BIOS_NO_WAIT);
#endif
    //    send_interface();
  }
  else
  {
#ifdef LINUX_GATEWAY
    pthread_t t_id;
    pthread_create(&t_id, NULL, send_init, NULL);
#else
    HARP_mailbox_msg_t h_msg;
    h_msg.type = HARP_MAILBOX_SEND_INIT;
    Mailbox_post(harp_mailbox, &h_msg, BIOS_NO_WAIT);
//     send_init();
#endif
  }
  sprintf(msg, "init harp");

  response->hdr->code = coap_add_data(response, strlen(msg),
                                      (const unsigned char *)msg)
                            ? COAP_RESPONSE_CODE(205)
                            : COAP_RESPONSE_CODE(413);
}

/*******************************************************************************
* @fn          hnd_post_harp_iface
*
* @brief       This function receives children's HARP_interface @ all layers
               encoding in LE, no need ntohs or htons
*
* @param
*
* @return      none
********************************************************************************
*/
void hnd_post_harp_iface(coap_context_t *ctx, struct coap_resource_t *resource,
                         coap_address_t *peer, coap_pdu_t *request, str *token,
                         coap_pdu_t *response)
{
  unsigned char *data;
  size_t size;
  char msg[64];

  coap_get_data(request, &size, &data);
  if (size == 0)
  {
    sprintf(msg, "no interface received");
    response->hdr->code = coap_add_data(response, strlen(msg), (const unsigned char *)msg) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
    return;
  }

  uint8_t child_id = peer->addr.u8[15];
  for (uint8_t i = 0; i < HARP_self.children_cnt; i++)
  {
    if (HARP_children[i].id == child_id && HARP_children[i].traffic == 0)
    {
      HARP_self.recv_iface_cnt++;
#ifdef LINUX_GATEWAY
      printf("got #%d's interface: ", child_id);
#endif
      for (; size >= 4; size -= 4, data += 4)
      {
        uint8_t layer = *(uint8_t *)(data + 0);
        uint16_t ts = *(uint16_t *)(data + 1);
        uint8_t ch = *(uint8_t *)(data + 3);
        if (layer == HARP_self.layer + 1)
        {
          HARP_children[i].traffic = ts + TRAFFIC_UNIT;
          HARP_self.traffic_fwd += ts + TRAFFIC_UNIT;
        }

        HARP_children[i].iface[layer].ts = ts;
        HARP_children[i].iface[layer].ch = ch;

#ifdef LINUX_GATEWAY
        printf("l%d-{%d, %d}, ", layer, ts, ch);
#endif
      }
#ifdef LINUX_GATEWAY
      printf("\n");
#endif
      break;
    }
  }

  if (HARP_self.recv_iface_cnt == HARP_self.children_cnt)
  {
    interfaceComposition();
    abstractInterface();
#ifdef LINUX_GATEWAY
    subpartitionAllocation();
    pthread_t t1;
    pthread_create(&t1, NULL, send_subpartition, NULL);
    // call distributed_scheduling() within send_subpartition()
#else
    HARP_mailbox_msg_t h_msg;
    h_msg.type = HARP_MAILBOX_SEND_IFACE;
    Mailbox_post(harp_mailbox, &h_msg, BIOS_NO_WAIT);
//    send_interface();
#endif
  }
  sprintf(msg, "got #%d's interface", peer->addr.u8[15]);

  response->hdr->code = coap_add_data(response, strlen(msg), (const unsigned char *)msg) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
}

/*******************************************************************************
* @fn          hnd_get_harp_iface
*
* @brief       This function returns its HARP_iface @ all layers
*
* @param
*
* @return      none
********************************************************************************
*/
void hnd_get_harp_iface(coap_context_t *ctx, struct coap_resource_t *resource,
                        coap_address_t *peer, coap_pdu_t *request, str *token,
                        coap_pdu_t *response)
{
  char msg[64] = "";
  uint8_t msg_size = 0;
  for (uint8_t layer = HARP_self.layer; layer <= MAX_HOP; layer++)
  {
    if (HARP_self.iface[layer].ts == 0)
      break;
    memcpy(msg + msg_size, &layer, 1);
    msg_size++;
    memcpy(msg + msg_size, &HARP_self.iface[layer].ts, 2);
    msg_size += 2;
    memcpy(msg + msg_size, &HARP_self.iface[layer].ch, 1);
    msg_size++;
  }

  response->hdr->code = coap_add_data(response, msg_size, (const unsigned char *)msg) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
}

/*******************************************************************************
* @fn          hnd_put_harp_iface
*
* @brief       This function handles interface update from children, at one layer
*
* @param
*
* @return      none
********************************************************************************
*/
void hnd_put_harp_iface(coap_context_t *ctx, struct coap_resource_t *resource,
                        coap_address_t *peer, coap_pdu_t *request, str *token,
                        coap_pdu_t *response)
{
  unsigned char *data;
  size_t size;
  char msg[32]="biu";

  coap_get_data(request, &size, &data);
  if (size == 0)
  {
    sprintf(msg, "no interface received");
    response->hdr->code = coap_add_data(response, strlen(msg), (const unsigned char *)msg) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
    return;
  }

  uint8_t child_id = peer->addr.u8[15];
  for (uint8_t i = 0; i < HARP_self.children_cnt; i++)
  {
    if (HARP_children[i].id == child_id)
    {
#ifdef LINUX_GATEWAY
      printf("got #%d's new interface: ", child_id);
#endif

      uint8_t layer = *(uint8_t *)(data + 0);
      uint16_t ts = *(uint16_t *)(data + 1);
      uint8_t ch = *(uint8_t *)(data + 3);

      HARP_self.adjustingNodes[0] = child_id;
      HARP_self.adjustingLayer = layer;
      HARP_children[i].iface[layer].ts = ts;
      HARP_children[i].iface[layer].ch = ch;

      subpartitionAdjustment();
#ifdef LINUX_GATEWAY
      printf("l%d-{%d, %d}, ", layer, ts, ch);
#endif
    }
    break;
  }
  
  response->hdr->code = coap_add_data(response, strlen(msg), (const unsigned char *)msg) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
}

/*******************************************************************************
* @fn          hnd_post_harp_sp
*
* @brief       This function receives HARP_subpartition @ one layer from parent
                encoding in LE, no need ntohs or htons
*
* @param
*
* @return      none
********************************************************************************
*/
void hnd_post_harp_sp(coap_context_t *ctx, struct coap_resource_t *resource,
                      coap_address_t *peer, coap_pdu_t *request, str *token,
                      coap_pdu_t *response)
{
  unsigned char *data;
  size_t size;
  coap_get_data(request, &size, &data);

  char msg[64];

  for (; size >= 7; size -= 7, data += 7)
  {
    uint8_t layer = *(uint8_t *)(data + 0);
    uint16_t ts_start = *(uint16_t *)(data + 1);
    uint16_t ts_end = *(uint16_t *)(data + 3);
    uint8_t ch_start = *(uint8_t *)(data + 5);
    uint8_t ch_end = *(uint8_t *)(data + 6);

    HARP_self.sp_abs[layer].ts_start = ts_start;
    HARP_self.sp_abs[layer].ts_end = ts_end;
    HARP_self.sp_abs[layer].ch_start = ch_start;
    HARP_self.sp_abs[layer].ch_end = ch_end;

#ifdef LINUX_GATEWAY
    printf("got sub-partition @ layer %d: {%d, %d, %d, %d}\n", layer, ts_start, ts_end, ch_start, ch_end);
#endif
    sprintf(msg, "got sub-partition @ layer %d: {%d, %d, %d, %d}", layer, ts_start, ts_end, ch_start, ch_end);
  }

#ifndef LINUX_GATEWAY
  subpartitionAllocation();
  HARP_mailbox_msg_t h_msg;
  h_msg.type = HARP_MAILBOX_SEND_SP;
  Mailbox_post(harp_mailbox, &h_msg, BIOS_NO_WAIT);
//  send_subpartition();
#endif

  response->hdr->code = coap_add_data(response, strlen(msg), (const unsigned char *)msg) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
}

/*******************************************************************************
* @fn          hnd_get_harp_sp
*
* @brief       This function returns HARP_subpartition @ all layers
*
* @param
*
* @return      none
********************************************************************************
*/
void hnd_get_harp_sp(coap_context_t *ctx, struct coap_resource_t *resource,
                     coap_address_t *peer, coap_pdu_t *request, str *token,
                     coap_pdu_t *response)
{
  char msg[64] = "";
  uint8_t msg_size = 0;
  for (uint8_t layer = HARP_self.layer; layer <= MAX_HOP; layer++)
  {
    if (HARP_self.iface[layer].ts == 0)
      break;
    memcpy(msg + msg_size, &layer, 1);
    msg_size++;
    memcpy(msg + msg_size, &HARP_self.sp_abs[layer].ts_start, 2);
    msg_size += 2;
    memcpy(msg + msg_size, &HARP_self.sp_abs[layer].ts_end, 2);
    msg_size += 2;
    memcpy(msg + msg_size, &HARP_self.sp_abs[layer].ch_start, 1);
    msg_size++;
    memcpy(msg + msg_size, &HARP_self.sp_abs[layer].ch_end, 1);
    msg_size++;
  }

  response->hdr->code = coap_add_data(response, msg_size, (const unsigned char *)msg) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
}

/*******************************************************************************
* @fn          hnd_put_harp_sp
*
* @brief       This function handles interface update from children
*
* @param
*
* @return      none
********************************************************************************
*/
void hnd_put_harp_sp(coap_context_t *ctx, struct coap_resource_t *resource,
                     coap_address_t *peer, coap_pdu_t *request, str *token,
                     coap_pdu_t *response)
{
}

void hnd_get_schedule(coap_context_t *ctx, struct coap_resource_t *resource,
                      coap_address_t *peer, coap_pdu_t *request, str *token,
                      coap_pdu_t *response)
{
#ifdef LINUX_GATEWAY
  uint8_t scheduleMsg[256];
#else
  uint8_t scheduleMsg[69];
#endif

  uint16_t msgSize = 0;
  size_t size;
  unsigned char *data;
  uint16_t startLinkIdx = 0;

  scheduleMsg[0] = 0; //Default seq = 0;

  coap_get_data(request, &size, &data);

  if (size >= 3)
  {
    scheduleMsg[0] = data[0];                     //data[0]: seq in 8-bit
    startLinkIdx = data[2];                       //data[1], data[2] (MSB): start index in LE
    startLinkIdx = (startLinkIdx << 8) | data[1]; //data[1], data[2]: start index in LE
  }

#ifdef LINUX_GATEWAY
  msgSize = DEV_NHLScheduleGet(startLinkIdx, scheduleMsg + 1, sizeof(scheduleMsg) - 1);
#else
  msgSize = TSCH_DB_getSchedule(startLinkIdx, scheduleMsg + 1, sizeof(scheduleMsg) - 1);
#endif

  response->hdr->code = coap_add_data(response, msgSize + 1, (const unsigned char *)scheduleMsg) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
}

void hnd_put_schedule(coap_context_t *ctx, struct coap_resource_t *resource,
                      coap_address_t *peer, coap_pdu_t *request, str *token,
                      coap_pdu_t *response)
{
  size_t size;
  unsigned char *data;
  response->hdr->code = COAP_RESPONSE_CODE(204);

  uint8_t respMsg[16], msgSize = 0;

  coap_get_data(request, &size, &data);

  uint8_t err_cnt = 0;
  for (; size >= 7; size -= 7, data += 7)
  {
    uint16_t res;
    uint16_t timeslot = ntohs(*(uint16_t *)(data + 0));
    uint16_t channelOffset = ntohs(*(uint16_t *)(data + 2));
    uint16_t nodeAddr = ntohs(*(uint16_t *)(data + 4));
    uint8_t linkOptions = data[6];

#ifdef LINUX_GATEWAY
    DEV_NHLScheduleAddSlot(timeslot, channelOffset, linkOptions, nodeAddr);
    printf("DEV_NHLScheduleAddSlot(%d, %d, %02x, %04x)\n", timeslot, channelOffset, linkOptions, nodeAddr);
#else
    res = NHL_scheduleAddSlot(timeslot, channelOffset, linkOptions, nodeAddr);

    coap_add_data(response, msgSize, (const unsigned char *)respMsg);

    if (res != LOWMAC_STAT_SUCCESS)
    {
      err_cnt++;
      response->hdr->code = COAP_RESPONSE_CODE(400);
    }
#endif
  }

#ifndef LINUX_GATEWAY
  if (err_cnt == 0)
  {

    if (HARP_self.parent != 0)
    {
      HARP_mailbox_msg_t h_msg;
      h_msg.type = HARP_MAILBOX_SEND_SCH;
      Mailbox_post(harp_mailbox, &h_msg, BIOS_NO_WAIT);
      //      distributed_scheduling();
      LED_set(2, 0);
    }
  }
  else
  {
    LED_set(1, 1);
  }
#endif
}

void hnd_delete_schedule(coap_context_t *ctx, struct coap_resource_t *resource,
                         coap_address_t *peer, coap_pdu_t *request, str *token,
                         coap_pdu_t *response)
{
  size_t size;
  unsigned char *data;
  response->hdr->code = COAP_RESPONSE_CODE(202);

  coap_get_data(request, &size, &data);

  for (; size >= 7; size -= 7, data += 7)
  {
    uint16_t res;
    uint16_t timeslot = ntohs(*(uint16_t *)(data + 0));
    uint16_t node = ntohs(*(uint16_t *)(data + 4));
#ifdef LINUX_GATEWAY
    DEV_NHLScheduleRemoveSlot(timeslot, node);
#else
    res = NHL_scheduleRemoveSlot(timeslot, node);
    if (res != LOWMAC_STAT_SUCCESS)
    {
      response->hdr->code = COAP_RESPONSE_CODE(400);
    }
#endif
  }
}

void COAP_fatalErrorHandler()
{
#if (DEBUG_HALT_ON_ERROR)
  volatile int errloop = 0;
  while (1)
  {
    errloop++;
  }
#endif
}
