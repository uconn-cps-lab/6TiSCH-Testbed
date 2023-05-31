/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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
 */

 //PD - COAP_MESSAGING_MOD - Added file
/**
 * MSP430 - Updated all calls to slip_arch_writeb to uart1_writeb to conform with uart driver
 * CC26xx - Updated all calls to slip_arch_writeb to UART_write to conform with uart driver
 */

#include "net/netstack.h"
#include "net/uip.h"
#include "net/packetbuf.h"
#include "slip/slip.h"
#include <stdio.h>

#if IS_ROOT
#include <ti/drivers/UART.h>
extern UART_Handle      handle; // Declared in tsch-uip-pan.c
#endif /* IS_ROOT */

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

#define DEBUG 0

extern SLIP_DBG SLIP_Dbg;
/*---------------------------------------------------------------------------*/
void
slipnet_init(void)
{
}
/*---------------------------------------------------------------------------*/
  const char end = SLIP_END;
  const char esc = SLIP_ESC;
void
slip_send_packet(uint8_t *ptr, int len)
{
  uint16_t i;
  uint8_t c;

  SLIP_Dbg.tx_pkt_num++;

  //uart1_writeb(SLIP_END);
  UART_write(handle, &end, 1);
  for(i = 0; i < len; ++i) {
    c = *ptr++;
    if(c == SLIP_END) {
      //uart1_writeb(SLIP_ESC);
      UART_write(handle, &esc, 1);
      c = SLIP_ESC_END;
    } else if(c == SLIP_ESC) {
      //uart1_writeb(SLIP_ESC);
      UART_write(handle, &esc, 1);
      c = SLIP_ESC_ESC;
    }
    //uart1_writeb(c);
    UART_write(handle, &c, 1);
  }
  //uart1_writeb(SLIP_END);
  UART_write(handle, &end, 1);
}
/*---------------------------------------------------------------------------*/
void
slipnet_input(void)
{
  int i;
  /* radio should be configured for filtering so this should be simple */
  /* this should be sent over SLIP! */
  /* so just copy into uip-but and send!!! */
  /* Format: !R<data> ? */
  uip_len = packetbuf_datalen();
  i = packetbuf_copyto(uip_buf);

  if(DEBUG) {
    printf("Slipnet got input of len: %d, copied: %d\n",
	   packetbuf_datalen(), i);

    for(i = 0; i < uip_len; i++) {
      printf("%02x", (unsigned char) uip_buf[i]);
      if((i & 15) == 15) printf("\n");
      else if((i & 7) == 7) printf(" ");
    }
    printf("\n");
  }

  /* printf("SUT: %u\n", uip_len); */
  slip_send_packet((uint8_t*)uip_buf, uip_len);
}
/*---------------------------------------------------------------------------*/
const struct network_driver slipnet_driver = {
  "slipnet",
  slipnet_init,
  slipnet_input
};
/*---------------------------------------------------------------------------*/
