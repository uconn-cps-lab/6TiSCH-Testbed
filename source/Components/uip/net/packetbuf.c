/**
* \addtogroup packetbuf
* @{
*/

/*
* Copyright (c) 2006, Swedish Institute of Computer Science.
* All rights reserved.
*
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
* $Id: packetbuf.c,v 1.1 2010/06/14 19:19:16 adamdunkels Exp $
*/

/**
* \file
*         Rime buffer (packetbuf) management
* \author
*         Adam Dunkels <adam@sics.se>
*/
/******************************************************************************
*
* Copyright (c) 2014 Texas Instruments Inc.  All rights reserved.
*
* DESCRIPTION:
*
* HISTORY:
*
*
******************************************************************************/

#include <string.h>

//#include "contiki-net.h"
#include "packetbuf.h"
//#include "net/rime.h"
#include "nv_params.h"
#include "rpl-conf.h"
#include "../apps/coap/config.h"
#include "../apps/coap/pdu.h"
#if UIP_PACKETBUF

struct packetbuf_attr packetbuf_attrs[PACKETBUF_NUM_ATTRS];
uint16_t buflen;  //fengmo debug GW packet loss
static uint16_t bufptr;  //fengmo debug GW packet loss
static uint8_t hdrptr;

/* The declarations below ensure that the packet buffer is aligned on
an even 16-bit boundary. On some platforms (most notably the
msp430), having apotentially misaligned packet buffer may lead to
problems when accessing 16-bit values. */

static uint8_t *packetbufptr;

