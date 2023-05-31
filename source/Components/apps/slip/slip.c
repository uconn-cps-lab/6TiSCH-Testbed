/* -*- C -*- */
/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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
 * @(#)$Id: slip.c,v 1.12 2010/12/23 22:38:47 dak664 Exp $
 */

//PD - COAP_MESSAGING_MOD - Added file - Updated process to task

/**
 * COAP_MESSAGING_MOD - NOTE:
 * 		MSP430 - All calls to slip_arch_writeb were replaced by uart1_writeb
 * 		         to correspond to the uart driver within the SmartNet Stack
 *              CC26xx - All calls to slip_arch_writeb were replaced by UART_write
 * 		         to correspond to the uart driver within the SmartNet Stack
 *
 * 		/platform/exp5438/uart1x.c
 */

#include <stdio.h>
#include <string.h>
#include "Board.h"
#include "led.h"
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/BIOS.h>
//extern Semaphore_Handle slip_poll_sem;

#include "net/uip.h"
#include "net/uip-fw.h"
#define BUF ((struct uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])

#if IS_ROOT
#include <ti/drivers/UART.h>
extern UART_Handle      handle; // Declared in tsch-uip-pan.c

const Char startup[] = "TSCH Root Node\n";

#endif /* IS_ROOT */

#include "slip/slip.h"

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335


#if 1
#define SLIP_STATISTICS(statement)
#else
uint16_t slip_rubbish, slip_twopackets, slip_overflow, slip_ip_drop;
#define SLIP_STATISTICS(statement) statement
#endif

/* Must be at least one byte larger than UIP_BUFSIZE! */
#define RX_BUFSIZE (UIP_BUFSIZE - UIP_LLH_LEN + 16)

static uint8_t rxbuf[RX_BUFSIZE];

extern void init_tsch(void);

static void (*input_callback)(void) = NULL;

SLIP_DBG SLIP_Dbg;

int16_t slip_read_packet(uint8_t *outbuf, uint16_t blen);

/*---------------------------------------------------------------------------*/
void slip_set_input_callback(void (*c)(void)) {
	input_callback = c;
}

Void gpioButtonFxn(Void) {
// do nothing
}

/*---------------------------------------------------------------------------*/
void slip_task(void) {
  int16_t rx_pkt_len;

  //init_tsch();
  /* Write 'Hello World' on UART */
  UART_write(handle, startup, sizeof(startup));

	while (1) {

    rx_pkt_len = slip_read_packet(rxbuf, UIP_BUFSIZE - UIP_LLH_LEN);

    if (rx_pkt_len == -1)
    {
      continue;
    }

    // copy data from rx buf to uip buffer
    memcpy(&uip_buf[UIP_LLH_LEN],rxbuf,rx_pkt_len);
    uip_len = rx_pkt_len;

#if !UIP_CONF_IPV6
		if(uip_len == 4 && strncmp((char*)&uip_buf[UIP_LLH_LEN], "?IPA", 4) == 0) {
			char buf[8];
			memcpy(&buf[0], "=IPA", 4);
			memcpy(&buf[4], &uip_hostaddr, 4);
			if(input_callback) {
				input_callback();
			}
			slip_write(buf, 8);
		} else if(uip_len > 0
				&& uip_len == (((uint16_t)(BUF->len[0]) << 8) + BUF->len[1])
				&& uip_ipchksum() == 0xffff) {
#define IP_DF   0x40
			if(BUF->ipid[0] == 0 && BUF->ipid[1] == 0 && BUF->ipoffset[0] & IP_DF) {
				static uint16_t ip_id;
				uint16_t nid = ip_id++;
				BUF->ipid[0] = nid >> 8;
				BUF->ipid[1] = nid;
				nid = uip_htons(nid);
				nid = ~nid; /* negate */
				BUF->ipchksum += nid; /* add */
				if(BUF->ipchksum < nid) { /* 1-complement overflow? */
					BUF->ipchksum++;
				}
			}
#ifdef SLIP_CONF_TCPIP_INPUT
			SLIP_CONF_TCPIP_INPUT();
#else
			tcpip_input();
#endif
		} else {
			uip_len = 0;
			SLIP_STATISTICS(slip_ip_drop++);
		}
#else /* UIP_CONF_IPV6 */
		if (uip_len > 0) {
			if (input_callback) {
				input_callback();
			}
#ifdef SLIP_CONF_TCPIP_INPUT
			SLIP_CONF_TCPIP_INPUT();
#else
			tcpip_ipv6_output();
#endif
		}
#endif /* UIP_CONF_IPV6 */
	}
}

int16_t slip_read_packet(uint8_t *outbuf, uint16_t blen)
{
  uint16_t pkt_len=0;
  uint8_t data_byte,num_data;

  while (1)
  {
    num_data = UART_read(handle,(Char *)&data_byte, 1);
    if (num_data != 1)
    {
      SLIP_Dbg.err_UART_read++;
      continue;
    }
    SLIP_Dbg.total_rx_byte++;

  	switch (data_byte)
  	{
      case SLIP_END:

        SLIP_Dbg.rx_pkt_num++;
        if (pkt_len == 0)
        {
          SLIP_Dbg.err_zero_len_pkt++;
        }
        else
          return pkt_len;
        break;

      case SLIP_ESC:
        SLIP_Dbg.rx_esc_num++;

        // read one more char to decode the stuffing byte
        UART_read(handle,(Char *)&data_byte, 1);
        if (data_byte == SLIP_ESC_END)
        {
          SLIP_Dbg.rx_esc_end++;
          outbuf[pkt_len] = SLIP_END;
        }
        else if (data_byte == SLIP_ESC_ESC)
        {
          SLIP_Dbg.rx_esc_esc++;
          outbuf[pkt_len] = SLIP_ESC;
        }
        else
        {
          SLIP_Dbg.err_slip_encoder++;
        }
        break;
      default:
        outbuf[pkt_len] = data_byte;
    }

    // check to see if we have space
    pkt_len++;
    if (pkt_len>=blen)
    { // we don't have space, return error
      SLIP_Dbg.err_pkt_len_too_big++;
      return -1;
    }
  }

}
/*---------------------------------------------------------------------------*/

