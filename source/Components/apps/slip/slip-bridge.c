/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 * $Id: slip-bridge.c,v 1.6 2011/01/17 20:05:51 joxe Exp $
 */

/**
 * \file
 *         Slip fallback interface
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Joel Hoglund <joel@sics.se>
 *         Nicolas Tsiftes <nvt@sics.se>
 */

#include "net/uip.h"
#include "net/uip-ds6.h"
#include "slip/slip.h"
#include "Board.h"
#include "led.h"
#include <string.h>

#define UIP_IP_BUF        ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

void set_prefix_64(uip_ipaddr_t *);

static uip_ipaddr_t last_sender;

extern void slip_send_packet(uint8_t *ptr, int len);
/*---------------------------------------------------------------------------*/
void
slip_input_callback(void)
{
 // PRINTF("SIN: %u\n", uip_len);
  if(uip_buf[0] == '!') {
    //PRINTF("Got configuration message of type %c\n", uip_buf[1]);
    uip_len = 0;
    if(uip_buf[1] == 'P') {
      uip_ipaddr_t prefix;
      /* Here we set a prefix !!! */
      memset(&prefix, 0, 16);
      memcpy(&prefix, &uip_buf[2], 8);
      //PRINTF("Setting prefix ");
      //PRINT6ADDR(&prefix);
      //PRINTF("\n");
      set_prefix_64(&prefix);
    }
  } else if (uip_buf[0] == '?') {
    //PRINTF("Got request message of type %c\n", uip_buf[1]);
    if(uip_buf[1] == 'M') {
      char* hexchar = "0123456789abcdef";
      int j;
      /* this is just a test so far... just to see if it works */
      uip_buf[0] = '!';
      for(j = 0; j < 8; j++) {
        uip_buf[2 + j * 2] = hexchar[uip_lladdr.addr[j] >> 4];
        uip_buf[3 + j * 2] = hexchar[uip_lladdr.addr[j] & 15];
      }
      uip_len = 18;
      slip_send();

    }
    uip_len = 0;
  }
  /* Save the last sender received over SLIP to avoid bouncing the
     packet back if no route is found */
  uip_ipaddr_copy(&last_sender, &UIP_IP_BUF->srcipaddr);
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  //slip_arch_init(BAUD2UBR(115200));
  //process_start(&slip_process, NULL);
  //slip_set_input_callback(slip_input_callback);
}
/*---------------------------------------------------------------------------*/
static void
output(void)
{
  if(uip_ipaddr_cmp(&last_sender, &UIP_IP_BUF->srcipaddr)) {
    /* Do not bounce packets back over SLIP if the packet was received
       over SLIP */
//    PRINTF("slip-bridge: Destination off-link but no route src=");
//    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
//    PRINTF(" dst=");
//    PRINT6ADDR(&UIP_IP_BUF->destipaddr);
//    PRINTF("\n");
  } else {
 //   PRINTF("SUT: %u\n", uip_len);
    slip_send_packet((uint8_t *)UIP_IP_BUF, uip_datalen());
  }
}

/*---------------------------------------------------------------------------*/
const struct uip_fallback_interface slip_interface = {
  init, output
};
/*---------------------------------------------------------------------------*/