#ifdef DEBUG  
#undef DEBUG
#endif
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*---------------------------------------------------------------------------*/
void packetbuf_clear(void) {
  buflen = bufptr = 0;
  hdrptr = PACKETBUF_HDR_SIZE;
  
  packetbufptr = &packetbuf[PACKETBUF_HDR_SIZE];
  packetbuf_attr_clear();  
}
/*---------------------------------------------------------------------------*/
void packetbuf_clear_hdr(void) {
  hdrptr = PACKETBUF_HDR_SIZE;
}
/*---------------------------------------------------------------------------*/
int packetbuf_copyfrom(const void *from, uint16_t len) {
  uint16_t l;
  
  packetbuf_clear();
  l = len > PACKETBUF_SIZE ? PACKETBUF_SIZE : len;
  memcpy(packetbufptr, from, l);
  buflen = l;
  return l;
}
/*---------------------------------------------------------------------------*/
void packetbuf_compact(void) {
  int i, len;
  
  if (packetbuf_is_reference()) {
    memcpy(&packetbuf[PACKETBUF_HDR_SIZE], packetbuf_reference_ptr(),
           packetbuf_datalen());
  } else if (bufptr > 0) {
    len = packetbuf_datalen() + PACKETBUF_HDR_SIZE;
    for (i = PACKETBUF_HDR_SIZE; i < len; i++) {
      packetbuf[i] = packetbuf[bufptr + i];
    }
    
    bufptr = 0;
  }
}
/*---------------------------------------------------------------------------*/
int packetbuf_copyto_hdr(uint8_t *to) {
#if DEBUG_LEVEL > 0
  {
    int i;
    PRINTF("packetbuf_write_hdr: header:\n");
    for(i = hdrptr; i < PACKETBUF_HDR_SIZE; ++i) {
      PRINTF("0x%02x, ", packetbuf[i]);
    }
    PRINTF("\n");
  }
#endif /* DEBUG_LEVEL */
  memcpy(to, packetbuf + hdrptr, PACKETBUF_HDR_SIZE - hdrptr);
  return PACKETBUF_HDR_SIZE - hdrptr;
}
/*---------------------------------------------------------------------------*/
int packetbuf_copyto(void *to) {
#if DEBUG_LEVEL > 0
  {
    int i;
    char buffer[1000];
    char *bufferptr = buffer;
    
    bufferptr[0] = 0;
    for(i = hdrptr; i < PACKETBUF_HDR_SIZE; ++i) {
      bufferptr += sprintf(bufferptr, "0x%02x, ", packetbuf[i]);
    }
    PRINTF("packetbuf_write: header: %s\n", buffer);
    bufferptr = buffer;
    bufferptr[0] = 0;
    for(i = bufptr; i < buflen + bufptr; ++i) {
      bufferptr += sprintf(bufferptr, "0x%02x, ", packetbufptr[i]);
    }
    PRINTF("packetbuf_write: data: %s\n", buffer);
  }
#endif /* DEBUG_LEVEL */
  if (PACKETBUF_HDR_SIZE - hdrptr + buflen > PACKETBUF_SIZE) {
    /* Too large packet */
    return 0;
  }
  memcpy(to, packetbuf + hdrptr, PACKETBUF_HDR_SIZE - hdrptr);
  memcpy((uint8_t *) to + PACKETBUF_HDR_SIZE - hdrptr, packetbufptr + bufptr,
         buflen);
  return PACKETBUF_HDR_SIZE - hdrptr + buflen;
}
/*---------------------------------------------------------------------------*/
int packetbuf_hdralloc(int size) {
  if (hdrptr >= size && packetbuf_totlen() + size <= PACKETBUF_SIZE) {
    hdrptr -= size;
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void packetbuf_hdr_remove(int size) {
  hdrptr += size;
}
/*---------------------------------------------------------------------------*/
int packetbuf_hdrreduce(int size) {
  if (buflen < size) {
    return 0;
  }
  
  bufptr += size;
  buflen -= size;
  return 1;
}
/*---------------------------------------------------------------------------*/
void packetbuf_set_datalen(uint16_t len) {
  PRINTF("packetbuf_set_len: len %d\n", len);
  buflen = len;
}
///*---------------------------------------------------------------------------*/
void *
packetbuf_dataptr(void) {
  return (void *) (&packetbuf[bufptr + PACKETBUF_HDR_SIZE]);
}
/*---------------------------------------------------------------------------*/
void *
packetbuf_hdrptr(void)
{
  return (void *)(&packetbuf[hdrptr]);
}
/*---------------------------------------------------------------------------*/
void
packetbuf_reference(void *ptr, uint16_t len)
{
  packetbuf_clear();
  packetbufptr = ptr;
  buflen = len;
}
/*---------------------------------------------------------------------------*/
int
packetbuf_is_reference(void)
{
  return packetbufptr != &packetbuf[PACKETBUF_HDR_SIZE];
}
/*---------------------------------------------------------------------------*/
void *
packetbuf_reference_ptr(void)
{
  return packetbufptr;
}
/*---------------------------------------------------------------------------*/
uint16_t
packetbuf_datalen(void)
{
  return buflen;
}
/*---------------------------------------------------------------------------*/
uint16_t
packetbuf_datalen_read(void)  //fengmo debug GW packet loss
{
  return buflen_in;
}
/*---------------------------------------------------------------------------*/
uint8_t
packetbuf_hdrlen(void)
{
  return PACKETBUF_HDR_SIZE - hdrptr;
}
/*---------------------------------------------------------------------------*/
uint16_t
packetbuf_totlen(void)
{
  return packetbuf_hdrlen() + packetbuf_datalen();
}
/*---------------------------------------------------------------------------*/
void packetbuf_attr_clear(void) {
  int i;
  for (i = 0; i < PACKETBUF_NUM_ATTRS; ++i) {
    packetbuf_attrs[i].val = 0;
  }
  int j;
  for (i = 0; i < PACKETBUF_NUM_ADDRS; ++i) {
    for (j = 0; j < RIMEADDR_SIZE; j++) {
      packetbuf_addrs[i].addr.u8[j] = 0;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
packetbuf_attr_copyto(struct packetbuf_attr *attrs,
                      struct packetbuf_addr *addrs)
{
  memcpy(attrs, packetbuf_attrs, sizeof(packetbuf_attrs));
  memcpy(addrs, packetbuf_addrs, sizeof(packetbuf_addrs));
}
/*---------------------------------------------------------------------------*/
void
packetbuf_attr_copyfrom(struct packetbuf_attr *attrs,
                        struct packetbuf_addr *addrs)
{
  memcpy(packetbuf_attrs, attrs, sizeof(packetbuf_attrs));
  memcpy(packetbuf_addrs, addrs, sizeof(packetbuf_addrs));
}
/*---------------------------------------------------------------------------*/
#if !PACKETBUF_CONF_ATTRS_INLINE
int
packetbuf_set_attr(uint8_t type, const packetbuf_attr_t val)
{
  /*   packetbuf_attrs[type].type = type; */
  packetbuf_attrs[type].val = val;
  return 1;
}
/*---------------------------------------------------------------------------*/
packetbuf_attr_t
packetbuf_attr(uint8_t type)
{
  return packetbuf_attrs[type].val;
}
/*---------------------------------------------------------------------------*/
int
packetbuf_set_addr(uint8_t type, const rimeaddr_t *addr)
{
  /*   packetbuf_addrs[type - PACKETBUF_ADDR_FIRST].type = type; */
  rimeaddr_copy(&packetbuf_addrs[type - PACKETBUF_ADDR_FIRST].addr, addr);
  return 1;
}
/*---------------------------------------------------------------------------*/
const rimeaddr_t *
packetbuf_addr(uint8_t type)
{
  //fengmo debug GW packet loss	return &packetbuf_addrs[type - PACKETBUF_ADDR_FIRST].addr;
  return &(uipPackerAddrss_p[type - PACKETBUF_ADDR_FIRST].addr);  //fengmo debug GW packet loss
}
/*---------------------------------------------------------------------------*/
#endif /* PACKETBUF_CONF_ATTRS_INLINE */

struct structUipPacket uipPakcets[MAX_NUM_OF_OUTSTANDING_PACKETS];  //fengmo debug GW packet loss
uint8_t uipPackets_ReadIdx = MAX_NUM_OF_OUTSTANDING_PACKETS - 1;  //fengmo debug GW packet loss
uint8_t uipPackets_WriteIdx = 0;  //fengmo debug GW packet loss
volatile uint8_t uipPackets_writeLock = 0;
uint16_t lowpanTxPacketLossCount=0;
uint16_t lowpanRxPacketLossCount=0;
/*--------------------------------------------------------------------*/
void uipPacketIndex_inc(uint8_t *idx_p)  //fengmo debug GW packet loss
{
  *idx_p = (*idx_p + 1) % MAX_NUM_OF_OUTSTANDING_PACKETS;
}
/*--------------------------------------------------------------------*/
void uipPakcets_lock() //fengmo debug GW packet loss
{
  while (uipPackets_writeLock)
  {
#ifdef LINUX_GATEWAY
    usleep(1000);
#else
    Task_sleep(CLOCK_SECOND/1000);
#endif
  }
  
  uipPackets_writeLock = 1;
}

/*--------------------------------------------------------------------*/
void uipPakcets_unlock() //fengmo debug GW packet loss
{
  uipPackets_writeLock = 0;
}
/*--------------------------------------------------------------------*/
struct structRplConfig rplConfig;

void rplConfig_init()
{
  rplConfig.rpl_dio_doublings = RPL_DIO_INTERVAL_DOUBLINGS;
}


struct structNvParams nvParams;

struct structCoapConfig coapConfig;
void coap_config_init()
{
   coapConfig.coap_resource_check_time       = COAP_RESOURCE_CHECK_TIME;
   coapConfig.coap_port                      = COAP_DEFAULT_PORT;
   coapConfig.coap_dtls_port                 = COAP_DTLS_UDP_PORT;
   coapConfig.coap_mac_sec_port              = COAP_MAC_SEC_UDP_PORT;
   coapConfig.coap_obs_max_non               = COAP_OBS_MAX_NON;
   coapConfig.coap_default_response_timeout  = COAP_DEFAULT_RESPONSE_TIMEOUT;
}

#endif /* UIP_PACKETBUF */
/** @} */
